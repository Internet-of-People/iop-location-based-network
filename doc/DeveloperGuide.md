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
works fine in practice. That means that nodes can always discover the network,
no network split can happen, etc.


## Technical debt

While prototyping we delayed a lot of minor technical decisions
to come up with an initial version as soon as possible.
Accordingly, you can find a lot of `// TODO` comments in the source
calling to (re)consider algorithms, error handling and other implementations.


## Architecture

During prototyping, features were added layer by layer,
starting from the application layer through messaging down to networking.
Consequently, network.h/cpp currently depends on messaging.h/cpp,
but dependency might be better in the opposite direction: messaging may be seen
as the highest layer connecting business logic (locnet.cpp) with networking (network.cpp).

A reconsideration of the current layering implementation (application: Node, messaging: Dispatcher, session: Session)
is also necessary because features "connection keepalive" and "Ip autodetection"
do not naturally fit into the picture, see the message loop implementation currently in
`ProtoBufDispatchingTcpServer::AsyncAcceptHandler()`.

Interfaces of provided operations should be better separated and checked for proper access rights by
different types of consumers (local service, node and client). This can be achieved
either by exposing different interfaces on different TCP ports or
offering different authentication methods on the same port for different consumer types.

We could improve both code structure and compile times. We could restructuring headers and
using a specific framework header (e.g. asio, ezOptionParser, etc) only in a single h/cpp pair.
We could also use precompiled headers to speed up compilation. We already tried it with the
[cotire CMake plugin](https://github.com/sakra/cotire), but it didn't really seem to make a difference.


## Convenience

To make administration of the Linux version easier, we could separate project files
into separate directories according to conventions of the distribution.
E.g. we could use `~/.config/iop-locnet` and `~/.local/iop-locnet` instead of current `~/.iop-locnet`.

When using config file, options could use format `option=value` instead/besides
the current `--option value` format. If this is needed, we might have to use
a different command line parser library, it might not be supported by ezOptionParser.


## Security

### Encryption and authentication

We should authenticate of nodes of the network and maybe also clients.
We should consider if we transfer sensitive information that should be encrypted.
TLS might be a good candidate for both tasks.

### Robustness and reliability

The main goal of the current implementation is to prove that the concept works.
So far the network is quite vulnerable as it does not implement appropriate protection from attacks.
If we find no conceptual problems we should identify attack vectors and protect from malicious nodes and clients, e.g.

- Protect network interfaces, e.g. LocalService interface should be exposed only to localhost
  and other nodes of the network should be authenticated to be accepted as colleague or neighbour.
- Use and check SHA256 hashes as node identifiers.
- Check integirty of advertised information. E.g. validte host is really accessible
  on the reported external address.
- Ensure that uniqueness of sensitive data (e.g. node ids and probably external addresses)
  is strictly enforced everywhere.
- Ban misbehaving nodes.


## Performance

Socket connections are accepted asynchronously using asio using only a single thread.
However, it is much harder to use async operations to implement interfaces
not designed directly for an async workflow (e.g. NetworkSession).
Consequently, each session currently uses its own separate thread.
If this consumes too much resources (memory and context switch costs),
it might worth to implement sessions asynchronously.
We might either redesign interfaces like session to directly support async operations or
try to implement the same interface with async operations using something like
[Boost stackful courutines](http://www.boost.org/doc/libs/1_62_0/doc/html/boost_asio/overview/core/spawn.html)
but then we would depend also on Boost.

If connections take up too much resources, we could also improve code by expiring accepted
inactive connections. Currently this is done only on the client side (connecting to other nodes) yet.
Implementing this on the accepting side is harder because of the blocking implementation.
Asio uses expiration timers to handle this which work fine most of the time,
so you can just set up a timeout after a minute and the connection will be automatically closed.
However, when the localservice interface receives a GetNeighbourhood request,
it must keep the connection alive and send notifications when neighbourhood changes.
Unfortunately we found no working way to disable the timer if it's already set up.
Using asynch mode one can differentiate the "timer expired" event from the "timer disabled"
event by checking an error code, but with the blocking streaming implementation
we found that the stream is closed even when you disable the timer,
probably this error code is not checked in the implementation as we'd expect.
