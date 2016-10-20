#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <iostream>

#include "dummynode.hpp"

using namespace std;


SCENARIO("Constructors", "[types]") {
    GpsLocation pos(1.0, 2.0);
    
    GIVEN("A location object") {
        THEN("its fields are properly filled in") {
            REQUIRE( pos.latitude() == 1.0 );
            REQUIRE( pos.longitude() == 2.0 );
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
}
