project (LOCATION_BASED_NETWORK)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# Needed to convince Travis to use proper compiler options
if ( CMAKE_COMPILER_IS_GNUCXX )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -Wextra -pedantic -ansi")
endif ( CMAKE_COMPILER_IS_GNUCXX )

link_directories(/usr/local/lib)

add_subdirectory(src)
add_subdirectory(test)
