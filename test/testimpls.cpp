#include <algorithm>
#include <cmath>

#include "easylogging++.h"
#include "testimpls.hpp"

using namespace std;



namespace LocNet
{
    

random_device DummySpatialDatabase::_randomDevice;


DummySpatialDatabase::DummySpatialDatabase(const GpsLocation& nodeLocation) :
    _myLocation(nodeLocation) {}



bool DummySpatialDatabase::Store(const NodeDbEntry &node)
{
    // TODO consider exception strategy, bool return value vs exception consistency
    auto it = _nodes.find( node.profile().id() );
    if ( it != _nodes.end() ) {
        throw runtime_error("Node is already present");
    }
    _nodes.emplace( unordered_map<std::string,NodeDbEntry>::value_type(
        node.profile().id(), node ) );
    return true;
}


shared_ptr<NodeDbEntry> DummySpatialDatabase::Load(const string &nodeId) const
{
    auto it = _nodes.find(nodeId);
    if ( it == _nodes.end() ) {
        throw runtime_error("Node is not found");
        //return shared_ptr<LocNetNodeDbEntry>();
    }
    
    return shared_ptr<NodeDbEntry>( new NodeDbEntry(it->second) );
}


bool DummySpatialDatabase::Update(const NodeInfo &node)
{
    auto it = _nodes.find( node.profile().id() );
    if ( it == _nodes.end() ) {
        throw runtime_error("Node is not found");
        // return false;
    }
    
    static_cast<NodeInfo&>(it->second) = node;
    return true;
}


bool DummySpatialDatabase::Remove(const string &nodeId)
{
    auto it = _nodes.find(nodeId);
    if ( it == _nodes.end() ) {
        throw runtime_error("Node is not found");
        // return false;
    }
    _nodes.erase(nodeId);
    return true;
}



vector<NodeInfo> DummySpatialDatabase::GetClosestNodesByDistance(
    const GpsLocation &location, Distance maxRadiusKm, size_t maxNodeCount, Neighbours filter) const
{
    // Start with all nodes
    vector<NodeDbEntry> remainingNodes;
    for (auto const &entry : _nodes)
        { remainingNodes.push_back(entry.second); }
    
    // Remove nodes out of range
    auto newEnd = remove_if( remainingNodes.begin(), remainingNodes.end(),
        [this, &location, maxRadiusKm](const NodeDbEntry &node) { return maxRadiusKm < this->GetDistanceKm(location, node.location() ); } );
    remainingNodes.erase( newEnd, remainingNodes.end() );

    // Remove nodes with wrong relationType
    if (filter == Neighbours::Excluded) {
        newEnd = remove_if( remainingNodes.begin(), remainingNodes.end(),
            [](const NodeDbEntry &node) { return node.relationType() == NodeRelationType::Neighbour; } );
        remainingNodes.erase( newEnd, remainingNodes.end() );
    }
    
    vector<NodeInfo> result;
    while ( result.size() < maxNodeCount )
    {
        // Select closest element
        auto minElement = min_element( remainingNodes.begin(), remainingNodes.end(),
            [this, &location](const NodeDbEntry &one, const NodeDbEntry &other) {
                return this->GetDistanceKm( location, one.location() ) < this->GetDistanceKm( location, other.location() ); } );
        if (minElement == remainingNodes.end() )
            { break; }

        // Save element to result and remove it from candidates
        result.push_back(*minElement);
        remainingNodes.erase(minElement);
    }
    return result;
}


std::vector<NodeInfo>DummySpatialDatabase::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
{
    // Start with all nodes
    vector<NodeDbEntry> remainingNodes;
    for (auto const &entry : _nodes)
        { remainingNodes.push_back(entry.second); }
        
    // Remove nodes with wrong relationType
    if (filter == Neighbours::Excluded) {
        auto newEnd = remove_if( remainingNodes.begin(), remainingNodes.end(),
            [](const NodeDbEntry &node) { return node.relationType() == NodeRelationType::Neighbour; } );
        remainingNodes.erase( newEnd, remainingNodes.end() );
    }

    vector<NodeInfo> result;
    while ( ! remainingNodes.empty() && result.size() < maxNodeCount )
    {
        // Select random element from remaining nodes
        uniform_int_distribution<int> fromRange( 0, remainingNodes.size() - 1 );
        size_t selectedNodeIdx = fromRange(_randomDevice);
        
        // Save element to result and remove it from candidates
        result.push_back( remainingNodes[selectedNodeIdx] );
        remainingNodes.erase( remainingNodes.begin() + selectedNodeIdx );
    }
    return result;
}


std::vector<NodeInfo> DummySpatialDatabase::GetNodes(NodeRelationType relationType) const
{
    vector<NodeInfo> result;
    for (auto const &entry : _nodes)
    {
        if ( entry.second.relationType() == relationType )
            { result.push_back(entry.second); }
    }
    return result;
}


// Distance DummySpatialDatabase::GetFarthestNeighbourDistanceKm() const
// {
//     vector<NodeInfo> neighbours( GetNodes(NodeRelationType::Neighbour) );
//     if ( neighbours.empty() )
//         { return 0; }
//         
//     auto farthestNeighbourIt =  max_element( neighbours.begin(), neighbours.end(),
//         [this] (const NodeInfo &one, const NodeInfo &other)
//             { return GetDistanceKm( one.location(), _myLocation ) < GetDistanceKm( other.location(), _myLocation ); } );
//     return GetDistanceKm( farthestNeighbourIt->location(), fromLocation );
// }

size_t DummySpatialDatabase::GetColleagueNodeCount() const
    { return GetNodes(NodeRelationType::Colleague).size(); }

vector<NodeInfo> DummySpatialDatabase::GetNeighbourNodesByDistance() const
{
    vector<NodeInfo> neighbours( GetNodes(NodeRelationType::Neighbour) );
    sort( neighbours.begin(), neighbours.end(), [this] (const NodeInfo &one, const NodeInfo &other)
        { return GetDistanceKm( one.location(), _myLocation ) < GetDistanceKm( other.location(), _myLocation ); } );
    return neighbours;
}
    
    

GpsCoordinate degreesToRadian(GpsCoordinate degrees)
{
    //static const GpsCoordinate PI = acos(-1);
    return degrees * M_PI / 180.0;
}


// Implementation based on Haversine formula, see e.g. http://www.movable-type.co.uk/scripts/latlong.html
// An original example code in Javascript:
// var R = 6371e3; // metres
// var φ1 = lat1.toRadians();
// var φ2 = lat2.toRadians();
// var Δφ = (lat2-lat1).toRadians();
// var Δλ = (lon2-lon1).toRadians();
// 
// var a = Math.sin(Δφ/2) * Math.sin(Δφ/2) +
//         Math.cos(φ1) * Math.cos(φ2) *
//         Math.sin(Δλ/2) * Math.sin(Δλ/2);
// var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
// 
// var d = R * c;

Distance DummySpatialDatabase::GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const
{
    static const double EARTH_RADIUS = 6371.;
    
    GpsCoordinate fi1 = degreesToRadian( one.latitude() );
    GpsCoordinate fi2 = degreesToRadian( other.latitude() );
    GpsCoordinate delta_fi = fi2 - fi1;
    GpsCoordinate delta_lambda = degreesToRadian( other.longitude() - one.longitude() );
    
    Distance a = sin(delta_fi / 2) * sin(delta_fi / 2) +
        cos(fi1) * cos(fi2) * sin(delta_lambda / 2) * sin(delta_lambda / 2);

    Distance c = 2 * atan2( sqrt(a), sqrt(1-a) );
    return EARTH_RADIUS * c;
}



shared_ptr<IRemoteNode> DummyLocNetRemoteNodeConnectionFactory::ConnectTo(const NodeProfile&)
{
    return shared_ptr<IRemoteNode>();
}



} // namespace LocNet
