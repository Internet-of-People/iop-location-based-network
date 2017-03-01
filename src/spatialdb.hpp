#ifndef __LOCNET_SPATIAL_DATABASE_H__
#define __LOCNET_SPATIAL_DATABASE_H__

#include <chrono>
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include <vector>

#include "basic.hpp"



namespace LocNet
{



enum class NodeRelationType : uint8_t
{
    Colleague   = 1,
    Neighbour   = 2,
    Self        = 3,
};


enum class NodeContactRoleType : uint8_t
{
    Initiator   = 1,
    Acceptor    = 2,
    Self        = 3,
};


// Filter flag for node queries
enum class Neighbours : uint8_t
{
    Included = 1,
    Excluded = 2,
};



// Data holder class for full node information stored in the database.
class NodeDbEntry : public NodeInfo
{
    NodeRelationType    _relationType;
    NodeContactRoleType _roleType;
    
public:
    
    NodeDbEntry(const NodeDbEntry& other);
    NodeDbEntry( const NodeInfo& info,
                 NodeRelationType relationType, NodeContactRoleType roleType );
    
    NodeRelationType relationType() const;
    NodeContactRoleType roleType() const;
    
    bool operator==(const NodeDbEntry &other) const;
};



// Interface to listen for any changes in the node map.
class IChangeListener
{
public:
    
    virtual ~IChangeListener() {}
    
    virtual const SessionId& sessionId() const = 0;
    
    virtual void OnRegistered() = 0;
    virtual void AddedNode  (const NodeDbEntry &node) = 0;
    virtual void UpdatedNode(const NodeDbEntry &node) = 0;
    virtual void RemovedNode(const NodeDbEntry &node) = 0;
};


// Interface where node listeners should be registered.
class IChangeListenerRegistry
{
public:
    
    virtual ~IChangeListenerRegistry() {}
    
    virtual void AddListener(std::shared_ptr<IChangeListener> listener) = 0;
    virtual void RemoveListener(const SessionId &sessionId) = 0;
};



// A node database without any business logic that serves usual CRUD operations
// plus some additional utility queries mostly based on ordering by node distance.
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
    
    virtual IChangeListenerRegistry& changeListenerRegistry() = 0;

    virtual NodeDbEntry ThisNode() const = 0;
    virtual std::vector<NodeDbEntry> GetNodes(NodeContactRoleType roleType) = 0;

    virtual size_t GetNodeCount() const = 0;
    virtual std::vector<NodeDbEntry> GetNeighbourNodesByDistance() const = 0;
    
    virtual std::vector<NodeDbEntry> GetClosestNodesByDistance(
        const GpsLocation &location, Distance maxRadiusKm, size_t maxNodeCount, Neighbours filter) const = 0;

    virtual std::vector<NodeDbEntry> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
};



// A simple map implementation of a listener registry, protected by a lock to be threadsafe.
class ThreadSafeChangeListenerRegistry : public IChangeListenerRegistry
{
    mutable std::mutex _mutex;
    
    std::unordered_map<SessionId, std::shared_ptr<IChangeListener>> _listeners;
    
public:
    
    std::vector<std::shared_ptr<IChangeListener>> listeners() const;
    
    void AddListener(std::shared_ptr<IChangeListener> listener);
    void RemoveListener(const SessionId &sessionId);
};



// A spatial database implementation that uses the SpatiaLite embedded SQL engine.
class SpatiaLiteDatabase : public ISpatialDatabase
{
    NodeInfo     _myNodeInfo;
    sqlite3     *_dbHandle;
    void        *_spatialiteConnection;
    
    std::chrono::duration<uint32_t> _entryExpirationPeriod;
    
    ThreadSafeChangeListenerRegistry _listenerRegistry;
    
    std::vector<NodeDbEntry> QueryEntries(const GpsLocation &fromLocation,
        const std::string &whereCondition = "", const std::string orderBy = "",
        const std::string &limit = "") const;
    
    NodeInfo::Services LoadServices(const NodeId &nodeId) const;
    void StoreServices(const NodeId &nodeId, const NodeInfo::Services &services);
    void RemoveServices(const NodeId &nodeId);
    
public:
    
    static const std::string IN_MEMORY_DB;
    
    
    SpatiaLiteDatabase(const NodeInfo &myNodeInfo, const std::string &dbPath,
                       std::chrono::duration<uint32_t> expirationPeriod);
    virtual ~SpatiaLiteDatabase();
    
    Distance GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const override;

    std::shared_ptr<NodeDbEntry> Load(const NodeId &nodeId) const override;
    void Store (const NodeDbEntry &node, bool expires = true) override;
    void Update(const NodeDbEntry &node, bool expires = true) override;
    void Remove(const NodeId &nodeId) override;
    void ExpireOldNodes() override;
    
    IChangeListenerRegistry& changeListenerRegistry() override;

    NodeDbEntry ThisNode() const override;
    std::vector<NodeDbEntry> GetNodes(NodeContactRoleType roleType) override;
    
    size_t GetNodeCount() const override;
    std::vector<NodeDbEntry> GetNeighbourNodesByDistance() const override;
    std::vector<NodeDbEntry> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeDbEntry> GetClosestNodesByDistance(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
};



} // namespace LocNet


#endif // __LOCNET_SPATIAL_DATABASE_H__