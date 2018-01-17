#ifndef __LOCNET_PROTOBUF_MESSAGING_H__
#define __LOCNET_PROTOBUF_MESSAGING_H__

#include <future>
#include <memory>

#include <google/protobuf/text_format.h>

#include "IopLocNet.pb.h"
#include "locnet.hpp"


namespace LocNet
{


// Utility class to convert between the data representation
// of the business logic and the messages of the protobuf protocol definition.
// NOTE simply could be a namespace in the current form, no need for a class.
struct Converter
{
    // Functions that convert from protobuf to internal representation, creating a new object
    static ServiceType FromProtoBuf(iop::locnet::ServiceType value);
    static ServiceInfo FromProtoBuf(const iop::locnet::ServiceInfo &value);
    static GpsLocation FromProtoBuf(const iop::locnet::GpsLocation &value);
    static NodeInfo FromProtoBuf(const iop::locnet::NodeInfo &value);
    
    // Functions that fill up an existing protobuf object from the internal representation
    static void FillProtoBuf(iop::locnet::ServiceInfo *target, const ServiceInfo &source);
    static void FillProtoBuf(iop::locnet::GpsLocation *target, const GpsLocation &source);
    static void FillProtoBuf(iop::locnet::NodeInfo *target, const NodeInfo &source);
    
    // Functions that convert from the internal representation to protobuf, creating a new object
    static iop::locnet::Status ToProtoBuf(ErrorCode value);
    static iop::locnet::ServiceType ToProtoBuf(ServiceType value);
    static iop::locnet::ServiceInfo* ToProtoBuf(const ServiceInfo &value);
    static iop::locnet::GpsLocation* ToProtoBuf(const GpsLocation &location);
    static iop::locnet::NodeInfo* ToProtoBuf(const NodeInfo &info);
};



// Interface to dispatch messages to serve incoming requests directly in a blocking way.
// Implementation should translate incoming protobuf requests to internal representation,
// serve the request with our business logic and translate the result into a protobuf response.
class IBlockingRequestDispatcher
{
public:
    
    virtual ~IBlockingRequestDispatcher() {}
    
    virtual std::unique_ptr<iop::locnet::Response> Dispatch(std::unique_ptr<iop::locnet::Request> &&request) = 0;
};



// Dispatch messages to serve requests on the local service interface.
class IncomingLocalServiceRequestDispatcher : public IBlockingRequestDispatcher
{
    std::shared_ptr<ILocalServiceMethods>   _iLocalService;
    std::shared_ptr<IChangeListenerFactory> _listenerFactory;
    
public:
    
    IncomingLocalServiceRequestDispatcher( std::shared_ptr<ILocalServiceMethods> iLocalService,
        std::shared_ptr<IChangeListenerFactory> listenerFactory );
    
    std::unique_ptr<iop::locnet::Response> Dispatch(std::unique_ptr<iop::locnet::Request> &&request) override;
};



// Dispatch messages to serve requests on the node interface.
class IncomingNodeRequestDispatcher : public IBlockingRequestDispatcher
{
    std::shared_ptr<INodeMethods> _iNode;
    
public:
    
    IncomingNodeRequestDispatcher(std::shared_ptr<INodeMethods> iNode);
    
    std::unique_ptr<iop::locnet::Response> Dispatch(std::unique_ptr<iop::locnet::Request> &&request) override;
};



// Dispatch messages to serve requests on the client interface.
class IncomingClientRequestDispatcher : public IBlockingRequestDispatcher
{
    std::shared_ptr<IClientMethods> _iClient;
    
public:
    
    IncomingClientRequestDispatcher(std::shared_ptr<IClientMethods> iClient);
    
    std::unique_ptr<iop::locnet::Response> Dispatch(std::unique_ptr<iop::locnet::Request> &&request) override;
};


// Unified server functionality, useful to serve requests of all interfaces on a single port.
class IncomingRequestDispatcher : public IBlockingRequestDispatcher
{
    std::shared_ptr<IncomingLocalServiceRequestDispatcher> _iLocalService;
    std::shared_ptr<IncomingNodeRequestDispatcher>         _iRemoteNode;
    std::shared_ptr<IncomingClientRequestDispatcher>       _iClient;
    
public:
    
    IncomingRequestDispatcher( std::shared_ptr<Node> node,
        std::shared_ptr<IChangeListenerFactory> listenerFactory );
    
    IncomingRequestDispatcher(
        std::shared_ptr<IncomingLocalServiceRequestDispatcher> iLocalServices,
        std::shared_ptr<IncomingNodeRequestDispatcher> iRemoteNode,
        std::shared_ptr<IncomingClientRequestDispatcher> iClient );
    
    std::unique_ptr<iop::locnet::Response> Dispatch(std::unique_ptr<iop::locnet::Request> &&request) override;
};



// // Serve requests that are probably slow, e.g. has to be sent over a network.
// class IDelayedRequestDispatcher
// {
// public:
//     
//     virtual ~IDelayedRequestDispatcher() {}
//     
//     virtual std::future<std::unique_ptr<iop::locnet::Response>>
//         Dispatch(std::unique_ptr<iop::locnet::Request> &&request) = 0;
// };



// Create a proxy interface that communicates with another node through protobuf messages.
// Translate methods into protobuf requests, send the request to the other node,
// then translate its response into our internal representation.
class NodeMethodsProtoBufClient : public INodeMethods
{
    // TODO use a IDelayedRequestDispatcher instance instead
    std::shared_ptr<IBlockingRequestDispatcher> _dispatcher;
    std::function<void(const Address&)> _detectedIpCallback;
    
    std::unique_ptr<iop::locnet::Response> WaitForDispatch(
        std::unique_ptr<iop::locnet::Request> request);
    
public:
    
    NodeMethodsProtoBufClient( std::shared_ptr<IBlockingRequestDispatcher> dispatcher,
                               std::function<void(const Address&)> detectedIpCallback );
    
    NodeInfo GetNodeInfo() const override;
    size_t GetNodeCount() const override;
    std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
    
    std::shared_ptr<NodeInfo> AcceptColleague(const NodeInfo &node) override;
    std::shared_ptr<NodeInfo> RenewColleague (const NodeInfo &node) override;
    std::shared_ptr<NodeInfo> AcceptNeighbour(const NodeInfo &node) override;
    std::shared_ptr<NodeInfo> RenewNeighbour (const NodeInfo &node) override;
};



} // namespace LocNet


#endif // __LOCNET_PROTOBUF_MESSAGING_H__
