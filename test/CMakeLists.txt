include_directories ("${CMAKE_SOURCE_DIR}/src")
add_executable (tests test.cpp)
target_link_libraries (tests LINK_PUBLIC iop-location)
