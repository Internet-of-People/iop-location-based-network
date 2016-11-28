#ifndef __LOCNET_BUSINESS_LOGIC_H__
#define __LOCNET_BUSINESS_LOGIC_H__

#include <random>
#include <unordered_map>

#include "spatialdb.hpp"



namespace LocNet
{

    
// Local interface for services running on the same hardware
class ILocalServiceMethods
{
public:
    
    virtual void RegisterService(ServiceType serviceType, const ServiceProfile &serviceInfo) = 0;
    virtual void DeregisterService(ServiceType serviceType) = 0;
    virtual std::vector<NodeInfo> GetNeighbourNodesByDistance() const = 0;
};


// Interface provided for the same network instances running on remote machines
class INodeMethods
{
public:
    
    virtual size_t GetColleagueNodeCount() const = 0;
    virtual std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
    
    virtual std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const = 0;
    
    virtual bool AcceptColleague(const NodeInfo &node) = 0;
    virtual bool RenewColleague(const NodeInfo &node) = 0;
    virtual bool AcceptNeighbour(const NodeInfo &node) = 0;
    virtual bool RenewNeighbour(const NodeInfo &node) = 0;
};


// Interface provided to serve higher level services and clients
class IClientMethods
{
public:

    virtual const std::unordered_map<ServiceType,ServiceProfile,EnumHasher>& GetServices() const = 0;
    
    virtual std::vector<NodeInfo> GetNeighbourNodesByDistance() const = 0;
    virtual std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const = 0;
};



class INodeConnectionFactory
{
public:
    
    virtual std::shared_ptr<INodeMethods> ConnectTo(const NodeProfile &node) = 0;
};



class Node : public ILocalServiceMethods, public IClientMethods, public INodeMethods
{
    static const std::vector<NodeInfo> _seedNodes;
    static std::random_device _randomDevice;
    
    NodeInfo _myNodeInfo;
    std::unordered_map<ServiceType, ServiceProfile, EnumHasher> _services;
    std::shared_ptr<ISpatialDatabase> _spatialDb;
    std::shared_ptr<INodeConnectionFactory> _connectionFactory;
    
    std::shared_ptr<INodeMethods> SafeConnectTo(const NodeProfile &node);
    bool SafeStoreNode(const NodeDbEntry &entry,
        std::shared_ptr<INodeMethods> nodeConnection = std::shared_ptr<INodeMethods>() );
    
    bool InitializeWorld();
    bool InitializeNeighbourhood();
    void RenewNodeRelations();
    void DiscoverUnknownAreas();
    
    Distance GetBubbleSize(const GpsLocation &location) const;
    bool BubbleOverlaps(const GpsLocation &newNodeLocation) const;
    
public:
    
    Node( const NodeInfo &nodeInfo,
          std::shared_ptr<ISpatialDatabase> spatialDb,
          std::shared_ptr<INodeConnectionFactory> connectionFactory,
          bool ignoreDiscovery = false );

    // Interface provided to serve higher level services and clients
    const std::unordered_map<ServiceType,ServiceProfile,EnumHasher>& GetServices() const override;
    // + GetClosestNodes() which is the same as for network instances on remote machines
    
    // Local interface for services running on the same hardware
    void RegisterService(ServiceType serviceType, const ServiceProfile &serviceInfo) override;
    void DeregisterService(ServiceType serviceType) override;
    
    // Interface provided for the same network instances running on remote machines
    size_t GetColleagueNodeCount() const override;
    std::vector<NodeInfo> GetNeighbourNodesByDistance() const override;
    
    std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;    
        
    bool AcceptColleague(const NodeInfo &node) override;
    bool RenewColleague(const NodeInfo &node) override;
    bool AcceptNeighbour(const NodeInfo &node) override;
    bool RenewNeighbour(const NodeInfo &node) override;
};


} // namespace LocNet


#endif // __LOCNET_BUSINESS_LOGIC_H__
