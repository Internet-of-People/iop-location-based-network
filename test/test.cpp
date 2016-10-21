#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <iostream>

#include "dummynode.hpp"

using namespace std;


SCENARIO("Constructors", "[types]")
{
    GpsLocation loc(1.0, 2.0);
    GIVEN("A location object") {
        THEN("its fields are properly filled in") {
            REQUIRE( loc.latitude() == 1.0 );
            REQUIRE( loc.longitude() == 2.0 );
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
    
    NodeLocation nodeLoc(prof, loc);
    GIVEN("A node profile + location object") {
        THEN("its fields are properly filled in") {
            REQUIRE( nodeLoc.profile().id() == prof.id() );
            REQUIRE( nodeLoc.profile().ipv4Address() == prof.ipv4Address() );
            REQUIRE( nodeLoc.profile().ipv4Port() == prof.ipv4Port() );
            REQUIRE( nodeLoc.profile().ipv6Address() == prof.ipv6Address() );
            REQUIRE( nodeLoc.profile().ipv6Port() == prof.ipv6Port() );
            REQUIRE( nodeLoc.latitude() == loc.latitude() );
            REQUIRE( nodeLoc.longitude() == loc.longitude() );
        }
    }
    
    GIVEN("A spatial database implementation") {
        SpatialDatabase db;
    }
    
    GIVEN("A location based network object") {
        shared_ptr<SpatialDatabase> db( new SpatialDatabase() );
        GeographicNetwork geonet(db);
        WHEN("it's newly created") {
            THEN("it has no registered servers") {
                auto servers = geonet.servers();
                REQUIRE( servers.empty() );
                REQUIRE( servers.find(ServerType::TokenServer) == servers.end() );
                REQUIRE( servers.find(ServerType::MintingServer) == servers.end() );
            }
        }
        WHEN("adding servers") {
            ServerInfo tokenServer("Token", "127.0.0.1", 1111, "", 0);
            ServerInfo minterServer("Minter", "127.0.0.1", 2222, "", 0);
            geonet.RegisterServer(ServerType::TokenServer, tokenServer);
            geonet.RegisterServer(ServerType::MintingServer, minterServer);
            THEN("they appear on queries") {
                //unordered_map<ServerType,ServerInfo,EnumHasher> servers = geonet.servers();
                auto servers = geonet.servers();
                REQUIRE( servers.find(ServerType::StunTurnServer) == servers.end() );
                REQUIRE( servers.find(ServerType::TokenServer) != servers.end() );
                REQUIRE( servers[ServerType::TokenServer] == tokenServer );
                REQUIRE( servers.find(ServerType::MintingServer) != servers.end() );
                REQUIRE( servers[ServerType::MintingServer] == minterServer );
            }
        }
    }
}
