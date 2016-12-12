#include <cmath>
#include <stdexcept>

#include "basic.hpp"

using namespace std;



namespace LocNet
{

NetworkInterface::NetworkInterface() {}
    
NetworkInterface::NetworkInterface(const NetworkInterface& other) :
    _addressType(other._addressType), _address(other._address), _port(other._port) {}

NetworkInterface::NetworkInterface(AddressType addressType, const Address& address, TcpPort port) :
    _addressType(addressType), _address(address), _port(port) {}

AddressType NetworkInterface::addressType() const { return _addressType; }
const Address& NetworkInterface::address() const { return _address; }
TcpPort NetworkInterface::port() const { return _port; }

bool NetworkInterface::operator==(const NetworkInterface& other) const
{
    return  _addressType == other._addressType &&
            _address == other._address &&
            _port == other._port;
}


std::ostream& operator<<(std::ostream& out, const NetworkInterface &value)
{
    return out << value.address() << ":" << value.port();
}



NodeProfile::NodeProfile() :
    _id(), _contact() {}

NodeProfile::NodeProfile(const NodeProfile& other) :
    _id(other._id), _contact(other._contact) {}

NodeProfile::NodeProfile(const NodeId& id, const NetworkInterface &contact) :
    _id(id), _contact(contact) {}

const NodeId& NodeProfile::id() const { return _id; }
const NetworkInterface& NodeProfile::contact() const { return _contact; }

bool NodeProfile::operator==(const NodeProfile& other) const
{
    return  _id == other._id &&
            _contact == other._contact;
}


std::ostream& operator<<(std::ostream& out, const NodeProfile &value)
{
    return out << value.id() << " (" << value.contact() << ")";
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
    // Ensure latitude is from range (-90,90] and longitude is from (-180,180], note that range is exclusive at the bottom and inclusive at the top
    if ( _latitude <= -90. || 90. < _latitude ||
         _longitude <= -180. || 180. < _longitude )
        { throw runtime_error("Invalid GPS location"); }
}

bool GpsLocation::operator==(const GpsLocation& other) const
{
    return abs(_latitude  - other._latitude)  < 0.00001 &&
           abs(_longitude - other._longitude) < 0.00001;
}

bool GpsLocation::operator!=(const GpsLocation& other) const
    { return ! operator==(other); }


std::ostream& operator<<(std::ostream& out, const GpsLocation &value)
{
    return out << value.latitude() << "," << value.longitude();
}




NodeInfo::NodeInfo(const NodeInfo& other) :
    _profile(other._profile), _location(other._location) {}

NodeInfo::NodeInfo(const NodeProfile &profile, const GpsLocation &location) :
    _profile(profile), _location(location) {}

NodeInfo::NodeInfo(const NodeProfile &profile, GpsCoordinate latitude, GpsCoordinate longitude) :
    _profile(profile), _location(latitude, longitude) {}

const NodeProfile& NodeInfo::profile() const { return _profile; }
const GpsLocation& NodeInfo::location() const { return _location; }

bool NodeInfo::operator==(const NodeInfo& other) const
{
    return _profile  == other._profile &&
           _location == other._location;
}


std::ostream& operator<<(std::ostream& out, const NodeInfo &value)
{
    return out << "Node " << value.profile() << "; Location " << value.location();
}



} // namespace LocNet
