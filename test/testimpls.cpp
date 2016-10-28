#include "testimpls.hpp"

using namespace std;



DummySpatialDatabase::DummySpatialDatabase(const GpsLocation& myLocation):
    _myLocation(myLocation) {}



Distance DummySpatialDatabase::GetDistance(const GpsLocation&, const GpsLocation&) const
{
    // TODO
    return 0;
}


bool DummySpatialDatabase::Store(const LocNetNodeDbEntry&)
{
    // TODO
    return true;
}


shared_ptr<LocNetNodeDbEntry> DummySpatialDatabase::Load(const string&) const
{
    // TODO
    return shared_ptr<LocNetNodeDbEntry>(
        new LocNetNodeDbEntry( NodeProfile("NodeId", "", 0, "", 0),
            GpsLocation(0., 0.), LocNetRelationType::Colleague, PeerRoleType::Acceptor ) );
}


bool DummySpatialDatabase::Update(const LocNetNodeInfo&) const
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

