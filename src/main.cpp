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
            Config::Instance().dbPath(), myNodeInfo.location() ) );

        shared_ptr<INodeConnectionFactory> connectionFactory( new TcpStreamConnectionFactory() );
        Node node(myNodeInfo, geodb, connectionFactory);

        shared_ptr<IProtoBufRequestDispatcher> dispatcher( new IncomingRequestDispatcher(node) );
        TcpNetwork network( myNodeInfo.profile().contact(), dispatcher );


        bool ShutdownRequested = false;

        mySignalHandlerFunc = [&ShutdownRequested, &network] (int)
        {
            ShutdownRequested = true;
            network.Shutdown();
        };
        
        std::signal(SIGINT,  signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        while (! ShutdownRequested)
            { this_thread::sleep_for( chrono::milliseconds(10) ); }
        
        LOG(INFO) << "Finished successfully";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
