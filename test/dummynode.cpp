#include "dummynode.hpp"

using namespace std;



GpsPosition::GpsPosition(const IGpsPosition &position) :
    _latitude( position.latitude() ), _longitude( position.longitude() ) {}
    
GpsPosition::GpsPosition(double latitude, double longitude) :
    _latitude(latitude), _longitude(longitude) {}

double GpsPosition::latitude()  const { return _latitude; }
double GpsPosition::longitude() const { return _longitude; }

    

Node::Node(const string &id, const IGpsPosition &position) :
    _id(id), _position(position) {}

Node::Node(const string &id, double latitude, double longitude) :
    _id(id), _position(latitude, longitude) {}

const string& Node::id() const { return _id; }
double Node::latitude()  const { return _position.latitude(); }
double Node::longitude() const { return _position.longitude(); }