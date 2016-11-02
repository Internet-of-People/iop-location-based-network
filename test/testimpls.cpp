#include <algorithm>
#include <cmath>

#include "testimpls.hpp"

using namespace std;



DummySpatialDatabase::DummySpatialDatabase(const GpsLocation& myLocation):
    _myLocation(myLocation) {}


    
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
    return count_if( _nodes.begin(), _nodes.end(),
        [relationType] (auto const &elem) { return elem.second.relationType() == relationType; } );
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

