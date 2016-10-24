#ifndef __IMPLEMENTATIONS_FOR_TESTING_H__
#define __IMPLEMENTATIONS_FOR_TESTING_H__

#include <string>

#include "geonet.hpp"



class DummySpatialDatabase : public ISpatialDatabase
{
    GpsLocation _myLocation;
    
//     class NodeEntry : public NodeLocation
//     {
//         bool _neighbour;
//         
//     protected:
//         
//         NodeEntry(const NodeProfile &profile, const GpsLocation &location, bool neighbour);
//         NodeEntry(const NodeProfile &profile, double latitude, double longitude, bool neighbour);
//         
//         bool neighbour();
//     };
//     std::vector<NodeLocation> _nodeMap;

    
public:
    
    DummySpatialDatabase(const GpsLocation& myLocation);

    double GetDistance(const GpsLocation &one, const GpsLocation &other) const override;
    double GetBubbleSize(const GpsLocation &location) const override;
    
    double GetNeighbourHoodRadiusKm() const;
    std::vector<NodeLocation> GetRandomNodes(uint16_t maxNodeCount, bool includeNeighbours) const override;
    
    std::vector<NodeLocation> GetClosestNodes(const GpsLocation &position,
        double radiusKm, size_t maxNodeCount, bool includeNeighbours) const override;

    void Store(const NodeLocation &node, bool isNeighbour) override;
    NodeLocation Load(const std::string &nodeId) const override;
    void Remove(const std::string &nodeId) override;
};



#endif // __IMPLEMENTATIONS_FOR_TESTING_H__
