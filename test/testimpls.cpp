#include <list>
#include <easylogging++.h>

#include "testimpls.hpp"

using namespace std;



namespace LocNet
{


shared_ptr<INodeMethods> DummyNodeConnectionFactory::ConnectTo(const NetworkEndpoint&)
{
    return shared_ptr<INodeMethods>();
}

shared_ptr<IChangeListener> DummyChangeListenerFactory::Create(shared_ptr<ILocalServiceMethods>)
{
    return shared_ptr<IChangeListener>();
}



ChangeCounter::ChangeCounter(const SessionId& sessionId) : _sessionId(sessionId) {}

const SessionId& ChangeCounter::sessionId() const { return _sessionId; }

void ChangeCounter::OnRegistered() {}
void ChangeCounter::AddedNode(const NodeDbEntry&)   { ++addedCount; }
void ChangeCounter::UpdatedNode(const NodeDbEntry&) { ++updatedCount; }
void ChangeCounter::RemovedNode(const NodeDbEntry&) { ++removedCount; }



void NodeRegistry::Register(shared_ptr<Node> node)
{
    if (! node)
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Received empty node"); }
    _nodes.emplace( node->GetNodeInfo().contact().nodeEndpoint().address(), node );
}

const NodeRegistry::NodeContainer& NodeRegistry::nodes() const
    { return _nodes; }

std::shared_ptr<INodeMethods> NodeRegistry::ConnectTo(const NetworkEndpoint &endpoint)
    { return _nodes[ endpoint.address() ]; }



InMemDbEntry::InMemDbEntry(const NodeDbEntry &other, chrono::system_clock::time_point expiresAt) :
    NodeDbEntry(other), _expiresAt(expiresAt) {}

InMemDbEntry::InMemDbEntry(const InMemDbEntry &other) :
    NodeDbEntry(other), _expiresAt(other._expiresAt) {}


random_device InMemorySpatialDatabase::_randomDevice;


InMemorySpatialDatabase::InMemorySpatialDatabase(const NodeInfo& myNodeInfo,
        chrono::duration<uint32_t> entryExpirationPeriod) :
    _myNodeInfo(myNodeInfo), _entryExpirationPeriod(entryExpirationPeriod)
{
    Store( NodeDbEntry(myNodeInfo, NodeRelationType::Self, NodeContactRoleType::Acceptor), false );
}


NodeDbEntry InMemorySpatialDatabase::ThisNode() const
    { return NodeDbEntry::FromSelfInfo(_myNodeInfo); }



void InMemorySpatialDatabase::Store(const NodeDbEntry &node, bool expires)
{
    auto it = _nodes.find( node.id() );
    if ( it != _nodes.end() ) {
        throw runtime_error("Node is already present");
    }
    
    chrono::system_clock::time_point expiresAt = expires ?
        chrono::system_clock::now() + _entryExpirationPeriod : chrono::system_clock::time_point::max();
    _nodes.emplace( node.id(), InMemDbEntry(node, expiresAt) );
}


shared_ptr<NodeDbEntry> InMemorySpatialDatabase::Load(const string &nodeId) const
{
    auto it = _nodes.find(nodeId);
    if ( it == _nodes.end() ) {
        // throw runtime_error("Node is not found");
        return shared_ptr<NodeDbEntry>();
    }
    
    return shared_ptr<NodeDbEntry>( new NodeDbEntry(it->second) );
}


void InMemorySpatialDatabase::Update(const NodeDbEntry &node, bool expires)
{
    auto it = _nodes.find( node.id() );
    if ( it == _nodes.end() ) {
        throw runtime_error("Node is not found");
    }
    
    chrono::system_clock::time_point expiresAt = expires ?
        chrono::system_clock::now() + _entryExpirationPeriod : chrono::system_clock::time_point::max();
    it->second = InMemDbEntry(node, expiresAt);
}


void InMemorySpatialDatabase::Remove(const string &nodeId)
{
    auto it = _nodes.find(nodeId);
    if ( it == _nodes.end() ) {
        throw runtime_error("Node is not found");
    }
    _nodes.erase(nodeId);
}


void InMemorySpatialDatabase::ExpireOldNodes()
{
// WTF: this is probably a GLIBC bug, it should compile
//     auto newEnd = remove_if( _nodes.begin(), _nodes.end(),
//         [] (const decltype(_nodes)::value_type &entry)
//             { return entry.second._expiresAt < chrono::system_clock::now(); } );
//     _nodes.erase( newEnd, _nodes.end() );
    
    decltype(_nodes) newCollection;
    for (auto &entry : _nodes)
    {
        if ( entry.second._expiresAt < chrono::system_clock::now() )
            { newCollection.emplace(entry); }
    }
    _nodes = newCollection;
}


IChangeListenerRegistry& InMemorySpatialDatabase::changeListenerRegistry()
    { return _listenerRegistry; }



vector<NodeDbEntry> InMemorySpatialDatabase::GetClosestNodesByDistance(
    const GpsLocation &location, Distance maxRadiusKm, size_t maxNodeCount, Neighbours filter) const
{
    //vector<NodeDbEntry> candidateNodes;
    list< pair<Distance,NodeDbEntry> > candidateNodes;
    for (auto const &entry : _nodes)
    {
        const NodeDbEntry &node = entry.second;
        Distance nodeDistance = GetDistanceKm( location, node.location() );
        if (maxRadiusKm >= nodeDistance)
        {
            if ( (filter == Neighbours::Included) ||
                 (filter == Neighbours::Excluded && node.relationType() != NodeRelationType::Neighbour) )
            {
                candidateNodes.emplace_back(nodeDistance, node);
            }
        }
    }
        
    vector<NodeDbEntry> result;
    while ( result.size() < maxNodeCount )
    {
        // Select closest element
        auto minElement = min_element( candidateNodes.begin(), candidateNodes.end(),
            [] (const pair<Distance,NodeDbEntry> &one, const pair<Distance,NodeDbEntry> &other)
                { return one.first < other.first; } );
        if (minElement == candidateNodes.end() )
            { break; }

        // Save element to result and remove it from candidates
        result.emplace_back(minElement->second);
        candidateNodes.erase(minElement);
    }
    return result;
}


std::vector<NodeDbEntry>
InMemorySpatialDatabase::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
{
    // Start with all nodes
    vector<NodeDbEntry> remainingNodes;
    for (auto const &entry : _nodes)
        { remainingNodes.emplace_back(entry.second); }
        
    // Remove nodes with wrong relationType
    if (filter == Neighbours::Excluded) {
        auto newEnd = remove_if( remainingNodes.begin(), remainingNodes.end(),
            [](const NodeDbEntry &node)
                { return node.relationType() == NodeRelationType::Neighbour; } );
        remainingNodes.erase( newEnd, remainingNodes.end() );
    }

    vector<NodeDbEntry> result;
    while ( ! remainingNodes.empty() && result.size() < maxNodeCount )
    {
        // Select random element from remaining nodes
        uniform_int_distribution<int> fromRange( 0, remainingNodes.size() - 1 );
        size_t selectedNodeIdx = fromRange(_randomDevice);
        
        // Save element to result and remove it from candidates
        result.emplace_back( remainingNodes[selectedNodeIdx] );
        remainingNodes.erase( remainingNodes.begin() + selectedNodeIdx );
    }
    return result;
}


std::vector<NodeDbEntry> InMemorySpatialDatabase::GetNodes(NodeRelationType relationType) const
{
    vector<NodeDbEntry> result;
    for (auto const &entry : _nodes)
    {
        if ( entry.second.relationType() == relationType )
            { result.emplace_back(entry.second); }
    }
    return result;
}


size_t InMemorySpatialDatabase::GetNodeCount() const
    { return _nodes.size(); }

size_t InMemorySpatialDatabase::GetNodeCount(NodeRelationType filter) const
{
    size_t result = 0;
    for (auto const &entry : _nodes)
        { if (entry.second.relationType() == filter)
            { ++result; } }
    return result;
}
    

vector<NodeDbEntry> InMemorySpatialDatabase::GetNodes(NodeContactRoleType roleType)
{
    vector<NodeDbEntry> result;
    for (auto const &entry : _nodes)
    {
        if ( entry.second.roleType() == roleType )
            { result.emplace_back(entry.second); }
    }
    return result;
}



vector<NodeDbEntry> InMemorySpatialDatabase::GetNeighbourNodesByDistance() const
{
    vector<NodeDbEntry> neighbours( GetNodes(NodeRelationType::Neighbour) );
    sort( neighbours.begin(), neighbours.end(),
        [this] (const NodeDbEntry &one, const NodeDbEntry &other)
            { return GetDistanceKm( one.location(), _myNodeInfo.location() ) <
                     GetDistanceKm( other.location(), _myNodeInfo.location() ); } );
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

Distance InMemorySpatialDatabase::GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const
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



string TestConfig::ExecPath("UNINITIALIZED");


TestConfig::TestConfig(const NodeInfo &aNodeInfo) : _nodeInfo(aNodeInfo) {}

bool TestConfig::isTestMode() const             { return true; }
const NodeInfo& TestConfig::myNodeInfo() const  { return _nodeInfo; }
TcpPort TestConfig::localServicePort() const    { return _localPort; }
const std::string& TestConfig::logPath() const  { return _logPath; }
const std::string& TestConfig::dbPath() const   { return _dbPath; }

size_t TestConfig::neighbourhoodTargetSize() const  { return _neighbourhoodTargetSize; }
const std::vector<NetworkEndpoint>& TestConfig::seedNodes() const           { return _seedNodes; }
std::chrono::duration<uint32_t> TestConfig::requestExpirationPeriod() const { return chrono::seconds(60); }
std::chrono::duration<uint32_t> TestConfig::dbMaintenancePeriod() const     { return chrono::hours(7); }
std::chrono::duration<uint32_t> TestConfig::dbExpirationPeriod() const      { return chrono::hours(24); }
std::chrono::duration<uint32_t> TestConfig::discoveryPeriod() const         { return chrono::minutes(5); }


// bool Config::InitForTest()
// {
//     _testMode = true;
//     auto testArgs = TestArgs();
//     return Initialize(testArgs.first, testArgs.second);
// }
//
//
// pair<size_t, const char**> Config::TestArgs()
// {
//     static const char* options[] = {
//         _argv0.c_str(),
//         "--test",
//         "--nodeid", "TestNodeId",
//         "--latitude", "0.0",
//         "--longitude", "0.0",
//     };
//     return make_pair(8, options);
// }



} // namespace LocNet
