# Future work and improvements

The project is currently a proof-of-concept prototype state.
Though the source code is supposed to be functional, there is a lot of room for improvements.


## Algorithmic correctness

Currently we are testing the network with the community.
We should also build a test environment that runs a whole network (i.e. lots of nodes)
on a single host to prove that the base algorithm and its current implementation
works fine in practice. That means that nodes should always discover the network,
no network split should happen without network blocks, a splitted network
should automatically reunite when network blocking is lifted, etc.


## Technical debt

While prototyping we delayed a lot of minor technical decisions
to come up with an initial version as soon as possible.
Accordingly, you can find a lot of `// TODO` comments in the source
calling to (re)consider algorithms, error handling and other implementations.

Though using the easylogging++ library is very convenient, we rarely experienced a strange behaviour.
To be thread-safe it needs to have `#define ELPP_THREAD_SAFE` in all files where it's used,
currently this is set as compiler option with cmake. However, we experienced very rare and undeterministic
errors (e.g. "pure virtual method called", probably it calls a method of an uninitialized derived class
during construction) with a stacktrace pointing to construction of logger objects.
Releases later than the currently used 9.89 may have already fixed this,
but they also changed to from the header-only model to splitted h/cpp compilation,
thus upgrading would also need changes in our project structure.
Currently we didn't cause problems, so upgrade was delayed.


## Architecture

Currently changes (e.g. changed host IP or new registered profile server on this node)
are spread within the network only during node relation renewals. It may worth considering
to broadcast changed IPs or services immediately if during tests we find that
slow refreshes cause an unsatisfying network behaviour.

Our current implementation uses a local database to store just a sparse subset of all the nodes
of the network, but this is not necessarily the only direction.
We could also experiment with using a distributed hash table (DHT).
The advantage would be having a single, full node list of the whole network,
readable by every node. Each node could write only its own details
(e.g. when joining the network or changing its IP addresS).
Note that this direction has shortcomings to solve: as far as we know
it would not work locally if a country blocks external internet access,
see e.g. Great Firewall of China. However, it might be possible to create
our own solution reusing some parts of DHT protocols and forging a custom solution.
We just didn't have enough time and expertise to fully discover this direction.

During prototyping, features were added layer by layer,
starting from the application layer through messaging down to networking.
Consequently, network.h/cpp currently depends on messaging.h/cpp,
but dependency might be better in the opposite direction: messaging may be seen
as the highest layer connecting business logic (locnet.cpp) with networking (network.cpp).

A reconsideration of the current layering implementation (application: Node, messaging: Dispatcher, session: Session)
also might be necessary because features "connection keepalive" and "Ip autodetection"
do not naturally fit into the picture, see the message loop implementation currently in
`ProtoBufDispatchingTcpServer::AsyncAcceptHandler()`.

All of our algorithm implementations (like discovery, relation renewal, etc) are translated
from an algorithmic description in the specification thus currently they are all sequential.
Though some algorithms like discovery naturally have sequential parts,
sequential execution might not scale well for huge networks and we may have to
parallelize or use asynchronous execution where possible.


## Convenience

To make testing network behaviour easier, we should have better diagnostic tools.
One possibility would be to implement sending server statistics or log files either on request
or periodically to the developers.

To make administration of the Linux version easier, we could separate project files
into separate directories according to conventions of the distribution.
E.g. we could use `~/.config/iop-locnet` and `~/.local/iop-locnet` instead of current `~/.iop-locnet`.

When using config file, options could use format `option=value` instead/besides
the current `--option value` format. If this is needed, we might have to use
a different command line parser library, it might not be supported by ezOptionParser.

When starting the very first seed node, there are no other nodes yet,
so it has to identify itself as a seed node. As seed nodes are currently
defined as a list of domain names and host resolution is done automatically
by the network library, we have to manually set option `--seednode myip`
to help it identifying itself, especially as the host cannot easily identify its own
external IP address. This should be somehow automated.


## Security

### Encryption and authentication

We should authenticate the nodes of the network and maybe also clients.
We should consider if we transfer sensitive information that should be encrypted.
TLS might be a good candidate for both tasks.

### Robustness and reliability

The main goal of the current implementation is to prove that the concept works.
So far the network is quite vulnerable as it does not implement appropriate protection from attacks.
If we find no conceptual problems we should identify attack vectors and protect from malicious nodes and clients, e.g.

- Protect network interfaces, e.g. LocalService interface should be exposed only to localhost
  and other nodes of the network should be authenticated to be accepted as colleague or neighbour.
- Use and check SHA256 hashes as node identifiers.
- Check integrity of advertised information. E.g. validate if host is really accessible
  on the reported external address and address matches to its reported location.
- Ensure that uniqueness of sensitive data (e.g. node ids and probably external addresses)
  is strictly enforced everywhere and cannot be impersonated.
- Ban misbehaving nodes.


## Optimization and Performance

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

We could improve both code structure and compile times. One direction could be restructuring
sources and using a specific framework header (e.g. asio, ezOptionParser, etc)
only in a single h/cpp pair so every huge framework header is compiled only once.
Another approach could be using precompiled headers to speed up compilation.
We already tried the [cotire CMake plugin](https://github.com/sakra/cotire),
but it didn't seem to make a difference.
