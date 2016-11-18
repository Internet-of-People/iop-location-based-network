#include <functional>
#include "easylogging++.h"

#include "network.hpp"

using namespace std;
using namespace asio::ip;



namespace LocNet
{

    
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


iop::locnet::MessageWithHeader* SyncProtoBufNetworkSession::ReceiveMessage()
{
    unique_ptr<iop::locnet::MessageWithHeader> message(
        new iop::locnet::MessageWithHeader() );
    
    string line;
    _stream >> line;
    return nullptr;
    
    // TODO
    //message.ParseFromIStream(_stream);
    //return message.release();
}


void SyncProtoBufNetworkSession::SendMessage(const iop::locnet::MessageWithHeader& message)
{
    _stream << "Response" << endl;
    
    // TODO
    // message->SerializeToOstream(_stream);
}




} // namespace LocNet