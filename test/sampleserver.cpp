#include <iostream>

#include "easylogging++.h"
#include "locnet.hpp"
#include "network.hpp"
#include "testimpls.hpp"
#include "testdata.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



int main()
{
    try
    {
        LOG(INFO) << "Initializing server";
        shared_ptr<ISpatialDatabase> geodb( new InMemorySpatialDatabase(TestData::Budapest) );
        geodb->Store(TestData::EntryKecskemet);
        geodb->Store(TestData::EntryLondon);
        geodb->Store(TestData::EntryNewYork);
        geodb->Store(TestData::EntryWien);
        geodb->Store(TestData::EntryCapeTown);
        
        shared_ptr<IRemoteNodeConnectionFactory> connectionFactory(
            new DummyRemoteNodeConnectionFactory() );
        NodeInfo NodeBudapest( NodeProfile("BudapestId",
            { NetworkInterface(AddressType::Ipv4, "127.0.0.1", 4567) } ), TestData::Budapest );
        Node node( NodeBudapest, geodb, connectionFactory );
        MessageDispatcher dispatcher(node);
        
        TcpNetwork network( NodeBudapest.profile().contacts().front() );
        
        LOG(INFO) << "Reading request";
        SyncProtoBufNetworkSession session( network.acceptor() );
        shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session.ReceiveMessage() );
        LOG(INFO) << "Got request body length: " << requestMsg->header()
                  << ", version " << requestMsg->body().request().version()
                  << ", local service operation: " << requestMsg->body().request().localservice().LocalServiceRequestType_case();
        
        LOG(INFO) << "Serving request";
        unique_ptr<iop::locnet::Response> response( dispatcher.Dispatch( requestMsg->body().request() ) );
        
        LOG(INFO) << "Sending response";
        iop::locnet::MessageWithHeader responseMsg;
        responseMsg.mutable_body()->set_allocated_response( response.release() );
        responseMsg.set_header( responseMsg.body().ByteSize() );
        session.SendMessage(responseMsg);
        
        LOG(INFO) << "Finished successfully";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
