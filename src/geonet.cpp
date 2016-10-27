#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>
#include <limits>
#include <unordered_set>

#include "geonet.hpp"

using namespace std;


static const Distance NEIGHBOURHOOD_MAX_RANGE_KM          = 100;
static const size_t   NEIGHBOURHOOD_MAX_NODE_COUNT        = 100;

static const size_t   INIT_WORLD_RANDOM_NODE_COUNT        = 100;
static const float    INIT_WORLD_NODE_FILL_TARGET_RATE    = 0.75;
static const size_t   INIT_NEIGHBOURHOOD_QUERY_NODE_COUNT = 10;




const vector<NodeInfo> GeoNetBusinessLogic::_seedNodes {
    // TODO put some real seed nodes in here
    NodeInfo( NodeProfile( NodeId("FirstSeedNodeId"), "1.2.3.4", 5555, "", 0 ),  GpsLocation(1.0, 2.0) ),
    NodeInfo( NodeProfile( NodeId("SecondSeedNodeId"), "6.7.8.9", 5555, "", 0 ), GpsLocation(3.0, 4.0) ),
};

random_device GeoNetBusinessLogic::_randomDevice;


GeoNetBusinessLogic::GeoNetBusinessLogic( const NodeInfo &nodeInfo,
                                      shared_ptr<ISpatialDatabase> spatialDb,
                                      std::shared_ptr<IGeographicNetworkConnectionFactory> connectionFactory ) :
    _myNodeInfo(nodeInfo), _spatialDb(spatialDb), _connectionFactory(connectionFactory)
{
    if (spatialDb == nullptr) {
        throw new runtime_error("Invalid spatial database argument");
    }
    if (connectionFactory == nullptr) {
        throw new runtime_error("Invalid connection factory argument");
    }
    
    if ( _spatialDb->GetNodeCount(NodeType::Colleague) == 0 ) {
        bool discovery = DiscoverWorld() && DiscoverNeighbourhood();
        if (! discovery)
            { throw new runtime_error("Network discovery failed"); }
    }
}


const unordered_map<ServerType,ServerInfo,EnumHasher>& GeoNetBusinessLogic::servers() const
    { return _servers; }
    
void GeoNetBusinessLogic::RegisterServer(ServerType serverType, const ServerInfo& serverInfo)
{
    auto it = _servers.find(serverType);
    if ( it != _servers.end() ) {
        throw runtime_error("Server type is already registered");
    }
    _servers[serverType] = serverInfo;
}

void GeoNetBusinessLogic::RemoveServer(ServerType serverType)
{
    auto it = _servers.find(serverType);
    if ( it == _servers.end() ) {
        throw runtime_error("Server type was not registered");
    }
    _servers.erase(serverType);
}


bool GeoNetBusinessLogic::AcceptColleague(const NodeInfo &newNode)
{
    return SafeStoreNode(newNode, NodeType::Colleague, false);
}


bool GeoNetBusinessLogic::RenewNodeConnection(const NodeInfo &updatedNode)
{
    shared_ptr<NodeInfo> storedProfile = _spatialDb->Load( updatedNode.profile().id() );
    if (storedProfile != nullptr) {
        if ( storedProfile->location() == updatedNode.location() ) {
            return _spatialDb->Update(updatedNode);
        }
        // TODO consider how we should handle changed location here: recalculate bubbles or simply deny renewal
    }
    return false;
}


bool GeoNetBusinessLogic::AcceptNeighbor(const NodeInfo &node)
{
    return SafeStoreNode(node, NodeType::Neighbour, false);
}



size_t GeoNetBusinessLogic::GetNodeCount(NodeType nodeType) const
    { return _spatialDb->GetNodeCount(nodeType); }

Distance GeoNetBusinessLogic::GetNeighbourhoodRadiusKm() const
    { return _spatialDb->GetNeighbourhoodRadiusKm(); }

vector<NodeInfo> GeoNetBusinessLogic::GetRandomNodes(
    size_t maxNodeCount, bool includeNeighbours) const
{
    return _spatialDb->GetRandomNodes(maxNodeCount, includeNeighbours);
}

vector<NodeInfo> GeoNetBusinessLogic::GetClosestNodes(const GpsLocation& location,
    Distance radiusKm, size_t maxNodeCount, bool includeNeighbours) const
{
    return _spatialDb->GetClosestNodes(location, radiusKm, maxNodeCount, includeNeighbours);
}


Distance GeoNetBusinessLogic::GetBubbleSize(const GpsLocation& location) const
{
    Distance distance = _spatialDb->GetDistance( _myNodeInfo.location(), location );
    Distance bubbleSize = log10(distance + 2500.) * 500. - 1700.;
    return bubbleSize;
}


bool GeoNetBusinessLogic::BubbleOverlaps(const GpsLocation& newNodeLocation) const
{
    // Get our closest node to location, no matter the radius
    vector<NodeInfo> closestNodes = _spatialDb->GetClosestNodes(
        newNodeLocation, numeric_limits<Distance>::max(), 1, false);
    
    // If there is no such point yet (i.e. map is still empty), it cannot overlap
    if ( closestNodes.empty() ) { return false; }
    
    // Get bubble sizes of both locations
    const GpsLocation &myClosesNodeLocation = closestNodes.front().location();
    Distance myClosestNodeBubbleSize = GetBubbleSize(myClosesNodeLocation);
    Distance newNodeBubbleSize       = GetBubbleSize(newNodeLocation);
    
    // If sum of bubble sizes greater than distance of points, the bubbles overlap
    Distance newNodeDistanceFromClosestNode = _spatialDb->GetDistance(newNodeLocation, myClosesNodeLocation);
    return myClosestNodeBubbleSize + newNodeBubbleSize > newNodeDistanceFromClosestNode;
}


shared_ptr<IGeographicNetwork> GeoNetBusinessLogic::SafeConnectTo(const NodeProfile& node)
{
    // There is no point in connecting to ourselves
    if ( node.id() == _myNodeInfo.profile().id() )
        { return shared_ptr<IGeographicNetwork>(); }
    
    try { return _connectionFactory->ConnectTo(node); }
    catch (exception &e)
    {
        // TODO log this instead
        cerr << "Failed to connect to " << node.ipv4Address() << ":" << node.ipv4Port() << " / "
                                        << node.ipv6Address() << ":" << node.ipv6Port() << endl;
        cerr << "Error was: " << e.what() << endl;
        return shared_ptr<IGeographicNetwork>();
    }
}


bool GeoNetBusinessLogic::SafeStoreNode(const NodeInfo& nodeInfo, NodeType nodeType,
    bool requireRemoteConsent, shared_ptr<IGeographicNetwork> nodeConnection)
{
    try
    {
        // Check if node is acceptable
        switch (nodeType)
        {
            case NodeType::Neighbour:
                if ( _spatialDb->GetNodeCount(NodeType::Neighbour) >= NEIGHBOURHOOD_MAX_NODE_COUNT ||
                     _spatialDb->GetDistance( _myNodeInfo.location(), nodeInfo.location() ) >= NEIGHBOURHOOD_MAX_RANGE_KM )
                    { return false; }
                break;
                
            case NodeType::Colleague:
                if ( BubbleOverlaps( nodeInfo.location() ) )
                    { return false; }
                break;
                
            default:
                throw runtime_error("Unknown nodetype, missing implementation");
        }
        
        if (requireRemoteConsent)
        {
            // If no connection argument is specified, try connecting to candidate node
            if (nodeConnection == nullptr)
                { nodeConnection = SafeConnectTo( nodeInfo.profile() ); }
            if (nodeConnection == nullptr)
                { return false; }
            
            // Ask for its permission to add it
            switch (nodeType)
            {
                case NodeType::Colleague:
                    if ( ! nodeConnection->AcceptColleague(_myNodeInfo) )
                        { return false; }
                    break;
                    
                case NodeType::Neighbour:
                    if ( ! nodeConnection->AcceptNeighbor(_myNodeInfo) )
                        { return false; }
                    break;
                    
                default:
                    throw runtime_error("Unknown nodetype, missing implementation");
            }
        }
        
        return _spatialDb->Store(nodeInfo, nodeType);
    }
    catch (exception &e)
    {
        // TODO log exception here
    }
    
    return false;
}


// TODO consider if this is guaranteed to stop
bool GeoNetBusinessLogic::DiscoverWorld()
{
    // Initialize random generator and utility variables
    uniform_int_distribution<int> fromRange( 0, _seedNodes.size() - 1 );
    vector<string> triedSeedNodes;
    
    size_t seedNodeColleagueCount = 0;
    vector<NodeInfo> randomColleagueCandidates;
    while ( triedSeedNodes.size() < _seedNodes.size() )
    {
        // Select random node from hardwired seed node list
        size_t selectedSeedNodeIdx = fromRange(_randomDevice);
        const NodeInfo& selectedSeedNode = _seedNodes[selectedSeedNodeIdx];
        
        // If node has been already tried and failed, choose another one
        auto it = find( triedSeedNodes.begin(), triedSeedNodes.end(), selectedSeedNode.profile().id() );
        if ( it != triedSeedNodes.end() )
            { continue; }
        
        try
        {
            triedSeedNodes.push_back( selectedSeedNode.profile().id() );
            
            // Try connecting to selected seed node
            shared_ptr<IGeographicNetwork> seedNodeConnection = SafeConnectTo( selectedSeedNode.profile() );
            if (seedNodeConnection == nullptr)
                { continue; }
            
            // And query both a target World Map size and an initial list of random nodes to start with
            seedNodeColleagueCount = seedNodeConnection->GetNodeCount(NodeType::Colleague);
            randomColleagueCandidates = seedNodeConnection->GetRandomNodes(
                min(INIT_WORLD_RANDOM_NODE_COUNT, seedNodeColleagueCount), false );
            
            // If got a reasonable response from a seed server, stop contacting other seeds
            if ( seedNodeColleagueCount > 0 && ! randomColleagueCandidates.empty() )
            {
                // Try to add seed node to our network and step out of seed node phase
                Distance seedNodeDistance = _spatialDb->GetDistance( _myNodeInfo.location(), selectedSeedNode.location() );
                NodeType seedNodeType = seedNodeDistance <= NEIGHBOURHOOD_MAX_RANGE_KM ?
                    NodeType::Neighbour : NodeType::Colleague;
                SafeStoreNode(selectedSeedNode, seedNodeType, true);
                break;
            }
        }
        catch (exception &e)
        {
            // TODO use some kind of logging framework all around the file
            cerr << "Failed to connect to seed node: " << e.what() << ", trying other seeds" << endl;
        }
    }
    
    // Check if all seed nodes tried and failed
    if ( seedNodeColleagueCount == 0 && randomColleagueCandidates.empty() &&
         triedSeedNodes.size() == _seedNodes.size() )
    {
        // This still might be normal if we're the very first seed node of the whole network
        auto seedIt = find_if( _seedNodes.begin(), _seedNodes.end(),
            [this] (const NodeInfo& seedProfile) { return _myNodeInfo.profile() == seedProfile.profile(); } );
        if ( seedIt == _seedNodes.end() )
        {
            // TODO reconsider error handling here, should we completely give up and maybe exit()?
            cerr << "All seed nodes have been tried and failed, giving up" << endl;
            return false;
        }
    }
    
    // We received a reasonable random node list from a seed, try to fill in our world map
    size_t targetColleageCound = INIT_WORLD_NODE_FILL_TARGET_RATE * seedNodeColleagueCount;
    for (size_t addedColleagueCount = 0; addedColleagueCount < targetColleageCound; )
    {
        if ( ! randomColleagueCandidates.empty() )
        {
            // Pick a single node from the candidate list and try to make it a colleague node
            NodeInfo nodeInfo( randomColleagueCandidates.back() );
            randomColleagueCandidates.pop_back();
            
            if ( SafeStoreNode(nodeInfo, NodeType::Colleague, true) )
                { ++addedColleagueCount; }
        }
        else
        {
            // We ran out of colleague candidates, pick some more random candidates
            while ( randomColleagueCandidates.empty() )
            {
                // Select random node that we already know so far
                randomColleagueCandidates = _spatialDb->GetRandomNodes(1, false);
                if ( randomColleagueCandidates.empty() )
                {
                    // TODO reconsider error handling here
                    cerr << "After trying all random nodes returned by seed, still have no colleagues, give up" << endl;
                    return false;
                }
                
                try
                {
                    // Connect to selected random node
                    shared_ptr<IGeographicNetwork> randomConnection = SafeConnectTo( randomColleagueCandidates.front().profile() );
                    if (randomConnection == nullptr)
                        { continue; }
                    
                    // Ask for random colleague candidates
                    randomColleagueCandidates = randomConnection->GetRandomNodes(INIT_WORLD_RANDOM_NODE_COUNT, false);
                }
                catch (exception &e)
                {
                    // TODO maybe log error here
                }
            }
        }
    }
    
    return true;
}



bool GeoNetBusinessLogic::DiscoverNeighbourhood()
{
    // Get the closest node known to us so far
    vector<NodeInfo> newClosestNode = _spatialDb->GetClosestNodes(
        _myNodeInfo.location(), numeric_limits<Distance>::max(), 1, true);
    if ( newClosestNode.empty() )
        { return false; }
    
    // Repeat asking (so far) closest node for an even closer node until no new node discovered
    vector<NodeInfo> oldClosestNode;
    while ( oldClosestNode.size() != newClosestNode.size() ||
            oldClosestNode.front().profile().id() != newClosestNode.front().profile().id() )
    {
        oldClosestNode = newClosestNode;
        try
        {
            shared_ptr<IGeographicNetwork> closestNodeConnection =
                SafeConnectTo( newClosestNode.front().profile() );
            if (closestNodeConnection == nullptr) {
                // TODO what to do if closest node is not reachable?
                return false;
            }
            
            newClosestNode = closestNodeConnection->GetClosestNodes(
                _myNodeInfo.location(), numeric_limits<Distance>::max(), 1, true);
        }
        catch (exception &e) {
            // TODO consider what to do besides debug logging exception here
        }
    }
    
    deque<NodeInfo> nodesToAskQueue( newClosestNode.begin(), newClosestNode.end() );
    unordered_set<string> askedNodeIds;
    
    // Try to fill neighbourhood map until limit reached or no new nodes left to ask
    vector<NodeInfo> neighbourCandidates;
    while ( neighbourCandidates.size() < NEIGHBOURHOOD_MAX_NODE_COUNT &&
            ! nodesToAskQueue.empty() )
    {
        // Get next candidate
        NodeInfo neighbourCandidate = nodesToAskQueue.front();
        nodesToAskQueue.pop_front();
        
        // Skip it if has been processed already
        auto askedIt = find( askedNodeIds.begin(), askedNodeIds.end(), neighbourCandidate.profile().id() );
        if ( askedIt != askedNodeIds.end() )
            { continue; }
            
        try
        {
            // Try connecting to the node
            shared_ptr<IGeographicNetwork> candidateConnection = SafeConnectTo( neighbourCandidate.profile() );
            SafeStoreNode(neighbourCandidate, NodeType::Neighbour, true, candidateConnection);
            
            // Get its neighbours closest to us
            vector<NodeInfo> newNeighbourCandidates = candidateConnection->GetClosestNodes(
                _myNodeInfo.location(), INIT_NEIGHBOURHOOD_QUERY_NODE_COUNT, 10, true );
            
            // Mark current node as processed and append its new neighbours to our todo list
            askedNodeIds.insert( neighbourCandidate.profile().id() );
            nodesToAskQueue.insert( nodesToAskQueue.end(),
                newNeighbourCandidates.begin(), newNeighbourCandidates.end() );
        }
        catch (exception &e) {
            // TODO consider what to do besides debug logging exception here?
        }
    }
    
    return true;
}

