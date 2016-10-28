#ifndef __LOCNET_BUSINESS_LOGIC_H__
#define __LOCNET_BUSINESS_LOGIC_H__

#include <random>
#include <unordered_map>

#include "spatialdb.hpp"



// Interface provided for the same network instances running on remote machines
class ILocNetRemoteNode
{
public:
    
    // TODO add operation GetColleagueNodeCount() to the specs
    virtual size_t GetNodeCount(LocNetRelationType nodeType) const = 0;
    virtual std::vector<LocNetNodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
    
    virtual std::vector<LocNetNodeInfo> GetClosestNodes(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const = 0;
    
    virtual bool AcceptColleague(const LocNetNodeInfo &node) = 0;
    virtual bool AcceptNeighbour(const LocNetNodeInfo &node) = 0;
    virtual bool RenewNodeConnection(const LocNetNodeInfo &node) = 0;
};


// Interface provided to serve higher level services and clients
class ILocNetClientMethods
{
public:
    
    virtual const std::unordered_map<ServiceType,ServiceProfile,EnumHasher>& services() const = 0;
    // TODO do this
    //GetClosestNodes() which is the same as for network instances on remote machines
};


// Local interface for services running on the same hardware
class ILocal
{
public:
    
    virtual void RegisterService(ServiceType serviceType, const ServiceProfile &serviceInfo) = 0;
    virtual void RemoveService(ServiceType serviceType) = 0;
    virtual Distance GetNeighbourhoodRadiusKm() const = 0;
};



class ILocNetRemoteNodeConnectionFactory
{
public:
    
    virtual std::shared_ptr<ILocNetRemoteNode> ConnectTo(const NodeProfile &node) = 0;
};



class LocNetNode : public ILocal, public ILocNetClientMethods, public ILocNetRemoteNode
{
    static const std::vector<LocNetNodeInfo> _seedNodes;
    static std::random_device _randomDevice;
    
    LocNetNodeInfo _myNodeInfo;
    std::unordered_map<ServiceType, ServiceProfile, EnumHasher> _services;
    std::shared_ptr<ISpatialDatabase> _spatialDb;
    std::shared_ptr<ILocNetRemoteNodeConnectionFactory> _connectionFactory;
    
    std::shared_ptr<ILocNetRemoteNode> SafeConnectTo(const NodeProfile &node);
    bool SafeStoreNode(const LocNetNodeDbEntry &entry,
        std::shared_ptr<ILocNetRemoteNode> nodeConnection = std::shared_ptr<ILocNetRemoteNode>() );
    
    bool DiscoverWorld();
    bool DiscoverNeighbourhood();
    
    Distance GetBubbleSize(const GpsLocation &location) const;
    bool BubbleOverlaps(const GpsLocation &newNodeLocation) const;
    
public:
    
    LocNetNode( const LocNetNodeInfo &nodeInfo,
                std::shared_ptr<ISpatialDatabase> spatialDb,
                std::shared_ptr<ILocNetRemoteNodeConnectionFactory> connectionFactory );

    // Interface provided to serve higher level services and clients
    const std::unordered_map<ServiceType,ServiceProfile,EnumHasher>& services() const override;
    // + GetClosestNodes() which is the same as for network instances on remote machines
    
    // Local interface for servers running on the same hardware
    void RegisterService(ServiceType serverType, const ServiceProfile &serverInfo) override;
    void RemoveService(ServiceType serverType) override;
    Distance GetNeighbourhoodRadiusKm() const override;
    
    // Interface provided for the same network instances running on remote machines
    size_t GetNodeCount(LocNetRelationType nodeType) const override;
    std::vector<LocNetNodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<LocNetNodeInfo> GetClosestNodes(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
    
    bool AcceptColleague(const LocNetNodeInfo &node) override;
    bool AcceptNeighbour(const LocNetNodeInfo &node) override;
    bool RenewNodeConnection(const LocNetNodeInfo &node) override;
};



#endif // __LOCNET_BUSINESS_LOGIC_H__
