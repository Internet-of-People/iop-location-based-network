// #define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <iostream>

#include "testimpls.hpp"

using namespace std;



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
    
    NodeProfile prof("NodeId", "127.0.0.1", 6666, "", 0);
    GIVEN("A node profile object") {
        THEN("its fields are properly filled in") {
            REQUIRE( prof.id() == "NodeId" );
            REQUIRE( prof.ipv4Address() == "127.0.0.1" );
            REQUIRE( prof.ipv4Port() == 6666 );
            REQUIRE( prof.ipv6Address() == "" );
            REQUIRE( prof.ipv6Port() == 0 );
        }
    }
    
    GIVEN("A node profile + location object") {
        NodeLocation nodeLoc(prof, loc);
        THEN("its fields are properly filled in") {
            REQUIRE( nodeLoc.profile().id() == prof.id() );
            REQUIRE( nodeLoc.profile().ipv4Address() == prof.ipv4Address() );
            REQUIRE( nodeLoc.profile().ipv4Port() == prof.ipv4Port() );
            REQUIRE( nodeLoc.profile().ipv6Address() == prof.ipv6Address() );
            REQUIRE( nodeLoc.profile().ipv6Port() == prof.ipv6Port() );
            REQUIRE( nodeLoc.location().latitude() == loc.latitude() );
            REQUIRE( nodeLoc.location().longitude() == loc.longitude() );
        }
    }
    
    GIVEN("A spatial database implementation") {
        DummySpatialDatabase geodb(loc);
        // TODO
    }
}


SCENARIO("Server registration", "")
{
    GIVEN("The location based network") {
        GpsLocation loc(1.0, 2.0);
        shared_ptr<ISpatialDatabase> geodb( new DummySpatialDatabase(loc) );
        shared_ptr<IGeographicNetworkConnectionFactory> connectionFactory( new DummyGeographicNetworkConnectionFactory() );
        GeographicNetwork geonet(geodb, connectionFactory);
        
        WHEN("it's newly created") {
            THEN("it has no registered servers") {
                auto servers = geonet.servers();
                REQUIRE( servers.empty() );
                REQUIRE( servers.find(ServerType::TokenServer) == servers.end() );
                REQUIRE( servers.find(ServerType::MintingServer) == servers.end() );
            }
        }
        ServerInfo tokenServer("Token", "127.0.0.1", 1111, "", 0);
        WHEN("adding servers") {
            ServerInfo minterServer("Minter", "127.0.0.1", 2222, "", 0);
            geonet.RegisterServer(ServerType::TokenServer, tokenServer);
            geonet.RegisterServer(ServerType::MintingServer, minterServer);
            THEN("added servers appear on queries") {
                auto const &servers = geonet.servers();
                REQUIRE( servers.find(ServerType::StunTurnServer) == servers.end() );
                REQUIRE( servers.find(ServerType::TokenServer) != servers.end() );
                REQUIRE( servers.at(ServerType::TokenServer) == tokenServer );
                REQUIRE( servers.find(ServerType::MintingServer) != servers.end() );
                REQUIRE( servers.at(ServerType::MintingServer) == minterServer );
            }
        }
        WHEN("removing servers") {
            ServerInfo minterServer("Minter", "127.0.0.1", 2222, "", 0);
            geonet.RegisterServer(ServerType::TokenServer, tokenServer);
            geonet.RegisterServer(ServerType::MintingServer, minterServer);
            geonet.RemoveServer(ServerType::MintingServer);
            THEN("they disappear from the list") {
                auto const &servers = geonet.servers();
                REQUIRE( servers.find(ServerType::StunTurnServer) == servers.end() );
                REQUIRE( servers.find(ServerType::MintingServer) == servers.end() );
                REQUIRE( servers.find(ServerType::TokenServer) != servers.end() );
                REQUIRE( servers.at(ServerType::TokenServer) == tokenServer );
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
