#ifndef __LOCNET_ASIO_NETWORK_H__
#define __LOCNET_ASIO_NETWORK_H__

#include <memory>
#include <thread>

#define ASIO_STANDALONE
#include "asio.hpp"

#include "messaging.hpp"



namespace LocNet
{


class IProtoBufNetworkClient
{
    virtual iop::locnet::MessageWithHeader* ReceiveMessage() = 0;
    virtual void SendMessage(const iop::locnet::MessageWithHeader &message) = 0;
};

class SyncProtoBufNetworkClient : public IProtoBufNetworkClient
{
    iop::locnet::MessageWithHeader* ReceiveMessage() override;
    void SendMessage(const iop::locnet::MessageWithHeader &message) override;
};



class INetworkSession
{
public:
    virtual void ServeClient() = 0;
};


class INetworkSessionFactory
{
public:
    
    virtual std::shared_ptr<INetworkSession> CreateSession(asio::ip::tcp::socket &clientSocket) = 0;
};


class TcpNetwork
{
    std::vector<std::thread> _threadPool;
    asio::io_service _ioService;
    std::unique_ptr<asio::io_service::work> _keepThreadPoolBusy;
    
//    asio::ip::tcp::socket _serverSocket;
//    asio::ip::tcp::acceptor _acceptor;
//    std::unordered_map<uint16_t, std::shared_ptr<INetworkSessionFactory>> _sessionFactories;
    
public:
    
    TcpNetwork(NetworkInterface &listenOn, size_t threadPoolSize = 1);
    //TcpNetwork(const std::vector<NetworkInterface> &listenOn, size_t threadPoolSize = 1);
    ~TcpNetwork();
};



} // namespace LocNet


#endif // __LOCNET_ASIO_NETWORK_H__