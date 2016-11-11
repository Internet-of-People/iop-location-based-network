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
    _id(), _contacts() {}

NodeProfile::NodeProfile(const NodeProfile& other) :
    _id(other._id), _contacts(other._contacts) {}

NodeProfile::NodeProfile(const NodeId& id, vector<NetworkInterface> contacts) :
    _id(id), _contacts(contacts) {}

const NodeId& NodeProfile::id() const { return _id; }
const vector<NetworkInterface>& NodeProfile::contacts() const { return _contacts; }

bool NodeProfile::operator==(const NodeProfile& other) const
{
    return  _id == other._id &&
            _contacts == other._contacts;
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
        { throw new runtime_error("Invalid GPS location"); }
}

bool GpsLocation::operator==(const GpsLocation& other) const
{
    return _latitude  == other._latitude &&
           _longitude == other._longitude;
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


} // namespace LocNet
