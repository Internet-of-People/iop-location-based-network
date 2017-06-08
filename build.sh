echo Generating makefiles
rm -rf build
mkdir build
cd build
cmake ..
echo Compiling all sources
make -j4
echo Running tests
test/tests
cd ..
