#ifndef __IMPLEMENTATIONS_FOR_TESTING_H__
#define __IMPLEMENTATIONS_FOR_TESTING_H__

#include "locnet.hpp"



namespace LocNet
{
    

class DummySpatialDatabase : public ISpatialDatabase
{
    static std::random_device _randomDevice;
    
    // TODO probably would be better to store nodes in a vector in ascending order of distance from myLocation
    GpsLocation _myLocation;
    std::unordered_map<NodeId,NodeDbEntry> _nodes;
    
    std::vector<NodeInfo> GetNodes(NodeRelationType relationType) const;
    
public:
    
    DummySpatialDatabase(const GpsLocation &nodeLocation);
    
    Distance GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const override;

    bool Store(const NodeDbEntry &node) override;
    std::shared_ptr<NodeDbEntry> Load(const NodeId &nodeId) const override;
    bool Update(const NodeDbEntry &node) override;
    bool Remove(const NodeId &nodeId) override;
    
    void ExpireOldNodes() override;
    
    size_t GetColleagueNodeCount() const override;
    std::vector<NodeInfo> GetNeighbourNodesByDistance() const override;
    std::vector<NodeInfo> GetRandomNodes(size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &position,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
};



class DummyLocNetRemoteNodeConnectionFactory: public IRemoteNodeConnectionFactory
{
public:
    
    std::shared_ptr<IRemoteNode> ConnectTo(const NodeProfile &node) override;
};


} // namespace LocNet


#endif // __IMPLEMENTATIONS_FOR_TESTING_H__
