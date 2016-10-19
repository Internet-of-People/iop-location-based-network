#include "node.h"

using namespace std;


Node::Node(const string &id, double latitude, double longitude) :
    _id(id), _latitude(latitude), _longitude(longitude) {}

const string& Node::id() const { return _id; }
double Node::latitude()  const { return _latitude; }
double Node::longitude() const { return _longitude; }
