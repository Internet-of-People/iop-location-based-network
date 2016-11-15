#include_directories ("${CMAKE_SOURCE_DIR}/src")
add_executable (tests testdata.cpp testimpls.cpp test_locnet.cpp test_messaging.cpp)
target_include_directories (tests PUBLIC
    "${CMAKE_SOURCE_DIR}/extlib" "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/generated")
target_link_libraries (tests LINK_PUBLIC iop-locnet protobuf)
