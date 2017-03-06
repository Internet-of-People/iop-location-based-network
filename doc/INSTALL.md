# Installing the packaged version

Packages can be downloaded as `iop-locnet` from the Fermat package repositories.
The Fermat repository is located at `repo.fermat.community` which you can configure
to your package manager. E.g. on Ubuntu add something like below into your
`/etc/apt/sources.list.d`

    deb [arch=amd64] http://repo.fermat.community/ 16.04 main

and accept the repository keys with the following commands

    gpg --keyserver keys.gnupg.net --recv-keys 0CC9EB6DA69C84F4
    gpg -a --export A69C84F4 | sudo apt-key add -

You can find more details about how to configure the repository at
[this document](https://github.com/Fermat-ORG/iop-token/blob/beta/INSTALL.md).


# Starting the application

After you successfully installed or unpacked the binary distribution,
you should start binary `iop-locnetd`. There are a lot of optional arguments,
but also some mandatory options that you must specify.
You can use the command line directly to do so, e.g. assuming GPS location of Budapest,
application can be run with a minimal configuration of

    iop-locnetd --nodeid YourNodeIdHere --latitude 48.2081743 --longitude 16.3738189

To persist configuration, you may save the same options into config file
`~/.iop-locnet/locnet.conf` with content

    --nodeid YourNodeIdHere
    --latitude 48.2081743
    --longitude 16.3738189

and run the executable without any command line options.
You can also have your config file at a custom path and use option
`--configfile` to specify its location on the command line.

To help filling in these details automatically, we provide a small python script
`generate-iop-locnet-config.py` with the binaries. It will generate a unique node id
and query a rough approximation of your GPS location based on your IP address.
It will print generated options to the console, so you can copy-paste them
to your command line or redirect them to your config file as

    generate-iop-locnet-config.py > ~/.iop-locnet/locnet.conf

Note that the application will create its database and log files under `~/.iop-locnet` by default.
If application startup is successful, it will open three sockets and serve requests on them.
By default port 16980 is used for node to node communication to build and maintain the network,
16981 for clients to discover the network and 16982 for local services to register or get the neighbourhood.

Please check available options using

    iop-locnetd --help
    
Currently the following options are useful for configuring your instance:

    --clientport ARG   TCP port to serve client (i.e. end user) queries. Optional,
                       default value: 16981

    --configfile ARG   Path to config file to load options from. Optional, default
                       value: ~/.iop-locnet/iop-locnet.cfg

    --dbpath ARG       Path to db file. Optional, default value:
                       ~/.iop-locnet/locnet.sqlite

    --host ARG         Externally accessible IP address (ipv4 or v6) to be
                       advertised for other nodes or clients. Required for seeds
                       only, autodetected otherwise.

    --latitude ARG     GPS latitude of this server as real number from range
                       (-90,90)

    --localport ARG    TCP port to serve other IoP services running on this node.
                       Optional, default value: 16982

    --logpath ARG      Path to log file. Optional, default value:
                       ~/.iop-locnet/debug.log

    --longitude ARG    GPS longitude of this server as real number from range
                       (-180,180)

    --nodeid ARG       Public node id, should be an SHA256 hash of the public key of
                       this node

    --nodeport ARG     TCP port to serve node to node communication. Optional,
                       default value: 16980

    --seednode ARG     Host name of seed node to be used instead of default seeds.
                       You can repeat this option to define multiple custom seed nodes.


# Using the sources

For conventional usage, the preferred way of installing the software
is using packages or installers created specifically for your operating system.
We suggest compiling your own version for development or packaging for a new platform.

## Dependencies

Though we mostly used different Ubuntu versions during development,
we use only standard C++11 features and platform-independent libraries.
Consequently, our sources should compile and work also on Windows or Mac OS with minimal efforts.

The project has multiple dependencies. Some of them are shipped with the sources,
others has to be installed independently.

Some (mostly header-only) dependencies already included in directory `extlib`:
- `asio` 1.11.0 standalone mode without Boost. Used for networking,
  [download here](http://think-async.com/Asio/Download).
  Note that this library is to be included in the C++ standard soon.
- `easylogging++` 9.94, used for logging in the whole source,
  [download here](https://github.com/easylogging/easyloggingpp)
- `ezOptionParser` 0.2.2 (with minimal changes to compile on Windows, see file history),
  used for handling options from command line and config files,
  [download here](http://ezoptionparser.sourceforge.net/)
- `catch` 1.5.9, used only for testing,
  [download here](https://github.com/philsquared/Catch)

Generated sources already included in directory `generated`:
- ProtoBuf classes used for messaging, generated from the protocol definitions
  [found here](https://raw.githubusercontent.com/Internet-of-People/message-protocol/master/IopLocNet.proto).
  Provided script generated/regenerate.sh also needs a protobuf compiler.

External dependencies to be manually installed on your system:
- A C++11 compiler. Developed with `g++-5` and `g++-6`, but sources are platform independent,
  should work with any standard compiler (maybe minimal changes to CMake project files are needed).
- `cmake` used to generated Makefiles. 3.x preferred, but 2.8 still should be enough.
- `protobuf3` used for messaging between nodes and clients.
  Included sources are generated with 3.x, but using 2.x should not be hard after minimal changes
  (e.g. marking fields optional in the protocol definition). Unless present in your package manager, you have to
  [download and compile cpp version manually](https://github.com/google/protobuf)
- `spatialite` used to persist node data with locations, any recent version is expected to work.
  Available as an Ubuntu package or
  [download and compile manually](https://www.gaia-gis.it/fossil/libspatialite/index)


## Compile on Ubuntu

First you have to install all external dependencies by

    sudo apt-get install git g++ cmake make libprotobuf-dev libspatialite-dev

and then clone the sources using git

    git clone https://github.com/Fermat-ORG/iop-location-based-network.git

After installing successfully, you can just run `build.sh` or perform the same steps
as the script as follows. First you have to generate build files with CMake, specifying the directory
containing our main `CMakeLists.txt` file. We suggest running the build in a dedicated directory,
the provided script uses directory `build`. Thus from the project root you can run

    mkdir build
    cd build
    cmake ..

CMake should have successfully generated a `Makefile`, so you can just execute

    make

Assuming you followed the suggested directory structure, you should find executable
file `src/iop-locnetd` created under your `build` directory, it is all you need.
If you also want to install the software to your system directories, you have to run

    sudo make install


## Compile on Windows


You should already have a version of the Visual Studio C++ compiler installed, we do not detail this here.
Please make sure that you have the directory of the 64bit binaries set on your PATH
(usually `<VisualStudioDirectory>\VC\bin\amd64`), not the 32bit version of the compiler or the linker.
You also have to install the Windows SDK,
[find binaries and instructions here](https://developer.microsoft.com/windows/downloads/windows-10-sdk).

Recent Visual Studio versions have a bundled Git, so you can easily clone the repository either from the IDE
or from the command line as

    git clone https://github.com/Fermat-ORG/iop-location-based-network.git
    
We couldn't easily achieve Windows compilation with CMake, so we're using a custom build system called Maiken.
Sources or some binaries [are available here](https://github.com/Dekken/maiken/).
After compiling or downloading binaries, you have to set up its configuration.
You have to create file `settings.yaml` in directory `<UserDir>/maiken` set up with the proper
directory paths to the Visual Studio and Windows SDK binaries.
E.g. my host has the following directory paths and settings:

    inc:
        C:/Program\ Files\ (x86)/Windows\ Kits/10/Include/10.0.14393.0/ucrt/
        C:/Program\ Files\ (x86)/Windows\ Kits/10/Include/10.0.14393.0/um
        C:/Program\ Files\ (x86)/Windows\ Kits/10/Include/10.0.14393.0/shared
        C:/Program\ Files\ (x86)/Microsoft\ Visual\ Studio\ 14.0/VC/include
    path:
        C:/Program\ Files\ (x86)/Windows\ Kits/10/Lib/10.0.14393.0/um/x64
        C:/Program\ Files\ (x86)/Windows\ Kits/10/Lib/10.0.14393.0/ucrt/x64
        C:/Program\ Files\ (x86)/Microsoft\ Visual\ Studio\ 14.0/VC/lib/amd64
    env:
    - name: PATH
        mode: prepend
        value: C:/Program\ Files\ (x86)/Microsoft\ Visual\ Studio\ 14.0/VC/bin/amd64
    file:
    - type: cpp:cxx:cc:c
        compiler: cl
        archiver: lib
        linker: link

Having the settings set up, all you have to do is to open a console in the
directory of our cloned sources and start the build

    mkn.exe

Binaries will be created in directory `bin\build`. Note that DLLs still have to be copied
from directory `win\lib` to near the generated executable.

<!--
You will also need CMake, at the time we're writing this, the latest stable installer
[can be downloaded here](https://cmake.org/files/v3.7/cmake-3.7.2-win64-x64.msi).

You also have to fetch all external dependent libraries, i.e. ProtoBuf and SpatiaLite.
Unfortunately ProtoBuf does not have native windows binaries for C++, you have to compile them for yourself.
We tried to use ProtoBuf 3.0 first, but it did not compile out of the box with Visual Studio.
Consequently we suggest using the latest release,
[3.2 can be downloaded here](https://github.com/google/protobuf/releases/download/v3.2.0/protobuf-cpp-3.2.0.zip).
After unpacking the sources, you have to run CMake to generate a VS solution file
in its `cmake` directory, then open the solution in VS and compile it.
Having the binaries you may have to run `protoc` to overwrite classes in `generated`
that might have been created with a different version.

Though SQLite has windows binaries, we did not find a nice development package
having libs and header files both included. You can download a single
[sqlite3.h header file from here](https://sqlite.org/2017/sqlite-amalgamation-3170000.zip)
and separately a
[compiled DLL from here](https://sqlite.org/2017/sqlite-dll-win64-x64-3170000.zip).
To be able to link in Visual Studio you have to create a LIB file for the DLL
using the LIB tool from the Visual Studio binaries as

    lib /def:sqlite3.dll

TODO how to properly do Spatialite binaries? The DLLs
[found here](http://www.gaia-gis.it/gaia-sins/windows-bin-x86/mod_spatialite-4.3.0a-win-x86.7z)
do not contain appropriate `init_ex/cleanup_ex` calls, sources compile only if those are removed
or replaced with deprecated init/cleanup calls. Probably that's why the resulted binaries
currently crash on startup.

You will also need git to fetch our latest sources. VS 2015 already has git bundled so you can
either use its GUI or open its git command line to clone the repository.

    git clone https://github.com/Fermat-ORG/iop-location-based-network.git

Then you have to make all ProtoBuf, SQLite and SpatiaLite headers and libs available for the compiler.
You can either change the project files to add header paths as an include directory or
link/copy all the required headers under `extlib`. You should also copy LIB files to
where VS can find them, e.g. to the build directory.

You also have to slightly modify the CMakeLists.txt by removing `pthread` from linked libraries
as it's not available on Windows and Visual Studio has its own threading implementation.
Run CMake to generate a Visual Studio solution, we suggest doing it in a separete `build` directory as

    mkdir build
    cd build
    cmake ..

Then open the solution in VS. Before compiling, you have to set the `NOMINMAX`
preprocessor macro for the project, see Project/Properties/C++/Preprocessor/Preprocessor Definitions.
Otherwise windows system headers will define macros `min` and `max`
and brake standard expressions like `std::numeric_limits<int>::max()`.
Also define `ELPP_THREAD_SAFE` to activate thread-safe mode for logging.
Before building the project, you also have to change the executable format
for the project under Project/Properties/C++/Code Generation/Runtime Library
from the selected "Multithreaded Debug DLL" to "Multithreaded Debug",
otherwise you will have linker errors like
`1>protobuf.lib(int128.obj) : error LNK2038: mismatch detected for 'RuntimeLibrary': value 'MTd_StaticDebug' doesn't match value 'MDd_DynamicDebug' in main.obj.`
After configuring project settings as above, the sources should compile and link fine.
-->


## Creating an install package for Linux

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
