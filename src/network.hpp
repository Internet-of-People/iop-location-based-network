#ifndef __LOCNET_ASIO_NETWORK_H__
#define __LOCNET_ASIO_NETWORK_H__

#include <functional>
#include <memory>
#include <thread>

#define ASIO_STANDALONE
#include <asio.hpp>

#include "messaging.hpp"



namespace LocNet
{



// Abstract TCP server that accepts clients asynchronously on a specific port number
// and has a customizable client accept callback to customize concrete provided service.
class TcpServer
{
protected:
    
    asio::io_service _ioService;
    asio::ip::tcp::acceptor _acceptor;
    
    std::vector<std::thread> _threadPool;
    bool _shutdownRequested;
    
    virtual void AsyncAcceptHandler( std::shared_ptr<asio::ip::tcp::socket> socket,
                                     const asio::error_code &ec ) = 0;
public:
    
    TcpServer(TcpPort portNumber);
    virtual ~TcpServer();
    
    void Shutdown();
};



// Interface of a network session, i.e. a client connection that allows
// sending and receiving protobuf messages (either request or response).
// TODO this would be more independent if would send/receive byte arrays,
//      but receiving a message we have to be aware of the message header
//      to know how many bytes to read, it cannot be determined in advance.
class IProtoBufNetworkSession
{
public:
 
    virtual ~IProtoBufNetworkSession() {}
    
    virtual const SessionId& id() const = 0;
    virtual const Address& remoteAddress() const = 0;
    
    virtual iop::locnet::MessageWithHeader* ReceiveMessage() = 0;
    virtual void SendMessage(iop::locnet::MessageWithHeader &message) = 0;

// TODO Would be nice and more convenient to implement using these methods,
//      but they do not seem to nicely fit ASIO
//     virtual void KeepAlive() = 0;
//     virtual bool IsAlive() const = 0;
//     virtual void Close() = 0;
};



// Factory interface to create a dispatcher object for a session.
// Implemented specifically for the keepalive feature, otherwise would not be needed.
class IProtoBufRequestDispatcherFactory
{
public:
    
    virtual ~IProtoBufRequestDispatcherFactory() {}
    
    virtual std::shared_ptr<IProtoBufRequestDispatcher> Create(
        std::shared_ptr<IProtoBufNetworkSession> session ) = 0;
};



// Tcp server implementation that serves protobuf requests for accepted clients.
class ProtoBufDispatchingTcpServer : public TcpServer
{
protected:
    
    std::shared_ptr<IProtoBufRequestDispatcherFactory> _dispatcherFactory;
    
    void AsyncAcceptHandler( std::shared_ptr<asio::ip::tcp::socket> socket,
                             const asio::error_code &ec ) override;
public:
    
    ProtoBufDispatchingTcpServer( TcpPort portNumber,
        std::shared_ptr<IProtoBufRequestDispatcherFactory> dispatcherFactory );
};



// Request dispatcher to serve incoming requests from clients.
// Implemented specifically for the keepalive feature.
class IncomingRequestDispatcherFactory : public IProtoBufRequestDispatcherFactory
{
    std::shared_ptr<Node> _node;
    
public:
    
    IncomingRequestDispatcherFactory(std::shared_ptr<Node> node);
    
    std::shared_ptr<IProtoBufRequestDispatcher> Create(
        std::shared_ptr<IProtoBufNetworkSession> session ) override;
};



// Network session that uses a blocking TCP stream for the easiest implementation.
// TODO ideally would use async networking, but it's hard in C++
//      to implement a simple (blocking) interface using async operations.
//      Maybe boost stackful coroutines could be useful here, but we shouldn't depend on boost.
class ProtoBufTcpStreamSession : public IProtoBufNetworkSession
{
    SessionId                               _id;
    Address                                 _remoteAddress;
    asio::ip::tcp::iostream                 _stream;
    std::shared_ptr<asio::ip::tcp::socket>  _socket;
    
public:
    
    ProtoBufTcpStreamSession(std::shared_ptr<asio::ip::tcp::socket> socket);
    ProtoBufTcpStreamSession(const NetworkInterface &contact);
    ~ProtoBufTcpStreamSession();
    
    const SessionId& id() const override;
    const Address& remoteAddress() const override;
    
    iop::locnet::MessageWithHeader* ReceiveMessage() override;
    void SendMessage(iop::locnet::MessageWithHeader &message) override;
    
// TODO implement these
//     void KeepAlive() override;
//     bool IsAlive() const override;
//     void Close() override;
};



// A protobuf request dispatcher that delivers requests through a network session
// and reads response messages from it.
class ProtoBufRequestNetworkDispatcher : public IProtoBufRequestDispatcher
{
    std::shared_ptr<IProtoBufNetworkSession> _session;
    
public:

    ProtoBufRequestNetworkDispatcher(std::shared_ptr<IProtoBufNetworkSession> session);
    virtual ~ProtoBufRequestNetworkDispatcher() {}
    
    std::unique_ptr<iop::locnet::Response> Dispatch(const iop::locnet::Request &request) override;
};



// Connection factory that creates a blocking TCP stream to communicate with remote node.
class TcpStreamConnectionFactory : public INodeConnectionFactory
{
    std::function<void(const Address&)> _detectedIpCallback;
    
public:
    
    std::shared_ptr<INodeMethods> ConnectTo(const NetworkInterface &address) override;
    
    void detectedIpCallback(std::function<void(const Address&)> detectedIpCallback);
};



// Factory implementation that creates ProtoBufTcpStreamChangeListener objects.
class ProtoBufTcpStreamChangeListenerFactory : public IChangeListenerFactory
{
    std::shared_ptr<IProtoBufNetworkSession> _session;
    
public:
    
    ProtoBufTcpStreamChangeListenerFactory(std::shared_ptr<IProtoBufNetworkSession> session);
    
    std::shared_ptr<IChangeListener> Create(
        std::shared_ptr<ILocalServiceMethods> localService) override;
};



// Listener implementation that translates node notifications to protobuf
// and uses a dispatcher to send them and notify a remote peer.
class ProtoBufTcpStreamChangeListener : public IChangeListener
{
    SessionId                                   _sessionId;
    std::shared_ptr<ILocalServiceMethods>       _localService;
    std::shared_ptr<IProtoBufRequestDispatcher> _dispatcher;
    
public:
    
    ProtoBufTcpStreamChangeListener(
        std::shared_ptr<IProtoBufNetworkSession> session,
        std::shared_ptr<ILocalServiceMethods> localService,
        std::shared_ptr<IProtoBufRequestDispatcher> dispatcher );
    
    ~ProtoBufTcpStreamChangeListener();
    void Deregister();
    
    const SessionId& sessionId() const override;
    
    void AddedNode  (const NodeDbEntry &node) override;
    void UpdatedNode(const NodeDbEntry &node) override;
    void RemovedNode(const NodeDbEntry &node) override;
};



} // namespace LocNet


#endif // __LOCNET_ASIO_NETWORK_H__