#ifndef __LOCNET_ASIO_NETWORK_H__
#define __LOCNET_ASIO_NETWORK_H__

#include <functional>
#include <memory>

#include "asio.hpp"
#include "asio/use_future.hpp"
#include "basic.hpp"



namespace LocNet
{



class Reactor
{
    static Reactor _instance;
    
    asio::io_service _asioService;
    
protected:
    
    Reactor();
    Reactor(const Reactor &other) = delete;
    Reactor& operator=(const Reactor &other) = delete;
    
public:
    
    static Reactor& Instance();

    void Shutdown();
    bool IsShutdown() const;
    
    asio::io_service& AsioService();
};



class AsyncConnection : public std::enable_shared_from_this<AsyncConnection>
{
    // TODO consider whether socket member should be weak_ptr or shared_ptr
    std::weak_ptr<asio::ip::tcp::socket>    _socket;
    std::unique_ptr<std::string>            _buffer;
    size_t                                  _offset;

    AsyncConnection( std::weak_ptr<asio::ip::tcp::socket> socket,
                   std::unique_ptr<std::string> &&buffer, size_t offset );
    
    void AsyncReadCallback ( const asio::error_code &error, size_t bytesRead,
        std::function< void ( std::unique_ptr<std::string> &&releasedBuffer ) > completionCallback );
    void AsyncWriteCallback( const asio::error_code &error, size_t bytesWritten,
        std::function< void ( std::unique_ptr<std::string> &&releasedBuffer ) > completionCallback );
    
public:
    
    static std::shared_ptr<AsyncConnection> Create(
        std::weak_ptr<asio::ip::tcp::socket> socket,
        std::unique_ptr<std::string> &&buffer, size_t offset = 0 );
    
    void ReadBuffer( std::function< void ( std::unique_ptr<std::string>&& ) > completionCallback );
//    std::future< std::unique_ptr<std::string> > ReadBuffer(asio::use_future_t<>);
    
    void WriteBuffer( std::function< void ( std::unique_ptr<std::string>&& ) > completionCallback );
//    std::future< std::unique_ptr<std::string> > WriteBuffer(asio::use_future_t<>);
};



// Abstract TCP server that accepts clients asynchronously on a specific port number
// and has a customizable client accept callback to customize concrete provided service.
class TcpServer: public std::enable_shared_from_this<TcpServer>
{
protected:
    
    asio::ip::tcp::acceptor _acceptor;

public:

    TcpServer(TcpPort portNumber);
    virtual ~TcpServer();
    
    virtual void StartListening() = 0;
    virtual void AsyncAcceptHandler( std::shared_ptr<asio::ip::tcp::socket> socket,
                                     const asio::error_code &ec ) = 0;
};



} // namespace LocNet


#endif // __LOCNET_ASIO_NETWORK_H__
