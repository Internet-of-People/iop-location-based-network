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
    if ( ! value.has_contact() )
        { throw runtime_error("No contact information for profile"); }
    
    const iop::locnet::Contact &contact = value.contact();
    if ( contact.has_ipv4() )
        { return NodeProfile( value.nodeid(), NetworkInterface( AddressType::Ipv4,
            contact.ipv4().host(), contact.ipv4().port() ) ); }
    else if ( contact.has_ipv6() )
        { return NodeProfile( value.nodeid(), NetworkInterface( AddressType::Ipv6,
            contact.ipv6().host(), contact.ipv6().port() ) ); }
    else { throw runtime_error("Unknown address type"); }
}


NodeInfo Converter::FromProtoBuf(const iop::locnet::NodeInfo& value)
{
    return NodeInfo( FromProtoBuf( value.profile() ), FromProtoBuf( value.location() ) );
}



void Converter::FillProtoBuf(iop::locnet::NodeProfile *target, const NodeProfile &source)
{
    const NetworkInterface &sourceContact = source.contact();
    
    target->set_nodeid( source.id() );
    iop::locnet::Contact *targetContact = target->mutable_contact();
    switch( sourceContact.addressType() )
    {
        case AddressType::Ipv4:
            targetContact->mutable_ipv4()->set_host( sourceContact.address() );
            targetContact->mutable_ipv4()->set_port( sourceContact.port() );
            break;

        case AddressType::Ipv6:
            targetContact->mutable_ipv6()->set_host( sourceContact.address() );
            targetContact->mutable_ipv6()->set_port( sourceContact.port() );
            break;

        default: throw runtime_error("Missing or unknown address type");
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



IncomingRequestDispatcher::IncomingRequestDispatcher(LocNet::Node& node) :
    _iLocalService(node), _iRemoteNode(node), _iClient(node) {}

IncomingRequestDispatcher::IncomingRequestDispatcher(ILocalServiceMethods& iLocalServices,
                                     INodeMethods& iRemoteNode, IClientMethods& iClient) :
    _iLocalService(iLocalServices), _iRemoteNode(iRemoteNode), _iClient(iClient) {}
    


unique_ptr<iop::locnet::Response> IncomingRequestDispatcher::Dispatch(const iop::locnet::Request& request)
{
    // TODO implement better version checks
    if ( request.version().empty() || request.version()[0] != '1' )
        { throw runtime_error("Missing or unknown request version"); }
    
    unique_ptr<iop::locnet::Response> result( new iop::locnet::Response() );
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



iop::locnet::LocalServiceResponse* IncomingRequestDispatcher::DispatchLocalService(
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
            auto const &getneighboursRequest = localServiceRequest.getneighbournodes();
            bool keepAlive = getneighboursRequest.keepaliveandsendupdates();
            
            // TODO how to handle here if keepAlive flag is set?
            //      It effects thw lower network layer, but we're unaware of that here.
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



iop::locnet::RemoteNodeResponse* IncomingRequestDispatcher::DispatchRemoteNode(
    const iop::locnet::RemoteNodeRequest& nodeRequest)
{
    switch ( nodeRequest.RemoteNodeRequestType_case() )
    {
        case iop::locnet::RemoteNodeRequest::kAcceptColleague:
        {
            auto acceptColleagueReq = nodeRequest.acceptcolleague();
            shared_ptr<NodeInfo> result = _iRemoteNode.AcceptColleague( Converter::FromProtoBuf(
                acceptColleagueReq.requestornodeinfo() ) );
            auto response = new iop::locnet::RemoteNodeResponse();
            response->mutable_acceptcolleague()->set_accepted( static_cast<bool>(result) );
            if (result) {
                response->mutable_acceptcolleague()->set_allocated_acceptornodeinfo(
                    Converter::ToProtoBuf(*result) );
            }
            return response;
        }
        
        case iop::locnet::RemoteNodeRequest::kRenewColleague:
        {
            auto renewColleagueReq = nodeRequest.renewcolleague();
            shared_ptr<NodeInfo> result = _iRemoteNode.RenewColleague( Converter::FromProtoBuf(
                renewColleagueReq.requestornodeinfo() ) );
            auto response = new iop::locnet::RemoteNodeResponse();
            response->mutable_renewcolleague()->set_accepted( static_cast<bool>(result) );
            if (result) {
                response->mutable_renewcolleague()->set_allocated_acceptornodeinfo(
                    Converter::ToProtoBuf(*result) );
            }
            return response;
        }
        
        case iop::locnet::RemoteNodeRequest::kAcceptNeighbour:
        {
            auto acceptNeighbourReq = nodeRequest.acceptneighbour();
            shared_ptr<NodeInfo> result = _iRemoteNode.AcceptNeighbour( Converter::FromProtoBuf(
                acceptNeighbourReq.requestornodeinfo() ) );
            auto response = new iop::locnet::RemoteNodeResponse();
            response->mutable_acceptneighbour()->set_accepted( static_cast<bool>(result) );
            if (result) {
                response->mutable_acceptneighbour()->set_allocated_acceptornodeinfo(
                    Converter::ToProtoBuf(*result) );
            }
            return response;
        }
        
        case iop::locnet::RemoteNodeRequest::kRenewNeighbour:
        {
            auto renewNeighbourReq = nodeRequest.renewneighbour();
            shared_ptr<NodeInfo> result = _iRemoteNode.RenewNeighbour( Converter::FromProtoBuf(
                renewNeighbourReq.requestornodeinfo() ) );
            auto response = new iop::locnet::RemoteNodeResponse();
            response->mutable_renewneighbour()->set_accepted( static_cast<bool>(result) );
            if (result) {
                response->mutable_renewneighbour()->set_allocated_acceptornodeinfo(
                    Converter::ToProtoBuf(*result) );
            }
            return response;
        }

        case iop::locnet::RemoteNodeRequest::kGetNodeCount:
        {
            size_t counter = _iRemoteNode.GetNodeCount();
            auto response = new iop::locnet::RemoteNodeResponse();
            response->mutable_getnodecount()->set_nodecount(counter);
            return response;
        }
        
        case iop::locnet::RemoteNodeRequest::kGetRandomNodes:
        {
            auto randomNodesReq = nodeRequest.getrandomnodes();
            Neighbours neighbourFilter = randomNodesReq.includeneighbours() ?
                Neighbours::Included : Neighbours::Excluded;
                
            vector<NodeInfo> randomNodes = _iRemoteNode.GetRandomNodes(
                randomNodesReq.maxnodecount(), neighbourFilter );
            
            auto result = new iop::locnet::RemoteNodeResponse();
            auto response = result->mutable_getrandomnodes();
            for (auto const &node : randomNodes)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, node);
            }
            return result;
        }
        
        case iop::locnet::RemoteNodeRequest::kGetClosestNodes:
        {
            auto const &closestRequest = nodeRequest.getclosestnodes();
            GpsLocation location = Converter::FromProtoBuf( closestRequest.location() );
            Neighbours neighbourFilter = closestRequest.includeneighbours() ?
                Neighbours::Included : Neighbours::Excluded;
            
            vector<NodeInfo> closeNodes( _iRemoteNode.GetClosestNodesByDistance( location,
                closestRequest.maxradiuskm(), closestRequest.maxnodecount(), neighbourFilter) );
            
            auto result = new iop::locnet::RemoteNodeResponse();
            auto response = result->mutable_getclosestnodes();
            for (auto const &node : closeNodes)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, node);
            }
            return result;
        }
        
        default: throw runtime_error("Missing or unknown remote node operation");
    }
}



iop::locnet::ClientResponse* IncomingRequestDispatcher::DispatchClient(
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
            for (auto const &node : closeNodes)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, node);
            }
            return result;
        }
        
        default: throw runtime_error("Missing or unknown client operation");
    }
}




NodeMethodsProtoBufClient::NodeMethodsProtoBufClient(std::shared_ptr<IProtoBufRequestDispatcher> dispatcher) :
    _dispatcher(dispatcher)
{
    if (! _dispatcher)
        { throw runtime_error("Invalid dispatcher argument"); }
}



size_t NodeMethodsProtoBufClient::GetNodeCount() const
{
    iop::locnet::Request request;
    //request.set_version("1");
    request.mutable_remotenode()->mutable_getnodecount();
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_getnodecount() )
        { throw runtime_error("Failed to get expected response"); }
        
    return response->remotenode().getnodecount().nodecount();
}



shared_ptr<NodeInfo> NodeMethodsProtoBufClient::AcceptColleague(const NodeInfo& node)
{
    iop::locnet::Request request;
    //request.set_version("1");
    request.mutable_remotenode()->mutable_acceptcolleague()->set_allocated_requestornodeinfo(
        Converter::ToProtoBuf(node) );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_acceptcolleague() )
        { throw runtime_error("Failed to get expected response"); }
    
    return response->remotenode().acceptcolleague().accepted() ?
        shared_ptr<NodeInfo>( new NodeInfo( Converter::FromProtoBuf(
            response->remotenode().acceptcolleague().acceptornodeinfo() ) ) ) :
        shared_ptr<NodeInfo>();
}



shared_ptr<NodeInfo> NodeMethodsProtoBufClient::RenewColleague(const NodeInfo& node)
{
    iop::locnet::Request request;
    //request.set_version("1");
    request.mutable_remotenode()->mutable_renewcolleague()->set_allocated_requestornodeinfo(
        Converter::ToProtoBuf(node) );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_renewcolleague() )
        { throw runtime_error("Failed to get expected response"); }
    
    return response->remotenode().renewcolleague().accepted() ?
        shared_ptr<NodeInfo>( new NodeInfo( Converter::FromProtoBuf(
            response->remotenode().renewcolleague().acceptornodeinfo() ) ) ) :
        shared_ptr<NodeInfo>();
}



shared_ptr<NodeInfo> NodeMethodsProtoBufClient::AcceptNeighbour(const NodeInfo& node)
{
    iop::locnet::Request request;
    //request.set_version("1");
    request.mutable_remotenode()->mutable_acceptneighbour()->set_allocated_requestornodeinfo(
        Converter::ToProtoBuf(node) );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_acceptneighbour() )
        { throw runtime_error("Failed to get expected response"); }
    
    return response->remotenode().acceptneighbour().accepted() ?
        shared_ptr<NodeInfo>( new NodeInfo( Converter::FromProtoBuf(
            response->remotenode().acceptneighbour().acceptornodeinfo() ) ) ) :
        shared_ptr<NodeInfo>();
}



shared_ptr<NodeInfo> NodeMethodsProtoBufClient::RenewNeighbour(const NodeInfo& node)
{
    iop::locnet::Request request;
    //request.set_version("1");
    request.mutable_remotenode()->mutable_renewneighbour()->set_allocated_requestornodeinfo(
        Converter::ToProtoBuf(node) );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_renewneighbour() )
        { throw runtime_error("Failed to get expected response"); }
    
    return response->remotenode().renewneighbour().accepted() ?
        shared_ptr<NodeInfo>( new NodeInfo( Converter::FromProtoBuf(
            response->remotenode().renewneighbour().acceptornodeinfo() ) ) ) :
        shared_ptr<NodeInfo>();
}



vector<NodeInfo> NodeMethodsProtoBufClient::GetRandomNodes(
    size_t maxNodeCount, Neighbours filter) const
{
    iop::locnet::Request request;
    //request.set_version("1");
    iop::locnet::GetRandomNodesRequest *getRandReq = request.mutable_remotenode()->mutable_getrandomnodes();
    getRandReq->set_maxnodecount(maxNodeCount);
    getRandReq->set_includeneighbours( filter == Neighbours::Included );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_getrandomnodes() )
        { throw runtime_error("Failed to get expected response"); }
    
    const iop::locnet::GetRandomNodesResponse &getRandResp = response->remotenode().getrandomnodes();
    vector<NodeInfo> result;
    for (int32_t idx = 0; idx < getRandResp.nodes_size(); ++idx)
        { result.push_back( Converter::FromProtoBuf( getRandResp.nodes(idx) ) ); }
    return result;
}



vector<NodeInfo> NodeMethodsProtoBufClient::GetClosestNodesByDistance(
    const GpsLocation& location, Distance radiusKm, size_t maxNodeCount, Neighbours filter) const
{
    iop::locnet::Request request;
    //request.set_version("1");
    iop::locnet::GetClosestNodesByDistanceRequest *getNodeReq =
        request.mutable_remotenode()->mutable_getclosestnodes();
    getNodeReq->set_allocated_location( Converter::ToProtoBuf(location) );
    getNodeReq->set_maxradiuskm(radiusKm);
    getNodeReq->set_maxnodecount(maxNodeCount);
    getNodeReq->set_includeneighbours( filter == Neighbours::Included );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_getclosestnodes() )
        { throw runtime_error("Failed to get expected response"); }
    
    const iop::locnet::GetClosestNodesByDistanceResponse &getNodeResp = response->remotenode().getclosestnodes();
    vector<NodeInfo> result;
    for (int32_t idx = 0; idx < getNodeResp.nodes_size(); ++idx)
        { result.push_back( Converter::FromProtoBuf( getNodeResp.nodes(idx) ) ); }
    return result;
}



} // namespace LocNet