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


int main(int argc, const char *argv[])
{
    try
    {
        bool configCreated = Config::Initialize(argc, argv);
        if (! configCreated)
            { return 1; }
            
        LOG(INFO) << "Initializing server";
        
        std::signal(SIGINT,  ShutdownSignalHandler);
        std::signal(SIGTERM, ShutdownSignalHandler);
        
        NodeInfo myNodeInfo( Config::Instance().myNodeInfo() );
cout << "Parsed node info: " << myNodeInfo << endl;
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase( myNodeInfo.location() ) );

//         shared_ptr<INodeConnectionFactory> connectionFactory( new DummyNodeConnectionFactory() );
//         Node node(myNodeInfo, geodb, connectionFactory);
//         IncomingRequestDispatcher dispatcher(node);
//         
//         const NetworkInterface &BudapestNodeContact( myNodeInfo.profile().contact() );
//         TcpNetwork network(BudapestNodeContact);
//         
//         while (! ShutdownRequested)
//         {
//             // TODO CTRL-C should break this loop immediately instead of after finishing next request
//             ProtoBufSyncTcpSession session(network);
//             
//             LOG(INFO) << "Reading request";
//             shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session.ReceiveMessage() );
//             
//             LOG(INFO) << "Serving request";
//             unique_ptr<iop::locnet::Response> response( dispatcher.Dispatch( requestMsg->body().request() ) );
//             
//             LOG(INFO) << "Sending response";
//             iop::locnet::MessageWithHeader responseMsg;
//             responseMsg.mutable_body()->set_allocated_response( response.release() );
//             session.SendMessage(responseMsg);
//             
//             session.Close();
//         }
        
        LOG(INFO) << "Finished successfully";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
