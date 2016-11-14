add_library(iop-locnet ../generated/IopLocNet.pb.cc basic.cpp spatialdb.cpp locnet.cpp messaging.cpp)
target_include_directories (iop-locnet PUBLIC
    "${CMAKE_SOURCE_DIR}/extlib" "${CMAKE_SOURCE_DIR}/generated")

# Compiling generated file results a lot of warnings (e.g. unused), suppress them all
set_source_files_properties(../generated/IopLocNet.pb.cc PROPERTIES COMPILE_FLAGS -w)

add_executable (iop-locnetd main.cpp)
target_include_directories (iop-locnetd PUBLIC
    "${CMAKE_SOURCE_DIR}/extlib" "${CMAKE_SOURCE_DIR}/generated")
target_link_libraries (iop-locnetd LINK_PUBLIC iop-locnet)
