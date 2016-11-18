#include "catch.hpp"
#include "easylogging++.h"
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
            
            REQUIRE( abs( protoBufBudapest.latitude() - 47480870 ) < 10 );
            REQUIRE( abs( protoBufBudapest.longitude() - 18849426 ) < 10 );
            
            GpsLocation locnetBudapest( Converter::FromProtoBuf(protoBufBudapest) );
            REQUIRE( locnetBudapest.latitude() == Approx( TestData::Budapest.latitude() ) );
            REQUIRE( locnetBudapest.longitude() == Approx( TestData::Budapest.longitude() ) );
        }
    }
    
    GIVEN("A message dispatcher") {
        shared_ptr<ISpatialDatabase> geodb( new DummySpatialDatabase(TestData::Budapest) );
        geodb->Store(TestData::EntryKecskemet);
        geodb->Store(TestData::EntryLondon);
        geodb->Store(TestData::EntryNewYork);
        geodb->Store(TestData::EntryWien);
        geodb->Store(TestData::EntryCapeTown);
        
        shared_ptr<IRemoteNodeConnectionFactory> connectionFactory(
            new DummyRemoteNodeConnectionFactory() );
        Node node( TestData::NodeBudapest, geodb, connectionFactory );
        MessageDispatcher dispatcher(node);
        
        THEN("Local service GetNeighbours requests are properly served") {
            iop::locnet::Request request;
            request.set_version("1");
            iop::locnet::GetNeighbourNodesByDistanceRequest *getNeighboursReq =
                request.mutable_localservice()->mutable_getneighbournodes();
                
            shared_ptr<iop::locnet::Response> response = dispatcher.Dispatch(request);
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