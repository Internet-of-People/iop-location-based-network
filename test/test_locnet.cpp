#include <catch.hpp>
#include <easylogging++.h>

#include "testdata.hpp"
#include "testimpls.hpp"

using namespace std;
using namespace LocNet;



SCENARIO("Construction and behaviour of data holder types", "[basic][logic]")
{
    GIVEN("A successful code block") {
        bool onExit     = false;
        bool onSuccess  = false;
        bool onError    = false;
        THEN("Scope guards work fine") {
            {
                scope_exit    ex(  [&onExit]    { onExit    = true; } );
                scope_error   err( [&onError]   { onError   = true; } );
                scope_success suc( [&onSuccess] { onSuccess = true; } );
            }
            REQUIRE( onExit );
            REQUIRE( onSuccess );
            REQUIRE( ! onError );
        }
    }

    GIVEN("A failing code block") {
        bool onExit     = false;
        bool onSuccess  = false;
        bool onError    = false;
        THEN("Scope guards work fine") {
            try {
                scope_exit    ex(  [&onExit]    { onExit    = true; } );
                scope_error   err( [&onError]   { onError   = true; } );
                scope_success suc( [&onSuccess] { onSuccess = true; } );
                throw runtime_error("Some error occured in this block");
            } catch (...) {} // We don't care about the exception, it's just used for testing scope guards
            REQUIRE( onExit );
            REQUIRE( ! onSuccess );
            REQUIRE( onError );
        }
    }
    
    GpsLocation loc(1.0, 2.0);
    GIVEN("A location object") {
        THEN("its fields are properly filled in") {
            REQUIRE( loc.latitude() == 1.0 );
            REQUIRE( loc.longitude() == 2.0 );
        }
    }
    
    GIVEN("A location object with invalid coordinates") {
        THEN("it will throw") {
            REQUIRE_THROWS( GpsLocation(100.0, 1.0) );
        }
    }
    
    GIVEN("A node info object") {
        NodeInfo node( "NodeId", loc, NodeContact("127.0.0.1", 6666, 7777) );
        THEN("its fields are properly filled in") {
            REQUIRE( node.id() == "NodeId" );
            REQUIRE( node.location() == loc );
            
            NodeContact &contact = node.contact();
            REQUIRE( contact.address() == "127.0.0.1" );
            REQUIRE( contact.nodeEndpoint().isLoopback() );
            REQUIRE( contact.nodePort() == 6666 );
            REQUIRE( contact.clientPort() == 7777 );
            
            contact.address( Address("1.2.3.4") );
            
            REQUIRE( contact.address() == "1.2.3.4" );
            REQUIRE( ! contact.nodeEndpoint().isLoopback() );
        }
    }
}



SCENARIO("Spatial database", "[spatialdb][logic]")
{
    GIVEN("A spatial database implementation") {
        SpatiaLiteDatabase geodb(TestData::NodeBudapest,
            SpatiaLiteDatabase::IN_MEMORY_DB, chrono::hours(1) );

        THEN("its initially empty") {
            REQUIRE( geodb.GetNodeCount() == 1 ); // contains only self
            REQUIRE( geodb.GetNeighbourNodesByDistance().empty() );
            REQUIRE_THROWS( geodb.Remove("NonExistingNodeId") );
            
            shared_ptr<NodeDbEntry> self = geodb.Load(TestData::NodeBudapest.id() );
            REQUIRE( self );
            REQUIRE( static_cast<NodeInfo>(*self) == TestData::NodeBudapest );
        }

        Distance Budapest_Kecskemet = geodb.GetDistanceKm(TestData::Budapest, TestData::Kecskemet);
        Distance Budapest_Wien = geodb.GetDistanceKm(TestData::Budapest, TestData::Wien);
        Distance Budapest_London = geodb.GetDistanceKm(TestData::Budapest, TestData::London);
        Distance Budapest_NewYork = geodb.GetDistanceKm(TestData::Budapest, TestData::NewYork);
        Distance Budapest_CapeTown = geodb.GetDistanceKm(TestData::Budapest, TestData::CapeTown);
        
        THEN("it can measure distances") {
            REQUIRE( Budapest_Kecskemet == Approx(83.03).epsilon(0.01) );
            REQUIRE( Budapest_Wien == Approx(214.59).epsilon(0.007) );
            REQUIRE( Budapest_London == Approx(1453.28).epsilon(0.005) );
            REQUIRE( Budapest_NewYork == Approx(7023.15).epsilon(0.005) );
            REQUIRE( Budapest_CapeTown == Approx(9053.66).epsilon(0.005) );
        }
        
        WHEN("adding nodes") {
            NodeDbEntry entry1( NodeInfo( "ColleagueNodeId1", GpsLocation(1.0, 1.0),
                NodeContact("127.0.0.1", 6666, 7777) ),
                    NodeRelationType::Colleague, NodeContactRoleType::Initiator );
            NodeDbEntry entry2( NodeInfo( "NeighbourNodeId2", GpsLocation(2.0, 2.0),
                NodeContact("127.0.0.1", 8888, 9999) ),
                    NodeRelationType::Neighbour, NodeContactRoleType::Acceptor );
            
            geodb.Store(entry1);
            geodb.Store(entry2);
            
            THEN("they can be queried and removed") {
                REQUIRE( geodb.GetNodeCount() == 3 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().size() == 1 );
                REQUIRE_THROWS( geodb.Remove("NonExistingNodeId") );
                
                geodb.Remove("ColleagueNodeId1");
                REQUIRE( geodb.GetNodeCount() == 2 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().size() == 1 );
                geodb.Remove("NeighbourNodeId2");
                
                REQUIRE_THROWS( geodb.Remove("NonExistingNodeId") );
                REQUIRE( geodb.GetNodeCount() == 1 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().empty() );
            }
        }
        
        WHEN("when having several nodes") {
            shared_ptr<ChangeCounter> listener( new ChangeCounter("TestListenerId") );
            geodb.changeListenerRegistry().AddListener(listener);
            
            REQUIRE( listener->addedCount == 0 );
            REQUIRE( listener->updatedCount == 0 );
            REQUIRE( listener->removedCount == 0 );
            
            geodb.Store(TestData::EntryKecskemet);
            geodb.Store(TestData::EntryLondon);
            geodb.Store(TestData::EntryNewYork);
            geodb.Store(TestData::EntryWien);
            geodb.Store(TestData::EntryCapeTown);

            REQUIRE( listener->addedCount == 5 );
            REQUIRE( listener->updatedCount == 0 );
            REQUIRE( listener->removedCount == 0 );
            
            THEN("closest nodes are properly selected") {
                {
                    vector<NodeDbEntry> closestNodes = geodb.GetClosestNodesByDistance(
                        TestData::Budapest, 20000.0, 2, Neighbours::Included );
                    REQUIRE( closestNodes.size() == 2 );
                    REQUIRE( closestNodes[0] == TestData::EntryBudapest);
                    REQUIRE( closestNodes[1] == TestData::EntryKecskemet );
                }
                {
                    vector<NodeDbEntry> closestNodes = geodb.GetClosestNodesByDistance(
                        TestData::Budapest, 20000.0, 1, Neighbours::Excluded );
                    REQUIRE( closestNodes.size() == 1 );
                    REQUIRE( closestNodes[0] == TestData::EntryLondon );
                }
                {
                    vector<NodeDbEntry> closestNodes = geodb.GetClosestNodesByDistance(
                        TestData::Budapest, 20000.0, 1000, Neighbours::Included );
                    REQUIRE( closestNodes.size() == 6 );
                    REQUIRE( closestNodes[0] == TestData::EntryBudapest );
                    REQUIRE( closestNodes[1] == TestData::EntryKecskemet );
                    REQUIRE( closestNodes[2] == TestData::EntryWien );
                    REQUIRE( closestNodes[3] == TestData::EntryLondon );
                    REQUIRE( closestNodes[4] == TestData::EntryNewYork );
                    REQUIRE( closestNodes[5] == TestData::EntryCapeTown );
                }
                {
                    vector<NodeDbEntry> closestNodes = geodb.GetClosestNodesByDistance(
                        TestData::Budapest, 5000.0, 1000, Neighbours::Excluded );
                    REQUIRE( closestNodes.size() == 1 );
                    REQUIRE( closestNodes[0] == TestData::EntryLondon );
                }
            }
            
            THEN("random nodes are properly selected") {
                {
                    vector<NodeDbEntry> randomNodes = geodb.GetRandomNodes(2, Neighbours::Included);
                    REQUIRE( randomNodes.size() == 2 );
                }
                {
                    vector<NodeDbEntry> randomNodes = geodb.GetRandomNodes(10, Neighbours::Excluded);
                    REQUIRE( randomNodes.size() == 3 );
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), TestData::EntryLondon ) != randomNodes.end() );
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), TestData::EntryNewYork ) != randomNodes.end() );
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), TestData::EntryCapeTown ) != randomNodes.end() );
                }
            }
            
            THEN("Neighbours are properly listed by distance") {
                vector<NodeDbEntry> neighboursByDistance( geodb.GetNeighbourNodesByDistance() );
                REQUIRE( neighboursByDistance.size() == 2 );
                REQUIRE( neighboursByDistance[0] == TestData::EntryKecskemet );
                REQUIRE( neighboursByDistance[1] == TestData::EntryWien );
            }
            THEN("Data is properly updated and deleted") {
                REQUIRE( geodb.GetNodeCount() == 6 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().size() == 2 );
                
                NodeDbEntry updatedLondonEntry(TestData::NodeLondon,
                    NodeRelationType::Neighbour, NodeContactRoleType::Initiator);
                geodb.Update(updatedLondonEntry);
                
                shared_ptr<NodeDbEntry> londonEntry = geodb.Load( TestData::NodeLondon.id() );
                REQUIRE( *londonEntry == updatedLondonEntry );
                
                vector<NodeDbEntry> neighboursByDistance( geodb.GetNeighbourNodesByDistance() );
                REQUIRE( geodb.GetNodeCount() == 6 );
                REQUIRE( neighboursByDistance.size() == 3 );
                REQUIRE( neighboursByDistance[0] == TestData::EntryKecskemet );
                REQUIRE( neighboursByDistance[1] == TestData::EntryWien );
                REQUIRE( neighboursByDistance[2] == updatedLondonEntry );

                REQUIRE( listener->addedCount == 5 );
                REQUIRE( listener->updatedCount == 1 );
                REQUIRE( listener->removedCount == 0 );
                
                geodb.Remove( TestData::NodeKecskemet.id() );
                geodb.Remove( TestData::NodeLondon.id() );
                geodb.Remove( TestData::NodeNewYork.id() );
                geodb.Remove( TestData::NodeWien.id() );
                geodb.Remove( TestData::NodeCapeTown.id() );
                
                REQUIRE( listener->addedCount == 5 );
                REQUIRE( listener->updatedCount == 1 );
                REQUIRE( listener->removedCount == 5 );
                
                REQUIRE( geodb.GetNodeCount() == 1 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().empty() );
            }
        }
    }
}



SCENARIO("Server registration", "[localservice][logic]")
{
    GIVEN("The location based network") {
        GpsLocation loc(1.0, 2.0);
        NodeInfo nodeInfo( NodeInfo("NodeId", loc, NodeContact("127.0.0.1", 6666, 7777) ) );
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase(nodeInfo,
            SpatiaLiteDatabase::IN_MEMORY_DB, chrono::hours(1) ) );
        shared_ptr<INodeConnectionFactory> connectionFactory( new DummyNodeConnectionFactory() );
        Node geonet(nodeInfo, geodb, connectionFactory, {});
        
        WHEN("it's newly created") {
            THEN("it has no registered servers") {
                auto services = geonet.GetServices();
                REQUIRE( services.empty() );
                REQUIRE( services.find(ServiceType::Token) == services.end() );
                REQUIRE( services.find(ServiceType::Minting) == services.end() );
            }
        }
        ServiceInfo tokenService(ServiceType::Token, 1111);
        ServiceInfo minterService(ServiceType::Minting, 2222);
        WHEN("adding services") {
            geonet.RegisterService(tokenService);
            geonet.RegisterService(minterService);
            THEN("added servers appear on queries") {
                auto const &services = geonet.GetServices();
                REQUIRE( services.find(ServiceType::Relay) == services.end() );
                REQUIRE( services.find(ServiceType::Token) != services.end() );
                REQUIRE( services.at(ServiceType::Token) == tokenService );
                REQUIRE( services.find(ServiceType::Minting) != services.end() );
                REQUIRE( services.at(ServiceType::Minting) == minterService );
            }
        }
        WHEN("removing servers") {
            geonet.RegisterService(tokenService);
            geonet.RegisterService(minterService);
            geonet.DeregisterService(ServiceType::Minting);
            THEN("they disappear from the list") {
                auto const &services = geonet.GetServices();
                REQUIRE( services.find(ServiceType::Relay) == services.end() );
                REQUIRE( services.find(ServiceType::Minting) == services.end() );
                REQUIRE( services.find(ServiceType::Token) != services.end() );
                REQUIRE( services.at(ServiceType::Token) == tokenService );
            }
        }
    }
}
