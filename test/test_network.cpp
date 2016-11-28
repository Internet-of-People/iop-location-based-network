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
        
        const LocNet::NetworkInterface &BudapestNodeContact(
            LocNet::TestData::NodeBudapest.profile().contacts().front() );
        TcpNetwork network(BudapestNodeContact);
        thread serverThread( [&dispatcher, &network]
        {
            SyncProtoBufNetworkSession session(network);
            shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session.ReceiveMessage() );

            unique_ptr<iop::locnet::Response> response( dispatcher.Dispatch( requestMsg->body().request() ) );
            
            iop::locnet::MessageWithHeader responseMsg;
            responseMsg.mutable_body()->set_allocated_response( response.release() );
            session.SendMessage(responseMsg);
        } );
        
        THEN("It serves clients") {
            SyncProtoBufNetworkSession session(BudapestNodeContact);

            iop::locnet::MessageWithHeader requestMsg;
            requestMsg.mutable_body()->mutable_request()->mutable_localservice()->mutable_getneighbournodes();
            requestMsg.mutable_body()->mutable_request()->set_version("1");
            session.SendMessage(requestMsg);
            
            unique_ptr<iop::locnet::MessageWithHeader> msgReceived( session.ReceiveMessage() );
            
            const iop::locnet::GetNeighbourNodesByDistanceResponse &response =
                msgReceived->body().response().localservice().getneighbournodes();
            REQUIRE( response.nodes_size() == 2 );
            REQUIRE( Converter::FromProtoBuf( response.nodes(0) ) == TestData::NodeKecskemet );
            REQUIRE( Converter::FromProtoBuf( response.nodes(1) ) == TestData::NodeWien );
        }
        
        serverThread.join();
    }
}
