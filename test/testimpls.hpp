#ifndef __LOCNET_TEST_IMPLEMENTATIONS_H__
#define __LOCNET_TEST_IMPLEMENTATIONS_H__

#include "locnet.hpp"



namespace LocNet
{


// NOTE NOT PERSISTENT, suited for development/testing only
class InMemorySpatialDatabase : public ISpatialDatabase
{
    static std::random_device _randomDevice;
    
    GpsLocation _myLocation;
    std::unordered_map<NodeId,NodeDbEntry> _nodes;
    
    std::vector<NodeInfo> GetNodes(NodeRelationType relationType) const;
    
public:
    
    InMemorySpatialDatabase(const NodeInfo &myNodeInfo);
    
    Distance GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const override;

    std::shared_ptr<NodeDbEntry> Load(const NodeId &nodeId) const override;
    void Store (const NodeDbEntry &node, bool expires = true) override;
    void Update(const NodeDbEntry &node, bool expires = true) override;
    void Remove(const NodeId &nodeId) override;
    
    void ExpireOldNodes() override;
    std::vector<NodeDbEntry> GetNodes(NodeContactRoleType roleType) override;
    
    size_t GetNodeCount() const override;
    std::vector<NodeInfo> GetNeighbourNodesByDistance() const override;
    std::vector<NodeInfo> GetRandomNodes(size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &position,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
};

    

class DummyNodeConnectionFactory: public INodeConnectionFactory
{
public:
    
    std::shared_ptr<INodeMethods> ConnectTo(const NodeProfile &node) override;
};


} // namespace LocNet


#endif // __LOCNET_TEST_IMPLEMENTATIONS_H__
