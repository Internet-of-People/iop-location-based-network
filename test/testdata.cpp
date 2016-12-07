#include "testdata.hpp"



namespace LocNet
{

GpsLocation TestData::Budapest(47.497912,19.040235);
GpsLocation TestData::Kecskemet(46.8963711,19.6896861);
GpsLocation TestData::Wien(48.2081743,16.3738189);
GpsLocation TestData::London(51.5073509,-0.1277583);
GpsLocation TestData::NewYork(40.741895,-73.989308);
GpsLocation TestData::CapeTown(-33.9248685,18.4240553);


NodeInfo TestData::NodeBudapest( NodeProfile("BudapestId",
    NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6371) ), Budapest );
NodeInfo TestData::NodeKecskemet( NodeProfile("KecskemetId",
    NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6372) ), Kecskemet );
NodeInfo TestData::NodeWien( NodeProfile("WienId",
    NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6373) ), Wien );
NodeInfo TestData::NodeLondon( NodeProfile("LondonId",
    NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6374) ), London );
NodeInfo TestData::NodeNewYork( NodeProfile("NewYorkId",
    NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6375) ), NewYork );
NodeInfo TestData::NodeCapeTown( NodeProfile("CapeTownId",
    NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6376) ), CapeTown );

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
