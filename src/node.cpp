#include "node.h"

using namespace std;

Node::Node(string id) : _id(id) {}

const string& Node::id() const { return _id; }