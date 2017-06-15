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
        case ErrorCode::ERROR_INVALID_VALUE:        return iop::locnet::Status::ERROR_INVALID_VALUE;
        case ErrorCode::ERROR_BAD_STATE:            return iop::locnet::Status::ERROR_INTERNAL;
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
        default: throw LocationNetworkError(ErrorCode::ERROR_INVALID_VALUE, "Missing or unknown service type");
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



ServiceInfo Converter::FromProtoBuf(const iop::locnet::ServiceInfo& value)
{
    return ServiceInfo( FromProtoBuf( value.type() ), value.port(), value.servicedata() );
}


NodeInfo Converter::FromProtoBuf(const iop::locnet::NodeInfo& value)
{
    if ( value.nodeid().empty() )
        { throw LocationNetworkError(ErrorCode::ERROR_INVALID_VALUE, "No id present for node"); }
    if ( ! value.has_contact() )
        { throw LocationNetworkError(ErrorCode::ERROR_INVALID_VALUE, "No contact information for node"); }

    const iop::locnet::NodeContact &contact = value.contact();
        
    NodeInfo::Services services;
    for (int idx = 0; idx < value.services_size(); ++idx)
    {
        const iop::locnet::ServiceInfo &sourceService = value.services(idx);
        ServiceInfo service = FromProtoBuf(sourceService);
        services[ service.type() ] = service;
    }
    
    return NodeInfo( value.nodeid(), FromProtoBuf( value.location() ), NodeContact(
        NodeContact::AddressFromBytes( contact.ipaddress() ), contact.nodeport(), contact.clientport() ),
        services );
}



void Converter::FillProtoBuf(iop::locnet::ServiceInfo *target, const ServiceInfo &source)
{
    target->set_type( ToProtoBuf( source.type() ) );
    target->set_port( source.port() );
    if ( ! source.customData().empty() )
        { target->set_servicedata( source.customData() ); }
}

iop::locnet::ServiceInfo* Converter::ToProtoBuf(const ServiceInfo &info)
{
    auto result = new iop::locnet::ServiceInfo();
    FillProtoBuf(result, info);
    return result;
}



void Converter::FillProtoBuf(
    iop::locnet::NodeInfo *target, const NodeInfo &source)
{
    target->set_nodeid( source.id() );
    target->set_allocated_location( ToProtoBuf( source.location() ) );
    
    const NodeContact &sourceContact = source.contact();
    iop::locnet::NodeContact *targetContact = target->mutable_contact();
    
    targetContact->set_nodeport( sourceContact.nodePort() );
    targetContact->set_clientport( sourceContact.clientPort() );
    targetContact->set_ipaddress( sourceContact.AddressBytes() );
    
    for ( const auto &serviceEntry : source.services() )
    {
        iop::locnet::ServiceInfo *targetService = target->add_services();
        FillProtoBuf(targetService, serviceEntry.second);
    }
}

iop::locnet::NodeInfo* Converter::ToProtoBuf(const NodeInfo &info)
{
    auto result = new iop::locnet::NodeInfo();
    FillProtoBuf(result, info);
    return result;
}



IncomingLocalServiceRequestDispatcher::IncomingLocalServiceRequestDispatcher(
        shared_ptr<ILocalServiceMethods> iLocalService, shared_ptr<IChangeListenerFactory> listenerFactory) :
    _iLocalService(iLocalService), _listenerFactory(listenerFactory)
{
    if (_iLocalService == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No local service logic instantiated");
    }
    if (_listenerFactory == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No listener factory instantiated");
    }
}



// TODO Dispatchers simply translate between different data formats, ideally this should be generated.
unique_ptr<iop::locnet::Response> IncomingLocalServiceRequestDispatcher::Dispatch(
    const iop::locnet::Request& request)
{
    // TODO this version check is shared between different interfaces, use a shared implementation here
    if ( request.version().empty() || request.version()[0] != 1 )
        { throw LocationNetworkError(ErrorCode::ERROR_UNSUPPORTED, "Missing or unknown request version"); }
    
    if ( ! request.has_localservice() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "This interface serves only local service requests"); }
    
    const iop::locnet::LocalServiceRequest &localServiceRequest = request.localservice();
    unique_ptr<iop::locnet::Response> result( new iop::locnet::Response() );
    iop::locnet::LocalServiceResponse *localServiceResponse = result->mutable_localservice();
    
    switch ( localServiceRequest.LocalServiceRequestType_case() )
    {
        case iop::locnet::LocalServiceRequest::kRegisterService:
        {
            auto const &registerRequest = localServiceRequest.registerservice();
            ServiceInfo service = Converter::FromProtoBuf( registerRequest.service() );
            GpsLocation location = _iLocalService->RegisterService(service);
            LOG(DEBUG) << "Served RegisterService()";
            
            localServiceResponse->mutable_registerservice()->set_allocated_location(
                Converter::ToProtoBuf(location) );
            break;
        }
            
        case iop::locnet::LocalServiceRequest::kDeregisterService:
        {
            auto const &deregisterRequest = localServiceRequest.deregisterservice();
            ServiceType serviceType = Converter::FromProtoBuf( deregisterRequest.servicetype() );
            
            _iLocalService->DeregisterService(serviceType);
            LOG(DEBUG) << "Served DeregisterService()";
            
            localServiceResponse->mutable_deregisterservice();
            break;
        }
            
        case iop::locnet::LocalServiceRequest::kGetNeighbourNodes:
        {
            auto const &getneighboursRequest = localServiceRequest.getneighbournodes();
            bool keepAlive = getneighboursRequest.keepaliveandsendupdates();
            
            vector<NodeInfo> neighbours = _iLocalService->GetNeighbourNodesByDistance();
            LOG(DEBUG) << "Served GetNeighbourNodes() with keepalive " << keepAlive
                       << ", node count : " << neighbours.size();
            
            auto responseContent = localServiceResponse->mutable_getneighbournodes();
            for (auto const &neighbour : neighbours)
            {
                iop::locnet::NodeInfo *info = responseContent->add_nodes();
                Converter::FillProtoBuf(info, neighbour);
            }
            
            if (keepAlive)
            {
                shared_ptr<IChangeListener> listener = _listenerFactory->Create(_iLocalService);
                _iLocalService->AddListener(listener);
            }
            break;
        }
        
        case iop::locnet::LocalServiceRequest::kNeighbourhoodChanged:
            throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST,
                "Invalid request type. This is a notification message: it's not supposed to be sent "
                "as a request to this server but to be received as notification "
                "after a GetNeighbourNodesRequest with flag keepAlive set.");
        
        default: throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing or unknown local service operation");
    }
    
    return result;
}



IncomingNodeRequestDispatcher::IncomingNodeRequestDispatcher(shared_ptr<INodeMethods> iNode) :
    _iNode(iNode)
{
    if (_iNode == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No remote node logic instantiated");
    }
}



unique_ptr<iop::locnet::Response> IncomingNodeRequestDispatcher::Dispatch(const iop::locnet::Request& request)
{
    if ( request.version().empty() || request.version()[0] != 1 )
        { throw LocationNetworkError(ErrorCode::ERROR_UNSUPPORTED, "Missing or unknown request version"); }
    
    if ( ! request.has_remotenode() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "This interface serves only remote node requests"); }
    
    const iop::locnet::RemoteNodeRequest &nodeRequest = request.remotenode();
    unique_ptr<iop::locnet::Response> result( new iop::locnet::Response() );
    iop::locnet::RemoteNodeResponse *nodeResponse = result->mutable_remotenode();
    
    switch ( nodeRequest.RemoteNodeRequestType_case() )
    {
        case iop::locnet::RemoteNodeRequest::kGetNodeInfo:
        {
            NodeInfo node = _iNode->GetNodeInfo();
            LOG(DEBUG) << "Served GetNodeInfo(): " << node;
            
            auto responseContent = nodeResponse->mutable_getnodeinfo();
            responseContent->set_allocated_nodeinfo( Converter::ToProtoBuf(node) );
            break;
        }
        
        case iop::locnet::RemoteNodeRequest::kAcceptColleague:
        {
            auto acceptColleagueReq = nodeRequest.acceptcolleague();
            auto nodeInfo = Converter::FromProtoBuf( acceptColleagueReq.requestornodeinfo() );
            
            shared_ptr<NodeInfo> result = _iNode->AcceptColleague(nodeInfo);
            LOG(DEBUG) << "Served AcceptColleague(" << nodeInfo
                       << "), accepted: " << static_cast<bool>(result);
            
            nodeResponse->mutable_acceptcolleague()->set_accepted( static_cast<bool>(result) );
            if (result) {
                nodeResponse->mutable_acceptcolleague()->set_allocated_acceptornodeinfo(
                    Converter::ToProtoBuf(*result) );
            }
            break;
        }
        
        case iop::locnet::RemoteNodeRequest::kRenewColleague:
        {
            auto renewColleagueReq = nodeRequest.renewcolleague();
            auto nodeInfo = Converter::FromProtoBuf( renewColleagueReq.requestornodeinfo() );
            
            shared_ptr<NodeInfo> result = _iNode->RenewColleague(nodeInfo);
            LOG(DEBUG) << "Served RenewColleague(" << nodeInfo
                       << "), accepted: " << static_cast<bool>(result);
                       
            nodeResponse->mutable_renewcolleague()->set_accepted( static_cast<bool>(result) );
            if (result) {
                nodeResponse->mutable_renewcolleague()->set_allocated_acceptornodeinfo(
                    Converter::ToProtoBuf(*result) );
            }
            break;
        }
        
        case iop::locnet::RemoteNodeRequest::kAcceptNeighbour:
        {
            auto acceptNeighbourReq = nodeRequest.acceptneighbour();
            auto nodeInfo = Converter::FromProtoBuf( acceptNeighbourReq.requestornodeinfo() );
            
            shared_ptr<NodeInfo> result = _iNode->AcceptNeighbour(nodeInfo);
            LOG(DEBUG) << "Served AcceptNeighbour(" << nodeInfo
                       << "), accepted: " << static_cast<bool>(result);
                       
            nodeResponse->mutable_acceptneighbour()->set_accepted( static_cast<bool>(result) );
            if (result) {
                nodeResponse->mutable_acceptneighbour()->set_allocated_acceptornodeinfo(
                    Converter::ToProtoBuf(*result) );
            }
            break;
        }
        
        case iop::locnet::RemoteNodeRequest::kRenewNeighbour:
        {
            auto renewNeighbourReq = nodeRequest.renewneighbour();
            auto nodeInfo = Converter::FromProtoBuf( renewNeighbourReq.requestornodeinfo() );
            
            shared_ptr<NodeInfo> result = _iNode->RenewNeighbour(nodeInfo);
            LOG(DEBUG) << "Served RenewNeighbour(" << nodeInfo
                       << "), accepted: " << static_cast<bool>(result);
                       
            nodeResponse->mutable_renewneighbour()->set_accepted( static_cast<bool>(result) );
            if (result) {
                nodeResponse->mutable_renewneighbour()->set_allocated_acceptornodeinfo(
                    Converter::ToProtoBuf(*result) );
            }
            break;
        }

        case iop::locnet::RemoteNodeRequest::kGetNodeCount:
        {
            size_t counter = _iNode->GetNodeCount();
            LOG(DEBUG) << "Served GetNodeCount(), node count: " << counter;
            
            nodeResponse->mutable_getnodecount()->set_nodecount(counter);
            break;
        }
        
        case iop::locnet::RemoteNodeRequest::kGetRandomNodes:
        {
            auto randomNodesReq = nodeRequest.getrandomnodes();
            Neighbours neighbourFilter = randomNodesReq.includeneighbours() ?
                Neighbours::Included : Neighbours::Excluded;
                
            vector<NodeInfo> randomNodes = _iNode->GetRandomNodes(
                randomNodesReq.maxnodecount(), neighbourFilter );
            LOG(DEBUG) << "Served GetRandomNodes(), node count: " << randomNodes.size();
            
            auto responseContent = nodeResponse->mutable_getrandomnodes();
            for (auto const &node : randomNodes)
            {
                iop::locnet::NodeInfo *info = responseContent->add_nodes();
                Converter::FillProtoBuf(info, node);
            }
            break;
        }
        
        case iop::locnet::RemoteNodeRequest::kGetClosestNodes:
        {
            auto const &closestRequest = nodeRequest.getclosestnodes();
            GpsLocation location = Converter::FromProtoBuf( closestRequest.location() );
            Neighbours neighbourFilter = closestRequest.includeneighbours() ?
                Neighbours::Included : Neighbours::Excluded;
            
            vector<NodeInfo> closeNodes( _iNode->GetClosestNodesByDistance( location,
                closestRequest.maxradiuskm(), closestRequest.maxnodecount(), neighbourFilter) );
            LOG(DEBUG) << "Served GetClosestNodes(), node count: " << closeNodes.size();
            
            auto responseContent = nodeResponse->mutable_getclosestnodes();
            for (auto const &node : closeNodes)
            {
                iop::locnet::NodeInfo *info = responseContent->add_nodes();
                Converter::FillProtoBuf(info, node);
            }
            break;
        }
        
        default: throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing or unknown remote node operation");
    }
    
    return result;
}



IncomingClientRequestDispatcher::IncomingClientRequestDispatcher(shared_ptr<IClientMethods> iClient) :
    _iClient(iClient)
{
    if (_iClient == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No client logic instantiated");
    }
}



unique_ptr<iop::locnet::Response> IncomingClientRequestDispatcher::Dispatch(const iop::locnet::Request& request)
{
    if ( request.version().empty() || request.version()[0] != 1 )
        { throw LocationNetworkError(ErrorCode::ERROR_UNSUPPORTED, "Missing or unknown request version"); }
    
    if ( ! request.has_client() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "This interface serves only client requests"); }
    
    const iop::locnet::ClientRequest &clientRequest = request.client();
    unique_ptr<iop::locnet::Response> result( new iop::locnet::Response() );
    iop::locnet::ClientResponse *clientResponse = result->mutable_client();
    
    switch ( clientRequest.ClientRequestType_case() )
    {
        case iop::locnet::ClientRequest::kGetNodeInfo:
        {
            NodeInfo node = _iClient->GetNodeInfo();
            LOG(DEBUG) << "Served GetNodeInfo(): " << node;
            
            auto responseContent = clientResponse->mutable_getnodeinfo();
            responseContent->set_allocated_nodeinfo( Converter::ToProtoBuf(node) );
            break;
        }
        
        case iop::locnet::ClientRequest::kGetNeighbourNodes:
        {
            vector<NodeInfo> neighbours = _iClient->GetNeighbourNodesByDistance();
            LOG(DEBUG) << "Served GetNeighbourNodes(), node count: " << neighbours.size();
            
            auto responseContent = clientResponse->mutable_getneighbournodes();
            for (auto const &neighbour : neighbours)
            {
                iop::locnet::NodeInfo *info = responseContent->add_nodes();
                Converter::FillProtoBuf(info, neighbour);
            }
            break;
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
            
            auto responseContent = clientResponse->mutable_getclosestnodes();
            for (auto const &node : closeNodes)
            {
                iop::locnet::NodeInfo *info = responseContent->add_nodes();
                Converter::FillProtoBuf(info, node);
            }
            break;
        }
        
        case iop::locnet::ClientRequest::kExploreNodes:
        {
            auto const &exploreRequest = clientRequest.explorenodes();
            GpsLocation location = Converter::FromProtoBuf( exploreRequest.location() );
            
            vector<NodeInfo> exploredNodes( _iClient->ExploreNetworkNodesByDistance( location,
                exploreRequest.targetnodecount(), exploreRequest.maxnodehops() ) );
            LOG(DEBUG) << "Served GetClosestNodes(), node count: " << exploredNodes.size();
            
            auto responseContent = clientResponse->mutable_explorenodes();
            for (auto const &node : exploredNodes)
            {
                iop::locnet::NodeInfo *info = responseContent->add_closestnodes();
                Converter::FillProtoBuf(info, node);
            }
            break;
        }
        
        default: throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing or unknown client operation");
    }
    
    return result;
}



IncomingRequestDispatcher::IncomingRequestDispatcher(
        shared_ptr<LocNet::Node> node, shared_ptr<IChangeListenerFactory> listenerFactory ) :
    _iLocalService( new IncomingLocalServiceRequestDispatcher(node, listenerFactory) ),
    _iRemoteNode( new IncomingNodeRequestDispatcher(node) ),
    _iClient( new IncomingClientRequestDispatcher(node) )
{
    if (node == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No node instantiated");
    }
}

IncomingRequestDispatcher::IncomingRequestDispatcher(
        shared_ptr<IncomingLocalServiceRequestDispatcher> iLocalServices,
        shared_ptr<IncomingNodeRequestDispatcher> iRemoteNode,
        shared_ptr<IncomingClientRequestDispatcher> iClient ) :
    _iLocalService(iLocalServices), _iRemoteNode(iRemoteNode), _iClient(iClient)
{
    if (_iLocalService == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No local request dispatcher instantiated");
    }
    if (_iRemoteNode == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No node request dispatcher instantiated");
    }
    if (_iClient == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No client request dispatcher instantiated");
    }
}
    


unique_ptr<iop::locnet::Response> IncomingRequestDispatcher::Dispatch(const iop::locnet::Request& request)
{
    switch ( request.RequestType_case() )
    {
        case iop::locnet::Request::kLocalService:
            return _iLocalService->Dispatch(request);
            
        case iop::locnet::Request::kRemoteNode:
            return _iRemoteNode->Dispatch(request);
            
        case iop::locnet::Request::kClient:
            return _iClient->Dispatch(request);
            
        default: throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing or unknown request type");
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
NodeInfo NodeMethodsProtoBufClient::GetNodeInfo() const
{
    iop::locnet::Request request;
    request.mutable_remotenode()->mutable_getnodeinfo();
    
    unique_ptr<iop::locnet::Response> response = _dispatcher->Dispatch(request);
    if (! response || ! response->has_remotenode() || ! response->remotenode().has_getnodeinfo() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_RESPONSE, "Failed to get expected response"); }
    
    auto result = Converter::FromProtoBuf( response->remotenode().getnodeinfo().nodeinfo() );
    LOG(DEBUG) << "Request GetNodeInfo() returned " << result;
    return result;
}



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
            { _detectedIpCallback( NodeContact::AddressFromBytes(address) ); }
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
            { _detectedIpCallback( NodeContact::AddressFromBytes(address) ); }
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
            { _detectedIpCallback( NodeContact::AddressFromBytes(address) ); }
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
            { _detectedIpCallback( NodeContact::AddressFromBytes(address) ); }
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
