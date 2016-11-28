#ifndef __LOCNET_ASIO_NETWORK_H__
#define __LOCNET_ASIO_NETWORK_H__

#include <memory>
#include <thread>

#define ASIO_STANDALONE
#include "asio.hpp"

#include "messaging.hpp"



namespace LocNet
{



class TcpNetwork
{
    std::vector<std::thread> _threadPool;
    asio::io_service _ioService;
    std::unique_ptr<asio::io_service::work> _keepThreadPoolBusy;
    
    asio::ip::tcp::acceptor _acceptor;
//    std::unordered_map<uint16_t, std::shared_ptr<INetworkSessionFactory>> _sessionFactories;
    
public:
    
    TcpNetwork(const NetworkInterface &listenOn, size_t threadPoolSize = 1);
    //TcpNetwork(const std::vector<NetworkInterface> &listenOn, size_t threadPoolSize = 1);
    ~TcpNetwork();
    
    asio::ip::tcp::acceptor& acceptor();
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



class SyncProtoBufNetworkSession : public IProtoBufNetworkSession
{
    asio::ip::tcp::iostream _stream;
    
public:
    
    SyncProtoBufNetworkSession(TcpNetwork &network);
    SyncProtoBufNetworkSession(const NetworkInterface &contact);
    
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



} // namespace LocNet


#endif // __LOCNET_ASIO_NETWORK_H__