add_library(iop-location node.cpp)

add_executable (iop-located main.cpp)
target_link_libraries (iop-located LINK_PUBLIC iop-location)
