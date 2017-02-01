#include "testdata.hpp"



namespace LocNet
{

GpsLocation TestData::Budapest(47.497912,19.040235);
GpsLocation TestData::Kecskemet(46.8963711,19.6896861);
GpsLocation TestData::Wien(48.2081743,16.3738189);
GpsLocation TestData::London(51.5073509,-0.1277583);
GpsLocation TestData::NewYork(40.741895,-73.989308);
GpsLocation TestData::CapeTown(-33.9248685,18.4240553);


NodeInfo TestData::NodeBudapest( "BudapestId", Budapest,
    NodeContact( "127.0.0.1", 6371, 16371), {} );
NodeInfo TestData::NodeKecskemet( "KecskemetId", Kecskemet,
    NodeContact( "127.0.0.1", 6372, 16372), {} );
NodeInfo TestData::NodeWien( "WienId", Wien,
    NodeContact( "127.0.0.1", 6373, 16373), {} );
NodeInfo TestData::NodeLondon( "LondonId", London,
    NodeContact( "127.0.0.1", 6374, 16374), {} );
NodeInfo TestData::NodeNewYork( "NewYorkId", NewYork,
    NodeContact( "127.0.0.1", 6375, 16375), {} );
NodeInfo TestData::NodeCapeTown( "CapeTownId", CapeTown,
    NodeContact( "127.0.0.1", 6376, 16376), {} );

NodeDbEntry TestData::EntryBudapest( NodeBudapest,
    NodeRelationType::Self, NodeContactRoleType::Self );
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
