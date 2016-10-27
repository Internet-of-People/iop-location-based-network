#include "testimpls.hpp"

using namespace std;



DummySpatialDatabase::DummySpatialDatabase(const GpsLocation& myLocation):
    _myLocation(myLocation) {}



Distance DummySpatialDatabase::GetDistance(const GpsLocation& one, const GpsLocation& other) const
{
    // TODO
    return 0;
}


bool DummySpatialDatabase::Store(const NodeInfo& node, NodeType nodeType, RelationType relationType)
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


Distance DummySpatialDatabase::GetNeighbourhoodRadiusKm() const
{
    // TODO
    return 42.;
}


size_t DummySpatialDatabase::GetNodeCount(NodeType nodeType) const
{
    // TODO
    return 1;
}



vector<NodeInfo> DummySpatialDatabase::GetClosestNodes(
    const GpsLocation& position, Distance radius, size_t maxNodeCount, Neighbours filter) const
{
    // TODO
    return vector<NodeInfo>();
}


std::vector<NodeInfo>DummySpatialDatabase::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
{
    // TODO
    return vector<NodeInfo>{ NodeInfo( NodeProfile("RandomNodeId", "", 0, "", 0),
                                       GpsLocation(0., 0.) ) };
}



shared_ptr<IGeographicNetwork> DummyGeographicNetworkConnectionFactory::ConnectTo(const NodeProfile& node)
{
    return shared_ptr<IGeographicNetwork>();
}

