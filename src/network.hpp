#ifndef __LOCNET_ASIO_NETWORK_H__
#define __LOCNET_ASIO_NETWORK_H__

#define ASIO_STANDALONE
#include "asio.hpp"

#include "messaging.hpp"



namespace LocNet
{


// class INetworkClient
// {
//     virtual void Close() = 0;
//     
//     virtual iop::locnet::MessageWithHeader* ReceiveMessage() = 0;
//     virtual void SendMessage(const iop::locnet::MessageWithHeader &message) = 9;
// };
// 
// class INetwork
// {
//     virtual void Initialize() = 0;
//     virtual void Close() = 0;
//     
//     virtual shared_ptr<INetworkClient> AcceptClient() = 0;
// };


class Network
{
    asio::io_service _io_service;
    asio::io_service::work _work;
};


}


#endif // __LOCNET_ASIO_NETWORK_H__