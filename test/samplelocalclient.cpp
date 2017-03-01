#include <iostream>
#include <string>
#include <thread>

#include <asio.hpp>
#include <easylogging++.h>

#include "config.hpp"
#include "IopLocNet.pb.h"
#include "network.hpp"

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
        uint16_t port = argc >= 3 ? stoul( argv[2] ) : 16982;
        
        signal(SIGINT,  signalHandler);
        signal(SIGTERM, signalHandler);
        
        Config::InitForTest();
        
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime %level %msg (%fbase:%line)");
        
        const NetworkEndpoint nodeContact(host, port);
        LOG(INFO) << "Connecting to server " << nodeContact;
        shared_ptr<IProtoBufNetworkSession> session( new ProtoBufTcpStreamSession(nodeContact) );
        shared_ptr<IProtoBufRequestDispatcher> dispatcher( new ProtoBufRequestNetworkDispatcher(session) );
 
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
                
                uint32_t requestsSent = 0;
                while (! ShutdownRequested && requestsSent < 3)
                {
                    LOG(INFO) << "Sending getneighbournodes request";
                    shared_ptr<iop::locnet::Request> neighbourhoodRequest( new iop::locnet::Request() );
                    neighbourhoodRequest->mutable_localservice()->mutable_getneighbournodes()->set_keepaliveandsendupdates(true);
                    unique_ptr<iop::locnet::Response> neighbourhoodResponse = dispatcher->Dispatch(*neighbourhoodRequest);
                    if ( ! neighbourhoodResponse->has_localservice() ||
                         ! neighbourhoodResponse->localservice().has_getneighbournodes() )
                    {
                        LOG(ERROR) << "Received unexpected response";
                        break;
                    }
                    LOG(INFO) << "Received getneighbournodes response";
                    ++requestsSent;
                }
                
                uint32_t notificationsReceived = 0;
                while (! ShutdownRequested && notificationsReceived < 2)
                {
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
            ShutdownRequested = true;
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
