#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <iostream>

#include "node.h"

using namespace std;


SCENARIO("Node constructor", "[constructor]") {
    Node node("NodeId", 1.0, 2.0);
    REQUIRE( node.id() == "NodeId" );
    REQUIRE( node.latitude() == 1.0 );
    REQUIRE( node.longitude() == 2.0 );
}
