#ifndef __GEONET_H__
#define __GEONET_H__

#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>



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
    
    NodeProfile();
    NodeProfile(const NodeProfile &other);
    NodeProfile(const NodeId &id,
                const Ipv4Address &ipv4Address, TcpPort ipv4Port,
                const Ipv6Address &ipv6Address, TcpPort ipv6Port);
    
    const NodeId& id() const;
    const Ipv4Address& ipv4Address() const;
    TcpPort ipv4Port() const;
    const Ipv6Address& ipv6Address() const;
    TcpPort ipv6Port() const;
    
    bool operator==(const NodeProfile &other) const;
};



class GpsLocation
{
    double _latitude;
    double _longitude;
    
    void Validate();
    
public:
    
    GpsLocation(const GpsLocation &position);
    GpsLocation(double latitude, double longitude);
    
    double latitude() const;
    double longitude() const;
    
    bool operator==(const GpsLocation &other) const;
};



class NodeLocation
{
    NodeProfile _profile;
    GpsLocation _location;
    
public:
    
    NodeLocation(const NodeProfile &profile, const GpsLocation &location);
    NodeLocation(const NodeProfile &profile, double latitude, double longitude);
    
    const NodeProfile& profile() const;
    const GpsLocation& location() const;
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


class ISpatialDatabase
{
public:
    virtual double GetDistance(const GpsLocation &one, const GpsLocation &other) const = 0;
    
    virtual bool Store(const NodeLocation &node, bool isNeighbour) = 0;
    virtual std::shared_ptr<NodeLocation> Load(const std::string &nodeId) const = 0;
    virtual bool Update(const NodeLocation &node) const = 0;
    virtual bool Remove(const std::string &nodeId) = 0;
    
    virtual size_t GetColleagueNodeCount() const = 0;
    virtual double GetNeighbourhoodRadiusKm() const = 0;
    
    virtual std::vector<NodeLocation> GetClosestNodes(const GpsLocation &position,
        double Km, size_t maxNodeCount, bool includeNeighbours) const = 0;

    virtual std::vector<NodeLocation> GetRandomNodes(
        uint16_t maxNodeCount, bool includeNeighbours) const = 0;
};



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
