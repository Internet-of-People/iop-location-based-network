#include <algorithm>
#include <cmath>
#include <chrono>
#include <deque>
#include <limits>
#include <unordered_set>

#include <easylogging++.h>

#include "config.hpp"
#include "locnet.hpp"

using namespace std;



namespace LocNet
{


const float    INIT_WORLD_NODE_FILL_TARGET_RATE     = 0.75;
const size_t   INIT_NEIGHBOURHOOD_QUERY_NODE_COUNT  = 10;

const size_t   PERIODIC_DISCOVERY_ATTEMPT_COUNT     = 5;



random_device Node::_randomDevice;


Node::Node( shared_ptr<ISpatialDatabase> spatialDb,
            std::shared_ptr<INodeConnectionFactory> connectionFactory) :
    _spatialDb(spatialDb), _connectionFactory(connectionFactory)
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
    const vector<NetworkEndpoint> &seedNodes = Config::Instance().seedNodes();
    if ( GetNodeCount() <= 1 && ! seedNodes.empty() )
    {
        LOG(INFO) << "Map is empty, discovering the network";
        
        bool discoverySucceeded = InitializeWorld(seedNodes) && InitializeNeighbourhood();
        if (! discoverySucceeded)
            { LOG(WARNING) << "Failed to properly discover the full network, current node count is " << GetNodeCount(); }
        if ( GetNodeCount() <= 1 )
        {
            // This still might be normal if we're the very first seed node of the whole network
            NodeInfo myInfo = _spatialDb->ThisNode();
            auto seedIt = find_if( seedNodes.begin(), seedNodes.end(),
                [&myInfo] (const NetworkEndpoint &contact)
                    { return myInfo.contact().nodeEndpoint() == contact; } );
            if ( seedIt == seedNodes.end() )
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
    
    RenewNeighbours();
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
    
    RenewNeighbours();
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
        
        // TODO normally we should immediately start distributing updated node info,
        //      but IP may change again during the process and may behave Unexpectedly. What to do here?
        // RenewNeighbours();
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
    if ( endpoint == _spatialDb->ThisNode().contact().nodeEndpoint() ||
         ( endpoint.isLoopback() && ! Config::Instance().isTestMode() ) )
    {
        LOG(TRACE) << "Address " << endpoint << " is self or local, refusing";
        return shared_ptr<INodeMethods>();
    }
    
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
        {
            LOG(TRACE) << "Attempt to store self, refusing";
            return false;
        }
     
        // Validate if node is acceptable
        shared_ptr<NodeDbEntry> storedInfo = _spatialDb->Load( plannedEntry.id() );
        if ( storedInfo && storedInfo->relationType() == NodeRelationType::Self )
        {
            LOG(TRACE) << "Attempt to overwrite self, refusing";
            throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Forbidden operation: must not overwrite self here");
        }
        
        switch ( plannedEntry.relationType() )
        {
            case NodeRelationType::Colleague:
            {
                if (storedInfo != nullptr)
                {
                    // Existing colleague info may be upgraded to neighbour but not vica versa
                    if ( storedInfo->relationType() == NodeRelationType::Neighbour )
                    {
                        LOG(TRACE) << "Attempt to downgrade neighbour as colleague, refusing colleague";
                        return false;
                    }
                    if ( storedInfo->location() != plannedEntry.location() ) {
                        // Node must not be moved away to a position that overlaps with anything other than itself
                        if ( BubbleOverlaps( plannedEntry.location(), plannedEntry.id() ) )
                        {
                            LOG(TRACE) << "Bubble of changed node location would overlap, refusing colleague";
                            return false;
                        }
                    }
                }
                else {
                    // New node must not overlap with other colleagues
                    if ( BubbleOverlaps( plannedEntry.location() ) )
                    {
                        LOG(TRACE) << "Node bubble would overlap, refusing colleague";
                        return false;
                    }
                }
                break;
            }
            
            case NodeRelationType::Neighbour:
            {
                size_t neighbourhoodTargetSize = Config::Instance().neighbourhoodTargetSize();
                vector<NodeInfo> neighboursByDistance( GetNeighbourNodesByDistance() );
                if (storedInfo == nullptr || storedInfo->relationType() == NodeRelationType::Colleague)
                {
                    // Received a new neighbour request
                    if ( neighboursByDistance.size() >= neighbourhoodTargetSize )
                    {
                        // Neighbour limit is exceeded by adding a new neighbour, but if it is closer
                        // than an old neighbour within the limit then we can temporarily break the limit
                        // and will later refuse renewal of the faraway old neighbour to let it expire
                        const NodeInfo &limitNeighbour = neighboursByDistance[neighbourhoodTargetSize - 1];
                        LOG(TRACE) << "We have reached the neighbour limit " << neighbourhoodTargetSize
                                   << ", farthest neighbour within limit is " << limitNeighbour;
                        if ( _spatialDb->GetDistanceKm( myNode.location(), limitNeighbour.location() ) <=
                             _spatialDb->GetDistanceKm( myNode.location(), plannedEntry.location() ) )
                        {
                            LOG(TRACE) << neighbourhoodTargetSize << " closer neighbours found, refusing to add new";
                            return false;
                        }
                    }
                }
                else
                {
                    // Renewal of an old neighbour
                    auto neighbourIter = find_if( neighboursByDistance.begin(), neighboursByDistance.end(),
                        [plannedEntry] (const NodeInfo &neighbour) { return neighbour.id() == plannedEntry.id(); } );
                    if ( neighbourIter == neighboursByDistance.end() )
                    {
                        LOG(ERROR) << "Implementation problem: stored neighbour is not found in neighbour list";
                        throw LocationNetworkError(ErrorCode::ERROR_CONCEPTUAL, "Please report this to the developers");
                    }
                    // Don't care about location change here. IF moved too far away we will expire it
                    // at the next renewal request when it's at its new place in the neighbour list.
                    size_t neighbourIndex = distance( neighboursByDistance.begin(), neighbourIter );
                    if (neighbourIndex >= neighbourhoodTargetSize)
                    {
                         LOG(TRACE) << neighbourhoodTargetSize << " neighbours limit reached, refusing to renew neighbour nr. " << neighbourIndex;
                         return false;
                    }
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
            {
                LOG(TRACE) << "Failed to connect to remote node to ask for permission, refusing";
                return false;
            }
            
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
            {
                LOG(TRACE) << "Accept/renew request was denied";
                return false;
            }
            
            // Node identity is questionable
            if ( freshInfo->id() != plannedEntry.id() )
            {
                LOG(WARNING) << endl
                    << "Contacted node has different identity than expected." << endl
                    << "  Expected: " << plannedEntry << endl
                    << "  Reported: " << *freshInfo << endl;
                return false;
            }
            
            entryToWrite = NodeDbEntry( *freshInfo, plannedEntry.relationType(), plannedEntry.roleType() );
        }
        
        // TODO consider if all further possible sanity checks are done already
        if (storedInfo == nullptr)
        {
            LOG(DEBUG) << "Storing node info " << entryToWrite;
            _spatialDb->Store(entryToWrite);
        }
        else
        {
            LOG(DEBUG) << "Updating node info " << entryToWrite;
            _spatialDb->Update(entryToWrite);
        }
        return true;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Unexpected error validating and storing node: " << e.what();
    }
    
    return false;
}



bool Node::InitializeWorld(const vector<NetworkEndpoint> &seedNodes)
{
    LOG(DEBUG) << "Discovering world map for colleagues";
    const size_t INIT_WORLD_RANDOM_NODE_COUNT = 2 * Config::Instance().neighbourhoodTargetSize();
    
    // Initialize random generator and utility variables
    uniform_int_distribution<int> fromRange( 0, seedNodes.size() - 1 );
    vector<NetworkEndpoint> triedNodes;
    
    size_t nodeCountAtSeed = 0;
    vector<NodeInfo> randomColleagueCandidates;
    while ( triedNodes.size() < seedNodes.size() )
    {
        // Select random node from hardwired seed node list
        // TODO it would be better to create a permutation of the seedNodes vector and just iterate on it
        size_t selectedSeedNodeIdx = fromRange(_randomDevice);
        const NetworkEndpoint& selectedSeedContact = seedNodes[selectedSeedNodeIdx];
                
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
         triedNodes.size() == seedNodes.size() )
    {
        LOG(ERROR) << "All seed nodes have been tried and failed";
        return false;
    }
    
    // We received a reasonable random node list from a seed, try to fill in our world map
    size_t targetNodeCount = static_cast<size_t>( ceil(INIT_WORLD_NODE_FILL_TARGET_RATE * nodeCountAtSeed) );
    LOG(DEBUG) << "Targeted node count is " << targetNodeCount;
    
    // Keep trying until we either reached targeted node count or run out of all candidates
    while ( GetNodeCount() < targetNodeCount &&
            ( ! randomColleagueCandidates.empty() || triedNodes.size() < GetNodeCount() ) )
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
    
    LOG(DEBUG) << "World discovery finished with total node count " << GetNodeCount();
    return true;
}



bool Node::InitializeNeighbourhood()
{
    LOG(DEBUG) << "Discovering neighbourhood";
    
    NodeDbEntry myNode = _spatialDb->ThisNode();
    vector<NodeInfo> newClosestNodes = GetClosestNodesByDistance(
        myNode.location(), numeric_limits<Distance>::max(), 2, Neighbours::Included);
    if ( newClosestNodes.size() >= 1 && newClosestNodes[0] != GetNodeInfo() )
    {
        LOG(ERROR) << "Implementation problem: assumption failed";
        throw LocationNetworkError(ErrorCode::ERROR_CONCEPTUAL, "Please report this to the developers");
    }
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
    while ( neighbourCandidates.size() < Config::Instance().neighbourhoodTargetSize() &&
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
    
    LOG(DEBUG) << "Neighbourhood discovery finished with total node count " << GetNodeCount()
               << ", neighbourhood size is " << _spatialDb->GetNeighbourNodesByDistance().size();
    return true;
}



void Node::ExpireOldNodes()
{
    LOG(DEBUG) << "Deleting expired node connections";
    _spatialDb->ExpireOldNodes();
    EnsureMapFilled();
}



void Node::RenewNodeRelations()
{
    vector<NodeDbEntry> nodesToContact( _spatialDb->GetNodes(NodeContactRoleType::Initiator) );
    LOG(DEBUG) << "We have " << nodesToContact.size() << " relations to renew";
    for (auto const &node : nodesToContact)
    {
        try
        {
            bool renewed = SafeStoreNode(node);
            LOG(DEBUG) << "Attempted renewing relation with node " << node.id() << ", result: " << renewed;
        }
        catch (exception &e)
        {
            LOG(WARNING) << "Unexpected error renewing relation with node "
                         << node.id() << " : " << e.what();
        }
    }
}


void Node::RenewNeighbours()
{
    vector<NodeDbEntry> neighbours( _spatialDb->GetNeighbourNodesByDistance() );
    LOG(DEBUG) << "Updating changed node details on " << neighbours.size() << " neighbours";
    for (auto const &neighbour : neighbours)
    {
        try
        {
            // TODO this should not block the response message, should just post tasks to the io_service to be executed later.
            bool updated = SafeStoreNode( NodeDbEntry(neighbour,
                NodeRelationType::Neighbour, NodeContactRoleType::Initiator) );
            LOG(DEBUG) << "Attempted updating changed self info on neighbour " << neighbour.id() << ", result: " << updated;
        }
        catch (exception &e)
        {
            LOG(WARNING) << "Unexpected error updating changed self info on neighbour "
                         << neighbour.id() << " : " << e.what();
        }
    }
}



void Node::DiscoverUnknownAreas()
{
    LOG(DEBUG) << "Exploring white spots of the map";
    
    for (size_t i = 0; i < PERIODIC_DISCOVERY_ATTEMPT_COUNT; ++i)
    {
        // Generate a random GPS location
        uniform_real_distribution<GpsCoordinate> latitudeRange(-90.0, 90.0);
        uniform_real_distribution<GpsCoordinate> longitudeRange(-180.0, 180.0);
        GpsLocation randomLocation( latitudeRange(_randomDevice), longitudeRange(_randomDevice) );
            
        try
        {
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
            shared_ptr<INodeMethods> knownNodeConnection = SafeConnectTo( myClosestNode.contact().nodeEndpoint() );
            if (knownNodeConnection == nullptr)
            {
                LOG(DEBUG) << "Failed to contact known node " << myClosestNode;
                continue;
            }
            
            // Ask closest node about its nodes closest to the random position
            vector<NodeInfo> newClosestNodes = knownNodeConnection->GetClosestNodesByDistance(
                randomLocation, numeric_limits<Distance>::max(), 1, Neighbours::Included );
            if ( newClosestNodes.empty() || newClosestNodes[0].id() == myNodeInfo.id() )
                { continue; }
            const auto &newClosestNode = newClosestNodes[0];
            LOG(DEBUG) << "Closest node to random position is " << newClosestNode;
            
// TODO probably we should also ask a random seed node here and get the closest of the two results
//      if both available. This might help rejoining a splitted network.
            
            // If we already know this node, nothing to do here, renewals will keep it alive
            shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( newClosestNode.id() );
            if (storedInfo != nullptr)
            {
                LOG(DEBUG) << "Closest node is already present: " << *storedInfo;
                continue;
            }
            
            // Connect to closest node
            shared_ptr<INodeMethods> discoveredNodeConnection = SafeConnectTo( newClosestNode.contact().nodeEndpoint() );
            if (discoveredNodeConnection == nullptr)
            {
                LOG(DEBUG) << "Failed to contact discovered node " << newClosestNode;
                continue;
            }
            
            // Try to add node to our database
            bool storedAsNeighbour = SafeStoreNode( NodeDbEntry( newClosestNode,
                NodeRelationType::Neighbour, NodeContactRoleType::Initiator), discoveredNodeConnection );
            if (! storedAsNeighbour) {
                SafeStoreNode( NodeDbEntry( newClosestNode,
                    NodeRelationType::Colleague, NodeContactRoleType::Initiator), discoveredNodeConnection );
            }
        }
        catch (exception &ex)
        {
            LOG(INFO) << "Failed to discover location " << randomLocation << ": " << ex.what();
        }
    }
    
    LOG(DEBUG) << "Exploration finished";
}



} // namespace LocNet
