#ifndef __LOCNET_SPATIAL_DATABASE_H__
#define __LOCNET_SPATIAL_DATABASE_H__

#include <memory>
#include <vector>

#include "basic.hpp"



enum class Neighbours : uint8_t
{
    Included = 1,
    Excluded = 2,
};



class LocNetNodeDbEntry : public LocNetNodeInfo
{
    LocNetRelationType  _relationType;
    PeerRoleType        _roleType;
    
public:
    
    LocNetNodeDbEntry( const NodeProfile& profile, const GpsLocation& location,
                       LocNetRelationType relationType, PeerRoleType roleType );
    LocNetNodeDbEntry( const LocNetNodeInfo& info,
                       LocNetRelationType relationType, PeerRoleType roleType );
    
    LocNetRelationType relationType() const;
    PeerRoleType       roleType() const;
};



class ISpatialDatabase
{
public:
    virtual Distance GetDistance(const GpsLocation &one, const GpsLocation &other) const = 0;
    
    virtual bool Store(const LocNetNodeDbEntry &node) = 0;
    virtual std::shared_ptr<LocNetNodeDbEntry> Load(const std::string &nodeId) const = 0;
    // NOTE relationtype and roletype cannot be modified during the update
    virtual bool Update(const LocNetNodeInfo &node) const = 0;
    virtual bool Remove(const std::string &nodeId) = 0;
    
    virtual size_t GetNodeCount(LocNetRelationType nodeType) const = 0;
    virtual Distance GetNeighbourhoodRadiusKm() const = 0;
    
    virtual std::vector<LocNetNodeInfo> GetClosestNodes(const GpsLocation &position,
        Distance Km, size_t maxNodeCount, Neighbours filter) const = 0;

    virtual std::vector<LocNetNodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
};




#endif // __LOCNET_SPATIAL_DATABASE_H__