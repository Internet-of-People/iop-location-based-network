#include <stdexcept>

#include "basic.hpp"

using namespace std;




NodeProfile::NodeProfile() :
    _id(), _ipv4Address(), _ipv4Port(0), _ipv6Address(), _ipv6Port(0) {}

NodeProfile::NodeProfile(const NodeProfile& other) :
    _id(other._id),
    _ipv4Address(other._ipv4Address), _ipv4Port(other._ipv4Port),
    _ipv6Address(other._ipv6Address), _ipv6Port(other._ipv6Port) {}

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

bool NodeProfile::operator==(const NodeProfile& other) const
{
    return  _id == other._id &&
            _ipv4Address == other._ipv4Address &&
            _ipv4Port == other._ipv4Port &&
            _ipv6Address == other._ipv6Address &&
            _ipv6Port == other._ipv6Port;
}




GpsLocation::GpsLocation(const GpsLocation &other) :
    _latitude( other.latitude() ), _longitude( other.longitude() ) {}
    
GpsLocation::GpsLocation(GpsCoordinate latitude, GpsCoordinate longitude) :
    _latitude(latitude), _longitude(longitude)
    { Validate(); }

GpsCoordinate GpsLocation::latitude()  const { return _latitude; }
GpsCoordinate GpsLocation::longitude() const { return _longitude; }

void GpsLocation::Validate()
{
    if ( _latitude < -90. || 90. < _latitude ||
         _longitude < -180. || 180. < _longitude )
        { throw new runtime_error("Invalid GPS location"); }
}

bool GpsLocation::operator==(const GpsLocation& other) const
{
    return _latitude  == other._latitude &&
           _longitude == other._longitude;
}




LocNetNodeInfo::LocNetNodeInfo(const LocNetNodeInfo& other) :
    _profile(other._profile), _location(other._location) {}

LocNetNodeInfo::LocNetNodeInfo(const NodeProfile &profile, const GpsLocation &location) :
    _profile(profile), _location(location) {}

LocNetNodeInfo::LocNetNodeInfo(const NodeProfile &profile, GpsCoordinate latitude, GpsCoordinate longitude) :
    _profile(profile), _location(latitude, longitude) {}

const NodeProfile& LocNetNodeInfo::profile() const { return _profile; }
const GpsLocation& LocNetNodeInfo::location() const { return _location; }

bool LocNetNodeInfo::operator==(const LocNetNodeInfo& other) const
{
    return _profile  == other._profile &&
           _location == other._location;
}
