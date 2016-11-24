#include <iostream>

#include "easylogging++.h"
#include "locnet.hpp"
#include "network.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



int main()
{
    try
    {
        LOG(INFO) << "Initializing server";
        
        TcpNetwork network( NetworkInterface(AddressType::Ipv4, "127.0.0.1", 4567) );
        
        LOG(INFO) << "Reading request";
        SyncProtoBufNetworkSession session( network.acceptor() );
        shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session.ReceiveMessage() );
        
        LOG(INFO) << "Serving request";
        unique_ptr<iop::locnet::Response> response;
            // TODO dispatcher.Dispatch( requestMsg->body().request() ) );
        
        LOG(INFO) << "Sending response";
        iop::locnet::MessageWithHeader responseMsg;
        // TODO responseMsg.mutable_body()->set_allocated_response( response.release() );
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
