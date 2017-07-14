#include <thread>

#include <asio.hpp>
#include <catch.hpp>

#include "network.hpp"
#include "testimpls.hpp"
#include "testdata.hpp"

#include <easylogging++.h>

using namespace std;
using namespace LocNet;
using namespace asio::ip;



void reactorLoop(const string &threadName)
{
    LOG(DEBUG) << "Thread " << threadName << " started";
    try
    {
        asio::io_service &reactor = Reactor::Instance().AsioService();
        reactor.run();
        reactor.reset();            
        LOG(DEBUG) << "Thread " << threadName << " succesfully executed a single task";
    }
    catch (exception &ex)
    {
        LOG(ERROR) << "Async operation failed: " << ex.what();
        LOG(DEBUG) << "Thread " << threadName << " failed at a single task";
    }
    LOG(DEBUG) << "Thread " << threadName << " shut down";
}



SCENARIO("Client-Server requests and responses with TCP networking", "[network]")
{
    GIVEN("A configured Node and Tcp networking")
    {
        const NodeContact &BudapestNodeContact( TestData::NodeBudapest.contact() );
        
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase( TestData::NodeBudapest,
            SpatiaLiteDatabase::IN_MEMORY_DB, chrono::hours(1) ) );
        geodb->Store(TestData::EntryKecskemet);
        geodb->Store(TestData::EntryLondon);
        geodb->Store(TestData::EntryNewYork);
        geodb->Store(TestData::EntryWien);
        geodb->Store(TestData::EntryCapeTown);

        shared_ptr<INodeProxyFactory> connectionFactory( new DummyNodeConnectionFactory() );
        shared_ptr<Node> node = Node::Create(geodb, connectionFactory);
        
        shared_ptr<IBlockingRequestDispatcherFactory> dispatcherFactory(
            new CombinedBlockingRequestDispatcherFactory(node) );
        shared_ptr<DispatchingTcpServer> tcpServer = DispatchingTcpServer::Create(
            BudapestNodeContact.nodePort(), dispatcherFactory );
        tcpServer->StartListening();
        
        thread reactorMainThread( [] { reactorLoop("ReactorMain"); } );
        reactorMainThread.detach();

        THEN("It serves clients via sync TCP")
        {
            shared_ptr<IProtoBufChannel> clientChannel( new AsyncProtoBufTcpChannel(
                BudapestNodeContact.nodeEndpoint() ) );
            {
                unique_ptr<iop::locnet::Message> requestMsg( new iop::locnet::Message() );
                requestMsg->mutable_request()->mutable_localservice()->mutable_getneighbournodes();
                requestMsg->mutable_request()->set_version({1,0,0});
                clientChannel->SendMessage( move(requestMsg), asio::use_future ).get();
                
                unique_ptr<iop::locnet::Message> msgReceived( clientChannel->ReceiveMessage(asio::use_future).get() );
                
                const iop::locnet::GetNeighbourNodesByDistanceResponse &response =
                    msgReceived->response().localservice().getneighbournodes();
                REQUIRE( response.nodes_size() == 2 );
                REQUIRE( Converter::FromProtoBuf( response.nodes(0) ) == TestData::NodeKecskemet );
                REQUIRE( Converter::FromProtoBuf( response.nodes(1) ) == TestData::NodeWien );
            }
            {
                unique_ptr<iop::locnet::Message> requestMsg( new iop::locnet::Message() );
                requestMsg->mutable_request()->mutable_remotenode()->mutable_getnodecount();
                requestMsg->mutable_request()->set_version({1,0,0});
                clientChannel->SendMessage( move(requestMsg), asio::use_future ).get();
                
                unique_ptr<iop::locnet::Message> msgReceived( clientChannel->ReceiveMessage(asio::use_future).get() );
                
                const iop::locnet::GetNodeCountResponse &response =
                    msgReceived->response().remotenode().getnodecount();
                REQUIRE( response.nodecount() == 6 );
            }
        }

        THEN("It serves transparent clients using ProtoBuf/TCP protocol")
        {
            shared_ptr<IProtoBufChannel> clientChannel( new AsyncProtoBufTcpChannel(
                BudapestNodeContact.nodeEndpoint() ) );
            shared_ptr<ProtoBufClientSession> clientSession( ProtoBufClientSession::Create(clientChannel) );
            clientSession->StartMessageLoop();

            shared_ptr<IBlockingRequestDispatcher> netDispatcher( new NetworkDispatcher(clientSession) );
            NodeMethodsProtoBufClient client(netDispatcher, {});
            
            size_t nodeCount = client.GetNodeCount();
            REQUIRE( nodeCount == 6 );
        }

        Reactor::Instance().Shutdown();
    }
}



SCENARIO("Neighbourhood notifications for local services", "[network]")
{
    GIVEN("A configured Node and Tcp networking")
    {
        const NodeContact &BudapestNodeContact( TestData::NodeBudapest.contact() );
        
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase( TestData::NodeBudapest,
            SpatiaLiteDatabase::IN_MEMORY_DB, chrono::hours(1) ) );

        shared_ptr<INodeProxyFactory> connectionFactory( new DummyNodeConnectionFactory() );
        shared_ptr<Node> node = Node::Create(geodb, connectionFactory);
        
        shared_ptr<IBlockingRequestDispatcherFactory> dispatcherFactory(
            new CombinedBlockingRequestDispatcherFactory(node) );
        shared_ptr<DispatchingTcpServer> tcpServer = DispatchingTcpServer::Create(
            BudapestNodeContact.nodePort(), dispatcherFactory );
        tcpServer->StartListening();

        thread reactorMainThread( [] { reactorLoop("ReactorMain"); } );
        reactorMainThread.detach();
        
        THEN("It properly notifies local services on changes")
        {
            shared_ptr<IProtoBufChannel> channel( new AsyncProtoBufTcpChannel(
                BudapestNodeContact.nodeEndpoint() ) );
            shared_ptr<ProtoBufClientSession> session( ProtoBufClientSession::Create(channel) );

            uint32_t notificationsReceived = 0;
            session->StartMessageLoop( [&notificationsReceived, channel, session]
                ( unique_ptr<iop::locnet::Message> &&requestMsg )
            {
                REQUIRE( requestMsg );
                REQUIRE( requestMsg->has_request() );
                REQUIRE( requestMsg->request().has_localservice() );
                REQUIRE( requestMsg->request().localservice().has_neighbourhoodchanged() );
            
                ++notificationsReceived;
            
//             const iop::locnet::NeighbourhoodChangedNotificationRequest &changeNote =
//                 requestMsg->request().localservice().neighbourhoodchanged();
            
                unique_ptr<iop::locnet::Message> changeAckn( new iop::locnet::Message() );
                changeAckn->set_id( requestMsg->id() );
                changeAckn->mutable_response()->mutable_localservice()->mutable_neighbourhoodupdated();
                LOG(INFO) << "Sending acknowledgement";
                channel->SendMessage( move(changeAckn), [] {} );
                LOG(INFO) << "Sent acknowledgement";
            } );
            
            shared_ptr<IBlockingRequestDispatcher> requestDispatcher( new NetworkDispatcher(session) );
            LOG(INFO) << "Sending registerservice request";
            unique_ptr<iop::locnet::Request> registerRequest( new iop::locnet::Request() );
            registerRequest->mutable_localservice()->mutable_registerservice()->set_allocated_service(
                Converter::ToProtoBuf( ServiceInfo(ServiceType::Profile, 16999, "ProfileServerId") ) );
            requestDispatcher->Dispatch( move(registerRequest) );
            
            for (size_t requestsSent = 0; requestsSent < 3; ++requestsSent)
            {
                LOG(INFO) << "Sending getneighbournodes request";
                unique_ptr<iop::locnet::Request> neighbourhoodRequest( new iop::locnet::Request() );
                neighbourhoodRequest->mutable_localservice()->mutable_getneighbournodes()->set_keepaliveandsendupdates(true);
                unique_ptr<iop::locnet::Response> neighbourhoodResponse = requestDispatcher->Dispatch( move(neighbourhoodRequest) );
                REQUIRE( neighbourhoodResponse->has_localservice() );
                REQUIRE( neighbourhoodResponse->localservice().has_getneighbournodes() );
                LOG(INFO) << "Received getneighbournodes response";
            }

            LOG(INFO) << "Generating neighbourhood change events";
            geodb->Store(TestData::EntryKecskemet);
            geodb->Store(TestData::EntryWien);

            LOG(INFO) << "Sending deregisterservice request";
            unique_ptr<iop::locnet::Request> deregisterRequest( new iop::locnet::Request() );
            deregisterRequest->mutable_localservice()->mutable_deregisterservice()->set_servicetype(
                Converter::ToProtoBuf(ServiceType::Profile) );
            requestDispatcher->Dispatch( move(deregisterRequest) );

            REQUIRE( notificationsReceived == 2 );
            
            Reactor::Instance().Shutdown();
        }
    }
}
