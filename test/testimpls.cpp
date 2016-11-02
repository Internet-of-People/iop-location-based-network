#include "testimpls.hpp"

using namespace std;



DummySpatialDatabase::DummySpatialDatabase(const GpsLocation& myLocation):
    _myLocation(myLocation) {}



Distance DummySpatialDatabase::GetDistance(const GpsLocation&, const GpsLocation&) const
{
    // TODO
    return 0;
}


bool DummySpatialDatabase::Store(const LocNetNodeDbEntry &node)
{
    auto it = _nodes.find( node.profile().id() );
    if ( it != _nodes.end() ) {
        throw runtime_error("Node is already present");
    }
    _nodes.emplace( unordered_map<std::string,LocNetNodeDbEntry>::value_type(
        node.profile().id(), node ) );
    return true;
}


shared_ptr<LocNetNodeDbEntry> DummySpatialDatabase::Load(const string &nodeId) const
{
    auto it = _nodes.find(nodeId);
    if ( it == _nodes.end() ) {
        throw runtime_error("Node is not found");
        //return shared_ptr<LocNetNodeDbEntry>();
    }
    
    return shared_ptr<LocNetNodeDbEntry>( new LocNetNodeDbEntry(it->second) );
}


bool DummySpatialDatabase::Update(const LocNetNodeInfo &) const
{
    // TODO
    return true;
}


bool DummySpatialDatabase::Remove(const string &nodeId)
{
    auto it = _nodes.find(nodeId);
    if ( it != _nodes.end() ) {
        throw runtime_error("Node is already present");
    }
    _nodes.erase(nodeId);
    return true;
}


Distance DummySpatialDatabase::GetNeighbourhoodRadiusKm() const
{
    // TODO
    return 42.;
}


size_t DummySpatialDatabase::GetNodeCount(LocNetRelationType relationType) const
{
    size_t result = 0;
    for (auto it : _nodes)
    {
        if ( it.second.relationType() == relationType )
            { ++result; }
    }
    return result;
}



vector<LocNetNodeInfo> DummySpatialDatabase::GetClosestNodes(
    const GpsLocation&, Distance, size_t, Neighbours) const
{
    // TODO
    return vector<LocNetNodeInfo>();
}


std::vector<LocNetNodeInfo>DummySpatialDatabase::GetRandomNodes(size_t, Neighbours) const
{
    // TODO
    return vector<LocNetNodeInfo>{ LocNetNodeInfo( NodeProfile("RandomNodeId", "", 0, "", 0),
                                       GpsLocation(0., 0.) ) };
}



shared_ptr<ILocNetRemoteNode> DummyLocNetRemoteNodeConnectionFactory::ConnectTo(const NodeProfile&)
{
    return shared_ptr<ILocNetRemoteNode>();
}

