#include "network.hpp"

// NOTE on Windows this includes <winsock(2).h> so must be after asio includes in "network.hpp"
#include <easylogging++.h>

using namespace std;
using namespace asio::ip;



namespace LocNet
{



bool NetworkEndpoint::isLoopback() const
{
    try { return address::from_string(_address).is_loopback(); }
    catch (...) { return false; }
}

Address NodeContact::AddressFromBytes(const std::string &bytes)
{
    if ( bytes.empty() )
        { return Address(); }
    else if ( bytes.size() == sizeof(address_v4::bytes_type) )
    {
        address_v4::bytes_type v4AddrBytes;
        copy( &bytes.front(), &bytes.front() + v4AddrBytes.size(), v4AddrBytes.data() );
        address_v4 ipv4(v4AddrBytes);
        return ipv4.to_string();
    }
    else if ( bytes.size() == sizeof(address_v6::bytes_type) )
    {
        address_v6::bytes_type v6AddrBytes;
        copy( &bytes.front(), &bytes.front() + v6AddrBytes.size(), v6AddrBytes.data() );
        address_v6 ipv6(v6AddrBytes);
        return ipv6.to_string();
    }
    else { throw LocationNetworkError(ErrorCode::ERROR_INVALID_VALUE, "Invalid bytearray length of IP address: " + to_string( bytes.size() ) ); }
}


string NodeContact::AddressToBytes(const Address &addr)
{
    string result;
    if ( addr.empty() )
        { return result; }
    
    auto ipAddress( address::from_string(addr) );
    if ( ipAddress.is_v4() )
    {
        auto bytes = ipAddress.to_v4().to_bytes();
        for (uint8_t byte : bytes)
            { result.push_back(byte); }
    }
    else if ( ipAddress.is_v6() )
    {
        auto bytes = ipAddress.to_v6().to_bytes();
        for (uint8_t byte : bytes)
            { result.push_back(byte); }
    }
    else { throw LocationNetworkError(ErrorCode::ERROR_INVALID_VALUE, "Unknown type of address: " + addr); }
    return result;
}

string  NodeContact::AddressBytes() const
    { return AddressToBytes(_address); }



Reactor Reactor::_instance;

Reactor::Reactor(): _asioService() {}

void Reactor::Shutdown() { _asioService.stop(); }
bool Reactor::IsShutdown() const { return _asioService.stopped(); }

Reactor& Reactor::Instance() { return _instance; }
asio::io_service& Reactor::AsioService() { return _asioService; }



shared_ptr<AsyncConnection> AsyncConnection::Create(
    weak_ptr<asio::ip::tcp::socket> socket, unique_ptr<string> &&buffer, size_t offset )
{
    return shared_ptr<AsyncConnection>( new AsyncConnection( socket, move(buffer), offset ) );
}

AsyncConnection::AsyncConnection( weak_ptr<tcp::socket> socket,
                                  unique_ptr<string> &&buffer, size_t offset ) :
    _socket(socket), _buffer( move(buffer) ), _offset(offset)
{
    if (_buffer == nullptr) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "No buffer instantiated");
    }
    if ( _offset >= _buffer->size() ) {
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Invalid buffer offset");
    }
}



void AsyncConnection::ReadBuffer( function< void ( unique_ptr<string>&& ) > completionCallback )
{
    shared_ptr<AsyncConnection> self = shared_from_this();
    shared_ptr<tcp::socket> socket = _socket.lock();
    if (! socket) {
        LOG(INFO) << "Socket was closed, stop reading";
        completionCallback( unique_ptr<string>() );
        return;
    }
    
    asio::async_read( *socket, asio::buffer( &_buffer->operator[](_offset), _buffer->size() - _offset ),
        [self, completionCallback] (const asio::error_code& error, std::size_t bytesRead)
        { self->AsyncReadCallback(error, bytesRead, completionCallback); } );
}


// future< unique_ptr<string> > AsyncBufferIO::ReadBuffer(asio::use_future_t<>)
// {
//     shared_ptr< promise< unique_ptr<string> > > result( new promise< unique_ptr<string> >() );
//     
//     ReadBuffer( [result] ( unique_ptr<string> &&buffer )
//         { result->set_value( move(buffer) ); } );
//     
//     return result->get_future();
// }


void AsyncConnection::AsyncReadCallback( const asio::error_code &error, size_t bytesRead,
                                       function< void ( unique_ptr<string>&& ) > completionCallback )
{
    if (error)
    {
        LOG(WARNING) << "Failed to read from socket: " << error;
        completionCallback( unique_ptr<string>() );
        return;
    }
    
    _offset += bytesRead;
    if ( _offset < _buffer->size() )
    {
        shared_ptr<AsyncConnection> self = shared_from_this();
        shared_ptr<tcp::socket> socket = _socket.lock();
        if (! socket) {
            LOG(INFO) << "Socket was closed, stop reading";
            completionCallback( unique_ptr<string>() );
            return;
        }
        
        asio::async_read( *socket, asio::buffer( &_buffer->operator[](_offset), _buffer->size() - _offset ),
            [self, completionCallback] (const asio::error_code& error, std::size_t bytesRead)
            { self->AsyncReadCallback(error, bytesRead, completionCallback); } );
    }
    else { completionCallback( move(_buffer) ); }
}



void AsyncConnection::WriteBuffer( function< void ( unique_ptr<string>&& ) > completionCallback )
{
    shared_ptr<AsyncConnection> self = shared_from_this();
    shared_ptr<tcp::socket> socket = _socket.lock();
    if (! socket) {
        LOG(INFO) << "Socket was closed, stop writing";
        completionCallback( unique_ptr<string>() );
        return;
    }
    asio::async_write( *socket, asio::buffer( &_buffer->operator[](_offset), _buffer->size() - _offset ),
        [self, completionCallback] (const asio::error_code& error, std::size_t bytesWritten)
    {
        self->AsyncWriteCallback(error, bytesWritten, completionCallback);
    } );
}


// future< unique_ptr<string> > AsyncBufferIO::WriteBuffer(asio::use_future_t<>)
// {
//     shared_ptr< promise< unique_ptr<string> > > result( new promise< unique_ptr<string> >() );
//     
//     WriteBuffer( [result] ( unique_ptr<string> &&buffer )
//         { result->set_value( move(buffer) ); } );
//     
//     return result->get_future();
// }


void AsyncConnection::AsyncWriteCallback( const asio::error_code &error, size_t bytesWritten,
                                        function< void ( unique_ptr<string>&& ) > completionCallback )
{
    if (error)
    {
        LOG(WARNING) << "Failed to write socket: " << error;
        completionCallback( unique_ptr<string>() );
        return;
    }
    
    _offset += bytesWritten;
    if ( _offset < _buffer->size() )
    {
        shared_ptr<AsyncConnection> self = shared_from_this();
        shared_ptr<tcp::socket> socket = _socket.lock();
        if (! socket) {
            LOG(INFO) << "Socket was closed, stop writing";
            completionCallback( unique_ptr<string>() );
            return;
        }
        
        asio::async_write( *socket, asio::buffer( &_buffer->operator[](_offset), _buffer->size() - _offset ),
            [self, completionCallback] (const asio::error_code& error, std::size_t bytesWritten)
            { self->AsyncWriteCallback(error, bytesWritten, completionCallback); } );
    }
    else
    {
        LOG(TRACE) << "Message was written, calling completion callback";
        completionCallback( move(_buffer) );
    }
}



TcpServer::TcpServer(TcpPort portNumber) :
    _acceptor( Reactor::Instance().AsioService(), tcp::endpoint( tcp::v4(), portNumber ) ) {}


TcpServer::~TcpServer() {}



} // namespace LocNet
