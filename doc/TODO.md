# Future work and improvements

The project is currently in a proof-of-concept prototype state.
Though the source code is supposed to be functional, there is a lot of room for improvements.


## Algorithmic correctness

Currently we are testing the network with the community.
We should also build a test environment that runs a whole network (i.e. lots of nodes)
on a single host to prove that the base algorithm and its current implementation
works fine in practice. That means that nodes should always discover the network,
no network split should happen without network blocks, a splitted network
should automatically reunite when network blocking is lifted, etc.


## Technical debt

While prototyping we delayed solving some minor technical issues.
Accordingly, you can find some `// TODO` comments in the source
reminding to (re)consider algorithms, error handling and other details.


## Architecture

Most of the network communication uses asynchronous calls already.
Connecting to a host is still blocking though, this should be transformed as well.
Another potential blocking problem can be the routing implementation for clients
(Node::ExploreNetworkNodesByDistance, see the TODO).

Our current implementation uses a local database to store just a sparse subset of all the nodes
of the network, but this is not necessarily the only direction.
We could also experiment with using a distributed hash table (DHT).
The advantage would be having a single, full node list of the whole network,
readable by every node. Each node could write only its own details
(e.g. when joining the network or changing its IP address).
Note that this direction has shortcomings to solve: as far as we know
it would not work locally if a country blocks external internet access,
see e.g. Great Firewall of China. However, it might be possible to create
our own solution reusing some parts of DHT protocols and forging a custom solution.
We just didn't have enough time and expertise to fully discover this direction.

During prototyping, features were added layer by layer,
starting from the application layer through messaging down to networking.
Previously networking used some ProtoBuf messages in its interfaces,
but it's fixed since. Currently network.h/cpp depends on messaging.h/cpp,
but it should be in the opposite direction, but it needs relocation of some classes
between these files.

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

We should improve performance and safety by expiring accepted
inactive connections. Implementing this will need async timers of Asio.
However, we have to keep in mind that when the localservice interface receives
a GetNeighbourhood request, it must not expire but keep the connection alive
and send notifications when neighbourhood changes.
