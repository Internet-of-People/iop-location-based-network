#ifndef __LOCNET_SPATIAL_DATABASE_H__
#define __LOCNET_SPATIAL_DATABASE_H__

#include <memory>
#include <sqlite3.h>
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
    NodeDbEntry( const NodeInfo& info,
                 NodeRelationType relationType, NodeContactRoleType roleType );
    NodeDbEntry( const NodeProfile& profile, const GpsLocation& location,
                 NodeRelationType relationType, NodeContactRoleType roleType );
    
    NodeRelationType  relationType() const;
    NodeContactRoleType roleType() const;
    
    bool operator==(const NodeDbEntry &other) const;
};



class ISpatialDatabase
{
public:
    virtual ~ISpatialDatabase() {}
    
    virtual Distance GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const = 0;
    
    virtual std::shared_ptr<NodeDbEntry> Load(const NodeId &nodeId) const = 0;
    virtual void Store (const NodeDbEntry &node, bool expires = true) = 0;
    virtual void Update(const NodeDbEntry &node, bool expires = true) = 0;
    virtual void Remove(const NodeId &nodeId) = 0;
    
    virtual void ExpireOldNodes() = 0;
    virtual std::vector<NodeDbEntry> GetNodes(NodeContactRoleType roleType) = 0;

    virtual size_t GetNodeCount() const = 0;
    virtual std::vector<NodeInfo> GetNeighbourNodesByDistance() const = 0;
    
    virtual std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &location,
        Distance maxRadiusKm, size_t maxNodeCount, Neighbours filter) const = 0;

    virtual std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
};



class SpatiaLiteDatabase : public ISpatialDatabase
{
    GpsLocation  _myLocation;
    sqlite3     *_dbHandle;
    void        *_spatialiteConnection;
    
public:
    
    static const std::string IN_MEMORY_DB;
    
    SpatiaLiteDatabase(const std::string &dbPath, const NodeInfo &myNodeInfo);
    virtual ~SpatiaLiteDatabase();
    
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
    
    std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
};


} // namespace LocNet


#endif // __LOCNET_SPATIAL_DATABASE_H__