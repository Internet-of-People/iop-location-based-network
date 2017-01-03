![alt text](https://raw.githubusercontent.com/Fermat-ORG/media-kit/00135845a9d1fbe3696c98454834efbd7b4329fb/MediaKit/Logotype/fermat_logo_3D/Fermat_logo_v2_readme_1024x466.png "Fermat Logo")

# iop-location-based-network
[![Build Status](https://travis-ci.org/Fermat-ORG/iop-location-based-network.svg?branch=master)](https://travis-ci.org/Fermat-ORG/iop-location-based-network)

## How to compile and Install

Though we mostly used different Ubuntu versions during development,
we use only standard C++11 features and platform-independent libraries.
Consequently, our sources should also work on Windows or Mac OS with minimal efforts.

### Dependencies

The project has multiple dependencies. Some of them are shipped with the sources,
others has to be installed independently.

Header-only dependencies already included in directory `extlib`:
- `easylogging++` 9.89, used for logging in the whole source,
  [download here](https://github.com/easylogging/easyloggingpp)
- `asio` 1.11.0, used for networking,
  [download here](http://think-async.com/Asio/Download)
- `ezOptionParser` 0.2.2, used for handling options from command line and config files,
  [download here](http://ezoptionparser.sourceforge.net/)
- `catch` 1.5.9, used only for testing,
  [download here](https://github.com/philsquared/Catch)

Generated sources already included in directory `generated`:
- ProtoBuf classes used for messaging, generated from the protocol definitions
  [found here](https://raw.githubusercontent.com/Internet-of-People/message-protocol/master/IopLocNet.proto).
  Provided script generated/regenerate.sh also needs a protobuf compiler.

External dependencies to be manually installed on your system:
- A C++11 compiler. Developed with `g++-5` and `g++-6`, but sources are platform independent,
  should work with clang or Visual Studio (maybe minimal changes to CMake project files are needed).
- `cmake` package used to generated Makefiles. 3.x preferred, but 2.8 still should be enough.
- `make` package used to build the project, any version should work.
- `libprotobuf-dev` package, used for messaging between nodes and clients.
  Included sources are generated with 3.x, but using 2.x should not be hard after minimal changes
  (e.g. marking fields optional in the protocol definition). Available as an Ubuntu package or
  [download and compile cpp version manually](https://github.com/google/protobuf)
- `libspatialite-dev` package, any recent version is expected to work. Available as an Ubuntu package or
  [download and compile manually](https://www.gaia-gis.it/fossil/libspatialite/index)


### Building the sources

After installing external dependencies, you can just run `build.sh` or perform the same steps
as the script. First you have to generate build files with CMake, specifying the directory
containing our main `CMakeLists.txt` file. We suggest running the build in a dedicated directory,
the provided script uses directory `build`. Thus from the project root you can run

    mkdir build
    cd build
    cmake ..

The steps afterwards are platform-dependent. E.g. CMake probably generates
Visual Studio project files on Windows, so you have to run the Visual Studio C++ compiler.
For Linux, a `Makefile` is generated, so you can just execute

    make

If you also want to install compiled binaries, you have to run

    sudo make install


### Creating an install package for Linux

There are probably a lot of ways to do this, we preferred using the `checkinstall` package
as an easy and minimalistic tool. As a summary, `checkinstall` simply runs `make install`
in a sandbox and deploys every file into a generated Linux package instead of your filesystem.
You just have to specify a few options to describe a few details of the generated package
like format (deb, rpm, etc), name, package dependencies or developer email and you're done.

Directory `package/locnet` contains a few shell scripts that generate packages
for a few Ubuntu versions. Based on these scripts it should be very easy to generate
a package for your favourite Linux distribution.

Note that we used libprotobuf-dev 3.0 which is available in package repository
only of the latest Ubuntu 16.10, not earlier. To support older releases,
we had to manually create custom libprotobuf-dev 3.0 packages.
For that purpose you can find some shell scripts also using `checkinstall`
in directory `package/libprotobuf3`.
