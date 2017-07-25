project (LOCATION_BASED_NETWORK)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -Wextra -pedantic -DELPP_THREAD_SAFE -DASIO_STANDALONE")

link_directories(/usr/local/lib)

add_subdirectory(src)
add_subdirectory(test)
