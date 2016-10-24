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


double DummySpatialDatabase::GetBubbleSize(const GpsLocation& location) const
{
    // TODO
    return 0;
}


void DummySpatialDatabase::Store(const NodeLocation& node, bool isNeighbour)
{
    // TODO
}


NodeLocation DummySpatialDatabase::Load(const string& nodeId) const
{
    // TODO
    return NodeLocation( NodeProfile("NodeId", "", 0, "", 0),
                         GpsLocation(0., 0.) );
}


void DummySpatialDatabase::Remove(const string& nodeId)
{
    // TODO
}


double DummySpatialDatabase::GetNeighbourHoodRadiusKm() const
{
    // TODO
    return 42.;
}


vector<NodeLocation> DummySpatialDatabase::GetClosestNodes(
    const GpsLocation& position, double radius, size_t maxNodeCount, bool includeNeighbours) const
{
    // TODO
    return vector<NodeLocation>();
}


std::vector<NodeLocation>DummySpatialDatabase::GetRandomNodes(uint16_t maxNodeCount, bool includeNeighbours) const
{
    // TODO
    return vector<NodeLocation>();
}
