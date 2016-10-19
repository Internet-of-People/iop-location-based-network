#ifndef NODE_H
#define NODE_H

#include <string>


class Node
{
    std::string _id;
    double _latitude  = 0.;
    double _longitude = 0.;
    
public:
    
    Node(const std::string &id, double latitude, double longitude);
    
    const std::string& id() const;
    double latitude() const;
    double longitude() const;
};

#endif // NODE_H
