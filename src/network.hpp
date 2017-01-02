#ifndef __LOCNET_ASIO_NETWORK_H__
#define __LOCNET_ASIO_NETWORK_H__

#include <memory>
#include <thread>

#define ASIO_STANDALONE
#include <asio.hpp>

#include "messaging.hpp"



namespace LocNet
{



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



class IProtoBufNetworkSession
{
public:
 
    virtual ~IProtoBufNetworkSession() {}
    
    virtual const SessionId& id() const = 0;
    
    virtual iop::locnet::MessageWithHeader* ReceiveMessage() = 0;
    virtual void SendMessage(iop::locnet::MessageWithHeader &message) = 0;

    virtual void KeepAlive() = 0;
// TODO implement these    
//     virtual bool IsAlive() const = 0;
//     virtual void Close() = 0;
};



class IProtoBufRequestDispatcherFactory
{
public:
    
    virtual ~IProtoBufRequestDispatcherFactory() {}
    
    virtual std::shared_ptr<IProtoBufRequestDispatcher> Create(
        std::shared_ptr<IProtoBufNetworkSession> session ) = 0;
};



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



class IncomingRequestDispatcherFactory : public IProtoBufRequestDispatcherFactory
{
    std::shared_ptr<Node> _node;
    
public:
    
    IncomingRequestDispatcherFactory(std::shared_ptr<Node> node);
    
    std::shared_ptr<IProtoBufRequestDispatcher> Create(
        std::shared_ptr<IProtoBufNetworkSession> session ) override;
};



class ProtoBufTcpStreamSession : public IProtoBufNetworkSession
{
    SessionId                               _id;
    asio::ip::tcp::iostream                 _stream;
    std::shared_ptr<asio::ip::tcp::socket>  _socket;
    
public:
    
    ProtoBufTcpStreamSession(std::shared_ptr<asio::ip::tcp::socket> socket);
    ProtoBufTcpStreamSession(const NetworkInterface &contact);
    ~ProtoBufTcpStreamSession();
    
    const SessionId& id() const override;
    
    iop::locnet::MessageWithHeader* ReceiveMessage() override;
    void SendMessage(iop::locnet::MessageWithHeader &message) override;
    
    void KeepAlive() override;
// TODO implement these
//     bool IsAlive() const override;
//     void Close() override;
};



class ProtoBufRequestNetworkDispatcher : public IProtoBufRequestDispatcher
{
    std::shared_ptr<IProtoBufNetworkSession> _session;
    
public:

    ProtoBufRequestNetworkDispatcher(std::shared_ptr<IProtoBufNetworkSession> session);
    virtual ~ProtoBufRequestNetworkDispatcher() {}
    
    std::unique_ptr<iop::locnet::Response> Dispatch(const iop::locnet::Request &request) override;
};



class TcpStreamConnectionFactory : public INodeConnectionFactory
{
public:
    
    std::shared_ptr<INodeMethods> ConnectTo(const NetworkInterface &address) override;
};



class ProtoBufTcpStreamChangeListenerFactory : public IChangeListenerFactory
{
    std::shared_ptr<IProtoBufNetworkSession> _session;
    
public:
    
    ProtoBufTcpStreamChangeListenerFactory(std::shared_ptr<IProtoBufNetworkSession> session);
    
    std::shared_ptr<IChangeListener> Create(
        std::shared_ptr<ILocalServiceMethods> localService) override;
};



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