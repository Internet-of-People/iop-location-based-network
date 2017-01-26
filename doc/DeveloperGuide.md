# Overview


## Project architecture

Directories, files and sources are organized as follows:
- `doc` contains project documentation like this file
- `extlib` contains external header-only libraries included with our sources
  for deterministic builds and easier deployment
- `generated` contains the ProtoBuf sources generated from the
  [protocol definitions for the location-based network](https://github.com/Internet-of-People/message-protocol/blob/master/IopLocNet.proto)
- `package` contains utility scripts for packaging already compiled sources
  into binary releases.
  - `libprotobuf3` contains scripts to create packages to backport protobuf version 3
    to older distributions where it's not yet available
  - `locnet` contains scripts to pack our own binaries
- `src` contains the source files for this software
  - `config` defines an interface as a collection of all configuration settings
    of the software and provides an implementation that parses options
    from command line or a config file.
  - `basic` `h/cpp` collects the essential typedefs, data holder structures
    and some utility classes used as basic building blocks in other sources
  - `spatialdb` defines an interface of a node database allowing search by location
    and gives an implementation with the SpatiaLite (an SQLite extension) engine
  - `locnet` provides interfaces for all different client types
    (local services, other network nodes and end user clients) and
    implements the application business logic for all interfaces in a single `Node` class
    on top of a spatial database
  - `messaging` provides utilities to convert between basic application data holder classes
    and ProtoBuf messages. Additionally it defines an interface to dispatch requests
    to a peer and return its responses (i.e. hiding the complexity of communication)
    and defines implementations used for serving remote requests and a proxy for
    using services of a potentially remote node without exposing any communication details.
  - `network` implements communication on top of TCP. Defines interfaces
    for client session and a generic Tcp server. Implements a session on
    top of blocking TCP streams and gives a server implementation
    that runs a request dispatch loop for its sessions.
  - `main` instantiates, links and runs all these building blocks that make up the software.
- `test` contains sources to test our software

Note that `basic.hpp` and `locnet.hpp` contain types and interfaces that are
redundant with the classes in `IopLocNet.pb.h` (generated from the ProtoBuf definitions).
This redundancy is intentional: prevents direct dependance of the core application logic
from generated sources used in another layer (messaging) and also
allows extending the sources with additional messaging frameworks if necessary.

## Components and workflow

TODO

## Summary of ProtoBuf definitions

TODO
