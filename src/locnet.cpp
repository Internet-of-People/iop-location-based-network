#include <algorithm>
#include <cmath>
#include <chrono>
#include <deque>
#include <limits>
#include <unordered_set>

#include <easylogging++.h>

#include "locnet.hpp"

using namespace std;



namespace LocNet
{


const size_t   NEIGHBOURHOOD_MAX_NODE_COUNT         = 100;

const size_t   INIT_WORLD_RANDOM_NODE_COUNT         = 100;
const float    INIT_WORLD_NODE_FILL_TARGET_RATE     = 0.75;
const size_t   INIT_NEIGHBOURHOOD_QUERY_NODE_COUNT  = 10;

const chrono::duration<uint32_t> MAINTENANCE_PERIOD = chrono::hours(24);
const size_t   MAINTENANCE_ATTEMPT_COUNT            = 100;




const vector<NodeInfo> Node::_seedNodes {
    // TODO put some real seed nodes in here
    NodeInfo( NodeProfile( NodeId("BudapestSeedNodeId"),
        NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6371) ), GpsLocation(48.2081743, 16.3738189) ),
//     NodeInfo( NodeProfile( NodeId("WienSeedNodeId"),
//         NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6372) ), GpsLocation(47.497912, 19.040235) ),
};

random_device Node::_randomDevice;


Node::Node( const NodeInfo &nodeInfo,
            shared_ptr<ISpatialDatabase> spatialDb,
            std::shared_ptr<INodeConnectionFactory> connectionFactory,
            bool ignoreDiscovery ) :
    _myNodeInfo(nodeInfo), _spatialDb(spatialDb), _connectionFactory(connectionFactory)
{
    if (spatialDb == nullptr) {
        throw runtime_error("Invalid spatial database argument");
    }
    if (connectionFactory == nullptr) {
        throw runtime_error("Invalid connection factory argument");
    }
    
    if ( GetNodeCount() == 0 && ! ignoreDiscovery )
    {
        bool discoverySucceeded = InitializeWorld() && InitializeNeighbourhood();
        if (! discoverySucceeded)
        {
            // This still might be normal if we're the very first seed node of the whole network
            auto seedIt = find_if( _seedNodes.begin(), _seedNodes.end(),
                [this] (const NodeInfo& seedProfile) { return _myNodeInfo.profile() == seedProfile.profile(); } );
            if ( seedIt == _seedNodes.end() )
                 { throw runtime_error("Network discovery failed"); }
            else { LOG(DEBUG) << "I'm a seed and may be the first one started up, don't give up yet"; }
        }
    }
}


const unordered_map<ServiceType,ServiceProfile,EnumHasher>& Node::GetServices() const
    { return _services; }
    
void Node::RegisterService(ServiceType serviceType, const ServiceProfile& serviceInfo)
{
    auto it = _services.find(serviceType);
    if ( it != _services.end() ) {
        throw runtime_error("Service type is already registered");
    }
    _services[serviceType] = serviceInfo;
}

void Node::DeregisterService(ServiceType serviceType)
{
    auto it = _services.find(serviceType);
    if ( it == _services.end() ) {
        throw runtime_error("Service type was not registered");
    }
    _services.erase(serviceType);
}


bool Node::AcceptColleague(const NodeInfo &newNode)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request
//      if the other node "forgot" the connection or use the wrong method (accept vs renew)?
//      Should the two methods maybe merged instead?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.profile().id() );
//     if (storedInfo != nullptr)
//         { return false; } // We shouldn't have this colleague already
    
    return SafeStoreNode( NodeDbEntry(
        newNode, NodeRelationType::Colleague, NodeContactRoleType::Acceptor) );
}


bool Node::RenewColleague(const NodeInfo& node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.profile().id() );
//     if (storedInfo == nullptr)
//         { return false; } // We should have this colleague already

    return SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Colleague, NodeContactRoleType::Acceptor) );
}



bool Node::AcceptNeighbour(const NodeInfo &node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.profile().id() );
//     if (storedInfo != nullptr)
//         { return false; } // We shouldn't have this colleague already
    
    return SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Neighbour, NodeContactRoleType::Acceptor) );
}


bool Node::RenewNeighbour(const NodeInfo& node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.profile().id() );
//     if (storedInfo == nullptr)
//         { return false; } // We should have this colleague already
        
    return SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Neighbour, NodeContactRoleType::Acceptor) );
}



vector<NodeInfo> Node::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
    { return _spatialDb->GetRandomNodes(maxNodeCount, filter); }
    
vector<NodeInfo> Node::GetNeighbourNodesByDistance() const
    { return _spatialDb->GetNeighbourNodesByDistance(); }
    
size_t Node::GetNodeCount() const
    { return _spatialDb->GetNodeCount(); }
    
    

vector<NodeInfo> Node::GetClosestNodesByDistance(const GpsLocation& location,
    Distance radiusKm, size_t maxNodeCount, Neighbours filter) const
{
    return _spatialDb->GetClosestNodesByDistance(location, radiusKm, maxNodeCount, filter);
}


Distance Node::GetBubbleSize(const GpsLocation& location) const
{
    Distance distance = _spatialDb->GetDistanceKm( _myNodeInfo.location(), location );
    Distance bubbleSize = log10(distance + 2500.) * 501. - 1700.;
    return bubbleSize;
}


bool Node::BubbleOverlaps(const GpsLocation& newNodeLocation, const string &nodeIdToIgnore) const
{
    // Get our closest node to location, no matter the radius
    vector<NodeInfo> closestNodes = _spatialDb->GetClosestNodesByDistance(
        newNodeLocation, numeric_limits<Distance>::max(), 2, Neighbours::Excluded);
    
    // If there are no points yet (i.e. map is still empty) or our single node is being updated, it cannot overlap
    if ( closestNodes.empty() ||
       ( closestNodes.size() == 1 && closestNodes.front().profile().id() == nodeIdToIgnore ) )
        { return false; }
    
    // If updating a node with specified nodeId, ignore overlapping with itself
    const GpsLocation &myClosesNodeLocation = closestNodes.front().profile().id() != nodeIdToIgnore ?
        closestNodes.front().location() : closestNodes.back().location();
        
    // Get bubble sizes of both locations
    Distance myClosestNodeBubbleSize = GetBubbleSize(myClosesNodeLocation);
    Distance newNodeBubbleSize       = GetBubbleSize(newNodeLocation);
    
    // If sum of bubble sizes greater than distance of points, the bubbles overlap
    Distance newNodeDistanceFromClosestNode = _spatialDb->GetDistanceKm(newNodeLocation, myClosesNodeLocation);
    return myClosestNodeBubbleSize + newNodeBubbleSize > newNodeDistanceFromClosestNode;
}


shared_ptr<INodeMethods> Node::SafeConnectTo(const NodeProfile& node)
{
    // There is no point in connecting to ourselves
    if ( node.id() == _myNodeInfo.profile().id() )
        { return shared_ptr<INodeMethods>(); }
    
    try { return _connectionFactory->ConnectTo(node); }
    catch (exception &e)
        { LOG(INFO) << "Failed to connect to " << node << ": " << e.what(); }
    return shared_ptr<INodeMethods>();
}


bool Node::SafeStoreNode(const NodeDbEntry& newEntry, shared_ptr<INodeMethods> nodeConnection)
{
    try
    {
        // Check if node is acceptable
        shared_ptr<NodeDbEntry> storedInfo = _spatialDb->Load( newEntry.profile().id() );
        switch ( newEntry.relationType() )
        {
            case NodeRelationType::Colleague:
            {
                if (storedInfo != nullptr)
                {
                    // Existing colleague info may be upgraded to neighbour but not vica versa
                    if ( storedInfo->relationType() == NodeRelationType::Neighbour )
                        { return false; }
                    if ( storedInfo->location() != newEntry.location() ) {
                        // Node must not be moved away to a position that overlaps with anything other than itself
                        if ( BubbleOverlaps( newEntry.location(), newEntry.profile().id() ) )
                            { return false; }
                    }
                }
                else {
                    // Node must not overlap with other colleagues
                    if ( BubbleOverlaps( newEntry.location() ) )
                        { return false; }
                }
                    
                break;
            }
            
            case NodeRelationType::Neighbour:
            {
                // Neighbour limit will be exceeded, check if new entry deserves to temporarilty go over limit
                vector<NodeInfo> neighboursByDistance( _spatialDb->GetNeighbourNodesByDistance() );
                if ( neighboursByDistance.size() >= NEIGHBOURHOOD_MAX_NODE_COUNT )
                {
                    // If we have too many closer neighbours already, deny request
                    const NodeInfo &limitNeighbour = neighboursByDistance[NEIGHBOURHOOD_MAX_NODE_COUNT - 1];
                    if ( _spatialDb->GetDistanceKm( limitNeighbour.location(), _myNodeInfo.location() ) <=
                         _spatialDb->GetDistanceKm( _myNodeInfo.location(), newEntry.location() ) )
                    { return false; }
                }
            }
                
            default:
                throw runtime_error("Unknown nodetype, missing implementation");
        }
        
        if ( newEntry.roleType() == NodeContactRoleType::Initiator )
        {
            // If no connection argument is specified, try connecting to candidate node
            if (nodeConnection == nullptr)
                { nodeConnection = SafeConnectTo( newEntry.profile() ); }
            if (nodeConnection == nullptr)
                { return false; }
            
            // Ask for its permission for mutual acceptance
            switch ( newEntry.relationType() )
            {
                case NodeRelationType::Neighbour:
                    if (storedInfo == nullptr) {
                        if ( ! nodeConnection->AcceptNeighbour(_myNodeInfo) )
                            { return false; }
                    }
                    else {
                        if ( ! nodeConnection->RenewNeighbour(_myNodeInfo) )
                            { return false; }
                    }
                    break;
                    
                case NodeRelationType::Colleague:
                    if (storedInfo == nullptr) {
                        if ( ! nodeConnection->AcceptColleague(_myNodeInfo) )
                            { return false; }
                    }
                    else {
                        if ( ! nodeConnection->RenewColleague(_myNodeInfo) )
                            { return false; }
                    }
                    break;
                    
                default:
                    throw runtime_error("Unknown nodetype, missing implementation");
            }
        }
        
        // TODO consider if all further possible sanity checks are done already
        if (storedInfo == nullptr) { _spatialDb->Store(newEntry); }
        else                       { _spatialDb->Update(newEntry); }
        return true;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Unexpected error storing node: " << e.what();
    }
    
    return false;
}


// TODO consider if this is guaranteed to stop
bool Node::InitializeWorld()
{
    // Initialize random generator and utility variables
    uniform_int_distribution<int> fromRange( 0, _seedNodes.size() - 1 );
    vector<string> triedNodes;
    
    size_t nodeCountAtSeed = 0;
    vector<NodeInfo> randomColleagueCandidates;
    while ( triedNodes.size() < _seedNodes.size() )
    {
        // Select random node from hardwired seed node list
        size_t selectedSeedNodeIdx = fromRange(_randomDevice);
        const NodeInfo& selectedSeedNode = _seedNodes[selectedSeedNodeIdx];
        
        // If node has been already tried and failed, choose another one
        auto it = find( triedNodes.begin(), triedNodes.end(), selectedSeedNode.profile().id() );
        if ( it != triedNodes.end() )
            { continue; }
        
        try
        {
            triedNodes.push_back( selectedSeedNode.profile().id() );
            
            // Try connecting to selected seed node
            shared_ptr<INodeMethods> seedNodeConnection = SafeConnectTo( selectedSeedNode.profile() );
            if (seedNodeConnection == nullptr)
                { continue; }
            
            // Try to add seed node to our network (no matter if fails)
            bool seedAsNeighbour = SafeStoreNode( NodeDbEntry(selectedSeedNode,
                NodeRelationType::Neighbour, NodeContactRoleType::Initiator), seedNodeConnection );
            if (! seedAsNeighbour) {
                SafeStoreNode( NodeDbEntry(selectedSeedNode,
                    NodeRelationType::Colleague, NodeContactRoleType::Initiator), seedNodeConnection ); }
            
            // Query both total node count and an initial list of random nodes to start with
            LOG(DEBUG) << "Getting node count from initial seed";
            nodeCountAtSeed = seedNodeConnection->GetNodeCount();
            LOG(DEBUG) << "Node count on seed is " << nodeCountAtSeed;
            randomColleagueCandidates = seedNodeConnection->GetRandomNodes(
                min(INIT_WORLD_RANDOM_NODE_COUNT, nodeCountAtSeed), Neighbours::Excluded );
            
            // If got a reasonable response from a seed server, stop contacting other seeds
            if ( nodeCountAtSeed > 0 && ! randomColleagueCandidates.empty() )
                { break; }
        }
        catch (exception &e)
        {
            LOG(WARNING) << "Failed to bootstrap from seed node " << selectedSeedNode
                         << ": " << e.what() << ", trying other seeds";
        }
    }
    
    // Check if all seed nodes tried and failed
    if ( nodeCountAtSeed == 0 && randomColleagueCandidates.empty() &&
         triedNodes.size() == _seedNodes.size() )
    {
        // TODO reconsider error handling here, should we completely give up and maybe exit()?
        LOG(ERROR) << "All seed nodes have been tried and failed";
        return false;
    }
    
    // We received a reasonable random node list from a seed, try to fill in our world map
    size_t targetNodeCount = max( NEIGHBOURHOOD_MAX_NODE_COUNT,
        static_cast<size_t>(INIT_WORLD_NODE_FILL_TARGET_RATE * nodeCountAtSeed) );
    LOG(DEBUG) << "Targeted node count is " << targetNodeCount;
    
    // Try why we either reached targeted node count or asked colleagues from all available nodes
    while ( _spatialDb->GetNodeCount() < targetNodeCount &&
            triedNodes.size() < _spatialDb->GetNodeCount() )
    {
        if ( ! randomColleagueCandidates.empty() )
        {
            // Pick a single node from the candidate list and try to make it a colleague node
            NodeInfo nodeInfo( randomColleagueCandidates.back() );
            randomColleagueCandidates.pop_back();
            
            SafeStoreNode( NodeDbEntry(nodeInfo, NodeRelationType::Colleague, NodeContactRoleType::Initiator) );
        }
        else // We ran out of colleague candidates, try pick some more randomly
        {
            // Get a shuffled list of all colleague nodes known so far
            vector<NodeInfo> nodesKnownSoFar = _spatialDb->GetRandomNodes(
                _spatialDb->GetNodeCount(), Neighbours::Excluded);
            
            for (const NodeInfo &nodeInfo : nodesKnownSoFar)
            {
                // Check if we tried it already
                if ( find( triedNodes.begin(), triedNodes.end(), nodeInfo.profile().id() ) != triedNodes.end() )
                    { continue; }
                    
                try
                {
                    triedNodes.push_back( nodeInfo.profile().id() );
                    
                    // Connect to selected random node
                    shared_ptr<INodeMethods> randomConnection = SafeConnectTo( nodeInfo.profile() );
                    if (randomConnection == nullptr)
                        { continue; }
                    
                    // Ask it for random colleague candidates
                    randomColleagueCandidates = randomConnection->GetRandomNodes(
                        INIT_WORLD_RANDOM_NODE_COUNT, Neighbours::Excluded);
                    break;
                }
                catch (exception &e)
                {
                    LOG(WARNING) << "Failed to fetch more random nodes: " << e.what();
                }
            }
        }
    }
    
    LOG(DEBUG) << "World discovery finished, got " << _spatialDb->GetNodeCount() << " nodes";
    return true;
}



bool Node::InitializeNeighbourhood()
{
    // Get the closest node known to us so far
    vector<NodeInfo> newClosestNode = _spatialDb->GetClosestNodesByDistance(
        _myNodeInfo.location(), numeric_limits<Distance>::max(), 1, Neighbours::Included);
    if ( newClosestNode.empty() )
        { return false; }
    
    // Repeat asking the currently closest node for an even closer node until no new node discovered
    vector<NodeInfo> oldClosestNode;
    do {
        LOG(TRACE) << "Closest node known so far: " << newClosestNode.front();
        oldClosestNode = newClosestNode;
        try
        {
            shared_ptr<INodeMethods> closestNodeConnection =
                SafeConnectTo( newClosestNode.front().profile() );
            if (closestNodeConnection == nullptr) {
                // TODO what to do if closest node is not reachable?
                return false;
            }
            
            newClosestNode = closestNodeConnection->GetClosestNodesByDistance(
                _myNodeInfo.location(), numeric_limits<Distance>::max(), 1, Neighbours::Included);
        }
        catch (exception &e) {
            LOG(WARNING) << "Failed to fetch neighbours: " << e.what();
            // TODO consider what else to do here
        }
    }
    while ( oldClosestNode.size() == newClosestNode.size() &&
            oldClosestNode.front().profile().id() != newClosestNode.front().profile().id() &&
            newClosestNode.front().profile().id() != _myNodeInfo.profile().id() );
    
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
            shared_ptr<INodeMethods> candidateConnection = SafeConnectTo( neighbourCandidate.profile() );
            if (candidateConnection == nullptr)
                { continue; }
                
            // Try to add node as neighbour, reusing connection
            SafeStoreNode( NodeDbEntry(neighbourCandidate, NodeRelationType::Neighbour, NodeContactRoleType::Initiator),
                           candidateConnection );
            
            // Get its neighbours closest to us
            vector<NodeInfo> newNeighbourCandidates = candidateConnection->GetClosestNodesByDistance(
                _myNodeInfo.location(), numeric_limits<Distance>::max(),
                INIT_NEIGHBOURHOOD_QUERY_NODE_COUNT, Neighbours::Included );
            
            // Mark current node as processed and append its new neighbours to our todo list
            askedNodeIds.insert( neighbourCandidate.profile().id() );
            nodesToAskQueue.insert( nodesToAskQueue.end(),
                newNeighbourCandidates.begin(), newNeighbourCandidates.end() );
        }
        catch (exception &e) {
            LOG(WARNING) << "Failed to add neighbour node: " << e.what();
            // TODO consider what else to do here?
        }
    }
    
    return true;
}



void Node::RenewNodeRelations()
{
    // TODO we probably have to run this in a separate thread
    vector<NodeDbEntry> contactedNodes = _spatialDb->GetNodes(NodeContactRoleType::Initiator);
    for (auto const &node : contactedNodes)
    {
        try
        {
            shared_ptr<INodeMethods> connection = SafeConnectTo( node.profile() );
            if (connection == nullptr)
            {
                LOG(INFO) << "Failed to contact node " << node.profile().id();
                continue;
            }
            
            bool renewed = false;
            switch ( node.relationType() )
            {
                case NodeRelationType::Colleague:
                    renewed = connection->RenewColleague(_myNodeInfo);
                    break;
                    
                case NodeRelationType::Neighbour:
                    renewed = connection->RenewNeighbour(_myNodeInfo);
                    break;
                    
                default:
                    throw runtime_error("Unknown relation type");
            }
            LOG(DEBUG) << "Renewing relation with node " << node.profile().id() << " status: " << renewed;
        }
        catch (exception &e)
        {
            LOG(WARNING) << "Unexpected error while contacting node "
                << node.profile().id() << " : " << e.what();
        }
    }
}


void Node::DiscoverUnknownAreas()
{
    // TODO we probably have to run this in a separate thread
    for (size_t i = 0; i < MAINTENANCE_ATTEMPT_COUNT; ++i)
    {
        // Generate a random GPS location
        uniform_real_distribution<float> latitudeRange(-90.0, 90.0);
        uniform_real_distribution<float> longitudeRange(-180.0, 180.0);
        GpsLocation randomLocation( latitudeRange(_randomDevice), longitudeRange(_randomDevice) );
        
        // TODO consider: the resulted node might be very far away from the generated random node,
        //      so prefiltering might cause bad results, but it also spares network costs. Test and decide!
        if ( BubbleOverlaps(randomLocation) )
            { continue; }
        
        // Get node closest to this position that is already present in our database
        vector<NodeInfo> myClosestNodes = _spatialDb->GetClosestNodesByDistance(
            randomLocation, numeric_limits<Distance>::max(), 1, Neighbours::Excluded );
        if ( myClosestNodes.empty() )
            { continue; }
        const auto &myClosestNode = myClosestNodes[0];
        
        // Connect to closest node
        shared_ptr<INodeMethods> connection = SafeConnectTo( myClosestNode.profile() );
        if (connection == nullptr)
        {
            LOG(INFO) << "Failed to contact node " << myClosestNode.profile().id();
            continue;
        }
        
        // Ask closest node about its nodes closest to the random position
        vector<NodeInfo> gotClosestNodes = connection->GetClosestNodesByDistance(
            randomLocation, numeric_limits<Distance>::max(), 1, Neighbours::Included );
        if ( gotClosestNodes.empty() )
            { continue; }
        const auto &gotClosestNode = gotClosestNodes[0];
        
        // Try to add node to our database
        bool storedAsNeighbour = SafeStoreNode( NodeDbEntry( gotClosestNode,
            NodeRelationType::Colleague, NodeContactRoleType::Initiator), connection );
        if (! storedAsNeighbour) {
            SafeStoreNode( NodeDbEntry( gotClosestNode,
                NodeRelationType::Colleague, NodeContactRoleType::Initiator), connection );
        }
    }
}



} // namespace LocNet
