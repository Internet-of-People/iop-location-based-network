#include <cmath>
#include <stdexcept>

#include "basic.hpp"

using namespace std;



namespace LocNet
{



LocationNetworkError::LocationNetworkError(ErrorCode code, const char* reason) :
    runtime_error(reason), _code(code) {}

LocationNetworkError::LocationNetworkError(ErrorCode code, const string& reason) :
    runtime_error(reason), _code(code) {}

ErrorCode LocationNetworkError::code() const
    { return _code; }



NetworkEndpoint::NetworkEndpoint() :
    _address(), _port() {}

NetworkEndpoint::NetworkEndpoint(const NetworkEndpoint& other) :
    _address(other._address), _port(other._port) {}

NetworkEndpoint::NetworkEndpoint(const Address& address, TcpPort port) :
    _address(address), _port(port) {}

Address NetworkEndpoint::address() const { return _address; }
TcpPort NetworkEndpoint::port() const { return _port; }

ostream& operator<<(ostream &out, const NetworkEndpoint &value)
    { return out << value.address() << ":" << value.port(); }



NodeContact::NodeContact() {}
    
NodeContact::NodeContact(const NodeContact& other) :
    _address(other._address), _nodePort(other._nodePort), _clientPort(other._clientPort) {}

NodeContact::NodeContact(const Address& address, TcpPort nodePort, TcpPort clientPort) :
    _address(address), _nodePort(nodePort), _clientPort(clientPort) {}


const Address& NodeContact::address() const { return _address; }
TcpPort NodeContact::nodePort() const { return _nodePort; }
TcpPort NodeContact::clientPort() const { return _clientPort; }

NetworkEndpoint NodeContact::nodeEndpoint() const
    { return NetworkEndpoint(_address, _nodePort); }

NetworkEndpoint NodeContact::clientEndpoint() const
    { return NetworkEndpoint(_address, _clientPort); }

bool NetworkEndpoint::operator==(const NetworkEndpoint& other) const
{
    return _address == other._address &&
           _port    == other._port;
}



void NodeContact::address(const Address& address)
    { _address = address; }


bool NodeContact::operator==(const NodeContact& other) const
{
    return  _address    == other._address &&
            _nodePort   == other._nodePort &&
            _clientPort == other._clientPort;
}


ostream& operator<<(ostream &out, const NodeContact &value)
    { return out << value.address() << ":" << value.nodePort() << "|" << value.clientPort(); }



NodeProfile::NodeProfile() :
    _id(), _contact() {}

NodeProfile::NodeProfile(const NodeProfile& other) :
    _id(other._id), _contact(other._contact) {}

NodeProfile::NodeProfile(const NodeId& id, const NodeContact &contact) :
    _id(id), _contact(contact) {}

const NodeId& NodeProfile::id() const { return _id; }
NodeContact& NodeProfile::contact() { return _contact; }
const NodeContact& NodeProfile::contact() const { return _contact; }

bool NodeProfile::operator==(const NodeProfile& other) const
{
    return  _id == other._id &&
            _contact == other._contact;
}

bool NodeProfile::operator!=(const NodeProfile& other) const
    { return ! operator==(other); }


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
        { throw LocationNetworkError(ErrorCode::ERROR_INVALID_DATA, "Invalid GPS location"); }
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

NodeProfile& NodeInfo::profile() { return _profile; }
const NodeProfile& NodeInfo::profile() const { return _profile; }
const GpsLocation& NodeInfo::location() const { return _location; }

bool NodeInfo::operator==(const NodeInfo& other) const
{
    return _profile  == other._profile &&
           _location == other._location;
}

bool NodeInfo::operator!=(const NodeInfo& other) const
    { return ! operator==(other); }


std::ostream& operator<<(std::ostream& out, const NodeInfo &value)
{
    return out << "Node " << value.profile() << "; Location " << value.location();
}



} // namespace LocNet
