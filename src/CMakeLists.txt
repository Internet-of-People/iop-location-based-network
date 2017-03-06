add_library(iop-locnet ../generated/IopLocNet.pb.cc ../extlib/easylogging++.cc
    basic.cpp config.cpp spatialdb.cpp locnet.cpp messaging.cpp network.cpp)
target_include_directories (iop-locnet PUBLIC
    "${CMAKE_SOURCE_DIR}/extlib" "${CMAKE_SOURCE_DIR}/generated")
target_link_libraries (iop-locnet LINK_PUBLIC pthread protobuf sqlite3 spatialite)

# Compiling generated file results a lot of warnings (e.g. unused), suppress them all
set_source_files_properties(../generated/IopLocNet.pb.cc PROPERTIES COMPILE_FLAGS -w)

add_executable (iop-locnetd main.cpp)
target_include_directories (iop-locnetd PUBLIC
    "${CMAKE_SOURCE_DIR}/extlib" "${CMAKE_SOURCE_DIR}/generated")
target_link_libraries (iop-locnetd LINK_PUBLIC iop-locnet)

install (TARGETS iop-locnetd DESTINATION bin)
install (FILES generate-iop-locnet-config.py DESTINATION bin)
