#include <iostream>
#include <string>
#include <thread>

#include <asio.hpp>
#include <easylogging++.h>
#include <google/protobuf/text_format.h>

#include "IopLocNet.pb.h"
#include "network.hpp"
#include "testdata.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace asio::ip;
using namespace LocNet;


bool ShutdownRequested = false;

void signalHandler(int)
    { ShutdownRequested = true; }



int main(int argc, const char* const argv[])
{
    try
    {
        cout << "Usage: sampleclient [port] [host]" << endl;
        
        uint16_t port = argc >= 2 ? stoul( argv[1] ) : 16980;
        string   host = argc >= 3 ? argv[2] : "localhost";
        
        signal(SIGINT,  signalHandler);
        signal(SIGTERM, signalHandler);
        
        const NetworkInterface nodeContact(AddressType::Ipv4, host, port);
        LOG(INFO) << "Connecting to server " << nodeContact;
        shared_ptr<IProtoBufNetworkSession> session( new ProtoBufTcpStreamSession(nodeContact) );

        LOG(INFO) << "Sending getcolleaguenodecount request";
        shared_ptr<IProtoBufRequestDispatcher> dispatcher( new ProtoBufRequestNetworkDispatcher(session) );
        NodeMethodsProtoBufClient client(dispatcher);
        size_t colleagueCount = client.GetNodeCount();
        LOG(INFO) << "Get node count " << colleagueCount;
        vector<NodeInfo> randomNodes = client.GetRandomNodes(10, Neighbours::Included);
        LOG(INFO) << "Got " << randomNodes.size() << " random nodes";
        for (const auto &node : randomNodes)
            { LOG(INFO) << "  " << node; }
        
        thread msgThread( [session, dispatcher]
        {
            try
            {
                LOG(INFO) << "Sending getneighbournodes request";
                iop::locnet::Request request;
                request.mutable_localservice()->mutable_getneighbournodes()->set_keepaliveandsendupdates(true);
                unique_ptr<iop::locnet::Response> response = dispatcher->Dispatch(request);
                
                LOG(INFO) << "Got " << response->localservice().getneighbournodes().nodes_size()
                        << " neighbours in the reponse";
                for (int idx = 0; idx < response->localservice().getneighbournodes().nodes_size(); ++idx)
                {
                    const iop::locnet::NodeInfo &neighbour =
                        response->localservice().getneighbournodes().nodes(idx);
                    string buffer;
                    google::protobuf::TextFormat::PrintToString(neighbour, &buffer);
                    LOG(INFO) << "  Neighbour " << buffer;
                }
                
                while (! ShutdownRequested)
                {
                    LOG(INFO) << "Reading change notification";
                    unique_ptr<iop::locnet::MessageWithHeader> changeNote( session->ReceiveMessage() );
                    if (! changeNote)
                        { break; }
                    
                    for (int idx = 0; idx < changeNote->body().request().localservice().neighbourhoodchanged().changes_size(); ++idx)
                    {
                        const iop::locnet::NeighbourhoodChange &change =
                            changeNote->body().request().localservice().neighbourhoodchanged().changes(idx);
                        string buffer;
                        google::protobuf::TextFormat::PrintToString(change, &buffer);
                        LOG(INFO) << "  Change: " << buffer;
                    }
                    
                    LOG(INFO) << "Sending acknowledgement";
                    iop::locnet::MessageWithHeader changeAckn;
                    changeAckn.mutable_body()->mutable_response()->mutable_localservice()->mutable_neighbourhoodupdated();
                    changeAckn.set_header(1);
                    session->SendMessage(changeAckn);
                }
            }
            catch (exception &ex)
            {
                LOG(ERROR) << "Error: " << ex.what();
            }
            LOG(INFO) << "Stopped to read notifications";
        } );
        msgThread.detach();
        
        while (! ShutdownRequested)
            { this_thread::sleep_for( chrono::milliseconds(50) ); }
        
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
        return 1;
    }
}
