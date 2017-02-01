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


const size_t   NEIGHBOURHOOD_MAX_NODE_COUNT         = 50;   // Count limit for accepted neighbours.

const size_t   INIT_WORLD_RANDOM_NODE_COUNT         = 2 * NEIGHBOURHOOD_MAX_NODE_COUNT;
const float    INIT_WORLD_NODE_FILL_TARGET_RATE     = 0.75;
const size_t   INIT_NEIGHBOURHOOD_QUERY_NODE_COUNT  = 10;

const size_t   PERIODIC_DISCOVERY_ATTEMPT_COUNT     = 5;



random_device Node::_randomDevice;


Node::Node( shared_ptr<ISpatialDatabase> spatialDb,
            std::shared_ptr<INodeConnectionFactory> connectionFactory,
            const vector<NetworkEndpoint> &seedNodes ) :
    _spatialDb(spatialDb), _connectionFactory(connectionFactory), _seedNodes(seedNodes)
{
    if (_spatialDb == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No spatial database instantiated");
    }
    if (_connectionFactory == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No connection factory instantiated");
    }
}


void Node::EnsureMapFilled()
{
    if ( GetNodeCount() <= 1 && ! _seedNodes.empty() )
    {
        LOG(INFO) << "Map is empty, discovering the network";
        
        bool discoverySucceeded = InitializeWorld() && InitializeNeighbourhood();
        if (! discoverySucceeded)
            { LOG(WARNING) << "Failed to properly discover the full network, current node count is " << GetNodeCount(); }
        if ( GetNodeCount() <= 1 )
        {
            // This still might be normal if we're the very first seed node of the whole network
            NodeInfo myInfo = _spatialDb->ThisNode();
            auto seedIt = find_if( _seedNodes.begin(), _seedNodes.end(),
                [&myInfo] (const NetworkEndpoint &contact)
                    { return myInfo.contact().nodeEndpoint() == contact; } );
            if ( seedIt == _seedNodes.end() )
                 { throw LocationNetworkError(ErrorCode::ERROR_CONCEPTUAL, "Failed to discover any node of the network"); }
            else { LOG(DEBUG) << "I'm a seed and may be the first one started up, don't give up yet"; }
        }
    }
}



NodeInfo Node::GetNodeInfo() const
    { return _spatialDb->ThisNode(); }
    
void Node::RegisterService(const ServiceInfo& serviceInfo)
{
// NOTE stricter check forbids registering again for connecting services after a restart
//     auto it = _services.find(serviceType);
//     if ( it != _services.end() ) {
//         throw LocationNetworkError(ErrorCode::ERROR_INVALID_STATE, "Service type is already registered");
//     }
    
    NodeDbEntry entry = _spatialDb->ThisNode();
    entry.services()[ serviceInfo.type() ] = serviceInfo;
    _spatialDb->Update(entry);
    // TODO should we immediately start distributing this change or is it enough to wait for the usual renewal?
}

void Node::DeregisterService(ServiceType serviceType)
{
// NOTE RegisterService() does not check. This would result more complex services and would be assymetric, better don't check
//     auto it = _services.find(serviceType);
//     if ( it == _services.end() ) {
//         throw LocationNetworkError(ErrorCode::ERROR_INVALID_STATE, "Service type was not registered");
//     }
    
    NodeDbEntry entry = _spatialDb->ThisNode();
    entry.services().erase(serviceType);
    _spatialDb->Update(entry);
    // TODO should we immediately start distributing this change or is it enough to wait for the usual renewal?
}


void Node::AddListener(shared_ptr<IChangeListener> listener)
    { _spatialDb->changeListenerRegistry().AddListener(listener); }

void Node::RemoveListener(const SessionId &sessionId)
    { _spatialDb->changeListenerRegistry().RemoveListener(sessionId); }



void Node::DetectedExternalAddress(const Address& address)
{
    NodeDbEntry myEntry = _spatialDb->ThisNode();
    if ( myEntry.contact().address() != address && ! address.empty() )
    {
        LOG(INFO) << "Detected external IP address " << address;
        myEntry.contact().address(address);
        _spatialDb->Update(myEntry);
    }
}


shared_ptr<NodeInfo> Node::AcceptColleague(const NodeInfo &node)
{
// TODO Sanity checks are performed in SafeStoreNode, should we reject the request
//      if the other node "forgot" about the existing relation or call the wrong method (accept vs renew)?
//      Should the two methods maybe merged instead?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.id() );
//     if (storedInfo != nullptr)
//         { return false; } // We shouldn't have this colleague already
    
    bool success = SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Colleague, NodeContactRoleType::Acceptor) );
    return success ? shared_ptr<NodeInfo>( new NodeInfo( _spatialDb->ThisNode() ) ) :
                     shared_ptr<NodeInfo>();
}


shared_ptr<NodeInfo> Node::RenewColleague(const NodeInfo& node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.id() );
//     if (storedInfo == nullptr)
//         { return false; } // We should have this colleague already

    bool success = SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Colleague, NodeContactRoleType::Acceptor) );
    return success ? shared_ptr<NodeInfo>( new NodeInfo( _spatialDb->ThisNode() ) ) :
                     shared_ptr<NodeInfo>();
}



shared_ptr<NodeInfo> Node::AcceptNeighbour(const NodeInfo &node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.id() );
//     if (storedInfo != nullptr)
//         { return false; } // We shouldn't have this colleague already
    
    bool success = SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Neighbour, NodeContactRoleType::Acceptor) );
    return success ? shared_ptr<NodeInfo>( new NodeInfo( _spatialDb->ThisNode() ) ) :
                     shared_ptr<NodeInfo>();
}


shared_ptr<NodeInfo> Node::RenewNeighbour(const NodeInfo& node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.id() );
//     if (storedInfo == nullptr)
//         { return false; } // We should have this colleague already
        
    bool success = SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Neighbour, NodeContactRoleType::Acceptor) );
    return success ? shared_ptr<NodeInfo>( new NodeInfo( _spatialDb->ThisNode() ) ) :
                     shared_ptr<NodeInfo>();
}



size_t Node::GetNodeCount() const
    { return _spatialDb->GetNodeCount(); }


vector<NodeInfo> Node::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
{
    vector<NodeDbEntry> entries( _spatialDb->GetRandomNodes(maxNodeCount, filter) );
    return vector<NodeInfo>( entries.begin(), entries.end() );
}
    
vector<NodeInfo> Node::GetNeighbourNodesByDistance() const
{
    vector<NodeDbEntry> entries( _spatialDb->GetNeighbourNodesByDistance() );
    return vector<NodeInfo>( entries.begin(), entries.end() );
}
    
    

vector<NodeInfo> Node::GetClosestNodesByDistance(const GpsLocation& location,
    Distance radiusKm, size_t maxNodeCount, Neighbours filter) const
{
    vector<NodeDbEntry> entries( _spatialDb->GetClosestNodesByDistance(location, radiusKm, maxNodeCount, filter) );
    return vector<NodeInfo>( entries.begin(), entries.end() );
}


Distance Node::GetBubbleSize(const GpsLocation& location) const
{
    Distance distance = _spatialDb->GetDistanceKm( _spatialDb->ThisNode().location(), location );
    Distance bubbleSize = log10(distance + 2500.) * 501. - 1700.;
    return bubbleSize;
}


bool Node::BubbleOverlaps(const GpsLocation& newNodeLocation, const string &nodeIdToIgnore) const
{
    // Get our closest node to location, no matter the radius
    vector<NodeInfo> closestNodes = GetClosestNodesByDistance(
        newNodeLocation, numeric_limits<Distance>::max(), 2, Neighbours::Excluded);
    
    // If there are no points yet (i.e. map is still empty) or our single node is being updated, it cannot overlap
    if ( closestNodes.empty() ||
       ( closestNodes.size() == 1 && closestNodes.front().id() == nodeIdToIgnore ) )
        { return false; }
    
    // If updating a node with specified nodeId, ignore overlapping with itself
    const GpsLocation &myClosesNodeLocation = closestNodes.front().id() != nodeIdToIgnore ?
        closestNodes.front().location() : closestNodes.back().location();
        
    // Get bubble sizes of both locations
    Distance myClosestNodeBubbleSize = GetBubbleSize(myClosesNodeLocation);
    Distance newNodeBubbleSize       = GetBubbleSize(newNodeLocation);
    
    // If sum of bubble sizes greater than distance of points, the bubbles overlap
    Distance newNodeDistanceFromClosestNode = _spatialDb->GetDistanceKm(newNodeLocation, myClosesNodeLocation);
    return myClosestNodeBubbleSize + newNodeBubbleSize > newNodeDistanceFromClosestNode;
}



shared_ptr<INodeMethods> Node::SafeConnectTo(const NetworkEndpoint& endpoint)
{
    // There is no point in connecting to ourselves
    if ( endpoint == _spatialDb->ThisNode().contact().nodeEndpoint() || endpoint.isLoopback() )
        { return shared_ptr<INodeMethods>(); }
    
    try { return _connectionFactory->ConnectTo(endpoint); }
    catch (exception &e)
        { LOG(INFO) << "Failed to connect to " << endpoint << ": " << e.what(); }
    return shared_ptr<INodeMethods>();
}



bool Node::SafeStoreNode(const NodeDbEntry& plannedEntry, shared_ptr<INodeMethods> nodeConnection)
{
    try
    {
        NodeDbEntry myNode = _spatialDb->ThisNode();
        
        // We must not explicitly add or overwrite our own node info here.
        // Whether or not our own nodeinfo is stored in the db is an implementation detail of the SpatialDatabase.
        if ( plannedEntry.id() == myNode.id() ||
             plannedEntry.relationType() == NodeRelationType::Self )
            { return false; }
     
        // Validate if node is acceptable
        shared_ptr<NodeDbEntry> storedInfo = _spatialDb->Load( plannedEntry.id() );
        if ( storedInfo && storedInfo->relationType() == NodeRelationType::Self )
            { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Forbidden operation: must not overwrite self here"); }
        
        switch ( plannedEntry.relationType() )
        {
            case NodeRelationType::Colleague:
            {
                if (storedInfo != nullptr)
                {
                    // Existing colleague info may be upgraded to neighbour but not vica versa
                    if ( storedInfo->relationType() == NodeRelationType::Neighbour )
                        { return false; }
                    if ( storedInfo->location() != plannedEntry.location() ) {
                        // Node must not be moved away to a position that overlaps with anything other than itself
                        if ( BubbleOverlaps( plannedEntry.location(), plannedEntry.id() ) )
                            { return false; }
                    }
                }
                else {
                    // Node must not overlap with other colleagues
                    if ( BubbleOverlaps( plannedEntry.location() ) )
                        { return false; }
                }
                break;
            }
            
            case NodeRelationType::Neighbour:
            {
                // Neighbour limit will be exceeded, check if new entry deserves to temporarilty go over limit
                vector<NodeInfo> neighboursByDistance( GetNeighbourNodesByDistance() );
                if ( neighboursByDistance.size() >= NEIGHBOURHOOD_MAX_NODE_COUNT )
                {
                    // If we have too many closer neighbours already, deny request
                    const NodeInfo &limitNeighbour = neighboursByDistance[NEIGHBOURHOOD_MAX_NODE_COUNT - 1];
                    if ( _spatialDb->GetDistanceKm( limitNeighbour.location(), myNode.location() ) <=
                         _spatialDb->GetDistanceKm( myNode.location(), plannedEntry.location() ) )
                    { return false; }
                }
                break;
            }
            
            case NodeRelationType::Self:
                throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Forbidden operation: must not overwrite self here");
            
            default:
                throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Unknown nodetype, missing implementation");
        }
        
        NodeDbEntry entryToWrite(plannedEntry);
        if ( plannedEntry.roleType() == NodeContactRoleType::Initiator )
        {
            // If no connection argument is specified, try connecting to candidate node
            if (nodeConnection == nullptr)
                { nodeConnection = SafeConnectTo( plannedEntry.contact().nodeEndpoint() ); }
            if (nodeConnection == nullptr)
                { return false; }
            
            // Ask for its permission for mutual acceptance
            shared_ptr<NodeInfo> freshInfo;
            switch ( plannedEntry.relationType() )
            {
                case NodeRelationType::Colleague:
                    freshInfo = storedInfo && storedInfo->relationType() == plannedEntry.relationType() ?
                        nodeConnection->RenewColleague(myNode) :
                        nodeConnection->AcceptColleague(myNode);
                    break;
                
                case NodeRelationType::Neighbour:
                    freshInfo = storedInfo && storedInfo->relationType() == plannedEntry.relationType() ?
                        nodeConnection->RenewNeighbour(myNode) :
                        nodeConnection->AcceptNeighbour(myNode);
                    break;
                
                case NodeRelationType::Self:
                    throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Forbidden operation: must not overwrite self here");
                
                default: throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Unknown relationtype, missing implementation");
            }
            
            // Request was denied
            if (freshInfo == nullptr)
                { return false; }
            
            // Node identity is questionable
            if ( freshInfo->id() != plannedEntry.id() )
            {
                LOG(WARNING) << "Asked permission from node with id " << plannedEntry.id()
                             << " but returned node info has node id " << freshInfo->id();
                return false;
            }
            
            entryToWrite = NodeDbEntry( *freshInfo, plannedEntry.relationType(), plannedEntry.roleType() );
        }
        
        // TODO consider if all further possible sanity checks are done already
        if (storedInfo == nullptr) { _spatialDb->Store(entryToWrite); }
        else                       { _spatialDb->Update(entryToWrite); }
        return true;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Unexpected error validating and storing node: " << e.what();
    }
    
    return false;
}



bool Node::InitializeWorld()
{
    LOG(DEBUG) << "Discovering world map for colleagues";
    
    // Initialize random generator and utility variables
    uniform_int_distribution<int> fromRange( 0, _seedNodes.size() - 1 );
    vector<NetworkEndpoint> triedNodes;
    
    size_t nodeCountAtSeed = 0;
    vector<NodeInfo> randomColleagueCandidates;
    while ( triedNodes.size() < _seedNodes.size() )
    {
        // Select random node from hardwired seed node list
        size_t selectedSeedNodeIdx = fromRange(_randomDevice);
        const NetworkEndpoint& selectedSeedContact = _seedNodes[selectedSeedNodeIdx];
                
        // If node has been already tried and failed, choose another one
        auto it = find( triedNodes.begin(), triedNodes.end(), selectedSeedContact );
        if ( it != triedNodes.end() )
            { continue; }
        
        try
        {
            triedNodes.push_back(selectedSeedContact);
            
            // Try connecting to selected seed node
            shared_ptr<INodeMethods> seedNodeConnection = SafeConnectTo(selectedSeedContact);
            if (seedNodeConnection == nullptr)
                { continue; }
            
            // Try to add seed node to our network (no matter if fails)
            NodeInfo seedInfo = seedNodeConnection->GetNodeInfo();
            SafeStoreNode( NodeDbEntry(seedInfo, NodeRelationType::Colleague, NodeContactRoleType::Initiator),
                           seedNodeConnection );
            
            // Query both total node count and an initial list of random nodes to start with
            LOG(DEBUG) << "Getting node count from initial seed";
            nodeCountAtSeed = seedNodeConnection->GetNodeCount();
            LOG(DEBUG) << "Node count on seed is " << nodeCountAtSeed;
            randomColleagueCandidates = seedNodeConnection->GetRandomNodes(
                min(INIT_WORLD_RANDOM_NODE_COUNT, nodeCountAtSeed), Neighbours::Included );
            
            // If got a reasonable response from a seed server, stop contacting other seeds
            if ( nodeCountAtSeed > 0 && ! randomColleagueCandidates.empty() )
                { break; }
        }
        catch (exception &e)
        {
            LOG(WARNING) << "Failed to bootstrap from seed node " << selectedSeedContact
                         << ": " << e.what() << ", trying other seeds";
        }
    }
    
    // Check if all seed nodes tried and failed
    if ( nodeCountAtSeed == 0 && randomColleagueCandidates.empty() &&
         triedNodes.size() == _seedNodes.size() )
    {
        LOG(ERROR) << "All seed nodes have been tried and failed";
        return false;
    }
    
    // We received a reasonable random node list from a seed, try to fill in our world map
    size_t targetNodeCount = static_cast<size_t>( ceil(INIT_WORLD_NODE_FILL_TARGET_RATE * nodeCountAtSeed) );
    LOG(DEBUG) << "Targeted node count is " << targetNodeCount;
    
    // Keep trying until we either reached targeted node count or queried colleagues from all available nodes
    while ( GetNodeCount() < targetNodeCount && triedNodes.size() < GetNodeCount() )
    {
        if ( ! randomColleagueCandidates.empty() )
        {
            // Pick a single node from the candidate list and try to make it a colleague node
            NodeInfo nodeInfo( randomColleagueCandidates.back() );
            randomColleagueCandidates.pop_back();
            
            // Check if we tried it already
            if ( find( triedNodes.begin(), triedNodes.end(), nodeInfo.contact().nodeEndpoint() ) != triedNodes.end() )
                { continue; }
            
            triedNodes.push_back( nodeInfo.contact().nodeEndpoint() );
            
            SafeStoreNode( NodeDbEntry(nodeInfo, NodeRelationType::Colleague, NodeContactRoleType::Initiator) );
        }
        else // We ran out of colleague candidates, try pick some more randomly
        {
            LOG(TRACE) << "Run out of colleague candidates, asking randomly for more";
            
            // Get a shuffled list of all colleague nodes known so far
            vector<NodeInfo> nodesKnownSoFar = GetRandomNodes( GetNodeCount(), Neighbours::Included );
            
            for (const auto &nodeInfo : nodesKnownSoFar)
            {
                try
                {
                    // Connect to selected random node
                    shared_ptr<INodeMethods> randomConnection = SafeConnectTo( nodeInfo.contact().nodeEndpoint() );
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
    
    LOG(DEBUG) << "World discovery finished with node count " << GetNodeCount();
    return true;
}



bool Node::InitializeNeighbourhood()
{
    LOG(DEBUG) << "Discovering neighbourhood";
    
    NodeDbEntry myNode = _spatialDb->ThisNode();
    vector<NodeInfo> newClosestNodes = GetClosestNodesByDistance(
        myNode.location(), numeric_limits<Distance>::max(), 2, Neighbours::Included);
    if ( newClosestNodes.size() < 2 ) // First node is self
    {
        LOG(DEBUG) << "No other nodes are available beyond self, cannot get neighbour candidates";
        return false;
    }
    
    // Repeat asking the currently closest node for an even closer node until no new node discovered
    NodeInfo newClosestNode = newClosestNodes[1];
    NodeInfo oldClosestNode = newClosestNode;
    do {
        LOG(TRACE) << "Closest node known so far: " << newClosestNode;
        oldClosestNode = newClosestNode;
        try
        {
            shared_ptr<INodeMethods> closestNodeConnection = SafeConnectTo( newClosestNode.contact().nodeEndpoint() );
            if (closestNodeConnection == nullptr) {
                // TODO consider what better to do if closest node is not reachable?
                continue;
            }
            
            newClosestNodes = closestNodeConnection->GetClosestNodesByDistance(
                myNode.location(), numeric_limits<Distance>::max(), 2, Neighbours::Included);
            if ( newClosestNodes.empty() )
                { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Node returned empty node list result"); }
                
            if ( newClosestNodes.front().id() != myNode.id() )
                { newClosestNode = newClosestNodes[0]; }
            else if ( newClosestNodes.size() > 1 )
                { newClosestNode = newClosestNodes[1]; }
        }
        catch (exception &e) {
            LOG(WARNING) << "Failed to fetch neighbours: " << e.what();
            // TODO consider what else to do here
        }
    }
    while ( oldClosestNode.id() != newClosestNode.id() );
    
    deque<NodeInfo> nodesToAskQueue{newClosestNode};
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
        auto askedIt = find( askedNodeIds.begin(), askedNodeIds.end(), neighbourCandidate.id() );
        if ( askedIt != askedNodeIds.end() )
            { continue; }
            
        try
        {
            // Try connecting to the node
            shared_ptr<INodeMethods> candidateConnection = SafeConnectTo( neighbourCandidate.contact().nodeEndpoint() );
            if (candidateConnection == nullptr)
                { continue; }
                
            // Try to add node as neighbour, reusing connection
            SafeStoreNode( NodeDbEntry(neighbourCandidate, NodeRelationType::Neighbour, NodeContactRoleType::Initiator),
                           candidateConnection );
            
            // Get its neighbours closest to us
            vector<NodeInfo> newNeighbourCandidates = candidateConnection->GetClosestNodesByDistance(
                myNode.location(), numeric_limits<Distance>::max(),
                INIT_NEIGHBOURHOOD_QUERY_NODE_COUNT, Neighbours::Included );
            
            // Mark current node as processed and append its new neighbours to our todo list
            askedNodeIds.insert( neighbourCandidate.id() );
            nodesToAskQueue.insert( nodesToAskQueue.end(),
                newNeighbourCandidates.begin(), newNeighbourCandidates.end() );
        }
        catch (exception &e) {
            LOG(WARNING) << "Failed to add neighbour node: " << e.what();
            // TODO consider what else to do here?
        }
    }
    
    LOG(DEBUG) << "Neighbourhood discovery finished with node count " << GetNodeCount();
    return true;
}



void Node::ExpireOldNodes()
{
    _spatialDb->ExpireOldNodes();
    EnsureMapFilled();
}



void Node::RenewNodeRelations()
{
    vector<NodeDbEntry> contactedNodes = _spatialDb->GetNodes(NodeContactRoleType::Initiator);
    for (auto const &node : contactedNodes)
    {
        try
        {
            bool renewed = SafeStoreNode(node);
            LOG(DEBUG) << "Attempted renewing relation with node " << node.id() << ", result: " << renewed;
        }
        catch (exception &e)
        {
            LOG(WARNING) << "Unexpected error renewing relation for node "
                         << node.id() << " : " << e.what();
        }
    }
}


void Node::DiscoverUnknownAreas()
{
    for (size_t i = 0; i < PERIODIC_DISCOVERY_ATTEMPT_COUNT; ++i)
    {
        // Generate a random GPS location
        uniform_real_distribution<GpsCoordinate> latitudeRange(-90.0, 90.0);
        uniform_real_distribution<GpsCoordinate> longitudeRange(-180.0, 180.0);
        GpsLocation randomLocation( latitudeRange(_randomDevice), longitudeRange(_randomDevice) );
            
        try
        {
            // TODO consider: the resulted node might be very far away from the generated random node,
            //      so prefiltering might cause bad results, but it also spares network costs. Test and decide!
            //      If this is not commented, we should separately take care about discovering new neighbours.
            // if ( BubbleOverlaps(randomLocation) )
            //    { continue; }
            
            NodeInfo myNodeInfo = _spatialDb->ThisNode();
            
            // Get node closest to this position that is already present in our database
            vector<NodeInfo> myClosestNodes = GetClosestNodesByDistance(
                randomLocation, numeric_limits<Distance>::max(), 2, Neighbours::Excluded );
            if ( myClosestNodes.empty() ||
                 ( myClosestNodes.size() == 1 && myNodeInfo == myClosestNodes[0] ) )
                { continue; }
            const auto &myClosestNode = myClosestNodes[0] != myNodeInfo ?
                myClosestNodes[0] : myClosestNodes[1];
            
            // Connect to closest node
            shared_ptr<INodeMethods> connection = SafeConnectTo( myClosestNode.contact().nodeEndpoint() );
            if (connection == nullptr)
            {
                LOG(DEBUG) << "Failed to contact node " << myClosestNode.id();
                continue;
            }
            
            // Ask closest node about its nodes closest to the random position
            vector<NodeInfo> gotClosestNodes = connection->GetClosestNodesByDistance(
                randomLocation, numeric_limits<Distance>::max(), 1, Neighbours::Included );
            if ( gotClosestNodes.empty() || gotClosestNodes[0] == myNodeInfo )
                { continue; }
            const auto &gotClosestNode = gotClosestNodes[0];
            
            // Try to add node to our database
            bool storedAsNeighbour = SafeStoreNode( NodeDbEntry( gotClosestNode,
                NodeRelationType::Neighbour, NodeContactRoleType::Initiator), connection );
            if (! storedAsNeighbour) {
                SafeStoreNode( NodeDbEntry( gotClosestNode,
                    NodeRelationType::Colleague, NodeContactRoleType::Initiator), connection );
            }
        }
        catch (exception &ex)
        {
            LOG(INFO) << "Failed to discover location " << randomLocation << ": " << ex.what();
        }
    }
}



} // namespace LocNet
