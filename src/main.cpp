#include <iostream>

#include "easylogging++.h"
#include "locnet.hpp"
#include "network.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



int main()
{
    NetworkInterface listenOn(AddressType::Ipv4, "127.0.0.1", 4567);
    TcpNetwork network(listenOn);
    
    SyncProtoBufNetworkSession session( network.acceptor() );
    shared_ptr<iop::locnet::MessageWithHeader> msgReceived( session.ReceiveMessage() );
    
    iop::locnet::MessageWithHeader msgToSend;
    session.SendMessage(msgToSend);
    
    string line;
    getline(cin, line);
    LOG(INFO) << "Located works";
    return 0;
}
