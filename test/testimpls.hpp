#ifndef __IMPLEMENTATIONS_FOR_TESTING_H__
#define __IMPLEMENTATIONS_FOR_TESTING_H__

#include <string>

#include "geonet.hpp"



class DummySpatialDatabase : public SpatialDatabase
{
public:
    
    DummySpatialDatabase(const GpsLocation& myLocation);
    
    double GetDistance(const GpsLocation &one, const GpsLocation &other) const override;
    
//     std::vector<NodeLocation> GetNeighbourHood() const override;
//     std::vector<NodeLocation> GetNodesCloseTo(
//         const GpsLocation &position, double radius, size_t maxNodeCount) const override;
//         
//     void Store(const NodeLocation &node) override;
//     NodeLocation Load(const std::string &nodeId) const override;
//     void Remove(const std::string &nodeId) override;
};



#endif // __IMPLEMENTATIONS_FOR_TESTING_H__
