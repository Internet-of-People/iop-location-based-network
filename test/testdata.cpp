#include "testdata.hpp"



namespace LocNet
{

GpsLocation TestData::Budapest(47.4808706,18.849426);
GpsLocation TestData::Kecskemet(46.8854726,19.538628);
GpsLocation TestData::Wien(48.2205998,16.2399759);
GpsLocation TestData::London(51.5283063,-0.3824722);
GpsLocation TestData::NewYork(40.7053094,-74.2588858);
GpsLocation TestData::CapeTown(-33.9135236,18.0941875);


NodeInfo TestData::NodeBudapest( NodeProfile("BudapestId",
    { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ), Budapest );
NodeInfo TestData::NodeKecskemet( NodeProfile("KecskemetId",
    { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ), Kecskemet );
NodeInfo TestData::NodeWien( NodeProfile("WienId",
    { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ), Wien );
NodeInfo TestData::NodeLondon( NodeProfile("LondonId",
    { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ), London );
NodeInfo TestData::NodeNewYork( NodeProfile("NewYorkId",
    { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ), NewYork );
NodeInfo TestData::NodeCapeTown( NodeProfile("CapeTownId",
    { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6666) } ), CapeTown );

NodeDbEntry TestData::EntryKecskemet( NodeKecskemet,
    NodeRelationType::Neighbour, NodeContactRoleType::Initiator );
NodeDbEntry TestData::EntryWien( NodeWien,
    NodeRelationType::Neighbour, NodeContactRoleType::Initiator );
NodeDbEntry TestData::EntryLondon( NodeLondon,
    NodeRelationType::Colleague, NodeContactRoleType::Initiator );
NodeDbEntry TestData::EntryNewYork( NodeNewYork,
    NodeRelationType::Colleague, NodeContactRoleType::Acceptor );
NodeDbEntry TestData::EntryCapeTown( NodeCapeTown,
    NodeRelationType::Colleague, NodeContactRoleType::Acceptor );

} // namespace LocNet
