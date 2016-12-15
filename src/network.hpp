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
    
    virtual void AsyncAcceptHandler(std::shared_ptr<asio::ip::tcp::socket> socket,
        const asio::error_code &ec) = 0;
public:
    
    TcpServer(const NetworkInterface &listenOn);
    virtual ~TcpServer();
    
    void Shutdown();
};



class ProtoBufDispatchingTcpServer : public TcpServer
{
    std::shared_ptr<IProtoBufRequestDispatcher> _dispatcher;
    
    void AsyncAcceptHandler(std::shared_ptr<asio::ip::tcp::socket> socket,
                            const asio::error_code &ec) override;
public:
    
    ProtoBufDispatchingTcpServer(const NetworkInterface &listenOn,
        std::shared_ptr<IProtoBufRequestDispatcher> dispatcher);
};



class IProtoBufNetworkSession
{
public:
 
    virtual ~IProtoBufNetworkSession() {}
    
    virtual iop::locnet::MessageWithHeader* ReceiveMessage() = 0;
    virtual void SendMessage(iop::locnet::MessageWithHeader &message) = 0;
    
    //virtual bool IsAlive() const = 0;
    virtual void Close() = 0;
};



class ProtoBufTcpStreamSession : public IProtoBufNetworkSession
{
    asio::ip::tcp::iostream _stream;
    
public:
    
    ProtoBufTcpStreamSession(asio::ip::tcp::socket &socket);
    ProtoBufTcpStreamSession(const NetworkInterface &contact);
    ~ProtoBufTcpStreamSession();
    
    iop::locnet::MessageWithHeader* ReceiveMessage() override;
    void SendMessage(iop::locnet::MessageWithHeader &message) override;
    
    //bool IsAlive() const override;
    void Close() override;
};



class ProtoBufRequestNetworkDispatcher : public IProtoBufRequestDispatcher
{
    std::shared_ptr<IProtoBufNetworkSession> _session;
    
public:

    ProtoBufRequestNetworkDispatcher(std::shared_ptr<IProtoBufNetworkSession> session);
    virtual ~ProtoBufRequestNetworkDispatcher() {}
    
    std::unique_ptr<iop::locnet::Response> Dispatch(const iop::locnet::Request &request) override;
};



// TODO what will be the connection to asio::io_service? Is it needed at all on sync streams?
class TcpStreamConnectionFactory : public INodeConnectionFactory
{
public:
    
    std::shared_ptr<INodeMethods> ConnectTo(const NodeProfile &node) override;
};



} // namespace LocNet


#endif // __LOCNET_ASIO_NETWORK_H__