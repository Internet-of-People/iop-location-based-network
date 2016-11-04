add_library(iop-locnet basic.cpp spatialdb.cpp locnet.cpp)
target_include_directories (iop-locnet PUBLIC "${CMAKE_SOURCE_DIR}/include")

add_executable (iop-locnetd main.cpp)
target_include_directories (iop-locnetd PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries (iop-locnetd LINK_PUBLIC iop-locnet)
