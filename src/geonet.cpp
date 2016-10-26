#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

#include "geonet.hpp"

using namespace std;


static const size_t INIT_WORLD_RANDOM_NODE_COUNT        = 100;
static const double INIT_WORLD_NODE_FILL_TARGET_RATE    = 0.75;




const vector<NodeProfile> GeographicNetwork::_seedNodes {
    // TODO put some real seed nodes in here
    NodeProfile( NodeId("FirstSeedNodeId"), "1.2.3.4", 5555, "", 0 ),
    NodeProfile( NodeId("SecondSeedNodeId"), "6.7.8.9", 5555, "", 0 ),
};

random_device GeographicNetwork::_randomDevice;


GeographicNetwork::GeographicNetwork( const NodeLocation &nodeInfo,
                                      shared_ptr<ISpatialDatabase> spatialDb,
                                      std::shared_ptr<IGeographicNetworkConnectionFactory> connectionFactory ) :
    _myNodeInfo(nodeInfo), _spatialDb(spatialDb), _connectionFactory(connectionFactory)
{
    if (spatialDb == nullptr) {
        throw new runtime_error("Invalid SpatialDatabase argument");
    }
    if (connectionFactory == nullptr) {
        throw new runtime_error("Invalid connection factory argument");
    }
    
    DiscoverWorld();
    DiscoverNeighbourhood();
}


const unordered_map<ServerType,ServerInfo,EnumHasher>& GeographicNetwork::servers() const
    { return _servers; }
    
void GeographicNetwork::RegisterServer(ServerType serverType, const ServerInfo& serverInfo)
{
    auto it = _servers.find(serverType);
    if ( it != _servers.end() ) {
        throw runtime_error("Server type is already registered");
    }
    _servers[serverType] = serverInfo;
}

void GeographicNetwork::RemoveServer(ServerType serverType)
{
    auto it = _servers.find(serverType);
    if ( it != _servers.end() ) {
        _servers.erase(serverType);
    }
}


bool GeographicNetwork::AcceptColleague(const NodeLocation &newNode)
{
    if (! Overlaps( newNode.location() ) )
        { return _spatialDb->Store(newNode, false); }
    return false;
}


bool GeographicNetwork::RenewNodeConnection(const NodeLocation &updatedNode)
{
    shared_ptr<NodeLocation> storedProfile = _spatialDb->Load( updatedNode.profile().id() );
    if (storedProfile != nullptr) {
        if ( storedProfile->location() == updatedNode.location() ) {
            return _spatialDb->Update(updatedNode);
        }
        // TODO consider how we should handle changed location here: recalculate bubbles or simply deny renewal
    }
    return false;
}


bool GeographicNetwork::AcceptNeighbor(const NodeLocation &node)
{
    return _spatialDb->Store(node, true);
}



size_t GeographicNetwork::GetColleagueNodeCount() const
    { return _spatialDb->GetColleagueNodeCount(); }

double GeographicNetwork::GetNeighbourhoodRadiusKm() const
    { return _spatialDb->GetNeighbourhoodRadiusKm(); }

vector<NodeLocation> GeographicNetwork::GetRandomNodes(
    uint16_t maxNodeCount, bool includeNeighbours) const
{
    return _spatialDb->GetRandomNodes(maxNodeCount, includeNeighbours);
}

vector<NodeLocation> GeographicNetwork::GetClosestNodes(const GpsLocation& location,
    double radiusKm, uint16_t maxNodeCount, bool includeNeighbours) const
{
    return _spatialDb->GetClosestNodes(location, radiusKm, maxNodeCount, includeNeighbours);
}


double GeographicNetwork::GetBubbleSize(const GpsLocation& location) const
{
    double distance = _spatialDb->GetDistance( _myNodeInfo.location(), location );
    double bubbleSize = log10(distance + 2500.) * 500. - 1700.;
    return bubbleSize;
}


bool GeographicNetwork::Overlaps(const GpsLocation& newNodeLocation) const
{
    vector<NodeLocation> closestNodes = _spatialDb->GetClosestNodes(
        newNodeLocation, numeric_limits<double>::max(), 1, false);
    if ( closestNodes.empty() ) { return false; }
    
    const GpsLocation &myClosesNodeLocation = closestNodes.front().location();
    double newNodeDistanceFromClosestNode = _spatialDb->GetDistance(newNodeLocation, myClosesNodeLocation);
    
    double myClosestNodeBubbleSize = GetBubbleSize(myClosesNodeLocation);
    double newNodeBubbleSize       = GetBubbleSize(newNodeLocation);
    
    return myClosestNodeBubbleSize + newNodeBubbleSize >= newNodeDistanceFromClosestNode;
}


shared_ptr<IGeographicNetwork> GeographicNetwork::SafeConnectTo(const NodeProfile& node)
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



// TODO consider if this is guaranteed to stop
void GeographicNetwork::DiscoverWorld()
{
    // Initialize random generator and utility variables
    uniform_int_distribution<int> fromRange( 0, _seedNodes.size() - 1 );
    vector<string> triedSeedNodes;
    
    size_t seedNodeColleagueCount = 0;
    vector<NodeLocation> randomColleagueCandidates;
    while ( triedSeedNodes.size() < _seedNodes.size() )
    {
        // Select random node from hardwired seed node list
        size_t selectedSeedNodeIdx = fromRange(_randomDevice);
        const NodeProfile& selectedSeedNode = _seedNodes[selectedSeedNodeIdx];
        
        // If node has been already tried and failed, choose another one
        auto it = find( triedSeedNodes.begin(), triedSeedNodes.end(), selectedSeedNode.id() );
        if ( it != triedSeedNodes.end() )
            { continue; }
        
        try
        {
            triedSeedNodes.push_back( selectedSeedNode.id() );
            
            // Try connecting to selected seed node
            shared_ptr<IGeographicNetwork> seedNodeConnection = SafeConnectTo(selectedSeedNode);
            if (seedNodeConnection == nullptr)
                { continue; }
            
            // And query both a target World Map size and an initial list of random nodes to start with
            seedNodeColleagueCount = seedNodeConnection->GetColleagueNodeCount();
            randomColleagueCandidates = seedNodeConnection->GetRandomNodes(
                min(INIT_WORLD_RANDOM_NODE_COUNT, seedNodeColleagueCount), false );
            
            // If got a reasonable response from a seed server, stop contacting other seeds
            if ( seedNodeColleagueCount > 0 && ! randomColleagueCandidates.empty() )
                { break; }
        }
        catch (exception &e)
        {
            // TODO use some kind of logging framework all around the file
            cerr << "Failed to connect to seed node: " << e.what() << ", trying other seeds" << endl;
        }
    }
    
    // Check if all nodes tried and failed
    if ( seedNodeColleagueCount == 0 && randomColleagueCandidates.empty() &&
         triedSeedNodes.size() == _seedNodes.size() )
    {
        // TODO reconsider error handling here, should we completely give up and maybe exit()?
        cerr << "All seed nodes have been tried and failed, giving up" << endl;
        return;
    }
    
    // We received a reasonable random node list from a seed, try to fill in our world map
    size_t targetColleageCound = INIT_WORLD_NODE_FILL_TARGET_RATE * seedNodeColleagueCount;
    for (size_t addedColleagueCount = 0; addedColleagueCount < targetColleageCound; )
    {
        if ( ! randomColleagueCandidates.empty() )
        {
            // Pick a single node from the candidate list and try to make it a colleague node
            NodeLocation nodeInfo( randomColleagueCandidates.back() );
            randomColleagueCandidates.pop_back();
            
            try
            {
                // Check if node location is acceptable
                if ( Overlaps( nodeInfo.location() ) )
                    { continue; }
                
                // Try connecting to candidate node
                shared_ptr<IGeographicNetwork> nodeConnection = SafeConnectTo( nodeInfo.profile() );
                if (nodeConnection == nullptr) {
                    continue;
                }
                
                // Ask for its permission to be colleagues
                if ( nodeConnection->AcceptColleague(_myNodeInfo) )
                {
                    // We both agreed, store it as colleague
                    _spatialDb->Store(nodeInfo, false);
                    ++addedColleagueCount;
                }
            }
            catch (exception &e)
            {
                // TODO should we do anything here besides printing some log message?
            }
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
                    return;
                }
                
                try
                {
                    // Connect to selected random node
                    shared_ptr<IGeographicNetwork> randomConnection = SafeConnectTo( randomColleagueCandidates.front().profile() );
                    if (randomConnection == nullptr) {
                        continue;
                    }
                    
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
}



void GeographicNetwork::DiscoverNeighbourhood()
{
    // TODO
}

