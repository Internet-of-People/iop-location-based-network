#include "geonet.hpp"

using namespace std;



GpsLocation::GpsLocation(const IGpsLocation &position) :
    _latitude( position.latitude() ), _longitude( position.longitude() ) {}
    
GpsLocation::GpsLocation(double latitude, double longitude) :
    _latitude(latitude), _longitude(longitude) {}

double GpsLocation::latitude()  const { return _latitude; }
double GpsLocation::longitude() const { return _longitude; }



NodeProfile::NodeProfile(const string &id, const IGpsLocation &position) :
    _id(id), _position(position) {}

NodeProfile::NodeProfile(const string &id, double latitude, double longitude) :
    _id(id), _position(latitude, longitude) {}

const string& NodeProfile::id() const { return _id; }
double NodeProfile::latitude()  const { return _position.latitude(); }
double NodeProfile::longitude() const { return _position.longitude(); }



ServerInfo::ServerInfo(const string& id, ServerType serverType,
                       const string& ipAddress, uint16_t tcpPort) :
    _id(id), _ipAddress(ipAddress), _tcpPort(tcpPort), _serverType(serverType) {}

const string& ServerInfo::id() const { return _id; }
ServerType ServerInfo::serverType() const { return _serverType; }
const string& ServerInfo::ipAddress() const { return _ipAddress; }
uint16_t ServerInfo::tcpPort() const { return _tcpPort; }
