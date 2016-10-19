#ifndef NODE_H
#define NODE_H

#include <string>


class Node
{
    std::string _id;
    double _latitude  = 0.;
    double _longitude = 0.;
    
public:
    
    Node(std::string idArg);
    
    const std::string& id() const;
};

#endif // NODE_H
