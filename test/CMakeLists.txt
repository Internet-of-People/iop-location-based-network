add_executable (tests test.cpp)
target_include_directories (tests PUBLIC ../src)
target_link_libraries (tests LINK_PUBLIC iop-location)
