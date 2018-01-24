#ifndef __LOCNET_SERVER_H__
#define __LOCNET_SERVER_H__


#include "network.hpp"
#include "messaging.hpp"



namespace LocNet
{


// Interface of an asynchronous channel that allows sending and receiving protobuf messages.
// NOTE just messages, requests and responses are not distinguished here.
class IProtoBufChannel
{
public:
    
    typedef void ReceivedMessageCallback( std::unique_ptr<iop::locnet::Message> &&receivedMessage );
    typedef void SentMessageCallback();
    
    virtual ~IProtoBufChannel() {}
    
    virtual const SessionId& id() const = 0;
    virtual const Address& remoteAddress() const = 0;
    
    virtual void ReceiveMessage( std::function<ReceivedMessageCallback> callback ) = 0;
    virtual std::future< std::unique_ptr<iop::locnet::Message> > ReceiveMessage(asio::use_future_t<>) = 0;
    
    virtual void SendMessage( std::unique_ptr<iop::locnet::Message> &&message, std::function<SentMessageCallback> callback ) = 0;
    virtual std::future<void> SendMessage(std::unique_ptr<iop::locnet::Message> &&message, asio::use_future_t<>) = 0;
};



// ProtoBuf message channel that sends messages through an async TCP network connection.
class AsyncProtoBufTcpChannel : public IProtoBufChannel
{
    std::shared_ptr<asio::ip::tcp::socket>  _socket;
    SessionId                               _id;
    Address                                 _remoteAddress;
    uint32_t                                _nextRequestId;
    
    //std::mutex                              _socketWriteMutex;
    //std::mutex                              _socketReadMutex;

public:

    // Server connection to client with accepted socket
    AsyncProtoBufTcpChannel(std::shared_ptr<asio::ip::tcp::socket> socket);
    // Client connection to server, endpoint resolution to be done
    AsyncProtoBufTcpChannel(const NetworkEndpoint &endpoint);
    ~AsyncProtoBufTcpChannel();

    const SessionId& id() const override;
    const Address& remoteAddress() const override;
    
    void ReceiveMessage( std::function<ReceivedMessageCallback> callback ) override;
    std::future< std::unique_ptr<iop::locnet::Message> > ReceiveMessage(asio::use_future_t<>) override;
    void SendMessage( std::unique_ptr<iop::locnet::Message> &&message, std::function<SentMessageCallback> callback ) override;
    std::future<void> SendMessage(std::unique_ptr<iop::locnet::Message> &&message, asio::use_future_t<>) override;
};



class ProtoBufClientSession : public std::enable_shared_from_this<ProtoBufClientSession>
{
public:
    
    typedef void IncomingRequestHandler( std::unique_ptr<iop::locnet::Message> &&incomingRequest );
    
private:
    
    std::shared_ptr<IProtoBufChannel> _messageChannel;
    
    uint32_t _nextMessageId;
    std::unordered_map< uint32_t, std::promise< std::unique_ptr<iop::locnet::Response> > > _pendingRequests;
    std::mutex _pendingRequestsMutex;

    static void AsyncMessageLoopHandler( std::weak_ptr<ProtoBufClientSession> sessionWeakRef,
                                         const std::string &sessionId,
                                         std::function<IncomingRequestHandler> requestHandler );
    
    ProtoBufClientSession(std::shared_ptr<IProtoBufChannel> connection);
    
public:
    
    static std::shared_ptr<ProtoBufClientSession> Create(std::shared_ptr<IProtoBufChannel> connection);
    
    virtual ~ProtoBufClientSession();
    
    virtual const SessionId& id() const;
    virtual std::shared_ptr<IProtoBufChannel> messageChannel();
    
    virtual void StartMessageLoop( std::function<IncomingRequestHandler> requestHandler = std::function<IncomingRequestHandler>() );
    virtual std::future< std::unique_ptr<iop::locnet::Response> > SendRequest(
        std::unique_ptr<iop::locnet::Message> &&requestMessage);
    virtual void ResponseArrived( std::unique_ptr<iop::locnet::Message> &&responseMessage);
};



// Factory interface to create a dispatcher object for a session.
// Implemented specifically for the notify/keepalive feature, otherwise would not be needed.
class IBlockingRequestDispatcherFactory
{
public:
    
    virtual ~IBlockingRequestDispatcherFactory() {}
    
    virtual std::shared_ptr<IBlockingRequestDispatcher> Create(
        std::shared_ptr<ProtoBufClientSession> session ) = 0;
};



// Tcp server implementation that serves protobuf requests for accepted clients.
class DispatchingTcpServer : public TcpServer
{
protected:
    
    std::shared_ptr<IBlockingRequestDispatcherFactory> _dispatcherFactory;
    
    DispatchingTcpServer( TcpPort portNumber,
        std::shared_ptr<IBlockingRequestDispatcherFactory> dispatcherFactory );
    DispatchingTcpServer( const std::string &interfaceName, TcpPort portNumber,
        std::shared_ptr<IBlockingRequestDispatcherFactory> dispatcherFactory );

public:
    
    static std::shared_ptr<DispatchingTcpServer> Create( TcpPort portNumber,
        std::shared_ptr<IBlockingRequestDispatcherFactory> dispatcherFactory );
    static std::shared_ptr<DispatchingTcpServer> Create( const std::string &interfaceName, TcpPort portNumber,
        std::shared_ptr<IBlockingRequestDispatcherFactory> dispatcherFactory );
    
    static void AsyncServeMessageHandler( std::unique_ptr<iop::locnet::Message> &&receivedMessage,
                                          std::shared_ptr<ProtoBufClientSession> session,
                                          std::shared_ptr<IBlockingRequestDispatcher> dispatcher );
    void StartListening() override;
    void AsyncAcceptHandler( std::shared_ptr<asio::ip::tcp::socket> socket,
                             const asio::error_code &ec ) override;
};



// Request dispatcher to serve incoming requests from clients.
// Implemented specifically for the keepalive feature.
class LocalServiceRequestDispatcherFactory : public IBlockingRequestDispatcherFactory
{
    std::shared_ptr<ILocalServiceMethods> _iLocal;
    
public:
    
    LocalServiceRequestDispatcherFactory(std::shared_ptr<ILocalServiceMethods> iLocal);
    
    std::shared_ptr<IBlockingRequestDispatcher> Create(
        std::shared_ptr<ProtoBufClientSession> session ) override;
};



// Dispatcher factory that ignores the session and returns a simple dispatcher
class StaticBlockingDispatcherFactory : public IBlockingRequestDispatcherFactory
{
    std::shared_ptr<IBlockingRequestDispatcher> _dispatcher;
    
public:
    
    StaticBlockingDispatcherFactory(std::shared_ptr<IBlockingRequestDispatcher> dispatcher);
    
    std::shared_ptr<IBlockingRequestDispatcher> Create(
        std::shared_ptr<ProtoBufClientSession> session ) override;
};



class CombinedBlockingRequestDispatcherFactory : public IBlockingRequestDispatcherFactory
{
    std::shared_ptr<Node> _node;
    
public:
    
    CombinedBlockingRequestDispatcherFactory(std::shared_ptr<Node> node);
    
    std::shared_ptr<IBlockingRequestDispatcher> Create(
        std::shared_ptr<ProtoBufClientSession> session ) override;
};



// A protobuf request dispatcher that delivers requests through a network session
// and reads response messages from it.
class NetworkDispatcher : public IBlockingRequestDispatcher
{
    std::shared_ptr<Config>                _config;
    std::shared_ptr<ProtoBufClientSession> _session;
    
public:

    NetworkDispatcher(std::shared_ptr<Config> config, std::shared_ptr<ProtoBufClientSession> session);
    virtual ~NetworkDispatcher() {}
    
    std::chrono::duration<uint32_t> RequestExpirationPeriod();
    std::unique_ptr<iop::locnet::Response> Dispatch(std::unique_ptr<iop::locnet::Request> &&request) override;
};



// Connection factory that creates proxies that transparently communicate with a remote node.
class TcpNodeConnectionFactory : public INodeProxyFactory
{
    std::shared_ptr<Config>             _config;
    std::function<void(const Address&)> _detectedIpCallback;
    
public:
    
    TcpNodeConnectionFactory(std::shared_ptr<Config> config);
    std::shared_ptr<INodeMethods> ConnectTo(const NetworkEndpoint &address) override;
    
    void detectedIpCallback(std::function<void(const Address&)> detectedIpCallback);
};



// Factory implementation that creates ProtoBufTcpStreamChangeListener objects.
class TcpChangeListenerFactory : public IChangeListenerFactory
{
    std::shared_ptr<ProtoBufClientSession> _session;
    
public:
    
    TcpChangeListenerFactory(std::shared_ptr<ProtoBufClientSession> session);
    
    std::shared_ptr<IChangeListener> Create(
        std::shared_ptr<ILocalServiceMethods> localService) override;
};



// Listener implementation that translates node notifications to protobuf
// and uses a dispatcher to send them and notify a remote peer.
class NeighbourChangeProtoBufNotifier : public IChangeListener
{
    SessionId                                      _sessionId;
    std::shared_ptr<ILocalServiceMethods>          _localService;
    // std::shared_ptr<IProtoBufRequestDispatcher> _dispatcher;
    std::shared_ptr<ProtoBufClientSession>  _session;
    
public:
    
    NeighbourChangeProtoBufNotifier(
        std::shared_ptr<ProtoBufClientSession> session,
        std::shared_ptr<ILocalServiceMethods> localService );
        // std::shared_ptr<IProtoBufRequestDispatcher> dispatcher );
    ~NeighbourChangeProtoBufNotifier();
    
    void Deregister();
    
    const SessionId& sessionId() const override;
    
    void OnRegistered() override;
    void AddedNode  (const NodeDbEntry &node) override;
    void UpdatedNode(const NodeDbEntry &node) override;
    void RemovedNode(const NodeDbEntry &node) override;
};



} // namespace LocNet


#endif // __LOCNET_SERVER_H__
