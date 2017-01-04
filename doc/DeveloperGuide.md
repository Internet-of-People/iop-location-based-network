# Overview


## Project architecture

TODO 


## Components and workflow

TODO

## Summary of ProtoBuf definitions

TODO


# Future work and improvements

The project is currently a proof-of-concept prototype state.
Though the source code is supposed to be functional, there is a lot of room for improvements.


## Algorithmic correctness

Currently we are testing the network with the community.
We should also build a test environment that runs a whole network (i.e. lots of nodes)
on a single host to prove that the base algorithm and its current implementation
works fine in practice.


## Technical debt

While prototyping we delayed a lot of minor technical decisions
to come up with an initial version as soon as possible.
Accordingly, you can find a lot of `// TODO` comments in the source
calling to (re)consider algorithms, error handling and other implementations.

### Error handling

TODO Create own exception hierarchy so as we can return proper status codes for the messaging interface


## Architecture

During prototyping, features were added layer by layer,
starting from the application layer through messaging down to networking.
Consequently, network.h/cpp currently depends on messaging.h/cpp,
but dependency might be better in the opposite direction: messaging may be seen
as the highest layer connecting business logic (locnet.cpp) with networking (network.cpp).

TODO separate network interfaces and check client access rights for different client types
(local service, node and client).


## Convenience

TODO should automatically detect external address to be advertised for clients
instead of mandatory parameter on the command line. We could either use an external service
(like http://whatismyipaddress.com/) or nodes could report the ip address to each other
when connected.

TODO Probably should use ~/.config/iop-locnet and ~/.local/iop-locnet
instead of current ~/.iop-locnet

When using config file, options could use format `option=value` instead/besides
the current `--option value` format. If this is needed, we might have to use
a different command line parser library, it might not be supported by ezOptionParser.


## Security

### Encryption and authentication

TODO Use TLS for encryption and/or authentication of nodes and clients

### Robustness and reliability

TODO Identify attack vectors and protect from malicious nodes and clients, e.g.

- Protect network interfaces, e.g. LocalService interface should be exposed only to localhost
  and other nodes of the network should be authenticated to be accepted as colleague or neighbour
- Use and check SHA256 hashes as node identifiers.
- Check if provided node data is real, e.g. host is really accessible
  on the reported network interface
- Better enforce uniqueness of node ids and external addresses.


## Performance

Socket connections are accepted asynchronously using asio using only a single thread.
However, it is much harder to use async operations to implement interfaces
not designed directly for an async workflow (e.g. NetworkSession).
Consequently, each session currently uses its own separate thread.
If this consumes to much resources (memory and context switch costs),
it might worth to implement sessions asynchronously.
We might either redesign interfaces like session to directly support async operations or
try to implement the same interface with async operations using something like
[Boost stackful courutines](http://www.boost.org/doc/libs/1_62_0/doc/html/boost_asio/overview/core/spawn.html)
but then we would depend also on Boost.
