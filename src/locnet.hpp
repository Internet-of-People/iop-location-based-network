#ifndef __LOCNET_BUSINESS_LOGIC_H__
#define __LOCNET_BUSINESS_LOGIC_H__

#include <random>
#include <unordered_map>

#include "spatialdb.hpp"



namespace LocNet
{


// Interface provided for the same network instances running on remote machines
class IRemoteNode
{
public:
    
    virtual std::vector<NodeInfo> GetColleagueNodes() const = 0;
    virtual std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
    
    virtual std::vector<NodeInfo> GetClosestNodes(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const = 0;
    
    virtual bool AcceptColleague(const NodeInfo &node) = 0;
    virtual bool AcceptNeighbour(const NodeInfo &node) = 0;
    virtual bool RenewNodeConnection(const NodeInfo &node) = 0;
};


// TODO build sample client to find out if we need a method to get neighbour radius or profile list here
// Interface provided to serve higher level services and clients
class IClientMethods
{
public:

    virtual const std::unordered_map<ServiceType,ServiceProfile,EnumHasher>& services() const = 0;
    
    virtual std::vector<NodeInfo> GetNeighbourNodes() const = 0;
    virtual std::vector<NodeInfo> GetClosestNodes(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const = 0;
};


// Local interface for services running on the same hardware
class ILocalServices
{
public:
    
    virtual void RegisterService(ServiceType serviceType, const ServiceProfile &serviceInfo) = 0;
    virtual void RemoveService(ServiceType serviceType) = 0;
    virtual std::vector<NodeInfo> GetNeighbourNodes() const = 0;
};



class IRemoteNodeConnectionFactory
{
public:
    
    virtual std::shared_ptr<IRemoteNode> ConnectTo(const NodeProfile &node) = 0;
};



class Node : public ILocalServices, public IClientMethods, public IRemoteNode
{
    static const std::vector<NodeInfo> _seedNodes;
    static std::random_device _randomDevice;
    
    NodeInfo _myNodeInfo;
    std::unordered_map<ServiceType, ServiceProfile, EnumHasher> _services;
    std::shared_ptr<ISpatialDatabase> _spatialDb;
    std::shared_ptr<IRemoteNodeConnectionFactory> _connectionFactory;
    
    std::shared_ptr<IRemoteNode> SafeConnectTo(const NodeProfile &node);
    bool SafeStoreNode(const NodeDbEntry &entry,
        std::shared_ptr<IRemoteNode> nodeConnection = std::shared_ptr<IRemoteNode>() );
    
    bool DiscoverWorld();
    bool DiscoverNeighbourhood();
    void PerformDbMaintenance();
    
    Distance GetBubbleSize(const GpsLocation &location) const;
    bool BubbleOverlaps(const GpsLocation &newNodeLocation) const;
    
public:
    
    Node( const NodeInfo &nodeInfo,
          std::shared_ptr<ISpatialDatabase> spatialDb,
          std::shared_ptr<IRemoteNodeConnectionFactory> connectionFactory,
          bool ignoreDiscovery = false );

    // Interface provided to serve higher level services and clients
    const std::unordered_map<ServiceType,ServiceProfile,EnumHasher>& services() const override;
    // + GetClosestNodes() which is the same as for network instances on remote machines
    
    // Local interface for services running on the same hardware
    void RegisterService(ServiceType serviceType, const ServiceProfile &serviceInfo) override;
    void RemoveService(ServiceType serviceType) override;
    
    // Interface provided for the same network instances running on remote machines
    std::vector<NodeInfo> GetColleagueNodes() const override;
    std::vector<NodeInfo> GetNeighbourNodes() const override;
    
    std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeInfo> GetClosestNodes(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;    
        
    bool AcceptColleague(const NodeInfo &node) override;
    bool AcceptNeighbour(const NodeInfo &node) override;
    bool RenewNodeConnection(const NodeInfo &node) override;
};


} // namespace LocNet


#endif // __LOCNET_BUSINESS_LOGIC_H__
