#include <thread>

#include <asio.hpp>
#include <catch.hpp>
#include <easylogging++.h>

#include "network.hpp"
#include "testimpls.hpp"
#include "testdata.hpp"

using namespace std;
using namespace LocNet;
using namespace asio::ip;



SCENARIO("TCP networking", "[network]")
{
    GIVEN("A configured Node and Tcp networking")
    {
        const NodeContact &BudapestNodeContact(
            TestData::NodeBudapest.profile().contact() );
        
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase( TestData::NodeBudapest,
            SpatiaLiteDatabase::IN_MEMORY_DB, chrono::hours(1) ) );
        geodb->Store(TestData::EntryKecskemet);
        geodb->Store(TestData::EntryLondon);
        geodb->Store(TestData::EntryNewYork);
        geodb->Store(TestData::EntryWien);
        geodb->Store(TestData::EntryCapeTown);

        shared_ptr<INodeConnectionFactory> connectionFactory( new DummyNodeConnectionFactory() );
        shared_ptr<Node> node( new Node( TestData::NodeBudapest, geodb, connectionFactory, {} ) );
        
        shared_ptr<IProtoBufRequestDispatcherFactory> dispatcherFactory(
            new CombinedRequestDispatcherFactory(node) );
        ProtoBufDispatchingTcpServer tcpServer( BudapestNodeContact.nodePort(), dispatcherFactory );
        
        THEN("It serves clients via sync TCP")
        {
            const NodeContact &BudapestNodeContact(
                TestData::NodeBudapest.profile().contact() );
            shared_ptr<IProtoBufNetworkSession> clientSession(
                new ProtoBufTcpStreamSession( BudapestNodeContact.nodeEndpoint() ) );
            
            {
                iop::locnet::MessageWithHeader requestMsg;
                requestMsg.mutable_body()->mutable_request()->mutable_localservice()->mutable_getneighbournodes();
                requestMsg.mutable_body()->mutable_request()->set_version({1,0,0});
                clientSession->SendMessage(requestMsg);
                
                unique_ptr<iop::locnet::MessageWithHeader> msgReceived( clientSession->ReceiveMessage() );
                
                const iop::locnet::GetNeighbourNodesByDistanceResponse &response =
                    msgReceived->body().response().localservice().getneighbournodes();
                REQUIRE( response.nodes_size() == 2 );
                REQUIRE( Converter::FromProtoBuf( response.nodes(0) ) == TestData::NodeKecskemet );
                REQUIRE( Converter::FromProtoBuf( response.nodes(1) ) == TestData::NodeWien );
            }
            {
                iop::locnet::MessageWithHeader requestMsg;
                requestMsg.mutable_body()->mutable_request()->mutable_remotenode()->mutable_getnodecount();
                requestMsg.mutable_body()->mutable_request()->set_version({1,0,0});
                clientSession->SendMessage(requestMsg);
                
                unique_ptr<iop::locnet::MessageWithHeader> msgReceived( clientSession->ReceiveMessage() );
                
                const iop::locnet::GetNodeCountResponse &response =
                    msgReceived->body().response().remotenode().getnodecount();
                REQUIRE( response.nodecount() == 6 );
            }
        }
        
        THEN("It serves transparent clients using ProtoBuf/TCP protocol")
        {
            const NodeContact &BudapestNodeContact(
                TestData::NodeBudapest.profile().contact() );
            shared_ptr<IProtoBufNetworkSession> clientSession(
                new ProtoBufTcpStreamSession( BudapestNodeContact.nodeEndpoint() ) );
            
            shared_ptr<IProtoBufRequestDispatcher> netDispatcher(
                new ProtoBufRequestNetworkDispatcher(clientSession) );
            NodeMethodsProtoBufClient client(netDispatcher, {});
            
            size_t nodeCount = client.GetNodeCount();
            REQUIRE( nodeCount == 6 );
        }
    }
}
