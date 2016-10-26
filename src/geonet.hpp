#ifndef __GEONET_H__
#define __GEONET_H__

#include <random>
#include <unordered_map>

#include "spatialdb.hpp"



class IGeographicNetwork
{
public:
    
    // Interface provided to serve higher level services and clients
    virtual const std::unordered_map<ServerType,ServerInfo,EnumHasher>& servers() const = 0;
    // + GetClosestNodes() which is the same as for network instances on remote machines
    
    // Local interface for servers running on the same hardware
    virtual void RegisterServer(ServerType serverType, const ServerInfo &serverInfo) = 0;
    virtual void RemoveServer(ServerType serverType) = 0;
    virtual double GetNeighbourhoodRadiusKm() const = 0;
    
    // Interface provided for the same network instances running on remote machines
    // TODO add operation GetColleagueNodeCount() to the specs
    virtual size_t GetColleagueNodeCount() const = 0;
    virtual std::vector<NodeLocation> GetRandomNodes(
        uint16_t maxNodeCount, bool includeNeighbours) const = 0;
    
    virtual std::vector<NodeLocation> GetClosestNodes(const GpsLocation &location,
        double radiusKm, uint16_t maxNodeCount, bool includeNeighbours) const = 0;
    
    virtual bool AcceptColleague(const NodeLocation &node) = 0;
    virtual bool AcceptNeighbor(const NodeLocation &node) = 0;
    virtual bool RenewNodeConnection(const NodeLocation &node) = 0;
};



class IGeographicNetworkConnectionFactory
{
public:
    
    virtual std::shared_ptr<IGeographicNetwork> ConnectTo(const NodeProfile &node) = 0;
};


class GeographicNetwork : public IGeographicNetwork
{
    static const std::vector<NodeProfile> _seedNodes;
    static std::random_device _randomDevice;
    
    NodeLocation _myNodeInfo;
    std::unordered_map<ServerType, ServerInfo, EnumHasher> _servers;
    std::shared_ptr<ISpatialDatabase> _spatialDb;
    std::shared_ptr<IGeographicNetworkConnectionFactory> _connectionFactory;
    
    std::shared_ptr<IGeographicNetwork> SafeConnectTo(const NodeProfile &node);
    void DiscoverWorld();
    void DiscoverNeighbourhood();
    
    double GetBubbleSize(const GpsLocation &location) const;
    bool Overlaps(const GpsLocation &newNodeLocation) const;
    
public:
    
    GeographicNetwork( const NodeLocation &nodeInfo,
                       std::shared_ptr<ISpatialDatabase> spatialDb,
                       std::shared_ptr<IGeographicNetworkConnectionFactory> connectionFactory );

    // Interface provided to serve higher level services and clients
    const std::unordered_map<ServerType,ServerInfo,EnumHasher>& servers() const override;
    // + GetClosestNodes() which is the same as for network instances on remote machines
    
    // Local interface for servers running on the same hardware
    void RegisterServer(ServerType serverType, const ServerInfo &serverInfo) override;
    void RemoveServer(ServerType serverType) override;
    double GetNeighbourhoodRadiusKm() const override;
    
    // Interface provided for the same network instances running on remote machines
    size_t GetColleagueNodeCount() const override;
    std::vector<NodeLocation> GetRandomNodes(
        uint16_t maxNodeCount, bool includeNeighbours) const override;
    
    std::vector<NodeLocation> GetClosestNodes(const GpsLocation &location,
        double radiusKm, uint16_t maxNodeCount, bool includeNeighbours) const override;
    
    bool AcceptColleague(const NodeLocation &node) override;
    bool AcceptNeighbor(const NodeLocation &node) override;
    bool RenewNodeConnection(const NodeLocation &node) override;
};



#endif // __GEONET_H__
