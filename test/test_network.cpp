#include <thread>

#include "asio.hpp"
#include "catch.hpp"
#include "easylogging++.h"
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
        const LocNet::NetworkInterface &BudapestNodeContact(
            LocNet::TestData::NodeBudapest.profile().contacts().front() );
        
        TcpNetwork network(BudapestNodeContact);
        
        thread serverThread( [&network]
        {
            try {
                shared_ptr<ISpatialDatabase> geodb( new InMemorySpatialDatabase(TestData::Budapest) );
                geodb->Store(TestData::EntryKecskemet);
                geodb->Store(TestData::EntryLondon);
                geodb->Store(TestData::EntryNewYork);
                geodb->Store(TestData::EntryWien);
                geodb->Store(TestData::EntryCapeTown);

                shared_ptr<INodeConnectionFactory> connectionFactory(
                    new DummyNodeConnectionFactory() );
                Node node( TestData::NodeBudapest, geodb, connectionFactory );
                IncomingRequestDispatcher dispatcher(node);
                
                SyncProtoBufNetworkSession serverSession(network);
                while (true)
                {
                    shared_ptr<iop::locnet::MessageWithHeader> requestMsg( serverSession.ReceiveMessage() );

                    unique_ptr<iop::locnet::Response> response( dispatcher.Dispatch( requestMsg->body().request() ) );
                    
                    iop::locnet::MessageWithHeader responseMsg;
                    responseMsg.mutable_body()->set_allocated_response( response.release() );
                    serverSession.SendMessage(responseMsg);
                }
            }
            catch (exception &e) {
                // LOG(ERROR) << "Server thread failed: " << e.what();
            }
        } );
        
        shared_ptr<IProtoBufNetworkSession> session(
            new SyncProtoBufNetworkSession(BudapestNodeContact) );

        THEN("It serves clients via sync TCP") {
            {
                iop::locnet::MessageWithHeader requestMsg;
                requestMsg.mutable_body()->mutable_request()->mutable_localservice()->mutable_getneighbournodes();
                requestMsg.mutable_body()->mutable_request()->set_version("1");
                session->SendMessage(requestMsg);
                
                unique_ptr<iop::locnet::MessageWithHeader> msgReceived( session->ReceiveMessage() );
                
                const iop::locnet::GetNeighbourNodesByDistanceResponse &response =
                    msgReceived->body().response().localservice().getneighbournodes();
                REQUIRE( response.nodes_size() == 2 );
                REQUIRE( Converter::FromProtoBuf( response.nodes(0) ) == TestData::NodeKecskemet );
                REQUIRE( Converter::FromProtoBuf( response.nodes(1) ) == TestData::NodeWien );
            }
            {
                iop::locnet::MessageWithHeader requestMsg;
                requestMsg.mutable_body()->mutable_request()->mutable_remotenode()->mutable_getcolleaguenodecount();
                requestMsg.mutable_body()->mutable_request()->set_version("1");
                session->SendMessage(requestMsg);
                
                unique_ptr<iop::locnet::MessageWithHeader> msgReceived( session->ReceiveMessage() );
                
                const iop::locnet::GetColleagueNodeCountResponse &response =
                    msgReceived->body().response().remotenode().getcolleaguenodecount();
                REQUIRE( response.nodecount() == 3 );
            }
        }
        
        session->Close();
        serverThread.join();
    }
}



SCENARIO("Transparent remote node client", "[network]") {
//         THEN("It serves transparent clients using ProtoBuf/TCP protocol") {
//             LOG(INFO) << "Sending request";
//             shared_ptr<IProtoBufRequestDispatcher> netDispatcher(
//                 new ProtoBufRequestNetworkDispatcher(session) );
//             NodeMethodsProtoBufClient client(netDispatcher);
//             
//             size_t colleagueCount = client.GetColleagueNodeCount();
//             REQUIRE( colleagueCount == 3 );
//             LOG(INFO) << "Assertions fine";
//         }
}
