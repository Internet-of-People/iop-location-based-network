add_executable (test test.cpp)
target_include_directories (test PUBLIC ../src)
target_link_libraries (test LINK_PUBLIC iop-location)
