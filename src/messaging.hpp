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


class MessageDispatcher
{
    //Node& _node;
    ILocalServices &_iLocalService;
    IRemoteNode    &_iRemoteNode;
    IClientMethods &_iClient;
    
    iop::locnet::LocalServiceResponse* DispatchLocalService(const iop::locnet::LocalServiceRequest &request);
    iop::locnet::RemoteNodeResponse* DispatchRemoteNode(const iop::locnet::RemoteNodeRequest &request);
    iop::locnet::ClientResponse* DispatchClient(const iop::locnet::ClientRequest &request);
    
public:
    
    MessageDispatcher(Node &node);
    MessageDispatcher(ILocalServices &iLocalServices, IRemoteNode &iRemoteNode, IClientMethods &iClient);
    
    std::shared_ptr<iop::locnet::Response> Dispatch(const iop::locnet::Request &request);
};


} // namespace LocNet


#endif // __LOCNET_PROTOBUF_MESSAGING_H__
