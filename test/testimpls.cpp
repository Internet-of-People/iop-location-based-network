#include "testimpls.hpp"

using namespace std;



// DummySpatialDatabase::NodeEntry::NodeEntry(const NodeProfile& profile, double latitude, double longitude, bool neighbour):
//     NodeLocation(profile, latitude, longitude), _neighbour(neighbour) {}
// 
// DummySpatialDatabase::NodeEntry::NodeEntry(const NodeProfile& profile, const GpsLocation& location, bool neighbour):
//     NodeLocation(profile, location), _neighbour(neighbour) {}
// 
// bool DummySpatialDatabase::NodeEntry::neighbour() { return _neighbour; }



DummySpatialDatabase::DummySpatialDatabase(const GpsLocation& myLocation):
    _myLocation(myLocation) {}



double DummySpatialDatabase::GetDistance(const GpsLocation& one, const GpsLocation& other) const
{
    // TODO
    return 0;
}


// double DummySpatialDatabase::GetBubbleSize(const GpsLocation& location) const
// {
//     // TODO
//     return 0;
// }


bool DummySpatialDatabase::Store(const NodeInfo& node, bool isNeighbour)
{
    // TODO
    return true;
}


shared_ptr<NodeInfo> DummySpatialDatabase::Load(const string& nodeId) const
{
    // TODO
    return shared_ptr<NodeInfo>(
        new NodeInfo( NodeProfile("NodeId", "", 0, "", 0),
                          GpsLocation(0., 0.) ) );
}


bool DummySpatialDatabase::Update(const NodeInfo& node) const
{
    // TODO
    return true;
}


bool DummySpatialDatabase::Remove(const string& nodeId)
{
    // TODO
    return true;
}


double DummySpatialDatabase::GetNeighbourhoodRadiusKm() const
{
    // TODO
    return 42.;
}


size_t DummySpatialDatabase::GetColleagueNodeCount() const
{
    // TODO
    return 1;
}



vector<NodeInfo> DummySpatialDatabase::GetClosestNodes(
    const GpsLocation& position, double radius, size_t maxNodeCount, bool includeNeighbours) const
{
    // TODO
    return vector<NodeInfo>();
}


std::vector<NodeInfo>DummySpatialDatabase::GetRandomNodes(uint16_t maxNodeCount, bool includeNeighbours) const
{
    // TODO
    return vector<NodeInfo>{ NodeInfo( NodeProfile("RandomNodeId", "", 0, "", 0),
                                       GpsLocation(0., 0.) ) };
}



shared_ptr<IGeographicNetwork> DummyGeographicNetworkConnectionFactory::ConnectTo(const NodeProfile& node)
{
    return shared_ptr<IGeographicNetwork>();
}

