#include "geonet.hpp"

using namespace std;



NodeProfile::NodeProfile(const NodeId& id,
                         const Ipv4Address& ipv4Address, TcpPort ipv4Port,
                         const Ipv6Address& ipv6Address, TcpPort ipv6Port) :
    _id(id),
    _ipv4Address(ipv4Address), _ipv4Port(ipv4Port),
    _ipv6Address(ipv6Address), _ipv6Port(ipv6Port) {}

const NodeId& NodeProfile::id() const { return _id; }
const Ipv4Address& NodeProfile::ipv4Address() const { return _ipv4Address; }
TcpPort NodeProfile::ipv4Port() const { return _ipv4Port; }
const Ipv6Address& NodeProfile::ipv6Address() const { return _ipv6Address; }
TcpPort NodeProfile::ipv6Port() const { return _ipv6Port; }



GpsLocation::GpsLocation(const GpsLocation &position) :
    _latitude( position.latitude() ), _longitude( position.longitude() ) {}
    
GpsLocation::GpsLocation(double latitude, double longitude) :
    _latitude(latitude), _longitude(longitude) {}

double GpsLocation::latitude()  const { return _latitude; }
double GpsLocation::longitude() const { return _longitude; }



NodeLocation::NodeLocation(const NodeProfile &profile, const GpsLocation &position) :
    _profile(profile), _position(position) {}

NodeLocation::NodeLocation(const NodeProfile &profile, double latitude, double longitude) :
    _profile(profile), _position(latitude, longitude) {}

const NodeProfile& NodeLocation::profile() const { return _profile; }
double NodeLocation::latitude()  const { return _position.latitude(); }
double NodeLocation::longitude() const { return _position.longitude(); }

