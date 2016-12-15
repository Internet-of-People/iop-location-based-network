#ifndef __LOCNET_TEST_IMPLEMENTATIONS_H__
#define __LOCNET_TEST_IMPLEMENTATIONS_H__

#include "locnet.hpp"



namespace LocNet
{


class ChangeCounter : public IChangeListener
{
    ChangeListenerId _id;
    
public:
    
    size_t addedCount   = 0;
    size_t updatedCount = 0;
    size_t removedCount = 0;
    
    ChangeCounter(const ChangeListenerId &id);
    
    const ChangeListenerId& id() const override;
    
    void AddedNode  (const NodeDbEntry &node) override;
    void UpdatedNode(const NodeDbEntry &node) override;
    void RemovedNode(const NodeDbEntry &node) override;
};



// NOTE NOT PERSISTENT, suited for development/testing only
class InMemorySpatialDatabase : public ISpatialDatabase
{
    static std::random_device _randomDevice;
    
    GpsLocation _myLocation;
    std::unordered_map<NodeId,NodeDbEntry> _nodes;
    
    std::vector<NodeDbEntry> GetNodes(NodeRelationType relationType) const;
    
public:
    
    InMemorySpatialDatabase(const NodeInfo &myNodeInfo);
    
    Distance GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const override;

    std::shared_ptr<NodeDbEntry> Load(const NodeId &nodeId) const override;
    void Store (const NodeDbEntry &node, bool expires = true) override;
    void Update(const NodeDbEntry &node, bool expires = true) override;
    void Remove(const NodeId &nodeId) override;
    void ExpireOldNodes() override;
    
    void AddListener(std::shared_ptr<IChangeListener> listener) override;
    void RemoveListener(ChangeListenerId listenerId) override;
    
    std::vector<NodeDbEntry> GetNodes(NodeContactRoleType roleType) override;
    
    size_t GetNodeCount() const override;
    std::vector<NodeDbEntry> GetNeighbourNodesByDistance() const override;
    std::vector<NodeDbEntry> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeDbEntry> GetClosestNodesByDistance(const GpsLocation &position,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
};

    

class DummyNodeConnectionFactory: public INodeConnectionFactory
{
public:
    
    std::shared_ptr<INodeMethods> ConnectTo(const NodeProfile &node) override;
};


} // namespace LocNet


#endif // __LOCNET_TEST_IMPLEMENTATIONS_H__
