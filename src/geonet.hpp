#ifndef __GEONET_H__
#define __GEONET_H__

#include <string>
#include <memory>
#include <vector>



// TODO consider if this interface makes sense or should be removed
class IGpsLocation
{
public:
    virtual double latitude() const = 0;
    virtual double longitude() const = 0;
};


class GpsLocation : public IGpsLocation
{
    double _latitude;
    double _longitude;
    
public:
    
    GpsLocation(const IGpsLocation &position);
    GpsLocation(double latitude, double longitude);
    double latitude() const override;
    double longitude() const override;
};



// TODO consider if this interface makes sense or should be removed
class INodeProfile : public IGpsLocation
{
public:
    virtual const std::string& id() const = 0;
};



class NodeProfile : public INodeProfile
{
    std::string _id;
    GpsLocation _position;
    
public:
    
    NodeProfile(const std::string &id, const IGpsLocation &position);
    NodeProfile(const std::string &id, double latitude, double longitude);
    
    const std::string& id() const override;
    double latitude() const override;
    double longitude() const override;
};



enum ServerType : uint8_t
{
    TokenServer         = 1,
    ProfileServer       = 2,
    ProximityServer     = 3,
    StunTurnServer      = 4,
    ReputationServer    = 5,
    MintingServer       = 6,
};



// TODO consider if this interface makes sense or should be removed
class IServerInfo
{
    virtual const std::string& id() const = 0;
    virtual ServerType serverType() const = 0;
    virtual const std::string& ipAddress() const = 0;
    virtual uint16_t tcpPort() const = 0;
};



class ServerInfo : IServerInfo
{
    std::string _id;
    std::string _ipAddress;
    uint16_t    _tcpPort;
    ServerType  _serverType;
    
public:
    
    ServerInfo(const std::string &id, ServerType serverType,
               const std::string &ipAddress, uint16_t tcpPort);
    
    const std::string& id() const override;
    ServerType serverType() const override;
    const std::string& ipAddress() const override;
    uint16_t tcpPort() const override;
};



// // TODO how to use this without too much trouble?
// #include <experimental/optional>
// namespace std {
//     // Class std::optional becomes standard only in C++17, but it's 2016 so we have only C++14.
//     // It is currently available in the experimental namespace, so import it to the normal std namespace
//     // to minimize later changes.
//     template<typename T> using optional = std::experimental::optional<T>;
// }


class ISpatialDb
{
// public:
//     
//     virtual double GetDistance(const IGpsPosition &one, const IGpsPosition &other) const = 0;
//     
//     virtual std::vector<std::unique_ptr<INode>> GetNeighbourHood() const = 0;    
//     virtual std::vector<std::unique_ptr<INode>> GetNodesCloseTo(
//         const IGpsPosition &position,
//         std::experimental::optional<double> radius,
//         std::experimental::optional<size_t> maxNodeCount) const = 0;
//         
//     virtual void Store(const INode &node) = 0;
//     virtual std::unique_ptr<INode> Load(const INode &node) const = 0;
//     virtual void Remove(const std::string &nodeId) = 0;
};



class GeographicNetwork
{
    std::shared_ptr<ISpatialDb> _spatialDb;
    std::vector<ServerInfo> _servers;
    
public:
    
    GeographicNetwork(std::shared_ptr<ISpatialDb> spatialDb);
    
    // Local interface for servers running on the same hardware
    virtual void registerServer(const ServerInfo &server);
    virtual std::vector<NodeProfile> GetNeighbourHood() const;
    
    // Interface provided for the same network instances running on remote machines
    virtual std::vector<NodeProfile> GetRandomNodes(
        uint16_t nodeCount, bool includeNeighbours = false) const;
    
    virtual std::vector<NodeProfile> GetClosestNodes(const GpsLocation &location,
        double radiusKm = 100, uint16_t maxNodeCount = 100) const;
    
    virtual void ExchangeNodeProfile(NodeProfile profile);
    virtual void RenewNodeProfile(NodeProfile profile);
    virtual void AcceptNeighbor(NodeProfile profile);
    
    // Interface provided to serve higher level services and clients
    virtual const std::vector<ServerInfo>& GetServers() const;
    // + GetClosestNodes() which is the same as for network instances on remote machines
};



#endif // __GEONET_H__
