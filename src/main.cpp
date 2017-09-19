#include <iostream>
#include <csignal>

#include "config.hpp"
#include "server.hpp"

#include <easylogging++.h>

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



function<void(int)> mySignalHandlerFunc;

void signalHandler(int signal)
    { mySignalHandlerFunc(signal); }


void reactorLoop(const string &threadName)
{
    LOG(DEBUG) << "Thread " << threadName << " started";
    while ( ! Reactor::Instance().IsShutdown() )
    {
        try
        {
            // NOTE Normally after run_one we should check if the io_service has run out of tasks and stopped,
            //      have to reset it if so. But async_accept is always in the queue, this cannot happen.
            Reactor::Instance().AsioService().run_one();
            LOG(TRACE) << "Thread " << threadName << " succesfully executed a single task";
        }
        catch (exception &ex)
        {
            LOG(WARNING) << "Async operation failed: " << ex.what();
            LOG(DEBUG) << "Thread " << threadName << " failed at a single task";
        }
    }
    LOG(DEBUG) << "Thread " << threadName << " shut down";
}



int main(int argc, const char *argv[])
{
    try
    {
        shared_ptr<Config> config( new EzParserConfig() );
        bool configCreated = config->Initialize(argc, argv);
        if (! configCreated)
            { return 1; }

        if ( config->versionRequested() )
        {
            cout << "Internet of People - Location based network " << config->version() << endl;
            return 0;
        }
        
        
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime %level %msg (%fbase:%line)");
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Filename, config->logPath());
        el::Loggers::reconfigureAllLoggers(el::Level::Trace, el::ConfigurationType::ToStandardOutput, "false");
        
        // Initialize server components
        NodeInfo myNodeInfo( config->myNodeInfo() );
        LOG(INFO) << "Initializing server with node info: " << myNodeInfo;
        
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase(
            myNodeInfo, config->dbPath(), config->dbExpirationPeriod() ) );

        TcpNodeConnectionFactory *connFactPtr = new TcpNodeConnectionFactory(config);
        shared_ptr<INodeProxyFactory> connectionFactory(connFactPtr);
        shared_ptr<Node> node = Node::Create(config, geodb, connectionFactory);

        LOG(INFO) << "Connecting node to the network";
        shared_ptr<IBlockingRequestDispatcherFactory> nodeDispatcherFactory(
            new StaticBlockingDispatcherFactory( shared_ptr<IBlockingRequestDispatcher>(
                new IncomingNodeRequestDispatcher(node) ) ) );
        shared_ptr<DispatchingTcpServer> nodeTcpServer = DispatchingTcpServer::Create(
            myNodeInfo.contact().nodePort(), nodeDispatcherFactory );
        nodeTcpServer->StartListening();
        
        connFactPtr->detectedIpCallback( [node](const Address &addr)
            { node->DetectedExternalAddress(addr); } );
        
        thread mainReactorThread( [] { reactorLoop("ReactorMain"); } );
        node->EnsureMapFilled();

        LOG(INFO) << "Serving local and client interfaces";
        shared_ptr<IBlockingRequestDispatcherFactory> localDispatcherFactory(
            new LocalServiceRequestDispatcherFactory(node) );
        shared_ptr<IBlockingRequestDispatcherFactory> clientDispatcherFactory(
            new StaticBlockingDispatcherFactory( shared_ptr<IBlockingRequestDispatcher>(
                new IncomingClientRequestDispatcher(node) ) ) );
        
        shared_ptr<DispatchingTcpServer> localTcpServer = DispatchingTcpServer::Create(
            config->localServicePort(), localDispatcherFactory );
        shared_ptr<DispatchingTcpServer> clientTcpServer = DispatchingTcpServer::Create(
            myNodeInfo.contact().clientPort(), clientDispatcherFactory );

        localTcpServer->StartListening();
        clientTcpServer->StartListening();
        
        // Set up signal handlers to stop on Ctrl-C and further events
        mySignalHandlerFunc = [] (int) { Reactor::Instance().Shutdown(); };
        signal(SIGINT,  signalHandler);
        signal(SIGTERM, signalHandler);
        
        // start threads for periodic db maintenance (relation renewal and expiration) and discovery
        thread dbMaintenanceThread( [config, node]
        {
            while ( ! Reactor::Instance().IsShutdown() )
            {
                try
                {
                    this_thread::sleep_for( config->dbMaintenancePeriod() );
                    node->RenewNodeRelations();
                    node->ExpireOldNodes();
                }
                catch (exception &ex)
                    { LOG(ERROR) << "Maintenance failed: " << ex.what(); }
            }
        } );
        dbMaintenanceThread.detach();
        
        thread discoveryThread( [config, node]
        {
            while ( ! Reactor::Instance().IsShutdown() )
            {
                try
                {
                    this_thread::sleep_for( config->discoveryPeriod() );
                    node->DiscoverUnknownAreas();
                }
                catch (exception &ex)
                    { LOG(ERROR) << "Periodic discovery failed: " << ex.what(); }
            }
        } );
        discoveryThread.detach();

        mainReactorThread.join();
        
        LOG(INFO) << "Shutting down location-based network";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
