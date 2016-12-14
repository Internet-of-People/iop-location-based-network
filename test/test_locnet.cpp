#define CATCH_CONFIG_RUNNER
//#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <easylogging++.h>

#include "testdata.hpp"
#include "testimpls.hpp"

using namespace std;
using namespace LocNet;


INITIALIZE_EASYLOGGINGPP



SCENARIO("Construction and behaviour of data holder types", "[types]")
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
    
    NodeProfile prof("NodeId", { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } );
    GIVEN("A node profile object") {
        THEN("its fields are properly filled in") {
            REQUIRE( prof.id() == "NodeId" );
            REQUIRE( prof.contact().addressType() == AddressType::Ipv4 );
            REQUIRE( prof.contact().address() == "127.0.0.1" );
            REQUIRE( prof.contact().port() == 6666 );
        }
    }
    
    GIVEN("A node info object") {
        NodeInfo nodeInfo(prof, loc);
        THEN("its fields are properly filled in") {
            REQUIRE( nodeInfo.profile() == prof );
            REQUIRE( nodeInfo.location() == loc );
        }
    }
}



SCENARIO("Spatial database", "")
{
    GIVEN("A spatial database implementation") {
        SpatiaLiteDatabase geodb(TestData::NodeBudapest,
            SpatiaLiteDatabase::IN_MEMORY_DB, chrono::hours(1) );

        THEN("its initially empty") {
            REQUIRE( geodb.GetNodeCount() == 1 ); // contains only self
            REQUIRE( geodb.GetNeighbourNodesByDistance().empty() );
            REQUIRE_THROWS( geodb.Remove("NonExistingNodeId") );
            
            shared_ptr<NodeDbEntry> self = geodb.Load(TestData::NodeBudapest.profile().id() );
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
            NodeDbEntry entry1( NodeProfile( "ColleagueNodeId1",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ),
                GpsLocation(1.0, 1.0), NodeRelationType::Colleague, NodeContactRoleType::Initiator );
            NodeDbEntry entry2( NodeProfile( "NeighbourNodeId2",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ),
                GpsLocation(2.0, 2.0), NodeRelationType::Neighbour, NodeContactRoleType::Acceptor );
            
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
            geodb.Store(TestData::EntryKecskemet);
            geodb.Store(TestData::EntryLondon);
            geodb.Store(TestData::EntryNewYork);
            geodb.Store(TestData::EntryWien);
            geodb.Store(TestData::EntryCapeTown);

            THEN("closest nodes are properly selected") {
                {
                    vector<NodeInfo> closestNodes = geodb.GetClosestNodesByDistance(
                        TestData::Budapest, 20000.0, 2, Neighbours::Included );
                    REQUIRE( closestNodes.size() == 2 );
                    REQUIRE( closestNodes[0] == TestData::NodeBudapest );
                    REQUIRE( closestNodes[1] == TestData::NodeKecskemet );
                }
                {
                    vector<NodeInfo> closestNodes = geodb.GetClosestNodesByDistance(
                        TestData::Budapest, 20000.0, 1, Neighbours::Excluded );
                    REQUIRE( closestNodes.size() == 1 );
                    REQUIRE( closestNodes[0] == TestData::NodeLondon );
                }
                {
                    vector<NodeInfo> closestNodes = geodb.GetClosestNodesByDistance(
                        TestData::Budapest, 20000.0, 1000, Neighbours::Included );
                    REQUIRE( closestNodes.size() == 6 );
                    REQUIRE( closestNodes[0] == TestData::NodeBudapest );
                    REQUIRE( closestNodes[1] == TestData::NodeKecskemet );
                    REQUIRE( closestNodes[2] == TestData::NodeWien );
                    REQUIRE( closestNodes[3] == TestData::NodeLondon );
                    REQUIRE( closestNodes[4] == TestData::NodeNewYork );
                    REQUIRE( closestNodes[5] == TestData::NodeCapeTown );
                }
                {
                    vector<NodeInfo> closestNodes = geodb.GetClosestNodesByDistance(
                        TestData::Budapest, 5000.0, 1000, Neighbours::Excluded );
                    REQUIRE( closestNodes.size() == 1 );
                    REQUIRE( closestNodes[0] == TestData::NodeLondon );
                }
            }
            
            THEN("random nodes are properly selected") {
                {
                    vector<NodeInfo> randomNodes = geodb.GetRandomNodes(2, Neighbours::Included);
                    REQUIRE( randomNodes.size() == 2 );
                }
                {
                    vector<NodeInfo> randomNodes = geodb.GetRandomNodes(10, Neighbours::Excluded);
                    REQUIRE( randomNodes.size() == 3 );
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), TestData::EntryLondon ) != randomNodes.end() );
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), TestData::EntryNewYork ) != randomNodes.end() );
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), TestData::EntryCapeTown ) != randomNodes.end() );
                }
            }
            
            THEN("farthest neighbour is properly selected") {
                vector<NodeInfo> neighboursByDistance( geodb.GetNeighbourNodesByDistance() );
                REQUIRE( neighboursByDistance.size() == 2 );
                REQUIRE( neighboursByDistance[0] == TestData::EntryKecskemet );
                REQUIRE( neighboursByDistance[1] == TestData::EntryWien );
            }
            
            THEN("Data is properly deleted") {
                REQUIRE( geodb.GetNodeCount() == 6 );
                
                geodb.Remove( TestData::NodeKecskemet.profile().id() );
                geodb.Remove( TestData::NodeLondon.profile().id() );
                geodb.Remove( TestData::NodeNewYork.profile().id() );
                geodb.Remove( TestData::NodeWien.profile().id() );
                geodb.Remove( TestData::NodeCapeTown.profile().id() );
                
                REQUIRE( geodb.GetNodeCount() == 1 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().empty() );
            }
        }
    }
}



SCENARIO("Server registration", "")
{
    GIVEN("The location based network") {
        GpsLocation loc(1.0, 2.0);
        NodeInfo nodeInfo( NodeProfile("NodeId",
            { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ), loc );
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
        ServiceProfile tokenService("Token",
            { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 1111) } );
        WHEN("adding services") {
            ServiceProfile minterService("Minter",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 2222) } );
            geonet.RegisterService(ServiceType::Token, tokenService);
            geonet.RegisterService(ServiceType::Minting, minterService);
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
            ServiceProfile minterService("Minter",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 2222) } );
            geonet.RegisterService(ServiceType::Token, tokenService);
            geonet.RegisterService(ServiceType::Minting, minterService);
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



int main( int, char* const [] )
{
    // Disable logging to prevent flooding the console
    el::Configurations logConf;
    logConf.setToDefault();
    logConf.setGlobally(el::ConfigurationType::Enabled, "false");
    el::Loggers::reconfigureLogger("default", logConf);
    
    // Perform all tests
    Catch::Session session; // There must be exactly once instance

//     // writing to session.configData() here sets defaults
//     // this is the preferred way to set them
// 
//     int returnCode = session.applyCommandLine( argc, argv );
//     if( returnCode != 0 ) // Indicates a command line error
//         { return returnCode; }
// 
//     // writing to session.configData() or session.Config() here 
//     // overrides command line args
//     // only do this if you know you need to

    try { return session.run(); }
    catch (exception &e)
        { cerr << "Caught exception: " << e.what() << endl; }
}
