#ifndef __GEONET_BUSINESS_LOGIC_H__
#define __GEONET_BUSINESS_LOGIC_H__

#include <random>
#include <unordered_map>

#include "spatialdb.hpp"



// Interface provided for the same network instances running on remote machines
class IGeoNetRemoteNode
{
public:
    
    // TODO add operation GetColleagueNodeCount() to the specs
    virtual size_t GetNodeCount(NodeType nodeType) const = 0;
    virtual std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const = 0;
    
    virtual std::vector<NodeInfo> GetClosestNodes(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const = 0;
    
    virtual bool AcceptColleague(const NodeInfo &node) = 0;
    virtual bool AcceptNeighbor(const NodeInfo &node) = 0;
    virtual bool RenewNodeConnection(const NodeInfo &node) = 0;
};


// Interface provided to serve higher level services and clients
class IHigherLevel
{
public:
    
    virtual const std::unordered_map<ServerType,ServerInfo,EnumHasher>& servers() const = 0;
    //GetClosestNodes() which is the same as for network instances on remote machines
};


// Local interface for servers running on the same hardware
class ILocal
{
public:
    
    virtual void RegisterServer(ServerType serverType, const ServerInfo &serverInfo) = 0;
    virtual void RemoveServer(ServerType serverType) = 0;
    virtual Distance GetNeighbourhoodRadiusKm() const = 0;
};



class IGeoNetRemoteNodeConnectionFactory
{
public:
    
    virtual std::shared_ptr<IGeoNetRemoteNode> ConnectTo(const NodeProfile &node) = 0;
};



class GeoNetBusinessLogic : public ILocal, public IHigherLevel, public IGeoNetRemoteNode
{
    static const std::vector<NodeInfo> _seedNodes;
    static std::random_device _randomDevice;
    
    NodeInfo _myNodeInfo;
    std::unordered_map<ServerType, ServerInfo, EnumHasher> _servers;
    std::shared_ptr<ISpatialDatabase> _spatialDb;
    std::shared_ptr<IGeoNetRemoteNodeConnectionFactory> _connectionFactory;
    
    std::shared_ptr<IGeoNetRemoteNode> SafeConnectTo(const NodeProfile &node);
    bool SafeStoreNode(const NodeInfo &nodeInfo, NodeType nodeType, RelationType relationType,
        std::shared_ptr<IGeoNetRemoteNode> nodeConnection = std::shared_ptr<IGeoNetRemoteNode>() );
    
    bool DiscoverWorld();
    bool DiscoverNeighbourhood();
    
    Distance GetBubbleSize(const GpsLocation &location) const;
    bool BubbleOverlaps(const GpsLocation &newNodeLocation) const;
    
public:
    
    GeoNetBusinessLogic( const NodeInfo &nodeInfo,
                         std::shared_ptr<ISpatialDatabase> spatialDb,
                         std::shared_ptr<IGeoNetRemoteNodeConnectionFactory> connectionFactory );

    // Interface provided to serve higher level services and clients
    const std::unordered_map<ServerType,ServerInfo,EnumHasher>& servers() const override;
    // + GetClosestNodes() which is the same as for network instances on remote machines
    
    // Local interface for servers running on the same hardware
    void RegisterServer(ServerType serverType, const ServerInfo &serverInfo) override;
    void RemoveServer(ServerType serverType) override;
    Distance GetNeighbourhoodRadiusKm() const override;
    
    // Interface provided for the same network instances running on remote machines
    size_t GetNodeCount(NodeType nodeType) const override;
    std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeInfo> GetClosestNodes(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
    
    bool AcceptColleague(const NodeInfo &node) override;
    bool AcceptNeighbor(const NodeInfo &node) override;
    bool RenewNodeConnection(const NodeInfo &node) override;
};



#endif // __GEONET_BUSINESS_LOGIC_H__
