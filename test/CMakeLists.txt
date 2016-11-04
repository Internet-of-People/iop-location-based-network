#include_directories ("${CMAKE_SOURCE_DIR}/src")
add_executable (tests test.cpp testimpls.cpp)
target_include_directories (tests PUBLIC "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/include")
target_link_libraries (tests LINK_PUBLIC iop-locnet)
