// #define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "easylogging++.h"
#include "testimpls.hpp"

using namespace std;
using namespace LocNet;


INITIALIZE_EASYLOGGINGPP



SCENARIO("Construction and behaviour of data holder types", "[types]")
{
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
            REQUIRE( prof.contacts().size() == 1 );
            REQUIRE( prof.contacts()[0].addressType() == AddressType::Ipv4 );
            REQUIRE( prof.contacts()[0].address() == "127.0.0.1" );
            REQUIRE( prof.contacts()[0].port() == 6666 );
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
    GpsLocation Budapest(47.4808706,18.849426);
    GpsLocation Kecskemet(46.8854726,19.538628);
    GpsLocation Wien(48.2205998,16.2399759);
    GpsLocation London(51.5283063,-0.3824722);
    GpsLocation NewYork(40.7053094,-74.2588858);
    GpsLocation CapeTown(-33.9135236,18.0941875);

    GIVEN("A spatial database implementation") {
        DummySpatialDatabase geodb(Budapest);

        THEN("its initially empty") {
            REQUIRE( geodb.GetColleagueNodeCount() == 0 );
            REQUIRE( geodb.GetNeighbourNodesByDistance().empty() );
            REQUIRE_THROWS( geodb.Remove("NonExistingNodeId") );
        }

        Distance Budapest_Kecskemet = geodb.GetDistanceKm(Budapest, Kecskemet);
        Distance Budapest_Wien = geodb.GetDistanceKm(Budapest, Wien);
        Distance Budapest_London = geodb.GetDistanceKm(Budapest, London);
        Distance Budapest_NewYork = geodb.GetDistanceKm(Budapest, NewYork);
        Distance Budapest_CapeTown = geodb.GetDistanceKm(Budapest, CapeTown);
        
        THEN("it can measure distances") {
            REQUIRE( Budapest_Kecskemet == Approx(83.56).epsilon(0.01) );
            REQUIRE( Budapest_Wien == Approx(212.24).epsilon(0.007) );
            REQUIRE( Budapest_London == Approx(1449.57).epsilon(0.005) );
            REQUIRE( Budapest_NewYork == Approx(7005.61).epsilon(0.003) );
            REQUIRE( Budapest_CapeTown == Approx(9053.66).epsilon(0.003) );
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
                REQUIRE( geodb.GetColleagueNodeCount() == 1 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().size() == 1 );
                REQUIRE_THROWS( geodb.Remove("NonExistingNodeId") );
                
                geodb.Remove("ColleagueNodeId1");
                REQUIRE( geodb.GetColleagueNodeCount() == 0 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().size() == 1 );
                geodb.Remove("NeighbourNodeId2");
                
                REQUIRE_THROWS( geodb.Remove("NonExistingNodeId") );
                REQUIRE( geodb.GetColleagueNodeCount() == 0 );
                REQUIRE( geodb.GetNeighbourNodesByDistance().empty() );
            }
        }
        
        WHEN("when having several nodes") {
            NodeDbEntry entryKecskemet( NodeProfile("KecskemetId",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ),
                Kecskemet, NodeRelationType::Neighbour, NodeContactRoleType::Initiator );
            NodeDbEntry entryWien( NodeProfile("WienId",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ),
                Wien, NodeRelationType::Neighbour, NodeContactRoleType::Initiator );
            NodeDbEntry entryLondon( NodeProfile("LondonId",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ),
                London, NodeRelationType::Colleague, NodeContactRoleType::Initiator );
            NodeDbEntry entryNewYork( NodeProfile("NewYorkId",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ),
                NewYork, NodeRelationType::Colleague, NodeContactRoleType::Acceptor );
            NodeDbEntry entryCapeTown( NodeProfile("CapeTownId",
                { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ),
                CapeTown, NodeRelationType::Colleague, NodeContactRoleType::Acceptor );
            
            geodb.Store(entryKecskemet);
            geodb.Store(entryLondon);
            geodb.Store(entryNewYork);
            geodb.Store(entryWien);
            geodb.Store(entryCapeTown);

            THEN("closest nodes are properly selected") {
                {
                    vector<NodeInfo> closestNodes = geodb.GetClosestNodesByDistance(
                        Budapest, 20000.0, 1, Neighbours::Included );
                    REQUIRE( closestNodes.size() == 1 );
                    REQUIRE( closestNodes[0] == entryKecskemet );
                }
                {
                    vector<NodeInfo> closestNodes = geodb.GetClosestNodesByDistance(
                        Budapest, 20000.0, 1, Neighbours::Excluded );
                    REQUIRE( closestNodes.size() == 1 );
                    REQUIRE( closestNodes[0] == entryLondon );
                }
                {
                    vector<NodeInfo> closestNodes = geodb.GetClosestNodesByDistance(
                        Budapest, 20000.0, 1000, Neighbours::Included );
                    REQUIRE( closestNodes.size() == 5 );
                    REQUIRE( closestNodes[0] == entryKecskemet );
                    REQUIRE( closestNodes[1] == entryWien );
                    REQUIRE( closestNodes[2] == entryLondon );
                    REQUIRE( closestNodes[3] == entryNewYork );
                    REQUIRE( closestNodes[4] == entryCapeTown );
                }
                {
                    vector<NodeInfo> closestNodes = geodb.GetClosestNodesByDistance(
                        Budapest, 5000.0, 1000, Neighbours::Excluded );
                    REQUIRE( closestNodes.size() == 1 );
                    REQUIRE( closestNodes[0] == entryLondon );
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
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), entryLondon ) != randomNodes.end() );
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), entryNewYork ) != randomNodes.end() );
                    REQUIRE( find( randomNodes.begin(), randomNodes.end(), entryCapeTown ) != randomNodes.end() );
                }
            }
            
            THEN("farthest neighbour is properly selected") {
                vector<NodeInfo> neighboursByDistance( geodb.GetNeighbourNodesByDistance() );
                REQUIRE( neighboursByDistance.size() == 2 );
                REQUIRE( neighboursByDistance[0] == entryKecskemet );
                REQUIRE( neighboursByDistance[1] == entryWien );
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
        shared_ptr<ISpatialDatabase> geodb( new DummySpatialDatabase(loc) );
        shared_ptr<IRemoteNodeConnectionFactory> connectionFactory( new DummyLocNetRemoteNodeConnectionFactory() );
        Node geonet(nodeInfo, geodb, connectionFactory, true);
        
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



// int main( int argc, char* const argv[] )
// {
//     Catch::Session session; // There must be exactly once instance
// 
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
// 
//     try {
//         return session.run();
//     } catch (exception &e) {
//         cerr << "Caught exception: " << e.what() << endl;
//     }
// }
