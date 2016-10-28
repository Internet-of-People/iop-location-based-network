#include "testimpls.hpp"

using namespace std;



DummySpatialDatabase::DummySpatialDatabase(const GpsLocation& myLocation):
    _myLocation(myLocation) {}



Distance DummySpatialDatabase::GetDistance(const GpsLocation&, const GpsLocation&) const
{
    // TODO
    return 0;
}


bool DummySpatialDatabase::Store(const NodeInfo&, LocNetRelationType, RelationType)
{
    // TODO
    return true;
}


shared_ptr<NodeInfo> DummySpatialDatabase::Load(const string&) const
{
    // TODO
    return shared_ptr<NodeInfo>(
        new NodeInfo( NodeProfile("NodeId", "", 0, "", 0),
                          GpsLocation(0., 0.) ) );
}


bool DummySpatialDatabase::Update(const NodeInfo&) const
{
    // TODO
    return true;
}


bool DummySpatialDatabase::Remove(const string&)
{
    // TODO
    return true;
}


Distance DummySpatialDatabase::GetNeighbourhoodRadiusKm() const
{
    // TODO
    return 42.;
}


size_t DummySpatialDatabase::GetNodeCount(LocNetRelationType) const
{
    // TODO
    return 1;
}



vector<NodeInfo> DummySpatialDatabase::GetClosestNodes(
    const GpsLocation&, Distance, size_t, Neighbours) const
{
    // TODO
    return vector<NodeInfo>();
}


std::vector<NodeInfo>DummySpatialDatabase::GetRandomNodes(size_t, Neighbours) const
{
    // TODO
    return vector<NodeInfo>{ NodeInfo( NodeProfile("RandomNodeId", "", 0, "", 0),
                                       GpsLocation(0., 0.) ) };
}



shared_ptr<ILocNetRemoteNode> DummyGeographicNetworkConnectionFactory::ConnectTo(const NodeProfile&)
{
    return shared_ptr<ILocNetRemoteNode>();
}

