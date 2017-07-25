#include <chrono>
#include <easylogging++.h>

#include "config.hpp"
#include "server.hpp"

using namespace std;
using namespace asio::ip;



namespace LocNet
{



static const size_t MaxMessageSize = 1024 * 1024;
static const size_t MessageHeaderSize = 5;
static const size_t MessageSizeOffset = 1;


// static chrono::duration<uint32_t> GetNetworkExpirationPeriod()
//     { return Config::Instance().isTestMode() ? chrono::seconds(1) : chrono::seconds(10); }
static chrono::duration<uint32_t> GetRequestExpirationPeriod()
    { return Config::Instance().isTestMode() ? chrono::seconds(60) : chrono::seconds(10); }

//static const chrono::duration<uint32_t> KeepAliveStreamExpirationPeriod = chrono::hours(168);



shared_ptr<DispatchingTcpServer> DispatchingTcpServer::Create(
        TcpPort portNumber, shared_ptr<IBlockingRequestDispatcherFactory> dispatcherFactory )
    { return shared_ptr<DispatchingTcpServer>( new DispatchingTcpServer(portNumber, dispatcherFactory) ); }


DispatchingTcpServer::DispatchingTcpServer( TcpPort portNumber,
        shared_ptr<IBlockingRequestDispatcherFactory> dispatcherFactory ) :
    TcpServer(portNumber), _dispatcherFactory(dispatcherFactory)
{
    if (_dispatcherFactory == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No dispatcher factory instantiated");
    }
}


void DispatchingTcpServer::StartListening()
{
    // Switch the acceptor to listening state
    LOG(DEBUG) << "Accepting connections on port " << _acceptor.local_endpoint().port();
    _acceptor.listen();
    
    shared_ptr<tcp::socket> socket( new tcp::socket( Reactor::Instance().AsioService() ) );
    weak_ptr<TcpServer> self = shared_from_this();
    _acceptor.async_accept( *socket,
        [self, socket] (const asio::error_code &ec)
    {
        shared_ptr<TcpServer> server = self.lock();
        if (server) { server->AsyncAcceptHandler(socket, ec); }
    } );
}


void DispatchingTcpServer::AsyncAcceptHandler(
    std::shared_ptr<asio::ip::tcp::socket> socket, const asio::error_code &ec)
{
    if (ec)
    {
        LOG(ERROR) << "Failed to accept connection: " << ec;
        return;
    }
    LOG(DEBUG) << "Connection accepted from "
        << socket->remote_endpoint().address().to_string() << ":" << socket->remote_endpoint().port() << " to "
        << socket->local_endpoint().address().to_string()  << ":" << socket->local_endpoint().port();
    
    // Keep accepting connections on the socket
    shared_ptr<tcp::socket> nextSocket( new tcp::socket( Reactor::Instance().AsioService() ) );
    weak_ptr<DispatchingTcpServer> self = static_pointer_cast<DispatchingTcpServer>( shared_from_this() );
    _acceptor.async_accept( *nextSocket,
        [self, nextSocket] (const asio::error_code &ec)
    {
        shared_ptr<DispatchingTcpServer> server = self.lock();
        if (server) { server->AsyncAcceptHandler(nextSocket, ec); }
    } );
    
    shared_ptr<IProtoBufChannel> connection( new AsyncProtoBufTcpChannel(socket) );
    shared_ptr<ProtoBufClientSession> session( ProtoBufClientSession::Create(connection) );
    shared_ptr<IBlockingRequestDispatcher> dispatcher( _dispatcherFactory->Create(session) );

    LOG(INFO) << "Starting server message loop for connection " << connection->id();
    
    connection->ReceiveMessage( [session, dispatcher] ( unique_ptr<iop::locnet::Message> &&incomingMessage )
        { AsyncServeMessageHandler( move(incomingMessage), session, dispatcher); } );
}


void DispatchingTcpServer::AsyncServeMessageHandler( unique_ptr<iop::locnet::Message> &&receivedMessage,
    shared_ptr<ProtoBufClientSession> session, shared_ptr<IBlockingRequestDispatcher> dispatcher )
{
    bool handlerSuccessful = false;
    bool sendResponse = true;
    
    uint32_t messageId = 0;
    unique_ptr<iop::locnet::Response> response;
    try
    {
        if (! receivedMessage)
            { throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Failed to read message"); }
            
        // If incoming response, connect it with the sent out request and skip further processing
        if ( receivedMessage->has_response() )
        {
            LOG(TRACE) << "Received response message, delivering it to requestor";
            session->ResponseArrived( move(receivedMessage) );
            sendResponse = false;
        }
        else
        {
            if ( ! receivedMessage->has_request() )
                { throw LocationNetworkError(ErrorCode::ERROR_BAD_REQUEST, "Missing request"); }
            
            LOG(TRACE) << "Serving request";
            
            messageId = receivedMessage->id();
            unique_ptr<iop::locnet::Request> request( receivedMessage->release_request() );
            
            // TODO the ip detection and keepalive features are violating the current abstraction layers.
            //      This is not a nice implementation, abstractions should be better prepared for these features
            if ( request->has_remotenode() )
            {
                if ( request->remotenode().has_acceptcolleague() ) {
                    request->mutable_remotenode()->mutable_acceptcolleague()->mutable_requestornodeinfo()->mutable_contact()->set_ipaddress(
                        NodeContact::AddressToBytes( session->messageChannel()->remoteAddress() ) );
                }
                else if ( request->remotenode().has_renewcolleague() ) {
                    request->mutable_remotenode()->mutable_renewcolleague()->mutable_requestornodeinfo()->mutable_contact()->set_ipaddress(
                        NodeContact::AddressToBytes( session->messageChannel()->remoteAddress() ) );
                }
                else if ( request->remotenode().has_acceptneighbour() ) {
                    request->mutable_remotenode()->mutable_acceptneighbour()->mutable_requestornodeinfo()->mutable_contact()->set_ipaddress(
                        NodeContact::AddressToBytes( session->messageChannel()->remoteAddress() ) );
                }
                else if ( request->remotenode().has_renewneighbour() ) {
                    request->mutable_remotenode()->mutable_renewneighbour()->mutable_requestornodeinfo()->mutable_contact()->set_ipaddress(
                        NodeContact::AddressToBytes( session->messageChannel()->remoteAddress() ) );
                }
            }
                
            response = dispatcher->Dispatch( move(request) );
            response->set_status(iop::locnet::Status::STATUS_OK);
            
            if ( response->has_remotenode() )
            {
                if ( response->remotenode().has_acceptcolleague() ) {
                    response->mutable_remotenode()->mutable_acceptcolleague()->set_remoteipaddress(
                        NodeContact::AddressToBytes( session->messageChannel()->remoteAddress() ) );
                }
                else if ( response->remotenode().has_renewcolleague() ) {
                    response->mutable_remotenode()->mutable_renewcolleague()->set_remoteipaddress(
                        NodeContact::AddressToBytes( session->messageChannel()->remoteAddress() ) );
                }
                else if ( response->remotenode().has_acceptneighbour() ) {
                    response->mutable_remotenode()->mutable_acceptneighbour()->set_remoteipaddress(
                        NodeContact::AddressToBytes( session->messageChannel()->remoteAddress() ) );
                }
                else if ( response->remotenode().has_renewneighbour() ) {
                    response->mutable_remotenode()->mutable_renewneighbour()->set_remoteipaddress(
                        NodeContact::AddressToBytes( session->messageChannel()->remoteAddress() ) );
                }
            }
        }
        
        handlerSuccessful = true;
    }
    catch (LocationNetworkError &lnex)
    {
        // TODO This warning is also given when the connection was simply closed by the remote peer
        //      thus no request can be read. This case should be distinguished and logged with a lower level.
        LOG(WARNING) << "Failed to serve request with code "
            << static_cast<uint32_t>( lnex.code() ) << ": " << lnex.what();
        response.reset( new iop::locnet::Response() );
        response->set_status( Converter::ToProtoBuf( lnex.code() ) );
        response->set_details( lnex.what() );
    }
    catch (exception &ex)
    {
        LOG(WARNING) << "Failed to serve request: " << ex.what();
        response.reset( new iop::locnet::Response() );
        response->set_status(iop::locnet::Status::ERROR_INTERNAL);
        response->set_details( ex.what() );
    }
    
    if (sendResponse)
    {
        LOG(TRACE) << "Sending response";
        unique_ptr<iop::locnet::Message> responseMsg( new iop::locnet::Message() );
        responseMsg->set_allocated_response( response.release() );
        responseMsg->set_id(messageId);
        
        session->messageChannel()->SendMessage( move(responseMsg), [] {} );
    }
    
    if (handlerSuccessful)
    {
        // Schedule next message loop iteration
        session->messageChannel()->ReceiveMessage( [session, dispatcher]
            ( unique_ptr<iop::locnet::Message> &&incomingMessage )
            { AsyncServeMessageHandler( move(incomingMessage), session, dispatcher); } );
    }
    else { LOG(INFO) << "Server message loop ended for session " << session->id(); }
}



AsyncProtoBufTcpChannel::AsyncProtoBufTcpChannel(shared_ptr<tcp::socket> socket) :
    _socket(socket), _id(), _remoteAddress(), _nextRequestId(1) // , _socketWriteMutex(), _socketReadMutex()
{
    if (! _socket)
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No socket instantiated"); }
    
    _remoteAddress = socket->remote_endpoint().address().to_string();
    _id = _remoteAddress + ":" + to_string( socket->remote_endpoint().port() );
    // TODO handle session expiration for clients with no keepalive
    //_stream.expires_after(NormalStreamExpirationPeriod);
}


AsyncProtoBufTcpChannel::AsyncProtoBufTcpChannel(const NetworkEndpoint &endpoint) :
    _socket( new tcp::socket( Reactor::Instance().AsioService() ) ),
    _id( endpoint.address() + ":" + to_string( endpoint.port() ) ),
    _remoteAddress( endpoint.address() ), _nextRequestId(1) // , _socketWriteMutex(), _socketReadMutex()
{
    tcp::resolver resolver( Reactor::Instance().AsioService() );
    tcp::resolver::query query( endpoint.address(), to_string( endpoint.port() ) );
    tcp::resolver::iterator addressIter = resolver.resolve(query);
    // TODO transform this to async to make every I/O call async also for client connections
    try { asio::connect(*_socket, addressIter); }
    catch (exception &ex) { throw LocationNetworkError(ErrorCode::ERROR_CONNECTION, "Failed connecting to " +
        endpoint.address() + ":" + to_string( endpoint.port() ) + " with error: " + ex.what() ); }
    LOG(DEBUG) << "Connected to " << endpoint;
    // TODO handle session expiration
    //_stream.expires_after( GetNormalStreamExpirationPeriod() );
}

AsyncProtoBufTcpChannel::~AsyncProtoBufTcpChannel()
{
    _socket->close();
    LOG(DEBUG) << "Connection closed to " << id();
}


const SessionId& AsyncProtoBufTcpChannel::id() const
    { return _id; }

const Address& AsyncProtoBufTcpChannel::remoteAddress() const
    { return _remoteAddress; }


uint32_t GetMessageSizeFromHeader(const char *bytes)
{
    // Adapt big endian value from network to local format
    const uint8_t *data = reinterpret_cast<const uint8_t*>(bytes);
    return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
}


void AsyncProtoBufTcpChannel::ReceiveMessage( function<ReceivedMessageCallback> callback )
{
    //lock_guard<mutex> readGuard(_socketReadMutex);
    //LOG(TRACE) << "Receive message called for connection " << id();
    
    if ( ! _socket->is_open() )
    {
        LOG(DEBUG) << "Connection to " << id() << " is already closed, cannot read message";
        callback( unique_ptr<iop::locnet::Message>() );
    }

    // Allocate a buffer for the message header and read it
    unique_ptr<string> buffer( new string(MessageHeaderSize, 0) );
    shared_ptr<AsyncConnection> bufferIO = AsyncConnection::Create( _socket, move(buffer) );
    
    string connectionId = id();
    shared_ptr<tcp::socket> socket = _socket;
    bufferIO->ReadBuffer( [socket, callback, connectionId] ( unique_ptr<string> &&transferredBuffer )
    {
        unique_ptr<string> buffer( move(transferredBuffer) );
        if ( ! buffer || buffer->size() < MessageHeaderSize )
        {
            LOG(DEBUG) << "Failed to read message header";
            callback( unique_ptr<iop::locnet::Message>() );
            return;
        }

        // Extract message size from the header to know how many bytes to read
        uint32_t bodySize = GetMessageSizeFromHeader( &buffer->operator[](MessageSizeOffset) );
        if (bodySize > MaxMessageSize)
        {
            LOG(DEBUG) << "Message size is over limit: " << bodySize;
            callback( unique_ptr<iop::locnet::Message>() );
            return;
        }
        
        // Extend buffer to fit remaining message size and read it
        buffer->resize(MessageHeaderSize + bodySize, 0);
        
        shared_ptr<AsyncConnection> bufferIO = AsyncConnection::Create( socket, move(buffer), MessageHeaderSize );
        bufferIO->ReadBuffer( [callback, connectionId] ( unique_ptr<string> &&buffer )
        {
            if (! buffer)
            {
                LOG(DEBUG) << "Received empty buffer due to an error, ignore it";
                return;
            }
            
            // Deserialize message from receive buffer, avoid leaks for failing cases with RAII-based unique_ptr
            unique_ptr<iop::locnet::MessageWithHeader> message( new iop::locnet::MessageWithHeader() );
            message->ParseFromString(*buffer);
            
            string msgDebugStr;
            google::protobuf::TextFormat::PrintToString(*message, &msgDebugStr);
            LOG(TRACE) << "Connection " << connectionId << " received message " << msgDebugStr;
            
            callback( unique_ptr<iop::locnet::Message>( message->release_body() ) );
        } );
    } );
}


future< unique_ptr<iop::locnet::Message> > AsyncProtoBufTcpChannel::ReceiveMessage(asio::use_future_t<>)
{
    //lock_guard<mutex> readGuard(_socketReadMutex);
    //LOG(TRACE) << "Receive message called for connection " << id();
    
    shared_ptr< promise< unique_ptr<iop::locnet::Message> > > result(
        new promise< unique_ptr<iop::locnet::Message> >() );

    ReceiveMessage( [result] ( unique_ptr<iop::locnet::Message> &&receivedMessage )
        { result->set_value( move(receivedMessage) ); } );
    
    return result->get_future();
}



void AsyncProtoBufTcpChannel::SendMessage( unique_ptr<iop::locnet::Message> &&messagePtr,
                                           function<SentMessageCallback> callback )
{
    if ( ! _socket->is_open() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_STATE,
            "Session " + id() + " socket is already closed, cannot write message"); }
    
    if (! messagePtr)
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Got empty message argument to send"); }
    if ( messagePtr->has_request() )
    {
        messagePtr->set_id(_nextRequestId);
        ++_nextRequestId;
    }
    
    iop::locnet::MessageWithHeader message;
    message.set_allocated_body( messagePtr.release() );
    
    message.set_header(1);
    message.set_header( message.ByteSize() - MessageHeaderSize );

    string msgDebugStr;
    google::protobuf::TextFormat::PrintToString(message, &msgDebugStr);
    LOG(TRACE) << "Connection " << id() << " sending message " << msgDebugStr;

    unique_ptr<string> serializedMessage( new string( message.SerializeAsString() ) );
    shared_ptr<AsyncConnection> bufferIO = AsyncConnection::Create( _socket, move(serializedMessage) );
    bufferIO->WriteBuffer( [callback] ( unique_ptr<string> && ) { callback(); } );
}



future<void> AsyncProtoBufTcpChannel::SendMessage(unique_ptr<iop::locnet::Message> &&messagePtr, asio::use_future_t<>)
{
    shared_ptr< promise<void> > result( new promise<void>() );
    SendMessage( move(messagePtr), [result] { result->set_value(); } );
    return result->get_future();
}



shared_ptr<ProtoBufClientSession> ProtoBufClientSession::Create(
        std::shared_ptr<IProtoBufChannel> connection)
    { return shared_ptr<ProtoBufClientSession>( new ProtoBufClientSession(connection) ); }

ProtoBufClientSession::ProtoBufClientSession(shared_ptr<IProtoBufChannel> connection) :
    _messageChannel(connection), _nextMessageId(1)
{
    if (_messageChannel == nullptr)
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No connection instantiated"); }
}


void ProtoBufClientSession::AsyncMessageLoopHandler(
    weak_ptr<ProtoBufClientSession> sessionWeakRef, const string &sessionId,
    std::function<IncomingRequestHandler> requestHandler )
{
    shared_ptr<ProtoBufClientSession> sessionPtr = sessionWeakRef.lock();
    if (! sessionPtr) // Session has been closed and destroyed, stop
    {
        LOG(DEBUG) << "Session " << sessionId << " was closed, stopping message loop";
        return;
    }
    
    sessionPtr->_messageChannel->ReceiveMessage(
        [sessionWeakRef, sessionId, requestHandler] ( unique_ptr<iop::locnet::Message> &&incomingMsg )
    {
        try
        {
            if (! incomingMsg)
                { throw runtime_error("No message received"); }
                
            if ( incomingMsg->has_request() )
            {
                if (requestHandler)
                    { requestHandler( move(incomingMsg) ); }
            }
            else
            {
                if ( ! incomingMsg->has_response() )
                    { throw runtime_error("Response message is expected"); }
                
                shared_ptr<ProtoBufClientSession> sessionPtr = sessionWeakRef.lock();
                if (! sessionPtr)
                    { throw runtime_error("Session is closed for " + sessionId); }
                
                LOG(TRACE) << "Dispatching incoming response to request sender";
                sessionPtr->ResponseArrived( move(incomingMsg) );
            }
            
            Reactor::Instance().AsioService().post( [sessionWeakRef, sessionId, requestHandler]
                { AsyncMessageLoopHandler(sessionWeakRef, sessionId, requestHandler); } );
        }
        catch (exception &ex)
            { LOG(WARNING) << "Failed to dispatch response, stopping message loop: " << ex.what(); }
    } );
}


void ProtoBufClientSession::StartMessageLoop( std::function<IncomingRequestHandler> requestHandler )
{
    shared_ptr<ProtoBufClientSession> session = shared_from_this();
    string sessionId = session->id();
    weak_ptr<ProtoBufClientSession> sessionWeakRef(session);
    Reactor::Instance().AsioService().post( [sessionWeakRef, sessionId, requestHandler]
        { AsyncMessageLoopHandler(sessionWeakRef, sessionId, requestHandler); } );
}


ProtoBufClientSession::~ProtoBufClientSession()
{
    // NOTE destruction of the promise<> values of the map will trigger sending a broken_promise exception to related future<> objects, no need to do this manually
}


const SessionId& ProtoBufClientSession::id() const
    { return _messageChannel->id(); }

shared_ptr<IProtoBufChannel> ProtoBufClientSession::messageChannel()
    { return _messageChannel; }

future< unique_ptr<iop::locnet::Response> > ProtoBufClientSession::SendRequest(
    unique_ptr<iop::locnet::Message> &&requestMessage)
{
    if (! requestMessage->has_request() )
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Attempt to send non-request message"); }

    unique_lock<mutex> pendingRequestGuard(_pendingRequestsMutex);
    uint32_t messageId = _nextMessageId++;
    auto emplaceResult = _pendingRequests.emplace(messageId, promise< unique_ptr<iop::locnet::Response> >());
    if (! emplaceResult.second)
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to store pending request"); }
    
    auto result = emplaceResult.first->second.get_future();
    pendingRequestGuard.unlock();
    
    requestMessage->set_id(messageId);
    _messageChannel->SendMessage( move(requestMessage), [] {} );
    
    return result;
}


void ProtoBufClientSession::ResponseArrived(unique_ptr<iop::locnet::Message> &&responseMessage)
{
    if (! responseMessage)
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Implementation error: received null response"); }
    if (! responseMessage->has_response() )
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Attempt to receive non-response message"); }
    
    LOG(TRACE) << "Looking up request for response message id " << responseMessage->id()
               << " between " << _pendingRequests.size() << " pending requests";

    lock_guard<mutex> pendingRequestGuard(_pendingRequestsMutex);
    auto requestIter = _pendingRequests.find( responseMessage->id() );
    if ( requestIter == _pendingRequests.end() )
        { throw LocationNetworkError( ErrorCode::ERROR_PROTOCOL_VIOLATION, "No request found for message id " + to_string( responseMessage->id() ) ); }
    
    LOG(TRACE) << "Found request for message id " << responseMessage->id() << ", notifying sender";
    
    requestIter->second.set_value( unique_ptr<iop::locnet::Response>(
        responseMessage->release_response() ) );
    _pendingRequests.erase(requestIter);
    
    LOG(TRACE) << "Response was dispatched, " << _pendingRequests.size() << " pending requests remain";
}




NetworkDispatcher::NetworkDispatcher(
    shared_ptr<ProtoBufClientSession> session) : _session(session) {}

    

unique_ptr<iop::locnet::Message> RequestToMessage(unique_ptr<iop::locnet::Request> &&request)
{
    request->set_version({1,0,0});
    unique_ptr<iop::locnet::Message> message( new iop::locnet::Message() );
    message->set_allocated_request( request.release() );
    return message;
}


unique_ptr<iop::locnet::Response> NetworkDispatcher::Dispatch(unique_ptr<iop::locnet::Request> &&request)
{
    unique_ptr<iop::locnet::Message> requestMessage( RequestToMessage( move(request) ) );
    future< unique_ptr<iop::locnet::Response> > futureResponse = _session->SendRequest( move(requestMessage) );
    if ( futureResponse.wait_for( GetRequestExpirationPeriod() ) != future_status::ready )
    {
        LOG(WARNING) << "Session " << _session->id() << " received no response, timed out";
        throw LocationNetworkError( ErrorCode::ERROR_BAD_RESPONSE, "Timeout waiting for response of dispatched request" );
    }
    unique_ptr<iop::locnet::Response> result( futureResponse.get() );
    if ( result && result->status() != iop::locnet::Status::STATUS_OK )
    {
        LOG(WARNING) << "Session " << _session->id() << " received response code " << result->status()
                     << ", error details: " << result->details();
        throw LocationNetworkError( ErrorCode::ERROR_BAD_RESPONSE, result->details() );
    }
    return result;
}



void TcpNodeConnectionFactory::detectedIpCallback(function<void(const Address&)> detectedIpCallback)
{
    _detectedIpCallback = detectedIpCallback;
    LOG(DEBUG) << "Callback for detecting external IP address is set " << static_cast<bool>(_detectedIpCallback); 
}


shared_ptr<INodeMethods> TcpNodeConnectionFactory::ConnectTo(const NetworkEndpoint& endpoint)
{
    LOG(DEBUG) << "Connecting to " << endpoint;
    shared_ptr<IProtoBufChannel> connection( new AsyncProtoBufTcpChannel(endpoint) );
    shared_ptr<ProtoBufClientSession> session( ProtoBufClientSession::Create(connection) );
    shared_ptr<IBlockingRequestDispatcher> dispatcher( new NetworkDispatcher(session) );
    shared_ptr<INodeMethods> result( new NodeMethodsProtoBufClient(dispatcher, _detectedIpCallback) );
    session->StartMessageLoop();
    return result;
}



LocalServiceRequestDispatcherFactory::LocalServiceRequestDispatcherFactory(
    shared_ptr<ILocalServiceMethods> iLocal) : _iLocal(iLocal) {}


shared_ptr<IBlockingRequestDispatcher> LocalServiceRequestDispatcherFactory::Create(
    shared_ptr<ProtoBufClientSession> session )
{
    shared_ptr<IChangeListenerFactory> listenerFactory(
        new TcpChangeListenerFactory(session) );
    return shared_ptr<IBlockingRequestDispatcher>(
        new IncomingLocalServiceRequestDispatcher(_iLocal, listenerFactory) );
}



StaticBlockingDispatcherFactory::StaticBlockingDispatcherFactory(shared_ptr<IBlockingRequestDispatcher> dispatcher) :
    _dispatcher(dispatcher) {}

shared_ptr<IBlockingRequestDispatcher> StaticBlockingDispatcherFactory::Create(shared_ptr<ProtoBufClientSession>)
    { return _dispatcher; }



CombinedBlockingRequestDispatcherFactory::CombinedBlockingRequestDispatcherFactory(shared_ptr<Node> node) :
    _node(node) {}

shared_ptr<IBlockingRequestDispatcher> CombinedBlockingRequestDispatcherFactory::Create(
    shared_ptr<ProtoBufClientSession> session)
{
    shared_ptr<IChangeListenerFactory> listenerFactory(
        new TcpChangeListenerFactory(session) );
    return shared_ptr<IBlockingRequestDispatcher>(
        new IncomingRequestDispatcher(_node, listenerFactory) );
}




TcpChangeListenerFactory::TcpChangeListenerFactory(shared_ptr<ProtoBufClientSession> session) :
    _session(session) {}



shared_ptr<IChangeListener> TcpChangeListenerFactory::Create(
    shared_ptr<ILocalServiceMethods> localService)
{
//     shared_ptr<IProtoBufRequestDispatcher> dispatcher(
//         new ProtoBufRequestNetworkDispatcher(_session) );
//     return shared_ptr<IChangeListener>(
//         new ProtoBufTcpStreamChangeListener(_session, localService, dispatcher) );
    return shared_ptr<IChangeListener>(
        new NeighbourChangeProtoBufNotifier(_session, localService) );
}



NeighbourChangeProtoBufNotifier::NeighbourChangeProtoBufNotifier(
        shared_ptr<ProtoBufClientSession> session,
        shared_ptr<ILocalServiceMethods> localService ) :
        // shared_ptr<IProtoBufRequestDispatcher> dispatcher ) :
    _sessionId(), _localService(localService), _session(session) //, _dispatcher(dispatcher)
{
    //session->KeepAlive();
}


NeighbourChangeProtoBufNotifier::~NeighbourChangeProtoBufNotifier()
{
    Deregister();
    LOG(DEBUG) << "ChangeListener for session " << _sessionId << " destroyed";
}

void NeighbourChangeProtoBufNotifier::OnRegistered()
    { _sessionId = _session->id(); }


void NeighbourChangeProtoBufNotifier::Deregister()
{
    // Remove only after successful registration of this instance, needed to protect
    // from deregistering another instance after failed (e.g. repeated) addlistener request for same session
    if ( ! _sessionId.empty() )
    {
        LOG(DEBUG) << "ChangeListener deregistering for session " << _sessionId;
        _localService->RemoveListener(_sessionId);
        _sessionId.clear();
    }
}



const SessionId& NeighbourChangeProtoBufNotifier::sessionId() const
    { return _sessionId; }



void NeighbourChangeProtoBufNotifier::AddedNode(const NodeDbEntry& node)
{
    if ( node.relationType() == NodeRelationType::Neighbour )
    {
        try
        {
            unique_ptr<iop::locnet::Request> req( new iop::locnet::Request() );
            iop::locnet::NeighbourhoodChange *change =
                req->mutable_localservice()->mutable_neighbourhoodchanged()->add_changes();
            iop::locnet::NodeInfo *info = change->mutable_addednodeinfo();
            Converter::FillProtoBuf(info, node);
            
            unique_ptr<iop::locnet::Message> msgToSend( RequestToMessage( move(req) ) );
            _session->SendRequest( move(msgToSend) );
        }
        catch (exception &ex)
        {
            LOG(ERROR) << "Failed to send change notification: " << ex.what();
            Deregister();
        }
    }
}


void NeighbourChangeProtoBufNotifier::UpdatedNode(const NodeDbEntry& node)
{
    if ( node.relationType() == NodeRelationType::Neighbour )
    {
        try
        {
            unique_ptr<iop::locnet::Request> req( new iop::locnet::Request() );
            iop::locnet::NeighbourhoodChange *change =
                req->mutable_localservice()->mutable_neighbourhoodchanged()->add_changes();
            iop::locnet::NodeInfo *info = change->mutable_updatednodeinfo();
            Converter::FillProtoBuf(info, node);
            
            unique_ptr<iop::locnet::Message> msgToSend( RequestToMessage( move(req) ) );
            _session->SendRequest( move(msgToSend) );
        }
        catch (exception &ex)
        {
            LOG(ERROR) << "Failed to send change notification: " << ex.what();
            Deregister();
        }
    }
}


void NeighbourChangeProtoBufNotifier::RemovedNode(const NodeDbEntry& node)
{
    if ( node.relationType() == NodeRelationType::Neighbour )
    {
        try
        {
            unique_ptr<iop::locnet::Request> req( new iop::locnet::Request() );
            iop::locnet::NeighbourhoodChange *change =
                req->mutable_localservice()->mutable_neighbourhoodchanged()->add_changes();
            change->set_removednodeid( node.id() );
        
            unique_ptr<iop::locnet::Message> msgToSend( RequestToMessage( move(req) ) );
            _session->SendRequest( move(msgToSend) );
        }
        catch (exception &ex)
        {
            LOG(ERROR) << "Failed to send change notification: " << ex.what();
            Deregister();
        }
    }
}



} // namespace LocNet
