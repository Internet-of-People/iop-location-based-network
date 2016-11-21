#include <functional>
#include "easylogging++.h"

#include "network.hpp"

using namespace std;
using namespace asio::ip;



namespace LocNet
{


const size_t MessageHeaderSize = 5;
const size_t MessageSizeOffset = 1;
const size_t MaxMessageSize = 1024 * 1024;



TcpNetwork::TcpNetwork(NetworkInterface &listenOn, size_t threadPoolSize) :
    _threadPool(), _ioService(), _keepThreadPoolBusy( new asio::io_service::work(_ioService) ),
    _acceptor( _ioService, tcp::endpoint( make_address( listenOn.address() ), listenOn.port() ) )
{
    // Start the specified number of job processor threads
    for (size_t idx = 0; idx < threadPoolSize; ++idx)
    {
        _threadPool.push_back( thread(
            [this] { _ioService.run(); } ) );
    }
}



// TcpNetwork::TcpNetwork(const vector<NetworkInterface> &listenOn, size_t threadPoolSize) :
//     _threadPool(), _ioService(), _keepThreadPoolBusy( new asio::io_service::work(_ioService) ),
//     _sessionFactories()
// {
//     for (auto const &netIf : listenOn)
//     {
//         //tcp::socket serverSocket(_ioService);
//         tcp::endpoint endpoint( make_address( netIf.address() ), netIf.port() );
//         tcp::acceptor acceptor(_ioService, endpoint);
//     }
//     
//     // Start the specified number of job processor threads
//     for (size_t idx = 0; idx < threadPoolSize; ++idx)
//     {
//         _threadPool.push_back( thread(
//             [this] { _ioService.run(); } ) );
//     }
// }



TcpNetwork::~TcpNetwork()
{
    // Release lock that disables return from io_service.run() even if job queue is empty
    _keepThreadPoolBusy.reset();
    
    _ioService.stop();
    for (auto &thr : _threadPool)
        { thr.join(); }
}


tcp::acceptor& TcpNetwork::acceptor() { return _acceptor; }



SyncProtoBufNetworkSession::SyncProtoBufNetworkSession(tcp::acceptor& acceptor) :
    _acceptor(acceptor), _stream()
{
    asio::error_code ec;
    _acceptor.accept( *_stream.rdbuf() );
}



uint32_t GetMessageSizeFromHeader(const char *bytes)
{
    const uint8_t *data = reinterpret_cast<const uint8_t*>(bytes);
    return data[MessageSizeOffset + 0] +
          (data[MessageSizeOffset + 1] << 8) +
          (data[MessageSizeOffset + 2] << 16) +
          (data[MessageSizeOffset + 3] << 24);
}


iop::locnet::MessageWithHeader* SyncProtoBufNetworkSession::ReceiveMessage()
{
    unique_ptr<iop::locnet::MessageWithHeader> message(
        new iop::locnet::MessageWithHeader() );

    string messageBytes(MessageHeaderSize, 0);
    _stream.read( &messageBytes[0], MessageHeaderSize );
    uint32_t bodySize = GetMessageSizeFromHeader( &messageBytes[0] );
    messageBytes.reserve(MessageHeaderSize + bodySize);
    _stream.read( &messageBytes[0] + MessageHeaderSize, bodySize );

    message->ParseFromString(messageBytes);
    return message.release();
}


void SyncProtoBufNetworkSession::SendMessage(iop::locnet::MessageWithHeader& message)
{
    message.set_header( message.body().ByteSize() );
    _stream << message.SerializeAsString() << endl;
}




} // namespace LocNet