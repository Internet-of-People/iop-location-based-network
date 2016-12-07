#include <iostream>
#include <csignal>

#include <easylogging++.h>

#include "config.hpp"
#include "network.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



bool ShutdownRequested = false;

void ShutdownSignalHandler(int)
{
    ShutdownRequested = true;
}



unique_ptr<std::thread> StartServerThread()
{
    bool initialized = false;
    thread *serverThread = new thread( [&initialized]()
    {
        try {
            NodeInfo myNodeInfo( Config::Instance().myNodeInfo() );
            LOG(INFO) << "Starting with node info: " << myNodeInfo;
            
            shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase(
                Config::Instance().dbPath(), myNodeInfo.location() ) );

            shared_ptr<INodeConnectionFactory> connectionFactory( new SyncTcpNodeConnectionFactory() );
            Node node(myNodeInfo, geodb, connectionFactory);
            IncomingRequestDispatcher dispatcher(node);
            
            TcpNetwork network( myNodeInfo.profile().contact() );
        
            initialized = true;
            LOG(INFO) << "Server initialized";
            
            while (! ShutdownRequested)
            {
                // TODO CTRL-C should break this loop immediately instead of after finishing next request
                ProtoBufSyncTcpSession session(network);
                
                LOG(DEBUG) << "Reading request";
                shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session.ReceiveMessage() );
                
                LOG(DEBUG) << "Serving request";
                unique_ptr<iop::locnet::Response> response( dispatcher.Dispatch( requestMsg->body().request() ) );
                
                LOG(DEBUG) << "Sending response";
                iop::locnet::MessageWithHeader responseMsg;
                responseMsg.mutable_body()->set_allocated_response( response.release() );
                session.SendMessage(responseMsg);
                
                //session.Close();
            }
        }
        catch (exception &e) {
            LOG(WARNING) << "Exception in server thread: " << e.what();
        }
    } );
    
    while (! initialized)
        { this_thread::sleep_for( chrono::milliseconds(1) ); }
    
    return unique_ptr<thread>(serverThread);
}




int main(int argc, const char *argv[])
{
    try
    {
        std::signal(SIGINT,  ShutdownSignalHandler);
        std::signal(SIGTERM, ShutdownSignalHandler);
        
        bool configCreated = Config::Init(argc, argv);
        if (! configCreated)
            { return 1; }
            
        LOG(INFO) << "Initializing server";
        
        unique_ptr<thread> serverThread = StartServerThread();
        while (! ShutdownRequested)
            { this_thread::sleep_for( chrono::milliseconds(1) ); }
        
        serverThread.reset();
        
        LOG(INFO) << "Finished successfully";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
