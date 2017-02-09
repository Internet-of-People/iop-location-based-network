#include <iostream>
#include <string>
#include <thread>

#include <asio.hpp>
#include <easylogging++.h>

#include "config.hpp"
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
        cout << "Usage: sampleclient [host] [port]" << endl;
        
        string   host = argc >= 2 ? argv[1] : "localhost";
        uint16_t port = argc >= 3 ? stoul( argv[2] ) : 16980;
        
        signal(SIGINT,  signalHandler);
        signal(SIGTERM, signalHandler);
        
        Config::InitForTest();
        
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime %level %msg (%fbase:%line)");
        
        const NetworkEndpoint nodeContact(host, port);
        LOG(INFO) << "Connecting to server " << nodeContact;
        shared_ptr<IProtoBufNetworkSession> session( new ProtoBufTcpStreamSession(nodeContact) );
        shared_ptr<IProtoBufRequestDispatcher> dispatcher( new ProtoBufRequestNetworkDispatcher(session) );

//         LOG(INFO) << "Sending getnodecount request";
//         NodeMethodsProtoBufClient client(dispatcher, nullptr);
//         size_t colleagueCount = client.GetNodeCount();
//         LOG(INFO) << "Get node count " << colleagueCount;
//         vector<NodeInfo> randomNodes = client.GetRandomNodes(10, Neighbours::Included);
//         LOG(INFO) << "Got " << randomNodes.size() << " random nodes";
//         for (const auto &node : randomNodes)
//             { LOG(INFO) << "  " << node; }
 
        // Test listening for neighbourhood notifications
        thread msgThread( [session, dispatcher]
        {
            try
            {
                LOG(INFO) << "Sending registerservice request";
                shared_ptr<iop::locnet::Request> registerRequest( new iop::locnet::Request() );
                registerRequest->mutable_localservice()->mutable_registerservice()->set_allocated_service(
                    Converter::ToProtoBuf( ServiceInfo(ServiceType::Profile, 16999, "ProfileServerId") ) );
                unique_ptr<iop::locnet::Response> registerResponse = dispatcher->Dispatch(*registerRequest);
                
                LOG(INFO) << "Sending getneighbournodes request";
                shared_ptr<iop::locnet::Request> neighbourhoodRequest( new iop::locnet::Request() );
                neighbourhoodRequest->mutable_localservice()->mutable_getneighbournodes()->set_keepaliveandsendupdates(true);
                unique_ptr<iop::locnet::Response> neighbourhoodResponse = dispatcher->Dispatch(*neighbourhoodRequest);
                
//                 LOG(INFO) << "Got " << response->localservice().getneighbournodes().nodes_size()
//                           << " neighbours in the reponse";
//                 for (int idx = 0; idx < response->localservice().getneighbournodes().nodes_size(); ++idx)
//                 {
//                     const iop::locnet::NodeInfo &neighbour =
//                         response->localservice().getneighbournodes().nodes(idx);
//                     string buffer;
//                     google::protobuf::TextFormat::PrintToString(neighbour, &buffer);
//                     LOG(INFO) << "  Neighbour " << buffer;
//                 }
                
                uint32_t notificationsReceived = 0;
                while (! ShutdownRequested)
                {
                    if (notificationsReceived >= 1)
                        { ShutdownRequested = true; }
                    
                    LOG(INFO) << "Reading change notification";
                    unique_ptr<iop::locnet::MessageWithHeader> changeNote( session->ReceiveMessage() );
                    if (! changeNote)
                    {
                        LOG(ERROR) << "Received empty message";
                        break;
                    }
                    if ( ! changeNote->has_body() || ! changeNote->body().has_request() ||
                         ! changeNote->body().request().has_localservice() ||
                         ! changeNote->body().request().localservice().has_neighbourhoodchanged() )
                    {
                        LOG(ERROR) << "Received unexpected message";
                        break;
                    }
                    
//                     for (int idx = 0; idx < changeNote->body().request().localservice().neighbourhoodchanged().changes_size(); ++idx)
//                     {
//                         const iop::locnet::NeighbourhoodChange &change =
//                             changeNote->body().request().localservice().neighbourhoodchanged().changes(idx);
//                             
//                         string buffer;
//                         google::protobuf::TextFormat::PrintToString(change, &buffer);
//                         LOG(INFO) << "  Change: " << buffer;
//                     }
                    
                    ++notificationsReceived;
                    LOG(INFO) << "Sending acknowledgement";
                    iop::locnet::MessageWithHeader changeAckn;
                    changeAckn.mutable_body()->mutable_response()->mutable_localservice()->mutable_neighbourhoodupdated();
                    session->SendMessage(changeAckn);
                }
                
                LOG(INFO) << "Sending deregisterservice request";
                shared_ptr<iop::locnet::Request> deregisterRequest( new iop::locnet::Request() );
                deregisterRequest->mutable_localservice()->mutable_deregisterservice()->set_servicetype(
                    Converter::ToProtoBuf(ServiceType::Profile) );
                unique_ptr<iop::locnet::Response> deregisterResponse = dispatcher->Dispatch(*deregisterRequest);
            }
            catch (exception &ex)
            {
                LOG(ERROR) << "Error: " << ex.what();
            }
            LOG(INFO) << "Stopped reading notifications";
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
