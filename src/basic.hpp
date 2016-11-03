#ifndef __LOCNET_BASIC_TYPES_H__
#define __LOCNET_BASIC_TYPES_H__

#include <string>



namespace LocNet
{


typedef std::string NodeId;
typedef std::string Ipv4Address;
typedef std::string Ipv6Address;
typedef uint16_t    TcpPort;
typedef float       GpsCoordinate;
typedef float       Distance;



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


typedef NodeProfile ServiceProfile;



enum class NodeRelationType : uint8_t
{
    Colleague   = 1,
    Neighbour   = 2,
};



class GpsLocation
{
    GpsCoordinate _latitude;
    GpsCoordinate _longitude;
    
    void Validate();
    
public:
    
    GpsLocation(const GpsLocation &other);
    GpsLocation(GpsCoordinate latitude, GpsCoordinate longitude);
    
    GpsCoordinate latitude() const;
    GpsCoordinate longitude() const;
    
    bool operator==(const GpsLocation &other) const;
};



enum class NodeContactRoleType : uint8_t
{
    Initiator   = 1,
    Acceptor    = 2,
};



class NodeInfo
{
    NodeProfile _profile;
    GpsLocation _location;
    
public:
    
    NodeInfo(const NodeInfo &other);
    NodeInfo(const NodeProfile &profile, const GpsLocation &location);
    NodeInfo(const NodeProfile &profile, float latitude, float longitude);
    
    const NodeProfile& profile() const;
    const GpsLocation& location() const;
    
    bool operator==(const NodeInfo &other) const;
};



enum class ServiceType : uint8_t
{
    // TODO are low level "networks" directly reachable by clients?
    //      If so, should be included here
    Token       = 1,
    Profile     = 2,
    Proximity   = 3,
    Relay       = 4,
    Reputation  = 5,
    Minting     = 6,
};

// Utility class to enable hash classes to be used as a hash key until fixed in C++ standard
// NOTE for simple (not class) enums, std::hash<int> also works instead of this class
struct EnumHasher
{
    template <typename EnumType>
    std::size_t operator()(EnumType e) const // static_cast any type to size_t using type deduction
        { return static_cast<std::size_t>(e); }
};




// // TODO how to use this without too much trouble?
// #include <experimental/optional>
// namespace std {
//     // Class std::optional becomes standard only in C++17, but it's 2016 so we have only C++14.
//     // It is currently available in the experimental namespace, so import it to the normal std namespace
//     // to minimize later changes.
//     template<typename T> using optional = std::experimental::optional<T>;
// }


} // namespace LocNet


#endif // __LOCNET_BASIC_TYPES_H__
