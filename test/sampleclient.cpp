#include <iostream>
#include <string>

#include <asio.hpp>
#include <easylogging++.h>

#include "IopLocNet.pb.h"
#include "network.hpp"
#include "testdata.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace asio::ip;
using namespace LocNet;



int main(int argc, const char* const argv[])
{
    try
    {
        cout << "Usage: sampleclient [port] [host]" << endl;
        
        uint16_t port = argc >= 2 ? stoul( argv[1] ) : 6371;
        string   host = argc >= 3 ? argv[2] : "localhost";
        
        const NetworkInterface nodeContact(AddressType::Ipv4, host, port);
        LOG(INFO) << "Connecting to server " << nodeContact;
        shared_ptr<IProtoBufNetworkSession> session( new ProtoBufTcpStreamSession(nodeContact) );

        LOG(INFO) << "Sending getcolleaguenodecount request";
        shared_ptr<IProtoBufRequestDispatcher> netDispatcher( new ProtoBufRequestNetworkDispatcher(session) );
        NodeMethodsProtoBufClient client(netDispatcher);
        size_t colleagueCount = client.GetNodeCount();
        LOG(INFO) << "Get node count " << colleagueCount;
        vector<NodeInfo> randomNodes = client.GetRandomNodes(10, Neighbours::Included);
        LOG(INFO) << "Got " << randomNodes.size() << " random nodes";
        for (const auto &node : randomNodes)
            { LOG(INFO) << "  " << node; }
        
//         LOG(INFO) << "Sending getneighbournodes request";
//         iop::locnet::MessageWithHeader requestMsg;
//         requestMsg.mutable_body()->mutable_request()->mutable_localservice()->mutable_getneighbournodes();
//         requestMsg.mutable_body()->mutable_request()->set_version("1");
//         session->SendMessage(requestMsg);
//         
//         unique_ptr<iop::locnet::MessageWithHeader> msgReceived( session->ReceiveMessage() );
//         
//         LOG(INFO) << "Got " << msgReceived->body().response().localservice().getneighbournodes().nodes_size()
//                   << " neighbours in the reponse";
        
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
        return 1;
    }
}
