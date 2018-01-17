#include <cmath>
#include <stdexcept>
#include <string>

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




// NetworkEndpoint::NetworkEndpoint() :
//     _address(), _port() {}

NetworkEndpoint::NetworkEndpoint(const NetworkEndpoint& other) :
    _address(other._address), _port(other._port) {}

NetworkEndpoint::NetworkEndpoint(const Address& address, TcpPort port) :
    _address(address), _port(port) {}

Address NetworkEndpoint::address() const { return _address; }
TcpPort NetworkEndpoint::port() const { return _port; }

bool NetworkEndpoint::operator==(const NetworkEndpoint& other) const
{
    return _address == other._address &&
           _port    == other._port;
}

bool NetworkEndpoint::operator!=(const NetworkEndpoint& other) const
    { return ! operator==(other); }


ostream& operator<<(ostream &out, const NetworkEndpoint &value)
    { return out << value.address() << ":" << value.port(); }




// NodeContact::NodeContact() {}
    
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


void NodeContact::address(const Address& address)
    { _address = address; }

bool NodeContact::operator==(const NodeContact& other) const
{
    return  _address    == other._address &&
            _nodePort   == other._nodePort &&
            _clientPort == other._clientPort;
}

bool NodeContact::operator!=(const NodeContact& other) const
    { return ! operator==(other); }


ostream& operator<<(ostream &out, const NodeContact &value)
    { return out << value.address() << ":" << value.nodePort() << "|" << value.clientPort(); }




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
        { throw LocationNetworkError(ErrorCode::ERROR_INVALID_VALUE, "Invalid GPS location"); }
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



ServiceInfo::ServiceInfo() : _type(), _port(0), _customData() {}

ServiceInfo::ServiceInfo(const ServiceInfo& other) :
    _type(other._type), _port(other._port), _customData(other._customData) {}

ServiceInfo::ServiceInfo(ServiceType type, TcpPort port, const string& customData) :
    _type(type), _port(port), _customData(customData) {}

ServiceType ServiceInfo::type() const { return _type; }
TcpPort ServiceInfo::port() const { return _port; }
const string& ServiceInfo::customData() const { return _customData; }

bool ServiceInfo::operator==(const ServiceInfo& other) const
{
    return _type       == other._type &&
           _port       == other._port &&
           _customData == other._customData;
}

bool ServiceInfo::operator!=(const ServiceInfo& other) const
    { return ! operator==(other); }




NodeInfo::NodeInfo(const NodeInfo& other) :
    _id(other._id), _location(other._location), _contact(other._contact),
    _services(other._services) {}

NodeInfo::NodeInfo( const NodeId &id, const GpsLocation &location,
                    const NodeContact &contact, const Services &services ) :
    _id(id), _location(location), _contact(contact), _services(services) {}


const NodeId&       NodeInfo::id()       const { return _id; }
const GpsLocation&  NodeInfo::location() const { return _location; }
const NodeContact&  NodeInfo::contact()  const { return _contact; }
const NodeInfo::Services& NodeInfo::services() const { return _services; }

NodeContact& NodeInfo::contact() { return _contact; }
NodeInfo::Services& NodeInfo::services() { return _services; }

bool NodeInfo::operator==(const NodeInfo& other) const
{
    return _id       == other._id &&
           _location == other._location &&
           _contact  == other._contact &&
           _services == other._services;
}

bool NodeInfo::operator!=(const NodeInfo& other) const
    { return ! operator==(other); }


std::ostream& operator<<(std::ostream& out, const NodeInfo &value)
{
    return out << "Node " << value.id() << " (" << value.contact() << "), Location "
               << value.location() << ", services: " << value.services().size();
}



} // namespace LocNet
