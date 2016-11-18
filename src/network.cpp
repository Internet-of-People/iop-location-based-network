#include <functional>
#include "easylogging++.h"

#include "network.hpp"

using namespace std;
using namespace asio::ip;



namespace LocNet
{

    
TcpNetwork::TcpNetwork(NetworkInterface &listenOn, size_t threadPoolSize) :
    _threadPool(), _ioService(), _keepThreadPoolBusy( new asio::io_service::work(_ioService) )
{
    //tcp::socket serverSocket(_ioService);
    tcp::endpoint endpoint( make_address( listenOn.address() ), listenOn.port() );
    tcp::acceptor acceptor(_ioService, endpoint);
    
    // Start the specified number of job processor threads
    for (size_t idx = 0; idx < threadPoolSize; ++idx)
    {
        _threadPool.push_back( thread(
            [this] { _ioService.run(); } ) );
    }
    
    tcp::iostream stream;
    asio::error_code ec;
    acceptor.accept( *stream.rdbuf(), ec );
    if (! ec)
    {
        string line;
        stream >> line;
        stream << line;
    }
    else {
        LOG(ERROR) << ec.message();
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


} // namespace LocNet