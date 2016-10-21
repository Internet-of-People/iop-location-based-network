project (LOCATION_BASED_NETWORK)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# Needed to convince Travis to use proper compiler options
if ( CMAKE_COMPILER_IS_GNUCXX )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14 -Wall -Wextra -pedantic")
endif ( CMAKE_COMPILER_IS_GNUCXX )

add_subdirectory(src)
add_subdirectory(test)
