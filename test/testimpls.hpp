#ifndef __IMPLEMENTATIONS_FOR_TESTING_H__
#define __IMPLEMENTATIONS_FOR_TESTING_H__

#include <string>

#include "locnet.hpp"



namespace LocNet
{
    

class DummySpatialDatabase : public ISpatialDatabase
{
    GpsLocation _myLocation;
    std::unordered_map<std::string,NodeDbEntry> _nodes;
    
public:
    
    DummySpatialDatabase(const GpsLocation& myLocation);

    Distance GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const override;

    bool Store(const NodeDbEntry &node) override;
    std::shared_ptr<NodeDbEntry> Load(const std::string &nodeId) const override;
    bool Update(const NodeInfo &node) const override;
    bool Remove(const std::string &nodeId) override;
    
    Distance GetNeighbourhoodRadiusKm() const override;
    size_t GetNodeCount(NodeRelationType nodeType) const override;
    std::vector<NodeInfo> GetRandomNodes(size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeInfo> GetClosestNodes(const GpsLocation &position,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
};



class DummyLocNetRemoteNodeConnectionFactory: public IRemoteNodeConnectionFactory
{
public:
    
    std::shared_ptr<IRemoteNode> ConnectTo(const NodeProfile &node) override;
};


} // namespace LocNet


#endif // __IMPLEMENTATIONS_FOR_TESTING_H__
