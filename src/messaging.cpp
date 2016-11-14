#include <stdexcept>

#include "messaging.hpp"

using namespace std;



namespace LocNet
{


ServiceType ProtoBuf2LocNet_ServiceTypeEnum(iop::locnet::ServiceType value)
{
    switch(value)
    {
        case iop::locnet::ServiceType::Token:      return ServiceType::Token;
        case iop::locnet::ServiceType::Profile:    return ServiceType::Profile;
        case iop::locnet::ServiceType::Proximity:  return ServiceType::Proximity;
        case iop::locnet::ServiceType::Relay:      return ServiceType::Relay;
        case iop::locnet::ServiceType::Reputation: return ServiceType::Reputation;
        case iop::locnet::ServiceType::Minting:    return ServiceType::Minting;
        default: throw runtime_error("Unknown service type");
    }
}

    
    
MessageDispatcher::MessageDispatcher(LocNet::Node& node) :
    _node(node) {}



shared_ptr<iop::locnet::Response> MessageDispatcher::Dispatch(const iop::locnet::Request& request)
{
    // TODO implement better version checks
    if ( request.version().empty() || request.version()[0] != '1' )
        { throw runtime_error("Unknown request version"); }
    
    shared_ptr<iop::locnet::Response> result( new iop::locnet::Response() );
    auto &response = *result;
    
    switch ( request.RequestType_case() )
    {
        case iop::locnet::Request::kLocalService:
            response.set_allocated_localservice(
                DispatchLocalService( request.localservice() ) );
            break;
            
        case iop::locnet::Request::kRemoteNode:
            response.set_allocated_remotenode( 
                DispatchRemoteNode( request.remotenode() ) );
            break;
            
        case iop::locnet::Request::kClient:
            response.set_allocated_client(
                DispatchClient( request.client() ) );
            break;
            
        default: throw runtime_error("Unknown request type");
    }
    
    return result;
}



iop::locnet::LocalServiceResponse* MessageDispatcher::DispatchLocalService(
    const iop::locnet::LocalServiceRequest& localServiceRequest)
{
    switch ( localServiceRequest.LocalServiceRequestType_case() )
    {
        case iop::locnet::LocalServiceRequest::kRegisterService:
        {
            auto const &registerRequest = localServiceRequest.registerservice();
            ServiceType serviceType = ProtoBuf2LocNet_ServiceTypeEnum( registerRequest.servicetype() );
            
            auto const &inputProfile = registerRequest.nodeprofile();
            vector<NetworkInterface> addresses;
            for (int idx = 0; idx < inputProfile.contacts_size(); ++idx)
            {
                auto const &inputAddr = inputProfile.contacts(idx);
                if ( inputAddr.has_ipv4() )
                    { addresses.emplace_back( AddressType::Ipv4,
                        inputAddr.ipv4().host(), inputAddr.ipv4().port() ); }
                else if ( inputAddr.has_ipv6() )
                    { addresses.emplace_back( AddressType::Ipv6,
                        inputAddr.ipv6().host(), inputAddr.ipv6().port() ); }
                else { throw runtime_error("Unknown address type"); }
            }
            _node.RegisterService( serviceType, NodeProfile( inputProfile.nodeid(), addresses ) );
            auto response = new iop::locnet::LocalServiceResponse();
            response->set_allocated_registerservice( new iop::locnet::RegisterServiceResponse() );
            return response;
        }
            
        case iop::locnet::LocalServiceRequest::kDeregisterService:
            break;
            
        case iop::locnet::LocalServiceRequest::kGetNeighbours:
            break;
            
        default: throw runtime_error("Unknown local service operation");
    }
}


iop::locnet::RemoteNodeResponse* MessageDispatcher::DispatchRemoteNode(
    const iop::locnet::RemoteNodeRequest& request)
{
    // TODO
    return nullptr;
}


iop::locnet::ClientResponse* MessageDispatcher::DispatchClient(
    const iop::locnet::ClientRequest& request)
{
    // TODO
    return nullptr;
}



} // namespace LocNet