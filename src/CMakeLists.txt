add_library(iop-location geonet.cpp)

add_executable (iop-located main.cpp)
target_link_libraries (iop-located LINK_PUBLIC iop-location)
