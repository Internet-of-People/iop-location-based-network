#ifndef DUMMY_NODE_H
#define DUMMY_NODE_H

#include <string>

#include "node.hpp"


class GpsPosition : public IGpsPosition
{
    double _latitude;
    double _longitude;
    
public:
    
    GpsPosition(const IGpsPosition &position);
    GpsPosition(double latitude, double longitude);
    double latitude() const override;
    double longitude() const override;
};


class Node : public INode
{
    std::string _id;
    GpsPosition _position;
    
public:
    
    Node(const std::string &id, const IGpsPosition &position);
    Node(const std::string &id, double latitude, double longitude);
    
    const std::string& id() const override;
    double latitude() const override;
    double longitude() const override;
};


#endif // DUMMY_NODE_H
