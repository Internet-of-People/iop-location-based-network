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


## Building the sources

After installing external dependencies, you can just run `build.sh` or perform the same steps
as the script. First you have to generate build files with CMake, specifying the directory
containing our main `CMakeLists.txt` file. We suggest running the build in a dedicated directory,
the provided script uses directory `build`. Thus from the project root you can run

    mkdir build
    cd build
    cmake ..

The steps afterwards are more platform-dependent. E.g. CMake probably generates
Visual Studio project files on Windows, so you have to run the Visual Studio C++ compiler.
For Linux, a `Makefile` is generated, so you can just execute

    make

Assuming you followed the suggested directory structure, you should find executable
file `src/iop-locnetd` created under your `build` directory, it is all you need.
If you also want to install the software to your system directories, you have to run

    sudo make install


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
