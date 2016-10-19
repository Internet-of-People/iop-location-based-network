#include_directories ("${CMAKE_SOURCE_DIR}/src")
add_executable (tests test.cpp)
target_include_directories (tests PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries (tests LINK_PUBLIC iop-location)
