#ifndef NODE_H
#define NODE_H

#include <string>
#include <memory>
#include <vector>


class IGpsPosition
{
public:
    virtual double latitude() const = 0;
    virtual double longitude() const = 0;
};


class INode : public IGpsPosition
{
public:
    virtual const std::string& id() const = 0;
};


// // TODO how to use this without too much trouble?
// #include <experimental/optional>
// 
// class ISpatialDb
// {
// public:
//     
//     virtual double GetDistance(const IGpsPosition &one, const IGpsPosition &other) const = 0;
//     
//     virtual void Store(const INode &node) = 0;
//     virtual std::unique_ptr<INode> Load(const INode &node) const = 0;
//     virtual void Remove(const std::string &nodeId) = 0;
//     
//     virtual std::vector<std::unique_ptr<INode>> GetNeighbourHood() const = 0;
//     virtual std::vector<std::unique_ptr<INode>> GetNodesCloseTo(
//         const IGpsPosition &position,
//         std::experimental::optional<double> radius,
//         std::experimental::optional<size_t> maxNodeCount) const = 0;
// };


#endif // NODE_H
