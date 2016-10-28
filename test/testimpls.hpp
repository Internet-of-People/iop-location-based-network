#ifndef __IMPLEMENTATIONS_FOR_TESTING_H__
#define __IMPLEMENTATIONS_FOR_TESTING_H__

#include <string>

#include "locnet.hpp"



class DummySpatialDatabase : public ISpatialDatabase
{
    GpsLocation _myLocation;
    
public:
    
    DummySpatialDatabase(const GpsLocation& myLocation);

    Distance GetDistance(const GpsLocation &one, const GpsLocation &other) const override;

    bool Store(const LocNetNodeDbEntry &node) override;
    std::shared_ptr<LocNetNodeDbEntry> Load(const std::string &nodeId) const override;
    bool Update(const LocNetNodeInfo &node) const override;
    bool Remove(const std::string &nodeId) override;
    
    Distance GetNeighbourhoodRadiusKm() const override;
    size_t GetNodeCount(LocNetRelationType nodeType) const override;
    std::vector<LocNetNodeInfo> GetRandomNodes(size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<LocNetNodeInfo> GetClosestNodes(const GpsLocation &position,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
};



class DummyLocNetRemoteNodeConnectionFactory: public ILocNetRemoteNodeConnectionFactory
{
public:
    
    std::shared_ptr<ILocNetRemoteNode> ConnectTo(const NodeProfile &node) override;
};


#endif // __IMPLEMENTATIONS_FOR_TESTING_H__
