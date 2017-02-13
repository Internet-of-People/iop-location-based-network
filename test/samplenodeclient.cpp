#include <iostream>
#include <string>
#include <thread>

#include <asio.hpp>
#include <easylogging++.h>

#include "config.hpp"
#include "IopLocNet.pb.h"
#include "network.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace asio::ip;
using namespace LocNet;


bool ShutdownRequested = false;

void signalHandler(int)
    { ShutdownRequested = true; }



int main(int argc, const char* const argv[])
{
    try
    {
        cout << "Usage: samplenodeclient [host] [port]" << endl;
        
        string   host = argc >= 2 ? argv[1] : "localhost";
        uint16_t port = argc >= 3 ? stoul( argv[2] ) : 16980;
        
        signal(SIGINT,  signalHandler);
        signal(SIGTERM, signalHandler);
        
        Config::InitForTest();
        
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime %level %msg (%fbase:%line)");
        
        const NetworkEndpoint nodeContact(host, port);
        LOG(INFO) << "Connecting to server " << nodeContact;
        shared_ptr<IProtoBufNetworkSession> session( new ProtoBufTcpStreamSession(nodeContact) );
        shared_ptr<IProtoBufRequestDispatcher> dispatcher( new ProtoBufRequestNetworkDispatcher(session) );

        LOG(INFO) << "Sending getnodecount request";
        NodeMethodsProtoBufClient client(dispatcher, nullptr);
        size_t colleagueCount = client.GetNodeCount();
        LOG(INFO) << "Get node count " << colleagueCount;
        vector<NodeInfo> randomNodes = client.GetRandomNodes(10, Neighbours::Included);
        LOG(INFO) << "Got " << randomNodes.size() << " random nodes";
        for (const auto &node : randomNodes)
            { LOG(INFO) << "  " << node; }
 
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
        return 1;
    }
}
