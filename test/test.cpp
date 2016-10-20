#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <iostream>

#include "dummynode.hpp"

using namespace std;


SCENARIO("Node construction", "[constructor]") {
    GpsLocation position(1.0, 2.0);
    IGpsLocation &ipos = static_cast<IGpsLocation&>(position);
    NodeProfile node("NodeId", ipos);
    
    GIVEN("A node object with its position") {
        THEN("Position fields are properly filled in") {
            REQUIRE( ipos.latitude() == 1.0 );
            REQUIRE( ipos.longitude() == 2.0 );
        }
        
        INodeProfile &inode = static_cast<INodeProfile&>(node);
        THEN("Node fields are properly filled in") {
            REQUIRE( inode.id() == "NodeId" );
            REQUIRE( inode.latitude() == 1.0 );
            REQUIRE( inode.longitude() == 2.0 );
        }
    }
}
