#include "testimpls.hpp"

using namespace std;



DummySpatialDatabase::DummySpatialDatabase(const GpsLocation& myLocation):
    SpatialDatabase(myLocation) {}



double DummySpatialDatabase::GetDistance(const GpsLocation& one, const GpsLocation& other) const
{
    // TODO
    return 0;
}


// void DummySpatialDatabase::Store(const NodeLocation& node)
// {
//     // TODO
// }
// 
// 
// NodeLocation DummySpatialDatabase::Load(const string& nodeId) const
// {
//     // TODO
//     return NodeLocation();
// }
// 
// 
// void DummySpatialDatabase::Remove(const string& nodeId)
// {
//     // TODO
// }
// 
// 
// vector<NodeLocation> DummySpatialDatabase::GetNeighbourHood() const
// {
//     // TODO
//     return vector<NodeLocation>();
// }
// 
// 
// vector<NodeLocation> DummySpatialDatabase::GetNodesCloseTo(
//     const GpsLocation& position, double radius, size_t maxNodeCount) const
// {
//     // TODO
//     return vector<NodeLocation>();
// }


