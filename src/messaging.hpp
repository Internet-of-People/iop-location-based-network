#ifndef __LOCNET_PROTOBUF_MESSAGING_H__
#define __LOCNET_PROTOBUF_MESSAGING_H__

#include <memory>

#include "IopLocNet.pb.h"
#include "locnet.hpp"


namespace LocNet
{


struct Converter
{
    static ServiceType FromProtoBuf(iop::locnet::ServiceType value);
    static GpsLocation FromProtoBuf(const iop::locnet::GpsLocation &value);
    static NodeProfile FromProtoBuf(const iop::locnet::NodeProfile &value);
    static NodeInfo FromProtoBuf(const iop::locnet::NodeInfo &value);
    
    static void FillProtoBuf(iop::locnet::GpsLocation *target, const GpsLocation &source);
    static void FillProtoBuf(iop::locnet::NodeProfile *target, const NodeProfile &source);
    static void FillProtoBuf(iop::locnet::NodeInfo *target, const NodeInfo &source);
    
    static iop::locnet::ServiceType ToProtoBuf(ServiceType value);
    static iop::locnet::GpsLocation* ToProtoBuf(const GpsLocation &location);
    static iop::locnet::NodeProfile* ToProtoBuf(const NodeProfile &profile);
    static iop::locnet::NodeInfo* ToProtoBuf(const NodeInfo &info);
};



class IProtoBufRequestDispatcher
{
public:
    
    virtual ~IProtoBufRequestDispatcher() {}
    
    virtual std::unique_ptr<iop::locnet::Response> Dispatch(const iop::locnet::Request &request) = 0;
};



class IncomingRequestDispatcher : public IProtoBufRequestDispatcher
{
    // TODO should we use some smart pointers here instead?
    ILocalServiceMethods &_iLocalService;
    INodeMethods         &_iRemoteNode;
    IClientMethods       &_iClient;
    
    iop::locnet::LocalServiceResponse* DispatchLocalService(const iop::locnet::LocalServiceRequest &request);
    iop::locnet::RemoteNodeResponse* DispatchRemoteNode(const iop::locnet::RemoteNodeRequest &request);
    iop::locnet::ClientResponse* DispatchClient(const iop::locnet::ClientRequest &request);
    
public:
    
    IncomingRequestDispatcher(Node &node);
    IncomingRequestDispatcher(ILocalServiceMethods &iLocalServices, INodeMethods &iRemoteNode, IClientMethods &iClient);
    
    virtual ~IncomingRequestDispatcher() {}
    
    std::unique_ptr<iop::locnet::Response> Dispatch(const iop::locnet::Request &request) override;
};



class NodeMethodsProtoBufClient : public INodeMethods
{
    std::shared_ptr<IProtoBufRequestDispatcher> _dispatcher;
    
public:
    
    NodeMethodsProtoBufClient(std::shared_ptr<IProtoBufRequestDispatcher> dispatcher);
    
    size_t GetNodeCount() const override;
    std::vector<NodeInfo> GetRandomNodes(
        size_t maxNodeCount, Neighbours filter) const override;
    
    std::vector<NodeInfo> GetClosestNodesByDistance(const GpsLocation &location,
        Distance radiusKm, size_t maxNodeCount, Neighbours filter) const override;
    
    bool AcceptColleague(const NodeInfo &node) override;
    bool RenewColleague(const NodeInfo &node) override;
    bool AcceptNeighbour(const NodeInfo &node) override;
    bool RenewNeighbour(const NodeInfo &node) override;
};



} // namespace LocNet


#endif // __LOCNET_PROTOBUF_MESSAGING_H__
