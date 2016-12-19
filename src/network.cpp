#include <chrono>
#include <functional>

#include <easylogging++.h>

#include "network.hpp"

using namespace std;
using namespace asio::ip;



namespace LocNet
{


static const size_t ThreadPoolSize = 1;
static const size_t MaxMessageSize = 1024 * 1024;
//static const chrono::duration<uint32_t> NormalStreamExpirationPeriod    = chrono::seconds(10);
//static const chrono::duration<uint32_t> KeepAliveStreamExpirationPeriod = chrono::hours(168);

static const size_t MessageHeaderSize = 5;
static const size_t MessageSizeOffset = 1;



TcpServer::TcpServer(const NetworkInterface &listenOn) : _ioService(),
    _acceptor( _ioService, tcp::endpoint( make_address( listenOn.address() ), listenOn.port() ) ),
    _threadPool(), _shutdownRequested(false)
{
    // Switch the acceptor to listening state
    LOG(DEBUG) << "Start accepting connections";
    _acceptor.listen();
    
    shared_ptr<tcp::socket> socket( new tcp::socket(_ioService) );
    _acceptor.async_accept( *socket,
        [this, socket] (const asio::error_code &ec) { AsyncAcceptHandler(socket, ec); } );
    
    // Start the specified number of job processor threads
    for (size_t idx = 0; idx < ThreadPoolSize; ++idx)
    {
        _threadPool.push_back( thread( [this]
        {
            while (! _shutdownRequested)
                { _ioService.run(); }
        } ) );
    }
}


TcpServer::~TcpServer()
{
    Shutdown();
    
    _ioService.stop();
    for (auto &thr : _threadPool)
        { thr.join(); }
}


void TcpServer::Shutdown()
    { _shutdownRequested = true; }



ProtoBufDispatchingTcpServer::ProtoBufDispatchingTcpServer( const NetworkInterface& listenOn,
        shared_ptr<IProtoBufRequestDispatcherFactory> dispatcherFactory ) :
    TcpServer(listenOn), _dispatcherFactory(dispatcherFactory) {}



void ProtoBufDispatchingTcpServer::AsyncAcceptHandler(
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
    shared_ptr<tcp::socket> nextSocket( new tcp::socket(_ioService) );
    _acceptor.async_accept( *nextSocket,
        [this, nextSocket] (const asio::error_code &ec) { AsyncAcceptHandler(nextSocket, ec); } );
    
    // Serve connected client on separate thread
    thread serveSessionThread( [this, socket] // copy by value to keep socket alive
    {
        shared_ptr<IProtoBufNetworkSession> session( new ProtoBufTcpStreamSession(socket) );
        try
        {
            shared_ptr<IProtoBufRequestDispatcher> dispatcher(
                _dispatcherFactory->Create(session) );

            while (! _shutdownRequested)
            {
                LOG(TRACE) << "Reading request";
                shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session->ReceiveMessage() );
                if (! requestMsg)
                    { break; }
                
                LOG(TRACE) << "Serving request";
                unique_ptr<iop::locnet::Response> response;
                try
                {
                    if ( ! requestMsg->has_body() || ! requestMsg->body().has_request() )
                        { throw runtime_error("Invalid request"); }
                    response = dispatcher->Dispatch( requestMsg->body().request() );
                    response->set_status(iop::locnet::Status::STATUS_OK);
                }
                catch (exception &ex)
                {
                    LOG(ERROR) << "Failed to serve request: " << ex.what();
                    response.reset( new iop::locnet::Response() );
                    response->set_status(iop::locnet::Status::ERROR_INTERNAL);
                    response->set_details( ex.what() );
                }
                
                LOG(TRACE) << "Sending response";
                iop::locnet::MessageWithHeader responseMsg;
                responseMsg.mutable_body()->set_allocated_response( response.release() );
                
                session->SendMessage(responseMsg);
                
                if ( requestMsg->has_body() && requestMsg->body().has_request() &&
                     requestMsg->body().request().has_localservice() &&
                     requestMsg->body().request().localservice().has_getneighbournodes() &&
                     requestMsg->body().request().localservice().getneighbournodes().keepaliveandsendupdates() )
                {
                    LOG(DEBUG) << "GetNeighbourhood with keepalive is requested, breaking dispatch loop and serve only notifications through ChangeListener";
                    break;
                }
            }
        }
        catch (exception &ex)
        {
            LOG(WARNING) << "Request dispatch loop failed: " << ex.what();
        }
        
        LOG(INFO) << "Request dispatch loop for session " << session->id() << " finished";
    } );
    
    // Keep thread running independently, don't block io_service here by joining it
    serveSessionThread.detach();
}




ProtoBufTcpStreamSession::ProtoBufTcpStreamSession(shared_ptr<tcp::socket> socket) :
    _id(), _stream(), _socket(socket)
{
    if (! _socket)
        { throw runtime_error("Invalid socket parameter"); }
    
    _id = socket->remote_endpoint().address().to_string() + ":" + to_string( socket->remote_endpoint().port() );
    // TODO consider possible ownership problems of this construct
    _stream.rdbuf()->assign( socket->local_endpoint().protocol(), socket->native_handle() );
    // TODO handle session expiration
    //_stream.expires_after(NormalStreamExpirationPeriod);
}


ProtoBufTcpStreamSession::ProtoBufTcpStreamSession(const NetworkInterface &contact) :
    _id( contact.address() + ":" + to_string( contact.port() ) ),
    _stream( contact.address(), to_string( contact.port() ) )
{
    if (! _stream)
        { throw runtime_error("Session failed to connect: " + _stream.error().message() ); }
    LOG(DEBUG) << "Connected to " << contact;
    // TODO handle session expiration
    //_stream.expires_after(NormalStreamExpirationPeriod);
}

ProtoBufTcpStreamSession::~ProtoBufTcpStreamSession()
{
    LOG(DEBUG) << "Session " << id() << " closed";
}


const SessionId& ProtoBufTcpStreamSession::id() const
    { return _id; }



uint32_t GetMessageSizeFromHeader(const char *bytes)
{
    // Adapt big endian value from network to local format
    const uint8_t *data = reinterpret_cast<const uint8_t*>(bytes);
    return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
}



iop::locnet::MessageWithHeader* ProtoBufTcpStreamSession::ReceiveMessage()
{
    if ( _stream.eof() )
        { throw runtime_error("Session " + id() + " not alive"); }
        
    // Allocate a buffer for the message header and read it
    string messageBytes(MessageHeaderSize, 0);
    _stream.read( &messageBytes[0], MessageHeaderSize );
    if ( _stream.eof() )
        { throw runtime_error("Session " + id() + " not alive"); }
    
    // Extract message size from the header to know how many bytes to read
    uint32_t bodySize = GetMessageSizeFromHeader( &messageBytes[MessageSizeOffset] );
    
    if (bodySize > MaxMessageSize)
        { throw runtime_error( "Message size is over limit: " + to_string(bodySize) ); }
    
    // Extend buffer to fit remaining message size and read it
    messageBytes.resize(MessageHeaderSize + bodySize, 0);
    _stream.read( &messageBytes[0] + MessageHeaderSize, bodySize );
    if ( _stream.eof() )
        { throw runtime_error("Session " + id() + " not alive"); }

    // Deserialize message from receive buffer, avoid leaks for failing cases with RAII-based unique_ptr
    unique_ptr<iop::locnet::MessageWithHeader> message( new iop::locnet::MessageWithHeader() );
    message->ParseFromString(messageBytes);
    
    string buffer;
    google::protobuf::TextFormat::PrintToString(*message, &buffer);
    LOG(TRACE) << "Session " << id() << " received message " << buffer;
    
    return message.release();
}



void ProtoBufTcpStreamSession::SendMessage(iop::locnet::MessageWithHeader& message)
{
    message.set_header(1);
    message.set_header( message.ByteSize() - MessageHeaderSize );
    _stream << message.SerializeAsString();
    
    string buffer;
    google::protobuf::TextFormat::PrintToString(message, &buffer);
    LOG(TRACE) << "Session " << id() << " sent message " << buffer;
}


void ProtoBufTcpStreamSession::KeepAlive()
{
    // TODO handle session expiration
    //_stream.expires_after(KeepAliveStreamExpirationPeriod);
}
    
// // TODO CHECK This doesn't really seem to work as on "normal" std::streamss
// bool ProtoBufTcpStreamSession::IsAlive() const
//     { return _stream.good(); }
// 
// void ProtoBufTcpStreamSession::Close()
//     { _stream.close(); }




ProtoBufRequestNetworkDispatcher::ProtoBufRequestNetworkDispatcher(shared_ptr<IProtoBufNetworkSession> session) :
    _session(session) {}

    

unique_ptr<iop::locnet::Response> ProtoBufRequestNetworkDispatcher::Dispatch(const iop::locnet::Request& request)
{
    iop::locnet::Request *clonedReq = new iop::locnet::Request(request);
    clonedReq->set_version("1");
    
    iop::locnet::MessageWithHeader reqMsg;
    reqMsg.mutable_body()->set_allocated_request(clonedReq);
    
    _session->SendMessage(reqMsg);
    unique_ptr<iop::locnet::MessageWithHeader> respMsg( _session->ReceiveMessage() );
    if ( ! respMsg || ! respMsg->has_body() || ! respMsg->body().has_response() )
        { throw runtime_error("Got invalid response from remote node"); }
        
    unique_ptr<iop::locnet::Response> result(
        new iop::locnet::Response( respMsg->body().response() ) );
    return result;
}



shared_ptr<INodeMethods> TcpStreamConnectionFactory::ConnectTo(const NodeProfile& node)
{
    LOG(DEBUG) << "Connecting to " << node;
    shared_ptr<IProtoBufNetworkSession> session( new ProtoBufTcpStreamSession( node.contact() ) );
    shared_ptr<IProtoBufRequestDispatcher> dispatcher( new ProtoBufRequestNetworkDispatcher(session) );
    shared_ptr<INodeMethods> result( new NodeMethodsProtoBufClient(dispatcher) );
    return result;
}



IncomingRequestDispatcherFactory::IncomingRequestDispatcherFactory(shared_ptr<Node> node) :
    _node(node) {}


shared_ptr<IProtoBufRequestDispatcher> IncomingRequestDispatcherFactory::Create(
    shared_ptr<IProtoBufNetworkSession> session )
{
    shared_ptr<IChangeListenerFactory> listenerFactory(
        new ProtoBufTcpStreamChangeListenerFactory(session) );
    return shared_ptr<IProtoBufRequestDispatcher>(
        new IncomingRequestDispatcher(_node, listenerFactory) );
}



ProtoBufTcpStreamChangeListenerFactory::ProtoBufTcpStreamChangeListenerFactory(
        shared_ptr<IProtoBufNetworkSession> session) :
    _session(session) {}



shared_ptr<IChangeListener> ProtoBufTcpStreamChangeListenerFactory::Create(
    shared_ptr<ILocalServiceMethods> localService)
{
    shared_ptr<IProtoBufRequestDispatcher> dispatcher(
        new ProtoBufRequestNetworkDispatcher(_session) );
    return shared_ptr<IChangeListener>(
        new ProtoBufTcpStreamChangeListener(_session, localService, dispatcher) );
}



ProtoBufTcpStreamChangeListener::ProtoBufTcpStreamChangeListener(
        shared_ptr<IProtoBufNetworkSession> session,
        shared_ptr<ILocalServiceMethods> localService,
        shared_ptr<IProtoBufRequestDispatcher> dispatcher ) :
    _sessionId( session->id() ), _localService(localService), _dispatcher(dispatcher)
{
    session->KeepAlive();
}


ProtoBufTcpStreamChangeListener::~ProtoBufTcpStreamChangeListener()
{
    Deregister();
    LOG(DEBUG) << "ChangeListener destroyed";
}

void ProtoBufTcpStreamChangeListener::Deregister()
{
    if ( ! _sessionId.empty() )
    {
        LOG(DEBUG) << "ChangeListener deregistering for session " << _sessionId;
        _localService->RemoveListener(_sessionId);
        _sessionId.clear();
    }
}



const SessionId& ProtoBufTcpStreamChangeListener::sessionId() const
    { return _sessionId; }



void ProtoBufTcpStreamChangeListener::AddedNode(const NodeDbEntry& node)
{
    if ( node.relationType() == NodeRelationType::Neighbour )
    {
        try
        {
            iop::locnet::Request req;
            iop::locnet::NeighbourhoodChange *change =
                req.mutable_localservice()->mutable_neighbourhoodchanged()->add_changes();
            iop::locnet::NodeInfo *info = change->mutable_addednodeinfo();
            Converter::FillProtoBuf(info, node);
            
            unique_ptr<iop::locnet::Response> response( _dispatcher->Dispatch(req) );
            // TODO what to do with response status codes here?
        }
        catch (exception &ex)
        {
            LOG(ERROR) << "Failed to send change notification";
            Deregister();
        }
    }
}


void ProtoBufTcpStreamChangeListener::UpdatedNode(const NodeDbEntry& node)
{
    if ( node.relationType() == NodeRelationType::Neighbour )
    {
        try
        {
            iop::locnet::Request req;
            iop::locnet::NeighbourhoodChange *change =
                req.mutable_localservice()->mutable_neighbourhoodchanged()->add_changes();
            iop::locnet::NodeInfo *info = change->mutable_addednodeinfo();
            Converter::FillProtoBuf(info, node);
            
            unique_ptr<iop::locnet::Response> response( _dispatcher->Dispatch(req) );
            // TODO what to do with response status codes here?
        }
        catch (exception &ex)
        {
            LOG(ERROR) << "Failed to send change notification";
            Deregister();
        }
    }
}


void ProtoBufTcpStreamChangeListener::RemovedNode(const NodeDbEntry& node)
{
    if ( node.relationType() == NodeRelationType::Neighbour )
    {
        try
        {
            iop::locnet::Request req;
            iop::locnet::NeighbourhoodChange *change =
                req.mutable_localservice()->mutable_neighbourhoodchanged()->add_changes();
            change->set_removednodeid( node.profile().id() );
            
            unique_ptr<iop::locnet::Response> response( _dispatcher->Dispatch(req) );
            // TODO what to do with response status codes here?
        }
        catch (exception &ex)
        {
            LOG(ERROR) << "Failed to send change notification";
            Deregister();
        }
    }
}



} // namespace LocNet