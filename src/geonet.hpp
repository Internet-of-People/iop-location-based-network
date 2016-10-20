#ifndef __GEONET_H__
#define __GEONET_H__

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>



typedef std::string NodeId;
typedef std::string Ipv4Address;
typedef std::string Ipv6Address;
typedef uint16_t    TcpPort;



class NodeProfile
{
    NodeId      _id;
    Ipv4Address _ipv4Address;
    TcpPort     _ipv4Port;
    Ipv6Address _ipv6Address;
    TcpPort     _ipv6Port;
    
public:
    
    NodeProfile(const NodeId &id,
                const Ipv4Address &ipv4Address, TcpPort ipv4Port,
                const Ipv6Address &ipv6Address, TcpPort ipv6Port);
    
    const NodeId& id() const;
    const Ipv4Address& ipv4Address() const;
    TcpPort ipv4Port() const;
    const Ipv6Address& ipv6Address() const;
    TcpPort ipv6Port() const;
};



class GpsLocation
{
    double _latitude;
    double _longitude;
    
public:
    
    GpsLocation(const GpsLocation &position);
    GpsLocation(double latitude, double longitude);
    double latitude() const;
    double longitude() const;
};



class NodeLocation
{
    NodeProfile _profile;
    GpsLocation _position;
    
public:
    
    NodeLocation(const NodeProfile &profile, const GpsLocation &position);
    NodeLocation(const NodeProfile &profile, double latitude, double longitude);
    
    const NodeProfile& profile() const;
    double latitude() const;
    double longitude() const;
};



enum class ServerType : uint8_t
{
    TokenServer         = 1,
    ProfileServer       = 2,
    ProximityServer     = 3,
    StunTurnServer      = 4,
    ReputationServer    = 5,
    MintingServer       = 6,
};

// Utility class to enable hash classes to be used as a hash key until fixed in C++ standard
// NOTE for simple (not class) enums, std::hash<int> also works instead of this class
struct EnumHasher
{
    template <typename EnumType>
    std::size_t operator()(EnumType e) const // static_cast any type to size_t using type deduction
        { return static_cast<std::size_t>(e); }
};

typedef NodeProfile ServerInfo;



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
    std::unordered_map<ServerType, ServerInfo, EnumHasher> _servers;
    
public:
    
    GeographicNetwork(std::shared_ptr<ISpatialDb> spatialDb);
    
    // Local interface for servers running on the same hardware
    virtual void registerServer(ServerType serverType, const ServerInfo &server);
    virtual std::vector<NodeLocation> GetNeighbourHood() const;
    
    // Interface provided for the same network instances running on remote machines
    virtual std::vector<NodeLocation> GetRandomNodes(
        uint16_t maxNodeCount, bool includeNeighbours = false) const;
    
    virtual std::vector<NodeLocation> GetClosestNodes(const GpsLocation &location,
        double radiusKm = 100, uint16_t maxNodeCount = 100) const;
    
    virtual void ExchangeNodeProfile(NodeLocation profile);
    virtual void RenewNodeProfile(NodeLocation profile);
    virtual void AcceptNeighbor(NodeLocation profile);
    
    // Interface provided to serve higher level services and clients
    virtual const std::unordered_map<ServerType,ServerInfo,EnumHasher>& GetServers() const;
    // + GetClosestNodes() which is the same as for network instances on remote machines
};



#endif // __GEONET_H__
