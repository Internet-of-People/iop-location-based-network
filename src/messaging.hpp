#ifndef __LOCNET_PROTOBUF_MESSAGING_H__
#define __LOCNET_PROTOBUF_MESSAGING_H__

#include <memory>

#include "IopLocNet.pb.h"
#include "locnet.hpp"


namespace LocNet
{


class MessageDispatcher
{
    Node& _node;
    
    iop::locnet::LocalServiceResponse* DispatchLocalService(const iop::locnet::LocalServiceRequest &request);
    iop::locnet::RemoteNodeResponse* DispatchRemoteNode(const iop::locnet::RemoteNodeRequest &request);
    iop::locnet::ClientResponse* DispatchClient(const iop::locnet::ClientRequest &request);
    
public:
    
    MessageDispatcher(Node &node);
    
    std::shared_ptr<iop::locnet::Response> Dispatch(const iop::locnet::Request &request);
};


} // namespace LocNet


#endif // __LOCNET_PROTOBUF_MESSAGING_H__
