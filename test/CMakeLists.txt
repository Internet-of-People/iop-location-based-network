#include_directories ("${CMAKE_SOURCE_DIR}/src")
add_executable (tests testdata.cpp testimpls.cpp
    test_locnet.cpp test_messaging.cpp test_network.cpp test_main.cpp)
target_include_directories (tests PUBLIC
    "${CMAKE_SOURCE_DIR}/extlib" "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/generated")
target_link_libraries (tests LINK_PUBLIC iop-locnet protobuf)


add_executable (sampleclient sampleclient.cpp testdata.cpp)
target_include_directories (sampleclient PUBLIC
    "${CMAKE_SOURCE_DIR}/extlib" "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/generated")
target_link_libraries (sampleclient LINK_PUBLIC iop-locnet protobuf pthread)


add_executable (sampleserver sampleserver.cpp testdata.cpp testimpls.cpp)
target_include_directories (sampleserver PUBLIC
    "${CMAKE_SOURCE_DIR}/extlib" "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/generated")
target_link_libraries (sampleserver LINK_PUBLIC iop-locnet protobuf pthread)
