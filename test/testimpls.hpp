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
    //double GetBubbleSize(const GpsLocation &location) const override;

    bool Store(const NodeInfo &node, bool isNeighbour) override;
    std::shared_ptr<NodeInfo> Load(const std::string &nodeId) const override;
    bool Update(const NodeInfo &node) const override;
    bool Remove(const std::string &nodeId) override;
    
    double GetNeighbourhoodRadiusKm() const override;
    size_t GetColleagueNodeCount() const override;
    std::vector<NodeInfo> GetRandomNodes(uint16_t maxNodeCount, bool includeNeighbours) const override;
    
    std::vector<NodeInfo> GetClosestNodes(const GpsLocation &position,
        double radiusKm, size_t maxNodeCount, bool includeNeighbours) const override;
};



class DummyGeographicNetworkConnectionFactory: public IGeographicNetworkConnectionFactory
{
public:
    
    std::shared_ptr<IGeographicNetwork> ConnectTo(const NodeProfile &node) override;
};


#endif // __IMPLEMENTATIONS_FOR_TESTING_H__
