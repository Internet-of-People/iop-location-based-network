#include <stdexcept>

#include "messaging.hpp"

using namespace std;



namespace LocNet
{


const GpsCoordinate GPS_COORDINATE_PROTOBUF_INT_MULTIPLIER = 1000000.;


GpsLocation Converter::FromProtoBuf(const iop::locnet::GpsLocation& value)
{
    return GpsLocation(
        value.latitude()  / GPS_COORDINATE_PROTOBUF_INT_MULTIPLIER,
        value.longitude() / GPS_COORDINATE_PROTOBUF_INT_MULTIPLIER );
}


void Converter::FillProtoBuf(iop::locnet::GpsLocation* target, const GpsLocation& source)
{
    target->set_latitude( static_cast<google::protobuf::int32>(
        source.latitude()  * GPS_COORDINATE_PROTOBUF_INT_MULTIPLIER ) );
    target->set_longitude( static_cast<google::protobuf::int32>(
        source.longitude() * GPS_COORDINATE_PROTOBUF_INT_MULTIPLIER ) );
}


iop::locnet::GpsLocation* Converter::ToProtoBuf(const GpsLocation &location)
{
    auto result = new iop::locnet::GpsLocation();
    FillProtoBuf(result, location);
    return result;
}



ServiceType Converter::FromProtoBuf(iop::locnet::ServiceType value)
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



void Converter::FillProtoBuf(
    iop::locnet::NodeProfile *target, const NodeProfile &source)
{
    target->set_nodeid( source.id() );
    for ( auto const &contact : source.contacts() )
    {
        auto outputContact = target->add_contacts();
        switch( contact.addressType() )
        {
            case AddressType::Ipv4:
            {
                auto address = new iop::locnet::Ipv4Address();
                address->set_host( contact.address() );
                address->set_port( contact.port() );
                outputContact->set_allocated_ipv4(address);
                break;
            }
            case AddressType::Ipv6:
            {
                auto address = new iop::locnet::Ipv6Address();
                address->set_host( contact.address() );
                address->set_port( contact.port() );
                outputContact->set_allocated_ipv6(address);
                break;
            }
            default: throw runtime_error("Unknown address type");
        }
    }
}

iop::locnet::NodeProfile* Converter::ToProtoBuf(const NodeProfile &profile)
{
    auto result = new iop::locnet::NodeProfile();
    FillProtoBuf(result, profile);
    return result;
}



void Converter::FillProtoBuf(
    iop::locnet::NodeInfo *target, const NodeInfo &source)
{
    target->set_allocated_profile( ToProtoBuf( source.profile() ) );
    target->set_allocated_location( ToProtoBuf( source.location() ) );
}

iop::locnet::NodeInfo* Converter::ToProtoBuf(const NodeInfo &info)
{
    auto result = new iop::locnet::NodeInfo();
    FillProtoBuf(result, info);
    return result;
}



MessageDispatcher::MessageDispatcher(LocNet::Node& node) :
    _iLocalService(node), _iRemoteNode(node), _iClient(node) {}

MessageDispatcher::MessageDispatcher(ILocalServices& iLocalServices,
                                     IRemoteNode& iRemoteNode, IClientMethods& iClient) :
    _iLocalService(iLocalServices), _iRemoteNode(iRemoteNode), _iClient(iClient) {}
    


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
            ServiceType serviceType = Converter::FromProtoBuf( registerRequest.servicetype() );
            
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
            
            _iLocalService.RegisterService( serviceType, NodeProfile( inputProfile.nodeid(), addresses ) );
            
            auto response = new iop::locnet::LocalServiceResponse();
            response->set_allocated_registerservice( new iop::locnet::RegisterServiceResponse() );
            return response;
        }
            
        case iop::locnet::LocalServiceRequest::kDeregisterService:
        {
            auto const &deregisterRequest = localServiceRequest.deregisterservice();
            ServiceType serviceType = Converter::FromProtoBuf( deregisterRequest.servicetype() );
            
            _iLocalService.DeregisterService(serviceType);
            
            auto result = new iop::locnet::LocalServiceResponse();
            result->set_allocated_deregisterservice( new iop::locnet::DeregisterServiceResponse() );
            return result;
        }
            
        case iop::locnet::LocalServiceRequest::kGetNeighbours:
        {
            vector<NodeInfo> neighbours = _iLocalService.GetNeighbourNodesByDistance();
            
            auto result = new iop::locnet::LocalServiceResponse();
            auto response = new iop::locnet::GetNeighbourNodesByDistanceResponse();
            for (auto const &neighbour : neighbours)
            {
                iop::locnet::NodeInfo *info = response->add_nodeinfo();
                Converter::FillProtoBuf(info, neighbour);
            }
            result->set_allocated_getneighbours(response);
            return result;
        }
            
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