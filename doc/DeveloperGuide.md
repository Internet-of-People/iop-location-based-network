# Overview

This software implements a node instance of the location based network.
Multiple such nodes form and maintain a geographically organized network
that allows searching for nodes and their hosted services
(e.g. profile server) by location.

Creating this implementation we had the following important design goals:
- scale to a huge number of nodes (potentially millions)
- survive both adding/losing a large number of nodes and the
  appearance of a national network block (e.g. the Chinese Firewall)

Consequently we made these decisions:
- there are no distinguished supernodes, all nodes are equal
- the full list of nodes may be too large, thus for nodes it should be enough
  to store only a part of the whole network.
- each node has its own unique local network map that is redundant with the map of other nodes
- nodes should know the geographically closer part of the network better,
  as the distance increases it knows less.

Consequently, nodes implement an algorithm that helps them pick which nodes they
need to know about and which nodes they don’t.
The Network Map of each node consists of two parts with different goals and properties.
The Neighborhood Map aims to maintain a comprehensive list of the closest nodes.
The World Map aims to provide a rough coverage of the rest of the world
outside the neighborhood. To prevent too much data and limit node density,
the World Map picks only a single node from each area.
Covered areas are defined with circular “bubbles” drawn around single nodes that must not overlap.
The World Map uses a changing bubble size to have a denser local and sparser remote node density.
So the greater the area, the bigger bubbles are, i.e. less nodes are included.
Area bubbles are defined autonomously and are specific to the Network Map of each node.
There is no node count limit for the World Map, the increasing size of bubbles
serves as a limit for node counts. For the Neighborhood Map the bubbles may overlap,
so nodes have a maximal count of neighboring nodes.


# Project architecture

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
  - `config.h/cpp` defines an interface collecting all configuration settings
    of the software and provides an implementation that parses options
    from the command line or a config file.
  - `basic` collects the essential typedefs, data holder structures
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


# Components and workflow

TODO
