#include <iostream>
#include <csignal>

#include "easylogging++.h"
#include "locnet.hpp"
#include "network.hpp"
#include "testimpls.hpp"
#include "testdata.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



bool ShutdownRequested = false;

void ShutdownSignalHandler(int)
{
    ShutdownRequested = true;
}


int main()
{
    std::signal(SIGINT,  ShutdownSignalHandler);
    std::signal(SIGTERM, ShutdownSignalHandler);
    
    try
    {
        LOG(INFO) << "Initializing server";
//        shared_ptr<ISpatialDatabase> geodb( new InMemorySpatialDatabase(TestData::Budapest) );
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase(TestData::Budapest) );
        geodb->Store(TestData::EntryKecskemet);
        geodb->Store(TestData::EntryLondon);
        geodb->Store(TestData::EntryNewYork);
        geodb->Store(TestData::EntryWien);
        geodb->Store(TestData::EntryCapeTown);
        
        shared_ptr<INodeConnectionFactory> connectionFactory(
            new DummyNodeConnectionFactory() );
        Node node( TestData::NodeBudapest, geodb, connectionFactory );
        IncomingRequestDispatcher dispatcher(node);
        
        const LocNet::NetworkInterface &BudapestNodeContact(
            LocNet::TestData::NodeBudapest.profile().contact() );
        TcpNetwork network(BudapestNodeContact);
        
        while (! ShutdownRequested)
        {
            // TODO CTRL-C should break this loop immediately instead of after finishing next request
            ProtoBufSyncTcpSession session(network);
            
            LOG(INFO) << "Reading request";
            shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session.ReceiveMessage() );
            
            LOG(INFO) << "Serving request";
            unique_ptr<iop::locnet::Response> response( dispatcher.Dispatch( requestMsg->body().request() ) );
            
            LOG(INFO) << "Sending response";
            iop::locnet::MessageWithHeader responseMsg;
            responseMsg.mutable_body()->set_allocated_response( response.release() );
            session.SendMessage(responseMsg);
            
            session.Close();
        }
        
        LOG(INFO) << "Finished successfully";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
