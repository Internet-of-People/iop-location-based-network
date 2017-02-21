#ifndef __LOCNET_BASIC_TYPES_H__
#define __LOCNET_BASIC_TYPES_H__

#include <exception>
#include <functional>
#include <iostream>
#include <unordered_map>



namespace LocNet
{


// Type definition of essential types used on multiple interfaces
typedef float       GpsCoordinate;
typedef float       Distance;

typedef std::string NodeId;
typedef std::string Address;
typedef uint16_t    TcpPort;

typedef std::string SessionId;



enum class ErrorCode : uint16_t
{
    // Problems with incoming message
    ERROR_UNSUPPORTED = 1,          // Unknown request version or protocol
    ERROR_PROTOCOL_VIOLATION = 2,   // Requests are not sent or responses not received as expected
    ERROR_BAD_REQUEST = 3,          // Request is in invalid format or cannot be properly interpreted
    ERROR_INVALID_VALUE = 4,        // Invalid data provided as an rgument of an operation
    ERROR_BAD_STATE = 5,            // Operation is requested too early (e.g. without initialization) or too late (e.g. after resources are releasedd)
    
    // Problems with outgoing messages
    ERROR_CONNECTION = 96,          // Failed to connect to another peer
    ERROR_BAD_RESPONSE = 97,        // Consumed service (i.e. remote network node) returned unexpected response message
    
    // Problems inside the server 
    ERROR_INTERNAL = 128,           // Implementation problem: this shouldn't happen, we are not well propared for this error.
    ERROR_CONCEPTUAL = 129,         // Failure in the network design or discovery algorithm. We might need algorithmic changes if this occurs.
};


class LocationNetworkError : public std::runtime_error
{
    ErrorCode _code;
    
public:
    
    LocationNetworkError(ErrorCode code, const std::string &reason);
    LocationNetworkError(ErrorCode code, const char *reason);
    
    ErrorCode code() const;
};



// Data holder class for a network endpoint to connect to.
class NetworkEndpoint
{
    Address     _address;
    TcpPort     _port;
    
public:
    
    NetworkEndpoint(const NetworkEndpoint &other);
    NetworkEndpoint(const Address &address, TcpPort port);
    
    Address address() const;
    TcpPort port() const;
    
    bool operator==(const NetworkEndpoint &other) const;
    bool operator!=(const NetworkEndpoint &other) const;
    
    // NOTE Following functions are implemented in network.cpp as being library-specific (currently with asio)
    bool isLoopback() const;
};

std::ostream& operator<<(std::ostream& out, const NetworkEndpoint &value);



// Data holder class for contact data of a single (remote) node of the network to be advertised.
class NodeContact
{
    Address     _address;
    TcpPort     _nodePort;
    TcpPort     _clientPort;

public:
    
    NodeContact(const NodeContact &other);
    NodeContact(const Address &address, TcpPort nodePort, TcpPort clientPort);
    
    const Address& address() const;
    TcpPort nodePort() const;
    TcpPort clientPort() const;
    
    NetworkEndpoint nodeEndpoint() const;
    NetworkEndpoint clientEndpoint() const;
    
    void address(const Address &address);
    
    bool operator==(const NodeContact &other) const;
    bool operator!=(const NodeContact &other) const;
    
    // NOTE Following functions are implemented in network.cpp as being library-specific (currently with asio)
    // TODO consider splitting class into an interface here and an implementation in network
    static Address AddressFromBytes(const std::string &bytes);
    static std::string AddressToBytes(const Address &address);
    
    std::string AddressBytes() const;
};

std::ostream& operator<<(std::ostream& out, const NodeContact &value);



// Data holder class for a GPS position (2D Latitude/longitude pair without height data).
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
    bool operator!=(const GpsLocation &other) const;
};

std::ostream& operator<<(std::ostream& out, const GpsLocation &value);



enum class ServiceType : uint8_t
{
    // Low level networks
    Unstructured = 0,
    Content      = 1,
    Latency      = 2,
    Location     = 3,

    // High level servers
    Token       = 11,
    Profile     = 12,
    Proximity   = 13,
    Relay       = 14,
    Reputation  = 15,
    Minting     = 16,
};

// Utility class to enable enum classes to be used as a hash key until fixed in C++ standard
// NOTE for simple (not class) enums, std::hash<int> also works instead of this class
struct EnumHasher
{
    template <typename EnumType>
    std::size_t operator()(EnumType e) const // static_cast any type to size_t using type deduction
        { return static_cast<std::size_t>(e); }
};



class ServiceInfo
{
    ServiceType _type;
    TcpPort     _port;
    std::string _customData;
    
public:
    
    ServiceInfo(); // Required to be a value in a map
    ServiceInfo(const ServiceInfo &other);
    ServiceInfo( ServiceType type, TcpPort port, const std::string &customData = std::string() );
    
    ServiceType type() const;
    TcpPort port() const;
    const std::string& customData() const;
    
    bool operator==(const ServiceInfo &other) const;
    bool operator!=(const ServiceInfo &other) const;
};



// Data holder class for complete node information exposed to the network,
// including node identity, network contact and position.
// TODO will we also need a public key here later as part of the identity?
class NodeInfo
{
public:

    typedef std::unordered_map<ServiceType, ServiceInfo, EnumHasher> Services;

private:
    
    NodeId      _id;
    GpsLocation _location;
    NodeContact _contact;
    Services    _services;
    
public:
    
    NodeInfo(const NodeInfo &other);
    NodeInfo( const NodeId &id, const GpsLocation &location, const NodeContact &contact,
              const Services &services );
    
    const NodeId& id() const;
    const GpsLocation& location() const;
    const NodeContact& contact() const;
    const Services& services() const;
    
    Services& services();
    NodeContact& contact();
    
    bool operator==(const NodeInfo &other) const;
    bool operator!=(const NodeInfo &other) const;
};

std::ostream& operator<<(std::ostream& out, const NodeInfo &value);



// RAII-style scope guard with a custom functor to avoid creating a new class for every resource release action.
// based on http://stackoverflow.com/questions/36644263/is-there-a-c-standard-class-to-set-a-variable-to-a-value-at-scope-exit
// a more complete example of the same concept http://stackoverflow.com/questions/31365013/what-is-scopeguard-in-c
struct scope_exit
{
    std::function<void()> _fun;
    
    // Store function and execute it on destruction, ie. when end of scope is reached
    explicit scope_exit(std::function<void()> fun) :
        _fun( std::move(fun) ) {}
    ~scope_exit() { if (_fun) { _fun(); } }
    
    // Disable copies
    scope_exit(scope_exit const&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
    scope_exit& operator=(scope_exit&& rhs) = delete;
};


struct scope_error : public scope_exit
{
    explicit scope_error(std::function<void()> fun) :
        scope_exit( [fun] { if( std::uncaught_exception() ) { fun(); } } ) {}
};

struct scope_success : public scope_exit
{
    explicit scope_success(std::function<void()> fun) :
        scope_exit( [fun] { if( ! std::uncaught_exception() ) { fun(); } } ) {}
};



} // namespace LocNet


#endif // __LOCNET_BASIC_TYPES_H__
