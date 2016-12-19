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


const size_t   NEIGHBOURHOOD_MAX_NODE_COUNT         = 50;

const size_t   INIT_WORLD_RANDOM_NODE_COUNT         = 50;
const float    INIT_WORLD_NODE_FILL_TARGET_RATE     = 0.75;
const size_t   INIT_NEIGHBOURHOOD_QUERY_NODE_COUNT  = 10;

const size_t   PERIODIC_DISCOVERY_ATTEMPT_COUNT     = 100;



random_device Node::_randomDevice;


Node::Node( const NodeInfo &myNodeInfo,
            shared_ptr<ISpatialDatabase> spatialDb,
            std::shared_ptr<INodeConnectionFactory> connectionFactory,
            const vector<NodeProfile> &seedNodes ) :
    _myNodeInfo(myNodeInfo), _spatialDb(spatialDb), _connectionFactory(connectionFactory)
{
    if (spatialDb == nullptr) {
        throw runtime_error("Invalid spatial database argument");
    }
    if (connectionFactory == nullptr) {
        throw runtime_error("Invalid connection factory argument");
    }
    
    if ( GetNodeCount() <= 1 && ! seedNodes.empty() )
    {
        bool discoverySucceeded = InitializeWorld(seedNodes) && InitializeNeighbourhood();
        if (! discoverySucceeded)
        {
            // This still might be normal if we're the very first seed node of the whole network
            auto seedIt = find( seedNodes.begin(), seedNodes.end(), _myNodeInfo.profile() );
            if ( seedIt == seedNodes.end() )
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


void Node::AddListener(shared_ptr<IChangeListener> listener)
    { _spatialDb->changeListenerRegistry().AddListener(listener); }

void Node::RemoveListener(const SessionId &sessionId)
    { _spatialDb->changeListenerRegistry().RemoveListener(sessionId); }



shared_ptr<NodeInfo> Node::AcceptColleague(const NodeInfo &node)
{
// TODO Sanity checks are performed in SafeStoreNode, should we reject the request
//      if the other node "forgot" about the existing relation or call the wrong method (accept vs renew)?
//      Should the two methods maybe merged instead?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.profile().id() );
//     if (storedInfo != nullptr)
//         { return false; } // We shouldn't have this colleague already
    
    bool success = SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Colleague, NodeContactRoleType::Acceptor) );
    return success ? shared_ptr<NodeInfo>( new NodeInfo(_myNodeInfo) ) : shared_ptr<NodeInfo>();
}


shared_ptr<NodeInfo> Node::RenewColleague(const NodeInfo& node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.profile().id() );
//     if (storedInfo == nullptr)
//         { return false; } // We should have this colleague already

    bool success = SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Colleague, NodeContactRoleType::Acceptor) );
    return success ? shared_ptr<NodeInfo>( new NodeInfo(_myNodeInfo) ) : shared_ptr<NodeInfo>();
}



shared_ptr<NodeInfo> Node::AcceptNeighbour(const NodeInfo &node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.profile().id() );
//     if (storedInfo != nullptr)
//         { return false; } // We shouldn't have this colleague already
    
    bool success = SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Neighbour, NodeContactRoleType::Acceptor) );
    return success ? shared_ptr<NodeInfo>( new NodeInfo(_myNodeInfo) ) : shared_ptr<NodeInfo>();
}


shared_ptr<NodeInfo> Node::RenewNeighbour(const NodeInfo& node)
{
// TODO Store conditions are checked in SafeStoreNode, should we reject the request?
//     shared_ptr<NodeInfo> storedInfo = _spatialDb->Load( node.profile().id() );
//     if (storedInfo == nullptr)
//         { return false; } // We should have this colleague already
        
    bool success = SafeStoreNode( NodeDbEntry(
        node, NodeRelationType::Neighbour, NodeContactRoleType::Acceptor) );
    return success ? shared_ptr<NodeInfo>( new NodeInfo(_myNodeInfo) ) : shared_ptr<NodeInfo>();
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
    Distance distance = _spatialDb->GetDistanceKm( _myNodeInfo.location(), location );
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


bool Node::SafeStoreNode(const NodeDbEntry& plannedEntry, shared_ptr<INodeMethods> nodeConnection)
{
    try
    {
        // We must not explicitly add or overwrite our own node info here.
        // Whether or not our own nodeinfo is stored in the db is an implementation detail of the SpatialDatabase.
        if ( static_cast<const NodeInfo&>(plannedEntry) == _myNodeInfo )
            { return false; }
        
        // Check if node is acceptable
        shared_ptr<NodeDbEntry> storedInfo = _spatialDb->Load( plannedEntry.profile().id() );
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
                        if ( BubbleOverlaps( plannedEntry.location(), plannedEntry.profile().id() ) )
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
                    if ( _spatialDb->GetDistanceKm( limitNeighbour.location(), _myNodeInfo.location() ) <=
                         _spatialDb->GetDistanceKm( _myNodeInfo.location(), plannedEntry.location() ) )
                    { return false; }
                }
                break;
            }
                
            default:
                throw runtime_error("Unknown nodetype, missing implementation");
        }
        
        NodeDbEntry entryToWrite(plannedEntry);
        if ( plannedEntry.roleType() == NodeContactRoleType::Initiator )
        {
            // If no connection argument is specified, try connecting to candidate node
            if (nodeConnection == nullptr)
                { nodeConnection = SafeConnectTo( plannedEntry.profile() ); }
            if (nodeConnection == nullptr)
                { return false; }
            
            // Ask for its permission for mutual acceptance
            shared_ptr<NodeInfo> freshInfo;
            switch ( plannedEntry.relationType() )
            {
                case NodeRelationType::Colleague:
                    freshInfo = storedInfo ? nodeConnection->RenewColleague(_myNodeInfo) :
                                             nodeConnection->AcceptColleague(_myNodeInfo);
                    break;
                
                case NodeRelationType::Neighbour:
                    freshInfo = storedInfo ? nodeConnection->RenewNeighbour(_myNodeInfo) :
                                             nodeConnection->AcceptNeighbour(_myNodeInfo);
                    break;
                    
                default: throw runtime_error("Unknown relationtype, missing implementation");
            }
            
            if (freshInfo == nullptr)
                { return false; }
            entryToWrite = NodeDbEntry( *freshInfo, plannedEntry.relationType(), plannedEntry.roleType() );
        }
        
        // TODO consider if all further possible sanity checks are done already
        if (storedInfo == nullptr) { _spatialDb->Store(entryToWrite); }
        else                       { _spatialDb->Update(entryToWrite); }
        return true;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Unexpected error storing node: " << e.what();
    }
    
    return false;
}



bool Node::InitializeWorld(const vector<NodeProfile> &seedNodes)
{
    // Initialize random generator and utility variables
    uniform_int_distribution<int> fromRange( 0, seedNodes.size() - 1 );
    vector<string> triedNodes;
    
    size_t nodeCountAtSeed = 0;
    vector<NodeInfo> randomColleagueCandidates;
    while ( triedNodes.size() < seedNodes.size() )
    {
        // Select random node from hardwired seed node list
        size_t selectedSeedNodeIdx = fromRange(_randomDevice);
        const NodeProfile& selectedSeedNode = seedNodes[selectedSeedNodeIdx];
        
        // If node has been already tried and failed, choose another one
        auto it = find( triedNodes.begin(), triedNodes.end(), selectedSeedNode.id() );
        if ( it != triedNodes.end() )
            { continue; }
        
        try
        {
            triedNodes.push_back( selectedSeedNode.id() );
            
            // Try connecting to selected seed node
            shared_ptr<INodeMethods> seedNodeConnection = SafeConnectTo(selectedSeedNode);
            if (seedNodeConnection == nullptr)
                { continue; }
            
            // Try to add seed node to our network (no matter if fails)
            bool seedAsNeighbour = SafeStoreNode( NodeDbEntry( selectedSeedNode, GpsLocation(0,0), // this will be queried in SafeStoreNode anyway
                NodeRelationType::Neighbour, NodeContactRoleType::Initiator), seedNodeConnection );
            if (! seedAsNeighbour) {
                SafeStoreNode( NodeDbEntry(selectedSeedNode, GpsLocation(0,0), // this will be queried in SafeStoreNode anyway
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
         triedNodes.size() == seedNodes.size() )
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
    while ( GetNodeCount() < targetNodeCount && triedNodes.size() < GetNodeCount() - 1 ) // Exclude self from tried count
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
            LOG(TRACE) << "Run out of colleague candites, asking randomly for more";
            
            // Get a shuffled list of all colleague nodes known so far
            vector<NodeInfo> nodesKnownSoFar = GetRandomNodes( GetNodeCount(), Neighbours::Excluded );
            
            for (const auto &nodeInfo : nodesKnownSoFar)
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
    
    LOG(DEBUG) << "World discovery finished, got " << GetNodeCount() << " nodes";
    return true;
}



bool Node::InitializeNeighbourhood()
{
    vector<NodeInfo> newClosestNodes = GetClosestNodesByDistance(
        _myNodeInfo.location(), numeric_limits<Distance>::max(), 2, Neighbours::Included);
    if ( newClosestNodes.size() < 2 ) // First node is self
        { return false; }
    
    // Repeat asking the currently closest node for an even closer node until no new node discovered
    NodeInfo newClosestNode = newClosestNodes[1];
    NodeInfo oldClosestNode = newClosestNode;
    do {
        LOG(TRACE) << "Closest node known so far: " << newClosestNode;
        oldClosestNode = newClosestNode;
        try
        {
            shared_ptr<INodeMethods> closestNodeConnection = SafeConnectTo( newClosestNode.profile() );
            if (closestNodeConnection == nullptr) {
                // TODO what to do if closest node is not reachable?
                return false;
            }
            
            newClosestNodes = closestNodeConnection->GetClosestNodesByDistance(
                _myNodeInfo.location(), numeric_limits<Distance>::max(), 2, Neighbours::Included);
            if ( newClosestNodes.empty() )
                { break; }
                
            if ( newClosestNodes.front().profile().id() != _myNodeInfo.profile().id() )
                { newClosestNode = newClosestNodes[0]; }
            else if ( newClosestNodes.size() > 1 )
                { newClosestNode = newClosestNodes[1]; }
        }
        catch (exception &e) {
            LOG(WARNING) << "Failed to fetch neighbours: " << e.what();
            // TODO consider what else to do here
        }
    }
    while ( oldClosestNode.profile().id() != newClosestNode.profile().id() );
    
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



void Node::ExpireOldNodes()
{
    _spatialDb->ExpireOldNodes();
}



void Node::RenewNodeRelations()
{
    vector<NodeDbEntry> contactedNodes = _spatialDb->GetNodes(NodeContactRoleType::Initiator);
    for (auto const &node : contactedNodes)
    {
        try
        {
            bool renewed = SafeStoreNode(node);
            LOG(DEBUG) << "Attempted renewing relation with node " << node.profile().id() << ", result: " << renewed;
        }
        catch (exception &e)
        {
            LOG(WARNING) << "Unexpected error renewing relation for node "
                         << node.profile().id() << " : " << e.what();
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
            // if ( BubbleOverlaps(randomLocation) )
            //    { continue; }
            
            // Get node closest to this position that is already present in our database
            vector<NodeInfo> myClosestNodes = GetClosestNodesByDistance(
                randomLocation, numeric_limits<Distance>::max(), 2, Neighbours::Excluded );
            if ( myClosestNodes.empty() ||
               ( myClosestNodes.size() == 1 && _myNodeInfo == myClosestNodes[0] ) )
                { continue; }
            const auto &myClosestNode = myClosestNodes[0] != _myNodeInfo ?
                myClosestNodes[0] : myClosestNodes[1];
            
            // Connect to closest node
            shared_ptr<INodeMethods> connection = SafeConnectTo( myClosestNode.profile() );
            if (connection == nullptr)
            {
                LOG(DEBUG) << "Failed to contact node " << myClosestNode.profile().id();
                continue;
            }
            
            // Ask closest node about its nodes closest to the random position
            vector<NodeInfo> gotClosestNodes = connection->GetClosestNodesByDistance(
                randomLocation, numeric_limits<Distance>::max(), 1, Neighbours::Included );
            if ( gotClosestNodes.empty() || gotClosestNodes[0] == _myNodeInfo )
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
