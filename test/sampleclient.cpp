#include <iostream>
#include <string>

#include "asio.hpp"
#include "easylogging++.h"
#include "IopLocNet.pb.h"
#include "network.hpp"
#include "testdata.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace asio::ip;
using namespace LocNet;



int main()
{
    try
    {
        LOG(INFO) << "Connecting to location based network";
        const LocNet::NetworkInterface &BudapestNodeContact( LocNet::TestData::NodeBudapest.profile().contacts().front() );
        SyncProtoBufNetworkSession session(BudapestNodeContact);

        LOG(INFO) << "Sending getneighbournodes request";
        iop::locnet::MessageWithHeader requestMsg;
        requestMsg.mutable_body()->mutable_request()->mutable_localservice()->mutable_getneighbournodes();
        requestMsg.mutable_body()->mutable_request()->set_version("1");
        session.SendMessage(requestMsg);
        
        unique_ptr<iop::locnet::MessageWithHeader> msgReceived( session.ReceiveMessage() );
        
        LOG(INFO) << "Got " << msgReceived->body().response().localservice().getneighbournodes().nodes_size()
                  << " neighbours in the reponse";
        
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
        return 1;
    }
}
