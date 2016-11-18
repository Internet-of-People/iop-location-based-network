#ifndef __LOCNET_ASIO_NETWORK_H__
#define __LOCNET_ASIO_NETWORK_H__

#include <memory>
#include <thread>

#define ASIO_STANDALONE
#include "asio.hpp"

#include "messaging.hpp"



namespace LocNet
{



// class INetworkSession
// {
// public:
//     virtual void ServeClient() = 0;
// };
// 
// 
// class INetworkSessionFactory
// {
// public:
//     
//     virtual std::shared_ptr<INetworkSession> CreateSession(asio::ip::tcp::socket &clientSocket) = 0;
// };


class TcpNetwork
{
    std::vector<std::thread> _threadPool;
    asio::io_service _ioService;
    std::unique_ptr<asio::io_service::work> _keepThreadPoolBusy;
    
    asio::ip::tcp::acceptor _acceptor;
//    std::unordered_map<uint16_t, std::shared_ptr<INetworkSessionFactory>> _sessionFactories;
    
public:
    
    TcpNetwork(NetworkInterface &listenOn, size_t threadPoolSize = 1);
    //TcpNetwork(const std::vector<NetworkInterface> &listenOn, size_t threadPoolSize = 1);
    ~TcpNetwork();
    
    // TODO this probably will not be here after testing
    asio::ip::tcp::acceptor& acceptor();
};



class IProtoBufNetworkSession
{
public:
    
    virtual iop::locnet::MessageWithHeader* ReceiveMessage() = 0;
    virtual void SendMessage(const iop::locnet::MessageWithHeader &message) = 0;
};



class SyncProtoBufNetworkSession : public IProtoBufNetworkSession
{
    asio::ip::tcp::acceptor &_acceptor;
    asio::ip::tcp::iostream _stream;
    
public:
    
    SyncProtoBufNetworkSession(asio::ip::tcp::acceptor &acceptor);
    
    iop::locnet::MessageWithHeader* ReceiveMessage() override;
    void SendMessage(const iop::locnet::MessageWithHeader &message) override;
};



} // namespace LocNet


#endif // __LOCNET_ASIO_NETWORK_H__