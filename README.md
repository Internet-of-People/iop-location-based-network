![alt text](https://raw.githubusercontent.com/Fermat-ORG/media-kit/00135845a9d1fbe3696c98454834efbd7b4329fb/MediaKit/Logotype/fermat_logo_3D/Fermat_logo_v2_readme_1024x466.png "Fermat Logo")

# iop-location-based-network
[![Build Status](https://travis-ci.org/Fermat-ORG/iop-location-based-network.svg?branch=master)](https://travis-ci.org/Fermat-ORG/iop-location-based-network)

## Use the packaged version

Packages can be downloaded from the Fermat package repositories.
You can either download manually from http://repo.fermat.community/
or use the very same URL as a repository for your package manager,
e.g. on Ubuntu add something like below into your `/etc/apt/sources.list.d`

    deb [arch=amd64] http://repo.fermat.community/ 16.04 main

More details are described in
[this document](https://github.com/Fermat-ORG/iop-token/blob/beta/INSTALL.md).

After you successfully installed or unpacked the binary distribution,
you should start binary `iop-locnetd`. Please check available options running

    iop-locnetd --help

There are some optional arguments and also mandatory arguments that you have to specify.
For configuration options you can use either the command line
(assuming GPS location of Budapest)

    iop-locnetd --nodeid YourNodeIdHere \
                --host your.external.host.address \
                --latitude 48.2081743 \
                --longitude 16.3738189

or you can save the options in exactly the same format into config file
`/home/myuser/.iop-locnet/locnet.conf`
and run the executable without any command line option.


## Use the source, Luke

For conventional usage, the preferred way of installing the software
is using packages or installers created specifically for your operating system.
We suggest compiling your own version for development or packaging for a new platform.

### Dependencies

Though we mostly used different Ubuntu versions during development,
we use only standard C++11 features and platform-independent libraries.
Consequently, our sources should compile and work also on Windows or Mac OS with minimal efforts.

The project has multiple dependencies. Some of them are shipped with the sources,
others has to be installed independently.

Header-only dependencies already included in directory `extlib`:
- `asio` 1.11.0 standalone mode without Boost. Used for networking,
  [download here](http://think-async.com/Asio/Download).
  Note that this library is to be included in the C++ standard soon.
- `easylogging++` 9.89, used for logging in the whole source,
  [download here](https://github.com/easylogging/easyloggingpp)
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
