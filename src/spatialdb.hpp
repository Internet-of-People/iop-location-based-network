#ifndef __LOCNET_SPATIAL_DATABASE_H__
#define __LOCNET_SPATIAL_DATABASE_H__

#include <memory>
#include <vector>

#include "basic.hpp"



namespace LocNet
{

    
enum class Neighbours : uint8_t
{
    Included = 1,
    Excluded = 2,
};



class NodeDbEntry : public NodeInfo
{
    NodeRelationType  _relationType;
    NodeContactRoleType _roleType;
    
public:
    
    NodeDbEntry(const NodeDbEntry& other);
    NodeDbEntry( const NodeProfile& profile, const GpsLocation& location,
                       NodeRelationType relationType, NodeContactRoleType roleType );
    NodeDbEntry( const NodeInfo& info,
                       NodeRelationType relationType, NodeContactRoleType roleType );
    
    NodeRelationType  relationType() const;
    NodeContactRoleType roleType() const;
};



class ISpatialDatabase
{
public:
    virtual Distance GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const = 0;
    
    virtual bool Store(const NodeDbEntry &node) = 0;
    virtual std::shared_ptr<NodeDbEntry> Load(const std::string &nodeId) const = 0;
    // NOTE relationtype and roletype cannot be modified during the update
    virtual bool Update(const NodeInfo &node) const = 0;
    virtual bool Remove(const std::string &nodeId) = 0;
    
    virtual size_t GetNodeCount(NodeRelationType relationType) const = 0;
    virtual Distance GetNeighbourhoodRadiusKm() const = 0;
    
    virtual std::vector<NodeInfo> GetClosestNodes(const GpsLocation &position,
        Distance Km, size_t maxNodeCount, Neighbours filter) const = 0;

    virtual std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
};


} // namespace LocNet


#endif // __LOCNET_SPATIAL_DATABASE_H__