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
        case iop::locnet::ServiceType::Unstructured:return ServiceType::Unstructured;
        case iop::locnet::ServiceType::Content:     return ServiceType::Content;
        case iop::locnet::ServiceType::Latency:     return ServiceType::Latency;
        case iop::locnet::ServiceType::Location:    return ServiceType::Location;
        case iop::locnet::ServiceType::Token:       return ServiceType::Token;
        case iop::locnet::ServiceType::Profile:     return ServiceType::Profile;
        case iop::locnet::ServiceType::Proximity:   return ServiceType::Proximity;
        case iop::locnet::ServiceType::Relay:       return ServiceType::Relay;
        case iop::locnet::ServiceType::Reputation:  return ServiceType::Reputation;
        case iop::locnet::ServiceType::Minting:     return ServiceType::Minting;
        default: throw runtime_error("Missing or unknown service type");
    }
}

iop::locnet::ServiceType Converter::ToProtoBuf(ServiceType value)
{
    switch(value)
    {
        case ServiceType::Unstructured: return iop::locnet::ServiceType::Unstructured;
        case ServiceType::Content:      return iop::locnet::ServiceType::Content;
        case ServiceType::Latency:      return iop::locnet::ServiceType::Latency;
        case ServiceType::Location:     return iop::locnet::ServiceType::Location;
        case ServiceType::Token:        return iop::locnet::ServiceType::Token;
        case ServiceType::Profile:      return iop::locnet::ServiceType::Profile;
        case ServiceType::Proximity:    return iop::locnet::ServiceType::Proximity;
        case ServiceType::Relay:        return iop::locnet::ServiceType::Relay;
        case ServiceType::Reputation:   return iop::locnet::ServiceType::Reputation;
        case ServiceType::Minting:      return iop::locnet::ServiceType::Minting;
        default: throw runtime_error("Missing or unknown service type");
    }
}



NodeProfile Converter::FromProtoBuf(const iop::locnet::NodeProfile& value)
{
    vector<NetworkInterface> addresses;
    for (int idx = 0; idx < value.contacts_size(); ++idx)
    {
        auto const &inputAddr = value.contacts(idx);
        if ( inputAddr.has_ipv4() )
            { addresses.emplace_back( AddressType::Ipv4,
                inputAddr.ipv4().host(), inputAddr.ipv4().port() ); }
        else if ( inputAddr.has_ipv6() )
            { addresses.emplace_back( AddressType::Ipv6,
                inputAddr.ipv6().host(), inputAddr.ipv6().port() ); }
        else { throw runtime_error("Unknown address type"); }
    }
    
    return NodeProfile( value.nodeid(), addresses );
}


NodeInfo Converter::FromProtoBuf(const iop::locnet::NodeInfo& value)
{
    return NodeInfo( FromProtoBuf( value.profile() ), FromProtoBuf( value.location() ) );
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
            default: throw runtime_error("Missing or unknown address type");
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
        { throw runtime_error("Missing or unknown request version"); }
    
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
            
        default: throw runtime_error("Missing or unknown request type");
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
            
            _iLocalService.RegisterService( serviceType, Converter::FromProtoBuf( registerRequest.nodeprofile() ) );
            
            auto response = new iop::locnet::LocalServiceResponse();
            response->mutable_registerservice();
            return response;
        }
            
        case iop::locnet::LocalServiceRequest::kDeregisterService:
        {
            auto const &deregisterRequest = localServiceRequest.deregisterservice();
            ServiceType serviceType = Converter::FromProtoBuf( deregisterRequest.servicetype() );
            
            _iLocalService.DeregisterService(serviceType);
            
            auto result = new iop::locnet::LocalServiceResponse();
            result->mutable_deregisterservice();
            return result;
        }
            
        case iop::locnet::LocalServiceRequest::kGetNeighbourNodes:
        {
            vector<NodeInfo> neighbours = _iLocalService.GetNeighbourNodesByDistance();
            
            auto result = new iop::locnet::LocalServiceResponse();
            auto response = result->mutable_getneighbournodes();
            for (auto const &neighbour : neighbours)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, neighbour);
            }
            return result;
        }
            
        default: throw runtime_error("Missing or unknown local service operation");
    }
}



iop::locnet::RemoteNodeResponse* MessageDispatcher::DispatchRemoteNode(
    const iop::locnet::RemoteNodeRequest& request)
{
    // TODO
    return nullptr;
}



iop::locnet::ClientResponse* MessageDispatcher::DispatchClient(
    const iop::locnet::ClientRequest& clientRequest)
{
    switch ( clientRequest.ClientRequestType_case() )
    {
        case iop::locnet::ClientRequest::kGetServices:
        {
            auto const &services = _iClient.GetServices();
            
            auto result = new iop::locnet::ClientResponse();
            auto response = result->mutable_getservices();
            for (auto const &service : services)
            {
                iop::locnet::ServiceProfile *entry = response->add_services();
                entry->set_servicetype( Converter::ToProtoBuf(service.first) );
                entry->set_allocated_profile( Converter::ToProtoBuf(service.second) );
            }
            return result;
        }
        
        case iop::locnet::ClientRequest::kGetNeighbourNodes:
        {
            vector<NodeInfo> neighbours = _iClient.GetNeighbourNodesByDistance();
            
            auto result = new iop::locnet::ClientResponse();
            auto response = result->mutable_getneighbournodes();
            for (auto const &neighbour : neighbours)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, neighbour);
            }
            return result;
        }
        
        case iop::locnet::ClientRequest::kGetClosestNodes:
        {
            auto const &closestRequest = clientRequest.getclosestnodes();
            GpsLocation location = Converter::FromProtoBuf( closestRequest.location() );
            Neighbours neighbourFilter = closestRequest.includeneighbours() ?
                Neighbours::Included : Neighbours::Excluded;
            
            vector<NodeInfo> closeNodes( _iClient.GetClosestNodesByDistance( location,
                closestRequest.maxradiuskm(), closestRequest.maxnodecount(), neighbourFilter) );
            
            auto result = new iop::locnet::ClientResponse();
            auto response = result->mutable_getclosestnodes();
            for (auto const &neighbour : closeNodes)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, neighbour);
            }
            return result;
        }
        
        default: throw runtime_error("Missing or unknown client operation");
    }
}



} // namespace LocNet