#include <catch.hpp>
#include <easylogging++.h>

#include "messaging.hpp"
#include "testdata.hpp"
#include "testimpls.hpp"

using namespace std;
using namespace LocNet;



SCENARIO("ProtoBuf messaging", "[messaging]")
{
    GIVEN("A GPS location") {
        THEN("Its coordinates are properly transformed to and back from ProtoBuf int representation") {
            iop::locnet::GpsLocation protoBufBudapest;
            Converter::FillProtoBuf(&protoBufBudapest, TestData::Budapest);
            
            REQUIRE( abs( protoBufBudapest.latitude() - 47497912 ) < 10 );
            REQUIRE( abs( protoBufBudapest.longitude() - 19040235 ) < 10 );
            
            GpsLocation locnetBudapest( Converter::FromProtoBuf(protoBufBudapest) );
            REQUIRE( locnetBudapest.latitude() == Approx( TestData::Budapest.latitude() ) );
            REQUIRE( locnetBudapest.longitude() == Approx( TestData::Budapest.longitude() ) );
        }
    }
    
    GIVEN("A message dispatcher") {
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase(TestData::NodeBudapest,
            SpatiaLiteDatabase::IN_MEMORY_DB, chrono::hours(1) ) );
        geodb->Store(TestData::EntryKecskemet);
        geodb->Store(TestData::EntryLondon);
        geodb->Store(TestData::EntryNewYork);
        geodb->Store(TestData::EntryWien);
        geodb->Store(TestData::EntryCapeTown);
        
        shared_ptr<INodeProxyFactory> connectionFactory( new DummyNodeConnectionFactory() );
        shared_ptr<IChangeListenerFactory> listenerFactory( new DummyChangeListenerFactory() );
        shared_ptr<Node> node = Node::Create(geodb, connectionFactory);
        IncomingRequestDispatcher dispatcher(node, listenerFactory);
        
        THEN("Local service GetNeighbours requests are properly served") {
            unique_ptr<iop::locnet::Request> request( new iop::locnet::Request() );
            request->set_version({1,0,0});
            request->mutable_localservice()->mutable_getneighbournodes();
                
            unique_ptr<iop::locnet::Response> response = dispatcher.Dispatch( move(request) );
            REQUIRE( response->has_localservice() );
            REQUIRE( response->localservice().has_getneighbournodes() );
            
            const iop::locnet::GetNeighbourNodesByDistanceResponse &getNeighboursResp =
                response->localservice().getneighbournodes();
            REQUIRE( getNeighboursResp.nodes_size() == 2 );
            
            NodeInfo closestNeighbour( Converter::FromProtoBuf( getNeighboursResp.nodes(0) ) );
            REQUIRE( closestNeighbour == TestData::NodeKecskemet );
            NodeInfo secondNeighbour( Converter::FromProtoBuf( getNeighboursResp.nodes(1) ) );
            REQUIRE( secondNeighbour == TestData::NodeWien );
        }
    }
    
}
