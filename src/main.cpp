#include <iostream>
#include <csignal>

#define ELPP_THREAD_SAFE
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
        
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime %level %msg (%fbase:%line)");
        el::Loggers::reconfigureAllLoggers(el::Level::Trace, el::ConfigurationType::ToStandardOutput, "false");
        
        // Initialize server components
        const Config &config( Config::Instance() ); 
        NodeInfo myNodeInfo( config.myNodeInfo() );
        LOG(INFO) << "Initializing server with node info: " << myNodeInfo;
        
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase(
            myNodeInfo, config.dbPath(), config.dbExpirationPeriod() ) );

        shared_ptr<INodeConnectionFactory> connectionFactory( new TcpStreamConnectionFactory() );
        shared_ptr<Node> node( new Node( myNodeInfo, geodb, connectionFactory, config.seedNodes() ) );

        shared_ptr<IProtoBufRequestDispatcherFactory> dispatcherFactory(
            new IncomingRequestDispatcherFactory(node) );
        ProtoBufDispatchingTcpServer tcpServer(
            myNodeInfo.profile().contact(), dispatcherFactory );

        // Set up signal handlers to stop on Ctrl-C and further events
        bool ShutdownRequested = false;
        
        mySignalHandlerFunc = [&ShutdownRequested, &tcpServer] (int)
        {
            ShutdownRequested = true;
            tcpServer.Shutdown();
        };
        
        signal(SIGINT,  signalHandler);
        signal(SIGTERM, signalHandler);
        
        // start threads for periodic db maintenance (relation renewal and expiration) and discovery
        thread dbMaintenanceThread( [&ShutdownRequested, &node, &config]
        {
            while (! ShutdownRequested)
            {
                try
                {
                    this_thread::sleep_for( config.dbMaintenancePeriod() );
                    node->ExpireOldNodes();
                    node->RenewNodeRelations();
                }
                catch (exception &ex)
                    { LOG(ERROR) << "Maintenance thread failed: " << ex.what(); }
            }
        } );
        dbMaintenanceThread.detach();
        
        thread discoveryThread( [&ShutdownRequested, &node, &config]
        {
            while (! ShutdownRequested)
            {
                try
                {
                    this_thread::sleep_for( config.discoveryPeriod() );
                    node->DiscoverUnknownAreas();
                }
                catch (exception &ex)
                    { LOG(ERROR) << "Periodic discovery thread failed: " << ex.what(); }
            }
        } );
        discoveryThread.detach();
        
        // TODO we should do something more useful here instead of polling,
        //      maybe run io_service::run, but don't know if it blocks or behaves with CTRL-C and other signals
        // Avoid exiting from this thread
        while (! ShutdownRequested)
            { this_thread::sleep_for( chrono::milliseconds(50) ); }
        
        LOG(INFO) << "Shutting down location-based network";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
