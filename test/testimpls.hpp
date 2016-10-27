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

    Distance GetDistance(const GpsLocation &one, const GpsLocation &other) const override;
    //double GetBubbleSize(const GpsLocation &location) const override;

    bool Store(const NodeInfo &node, NodeType nodeType) override;
    std::shared_ptr<NodeInfo> Load(const std::string &nodeId) const override;
    bool Update(const NodeInfo &node) const override;
    bool Remove(const std::string &nodeId) override;
    
    Distance GetNeighbourhoodRadiusKm() const override;
    size_t GetNodeCount(NodeType nodeType) const override;
    std::vector<NodeInfo> GetRandomNodes(size_t maxNodeCount, bool includeNeighbours) const override;
    
    std::vector<NodeInfo> GetClosestNodes(const GpsLocation &position,
        Distance radiusKm, size_t maxNodeCount, bool includeNeighbours) const override;
};



class DummyGeographicNetworkConnectionFactory: public IGeographicNetworkConnectionFactory
{
public:
    
    std::shared_ptr<IGeographicNetwork> ConnectTo(const NodeProfile &node) override;
};


#endif // __IMPLEMENTATIONS_FOR_TESTING_H__
