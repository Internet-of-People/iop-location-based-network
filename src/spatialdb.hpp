#ifndef __GEONET_SPATIAL_DATABASE_H__
#define __GEONET_SPATIAL_DATABASE_H__

#include <memory>
#include <vector>

#include "basic.hpp"



enum class Neighbours : uint8_t
{
    Included = 1,
    Ignored  = 2,
};



class ISpatialDatabase
{
public:
    virtual Distance GetDistance(const GpsLocation &one, const GpsLocation &other) const = 0;
    
    virtual bool Store(const NodeInfo &node, NodeType nodeType, RelationType relationType) = 0;
    virtual std::shared_ptr<NodeInfo> Load(const std::string &nodeId) const = 0;
    virtual bool Update(const NodeInfo &node) const = 0;
    virtual bool Remove(const std::string &nodeId) = 0;
    
    virtual size_t GetNodeCount(NodeType nodeType) const = 0;
    virtual Distance GetNeighbourhoodRadiusKm() const = 0;
    
    virtual std::vector<NodeInfo> GetClosestNodes(const GpsLocation &position,
        Distance Km, size_t maxNodeCount, Neighbours filter) const = 0;

    virtual std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
};




#endif // __GEONET_SPATIAL_DATABASE_H__