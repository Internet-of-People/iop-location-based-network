add_library(iop-locnet basic.cpp spatialdb.cpp locnet.cpp)

add_executable (iop-locnetd main.cpp)
target_link_libraries (iop-locnetd LINK_PUBLIC iop-locnet)
