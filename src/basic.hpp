#ifndef __LOCNET_BASIC_TYPES_H__
#define __LOCNET_BASIC_TYPES_H__

#include <exception>
#include <functional>
#include <iostream>



namespace LocNet
{


typedef std::string NodeId;
typedef std::string Address;
typedef uint16_t    TcpPort;
typedef float       GpsCoordinate;
typedef float       Distance;



enum class AddressType : uint8_t
{
    Ipv4 = 1,
    Ipv6 = 2,
};


class NetworkInterface
{
    AddressType _addressType;
    Address     _address;
    TcpPort     _port;
    
public:
    
    NetworkInterface();
    NetworkInterface(const NetworkInterface &other);
    NetworkInterface(AddressType addressType, const Address &address, TcpPort port);
    
    AddressType addressType() const;
    const Address& address() const;
    TcpPort port() const;
    
    bool operator==(const NetworkInterface &other) const;
};

std::ostream& operator<<(std::ostream& out, const NetworkInterface &value);


class NodeProfile
{
    NodeId _id;
    // TODO will we also need a public key here?
    NetworkInterface _contact;
    
public:
    
    NodeProfile();
    NodeProfile(const NodeProfile &other);
    NodeProfile(const NodeId &id, const NetworkInterface &contact);
    
    const NodeId& id() const;
    const NetworkInterface& contact() const;
    
    bool operator==(const NodeProfile &other) const;
};

std::ostream& operator<<(std::ostream& out, const NodeProfile &value);


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

std::ostream& operator<<(std::ostream& out, const GpsLocation &value);



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
    NodeInfo(const NodeProfile &profile, GpsCoordinate latitude, GpsCoordinate longitude);
    
    const NodeProfile& profile() const;
    const GpsLocation& location() const;
    
    bool operator==(const NodeInfo &other) const;
};

std::ostream& operator<<(std::ostream& out, const NodeInfo &value);



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

// Utility class to enable hash classes to be used as a hash key until fixed in C++ standard
// NOTE for simple (not class) enums, std::hash<int> also works instead of this class
struct EnumHasher
{
    template <typename EnumType>
    std::size_t operator()(EnumType e) const // static_cast any type to size_t using type deduction
        { return static_cast<std::size_t>(e); }
};



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
