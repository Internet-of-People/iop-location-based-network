#include <stdexcept>

#include <easylogging++.h>

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


iop::locnet::Status Converter::ToProtoBuf(ErrorCode value)
{
    switch(value)
    {
        case ErrorCode::ERROR_BAD_REQUEST:          return iop::locnet::Status::ERROR_PROTOCOL_VIOLATION;
        case ErrorCode::ERROR_BAD_RESPONSE:         return iop::locnet::Status::ERROR_INTERNAL;
        case ErrorCode::ERROR_CONCEPTUAL:           return iop::locnet::Status::ERROR_INTERNAL;
        case ErrorCode::ERROR_CONNECTION:           return iop::locnet::Status::ERROR_INTERNAL;
        case ErrorCode::ERROR_INTERNAL:             return iop::locnet::Status::ERROR_INTERNAL;
        case ErrorCode::ERROR_INVALID_DATA:         return iop::locnet::Status::ERROR_INVALID_VALUE;
        case ErrorCode::ERROR_INVALID_STATE:        return iop::locnet::Status::ERROR_INTERNAL;
        case ErrorCode::ERROR_PROTOCOL_VIOLATION:   return iop::locnet::Status::ERROR_PROTOCOL_VIOLATION;
        case ErrorCode::ERROR_UNSUPPORTED:          return iop::locnet::Status::ERROR_UNSUPPORTED;
        default: throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Conversion for error code not implemented");
    }
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
        default: throw LocationNetworkError(ErrorCode::ERROR_INVALID_DATA, "Missing or unknown service type");
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
        default: throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Conversion for service type not implemented");
    }
}



NodeProfile Converter::FromProtoBuf(const iop::locnet::NodeProfile& value)
{
    if ( ! value.has_contact() )
        { throw LocationNetworkError(ErrorCode::ERROR_INVALID_DATA, "No contact information for profile"); }
    
    const iop::locnet::Contact &contact = value.contact();
    return NodeProfile( value.nodeid(), NetworkInterface(
        NetworkInterface::AddressFromBytes( contact.ipaddress() ), contact.port() ) );
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
    targetContact->set_port( sourceContact.port() );
    targetContact->set_ipaddress( sourceContact.AddressBytes() );
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



IncomingRequestDispatcher::IncomingRequestDispatcher(
        shared_ptr<LocNet::Node> node, shared_ptr<IChangeListenerFactory> listenerFactory ) :
    _iLocalService(node), _iRemoteNode(node), _iClient(node),
    _listenerFactory(listenerFactory) {}

IncomingRequestDispatcher::IncomingRequestDispatcher( shared_ptr<ILocalServiceMethods> iLocalServices,
        shared_ptr<INodeMethods> iRemoteNode, shared_ptr<IClientMethods> iClient,
        shared_ptr<IChangeListenerFactory> listenerFactory ) :
    _iLocalService(iLocalServices), _iRemoteNode(iRemoteNode), _iClient(iClient),
    _listenerFactory(listenerFactory) {}
    


unique_ptr<iop::locnet::Response> IncomingRequestDispatcher::Dispatch(const iop::locnet::Request& request)
{
    if ( request.version().empty() || request.version()[0] != 1 )
        { throw LocationNetworkError(ErrorCode::ERROR_UNSUPPORTED, "Missing or unknown request version"); }
    
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
            
        default: throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing or unknown request type");
    }
    
    return result;
}



// TODO All interface dispatchers ( DispatchLocalService(), DispatchRemoteNode(), DispatchClient() )
//      simply translate between different data formats, ideally this should be generated.
iop::locnet::LocalServiceResponse* IncomingRequestDispatcher::DispatchLocalService(
    const iop::locnet::LocalServiceRequest& localServiceRequest)
{
    switch ( localServiceRequest.LocalServiceRequestType_case() )
    {
        case iop::locnet::LocalServiceRequest::kRegisterService:
        {
            auto const &registerRequest = localServiceRequest.registerservice();
            ServiceType serviceType = Converter::FromProtoBuf( registerRequest.servicetype() );
            
            _iLocalService->RegisterService( serviceType, Converter::FromProtoBuf( registerRequest.nodeprofile() ) );
            LOG(DEBUG) << "Served RegisterService()";
            
            auto response = new iop::locnet::LocalServiceResponse();
            response->mutable_registerservice();
            return response;
        }
            
        case iop::locnet::LocalServiceRequest::kDeregisterService:
        {
            auto const &deregisterRequest = localServiceRequest.deregisterservice();
            ServiceType serviceType = Converter::FromProtoBuf( deregisterRequest.servicetype() );
            
            _iLocalService->DeregisterService(serviceType);
            LOG(DEBUG) << "Served DeregisterService()";
            
            auto result = new iop::locnet::LocalServiceResponse();
            result->mutable_deregisterservice();
            return result;
        }
            
        case iop::locnet::LocalServiceRequest::kGetNeighbourNodes:
        {
            auto const &getneighboursRequest = localServiceRequest.getneighbournodes();
            bool keepAlive = getneighboursRequest.keepaliveandsendupdates();
            
            vector<NodeInfo> neighbours = _iLocalService->GetNeighbourNodesByDistance();
            LOG(DEBUG) << "Served GetNeighbourNodes() with keepalive " << keepAlive
                       << ", node count : " << neighbours.size();
            
            auto result = new iop::locnet::LocalServiceResponse();
            auto response = result->mutable_getneighbournodes();
            for (auto const &neighbour : neighbours)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, neighbour);
            }
            
            if (keepAlive)
            {
                shared_ptr<IChangeListener> listener = _listenerFactory->Create(_iLocalService);
                _iLocalService->AddListener(listener);
            }
            
            return result;
        }
        
        case iop::locnet::LocalServiceRequest::kNeighbourhoodChanged:
            throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST,
                "Invalid request type. This is a notification message: it's not supposed to be sent "
                "as a request to this server but to be received as notification "
                "after a GetNeighbourNodesRequest with flag keepAlive set.");
        
        default: throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing or unknown local service operation");
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
            auto nodeInfo( Converter::FromProtoBuf( acceptColleagueReq.requestornodeinfo() ) );
            
            shared_ptr<NodeInfo> result = _iRemoteNode->AcceptColleague(nodeInfo);
            LOG(DEBUG) << "Served AcceptColleague(" << nodeInfo
                       << "), accepted: " << static_cast<bool>(result);
            
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
            auto nodeInfo( Converter::FromProtoBuf( renewColleagueReq.requestornodeinfo() ) );
            
            shared_ptr<NodeInfo> result = _iRemoteNode->RenewColleague(nodeInfo);
            LOG(DEBUG) << "Served RenewColleague(" << nodeInfo
                       << "), accepted: " << static_cast<bool>(result);
                       
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
            auto nodeInfo( Converter::FromProtoBuf( acceptNeighbourReq.requestornodeinfo() ) );
            
            shared_ptr<NodeInfo> result = _iRemoteNode->AcceptNeighbour(nodeInfo);
            LOG(DEBUG) << "Served AcceptNeighbour(" << nodeInfo
                       << "), accepted: " << static_cast<bool>(result);
                       
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
            auto nodeInfo( Converter::FromProtoBuf( renewNeighbourReq.requestornodeinfo() ) );
            
            shared_ptr<NodeInfo> result = _iRemoteNode->RenewNeighbour(nodeInfo);
            LOG(DEBUG) << "Served RenewNeighbour(" << nodeInfo
                       << "), accepted: " << static_cast<bool>(result);
                       
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
            size_t counter = _iRemoteNode->GetNodeCount();
            LOG(DEBUG) << "Served GetNodeCount(), node count: " << counter;
            
            auto response = new iop::locnet::RemoteNodeResponse();
            response->mutable_getnodecount()->set_nodecount(counter);
            return response;
        }
        
        case iop::locnet::RemoteNodeRequest::kGetRandomNodes:
        {
            auto randomNodesReq = nodeRequest.getrandomnodes();
            Neighbours neighbourFilter = randomNodesReq.includeneighbours() ?
                Neighbours::Included : Neighbours::Excluded;
                
            vector<NodeInfo> randomNodes = _iRemoteNode->GetRandomNodes(
                randomNodesReq.maxnodecount(), neighbourFilter );
            LOG(DEBUG) << "Served GetRandomNodes(), node count: " << randomNodes.size();
            
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
            
            vector<NodeInfo> closeNodes( _iRemoteNode->GetClosestNodesByDistance( location,
                closestRequest.maxradiuskm(), closestRequest.maxnodecount(), neighbourFilter) );
            LOG(DEBUG) << "Served GetClosestNodes(), node count: " << closeNodes.size();
            
            auto result = new iop::locnet::RemoteNodeResponse();
            auto response = result->mutable_getclosestnodes();
            for (auto const &node : closeNodes)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, node);
            }
            return result;
        }
        
        default: throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing or unknown remote node operation");
    }
}



iop::locnet::ClientResponse* IncomingRequestDispatcher::DispatchClient(
    const iop::locnet::ClientRequest& clientRequest)
{
    switch ( clientRequest.ClientRequestType_case() )
    {
        case iop::locnet::ClientRequest::kGetServices:
        {
            auto const &services = _iClient->GetServices();
            LOG(DEBUG) << "Served GetServices(), service count: " << services.size();
            
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
            vector<NodeInfo> neighbours = _iClient->GetNeighbourNodesByDistance();
            LOG(DEBUG) << "Served GetNeighbourNodes(), node count: " << neighbours.size();
            
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
            
            vector<NodeInfo> closeNodes( _iClient->GetClosestNodesByDistance( location,
                closestRequest.maxradiuskm(), closestRequest.maxnodecount(), neighbourFilter) );
            LOG(DEBUG) << "Served GetClosestNodes(), node count: " << closeNodes.size();
            
            auto result = new iop::locnet::ClientResponse();
            auto response = result->mutable_getclosestnodes();
            for (auto const &node : closeNodes)
            {
                iop::locnet::NodeInfo *info = response->add_nodes();
                Converter::FillProtoBuf(info, node);
            }
            return result;
        }
        
        default: throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing or unknown client operation");
    }
}




NodeMethodsProtoBufClient::NodeMethodsProtoBufClient(
    std::shared_ptr<IProtoBufRequestDispatcher> dispatcher, std::function<void(const Address&)> detectedIpCallback) :
    _dispatcher(dispatcher), _detectedIpCallback(detectedIpCallback)
{
    if (! _dispatcher)
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No dispatcher instantiated"); }
}



// TODO All methods simply translate between different data formats, ideally this should be generated.
size_t NodeMethodsProtoBufClient::GetNodeCount() const
{
    iop::locnet::Request request;
    request.mutable_remotenode()->mutable_getnodecount();
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_getnodecount() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Failed to get expected response"); }
    
    auto result = response->remotenode().getnodecount().nodecount();
    LOG(DEBUG) << "Request GetNodeCount() returned " << result;
    return result;
}



shared_ptr<NodeInfo> NodeMethodsProtoBufClient::AcceptColleague(const NodeInfo& node)
{
    iop::locnet::Request request;
    request.mutable_remotenode()->mutable_acceptcolleague()->set_allocated_requestornodeinfo(
        Converter::ToProtoBuf(node) );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_acceptcolleague() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Failed to get expected response"); }
    
    auto result = response->remotenode().acceptcolleague().accepted() ?
        shared_ptr<NodeInfo>( new NodeInfo( Converter::FromProtoBuf(
            response->remotenode().acceptcolleague().acceptornodeinfo() ) ) ) :
        shared_ptr<NodeInfo>();
    LOG(DEBUG) << "Request AcceptColleague() returned " << static_cast<bool>(result);
    
    if (_detectedIpCallback)
    {
        const string &address = response->remotenode().acceptcolleague().remoteipaddress();
        if ( ! address.empty() )
            { _detectedIpCallback( NetworkInterface::AddressFromBytes(address) ); }
    }
    return result;
}



shared_ptr<NodeInfo> NodeMethodsProtoBufClient::RenewColleague(const NodeInfo& node)
{
    iop::locnet::Request request;
    request.mutable_remotenode()->mutable_renewcolleague()->set_allocated_requestornodeinfo(
        Converter::ToProtoBuf(node) );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_renewcolleague() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Failed to get expected response"); }
    
    auto result = response->remotenode().renewcolleague().accepted() ?
        shared_ptr<NodeInfo>( new NodeInfo( Converter::FromProtoBuf(
            response->remotenode().renewcolleague().acceptornodeinfo() ) ) ) :
        shared_ptr<NodeInfo>();
    LOG(DEBUG) << "Request RenewColleague() returned " << static_cast<bool>(result);
    
    if (_detectedIpCallback)
    {
        const string &address = response->remotenode().renewcolleague().remoteipaddress();
        if ( ! address.empty() )
            { _detectedIpCallback( NetworkInterface::AddressFromBytes(address) ); }
    }
    return result;
}



shared_ptr<NodeInfo> NodeMethodsProtoBufClient::AcceptNeighbour(const NodeInfo& node)
{
    iop::locnet::Request request;
    request.mutable_remotenode()->mutable_acceptneighbour()->set_allocated_requestornodeinfo(
        Converter::ToProtoBuf(node) );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_acceptneighbour() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Failed to get expected response"); }
    
    auto result = response->remotenode().acceptneighbour().accepted() ?
        shared_ptr<NodeInfo>( new NodeInfo( Converter::FromProtoBuf(
            response->remotenode().acceptneighbour().acceptornodeinfo() ) ) ) :
        shared_ptr<NodeInfo>();
    LOG(DEBUG) << "Request AcceptNeighbour() returned " << static_cast<bool>(result);
    
    if (_detectedIpCallback)
    {
        const string &address = response->remotenode().acceptneighbour().remoteipaddress();
        if ( ! address.empty() )
            { _detectedIpCallback( NetworkInterface::AddressFromBytes(address) ); }
    }
    return result;
}



shared_ptr<NodeInfo> NodeMethodsProtoBufClient::RenewNeighbour(const NodeInfo& node)
{
    iop::locnet::Request request;
    request.mutable_remotenode()->mutable_renewneighbour()->set_allocated_requestornodeinfo(
        Converter::ToProtoBuf(node) );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_renewneighbour() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Failed to get expected response"); }
    
    auto result = response->remotenode().renewneighbour().accepted() ?
        shared_ptr<NodeInfo>( new NodeInfo( Converter::FromProtoBuf(
            response->remotenode().renewneighbour().acceptornodeinfo() ) ) ) :
        shared_ptr<NodeInfo>();
    LOG(DEBUG) << "Request RenewNeighbour() returned " << static_cast<bool>(result);
    
    if (_detectedIpCallback)
    {
        const string &address = response->remotenode().renewneighbour().remoteipaddress();
        if ( ! address.empty() )
            { _detectedIpCallback( NetworkInterface::AddressFromBytes(address) ); }
    }
    return result;
}



vector<NodeInfo> NodeMethodsProtoBufClient::GetRandomNodes(
    size_t maxNodeCount, Neighbours filter) const
{
    iop::locnet::Request request;
    iop::locnet::GetRandomNodesRequest *getRandReq = request.mutable_remotenode()->mutable_getrandomnodes();
    getRandReq->set_maxnodecount(maxNodeCount);
    getRandReq->set_includeneighbours( filter == Neighbours::Included );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_getrandomnodes() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Failed to get expected response"); }
    
    const iop::locnet::GetRandomNodesResponse &getRandResp = response->remotenode().getrandomnodes();
    vector<NodeInfo> result;
    for (int32_t idx = 0; idx < getRandResp.nodes_size(); ++idx)
        { result.push_back( Converter::FromProtoBuf( getRandResp.nodes(idx) ) ); }
    LOG(DEBUG) << "Request GetRandomNodes() returned " << result.size() << " nodes";
    return result;
}



vector<NodeInfo> NodeMethodsProtoBufClient::GetClosestNodesByDistance(
    const GpsLocation& location, Distance radiusKm, size_t maxNodeCount, Neighbours filter) const
{
    iop::locnet::Request request;
    iop::locnet::GetClosestNodesByDistanceRequest *getNodeReq =
        request.mutable_remotenode()->mutable_getclosestnodes();
    getNodeReq->set_allocated_location( Converter::ToProtoBuf(location) );
    getNodeReq->set_maxradiuskm(radiusKm);
    getNodeReq->set_maxnodecount(maxNodeCount);
    getNodeReq->set_includeneighbours( filter == Neighbours::Included );
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_getclosestnodes() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Failed to get expected response"); }
    
    const iop::locnet::GetClosestNodesByDistanceResponse &getNodeResp = response->remotenode().getclosestnodes();
    vector<NodeInfo> result;
    for (int32_t idx = 0; idx < getNodeResp.nodes_size(); ++idx)
        { result.push_back( Converter::FromProtoBuf( getNodeResp.nodes(idx) ) ); }
    LOG(DEBUG) << "Request GetClosestNodesByDistance() returned " << result.size() << " nodes";
    return result;
}



} // namespace LocNet