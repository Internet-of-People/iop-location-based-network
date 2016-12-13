#include <iostream>
#include <csignal>

#include <easylogging++.h>

#include "config.hpp"
#include "network.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



function<void(int)> mySignalHandlerFunc;

void signalHandler(int signal)
    { mySignalHandlerFunc(signal); }


int main(int argc, const char *argv[])
{
    try
    {
        bool configCreated = Config::Init(argc, argv);
        if (! configCreated)
            { return 1; }
        
        
        NodeInfo myNodeInfo( Config::Instance().myNodeInfo() );
        LOG(INFO) << "Initializing server with node info: " << myNodeInfo;
        
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase(
            Config::Instance().dbPath(), myNodeInfo ) );

        shared_ptr<INodeConnectionFactory> connectionFactory( new TcpStreamConnectionFactory() );
        Node node( myNodeInfo, geodb, connectionFactory, Config::Instance().seedNodes() );

        shared_ptr<IProtoBufRequestDispatcher> dispatcher( new IncomingRequestDispatcher(node) );
        ProtoBufDispatchingTcpServer tcpServer( myNodeInfo.profile().contact(), dispatcher );

        // TODO add thread(s) for periodic discovery and db maintenance
        

        bool ShutdownRequested = false;

        mySignalHandlerFunc = [&ShutdownRequested, &tcpServer] (int)
        {
            ShutdownRequested = true;
            tcpServer.Shutdown();
        };
        
        signal(SIGINT,  signalHandler);
        signal(SIGTERM, signalHandler);
        
        while (! ShutdownRequested)
            { this_thread::sleep_for( chrono::milliseconds(50) ); }
        
        LOG(INFO) << "Finished successfully";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
