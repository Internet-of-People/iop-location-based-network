#include <iostream>
#include <string>
#include <thread>

#include <asio.hpp>
#include <easylogging++.h>

#include "config.hpp"
#include "IopLocNet.pb.h"
#include "server.hpp"

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
        
        shared_ptr<Config> config( new EzParserConfig() );
        config->InitForTest();
        
        el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime %level %msg (%fbase:%line)");
        
        const NetworkEndpoint nodeContact(host, port);
        LOG(INFO) << "Connecting to server " << nodeContact;
        shared_ptr<IProtoBufChannel> channel( new AsyncProtoBufTcpChannel(nodeContact) );
        shared_ptr<ProtoBufClientSession> session( ProtoBufClientSession::Create(channel) );
        
        uint32_t notificationsReceived = 0;
        session->StartMessageLoop( [&notificationsReceived, config, channel, session]
            ( unique_ptr<iop::locnet::Message> &&requestMsg )
        {
            if ( ! requestMsg ||
                 ! requestMsg->has_request() ||
                 ! requestMsg->request().has_local_service() ||
                 ! requestMsg->request().local_service().has_neighbourhood_changed() )
            {
                LOG(ERROR) << "Received unexpected request";
                return;
            }
            
//             const iop::locnet::NeighbourhoodChangedNotificationRequest &changeNote =
//                 requestMsg->request().localservice().neighbourhoodchanged();
            
            ++notificationsReceived;
            
            unique_ptr<iop::locnet::Message> changeAckn( new iop::locnet::Message() );
            changeAckn->set_id( requestMsg->id() );
            changeAckn->mutable_response()->mutable_local_service()->mutable_neighbourhood_updated();
            LOG(INFO) << "Sending acknowledgement";
            channel->SendMessage( move(changeAckn), [] {} );
            LOG(INFO) << "Sent acknowledgement";

            if (notificationsReceived == 2)
            {
                LOG(INFO) << "Sending deregisterservice request";
                unique_ptr<iop::locnet::Message> deregisterRequest( new iop::locnet::Message() );
                deregisterRequest->mutable_request()->set_version({1,0,0});
                deregisterRequest->mutable_request()->mutable_local_service()->mutable_deregister_service()->set_service_type(
                    Converter::ToProtoBuf(ServiceType::Profile) );
                session->SendRequest( move(deregisterRequest) );
                
                LOG(INFO) << "Shutting down test";
                signalHandler(SIGINT);
            }
        } );
         
        thread reactorThread( []
        {
            while (! ShutdownRequested)
            {
                Reactor::Instance().AsioService().run_one();
                if ( Reactor::Instance().AsioService().stopped() )
                    { Reactor::Instance().AsioService().reset(); }
            }
        } );
 
        try
        {
            shared_ptr<IBlockingRequestDispatcher> dispatcher( new NetworkDispatcher(config, session) );
            LOG(INFO) << "Sending registerservice request";
            unique_ptr<iop::locnet::Request> registerRequest( new iop::locnet::Request() );
            registerRequest->mutable_local_service()->mutable_register_service()->set_allocated_service(
                Converter::ToProtoBuf( ServiceInfo(ServiceType::Profile, 16999, "ProfileServerId") ) );
            unique_ptr<iop::locnet::Response> registerResponse = dispatcher->Dispatch( move(registerRequest) );
            
            uint32_t requestsSent = 0;
            while (! ShutdownRequested && requestsSent < 3)
            {
                LOG(INFO) << "Sending getneighbournodes request";
                unique_ptr<iop::locnet::Request> neighbourhoodRequest( new iop::locnet::Request() );
                neighbourhoodRequest->mutable_local_service()->mutable_get_neighbour_nodes()->set_keep_alive_and_send_updates(true);
                unique_ptr<iop::locnet::Response> neighbourhoodResponse = dispatcher->Dispatch( move(neighbourhoodRequest) );
                if ( ! neighbourhoodResponse->has_local_service() ||
                        ! neighbourhoodResponse->local_service().has_get_neighbour_nodes() )
                {
                    LOG(ERROR) << "Received unexpected response";
                    break;
                }
                LOG(INFO) << "Received getneighbournodes response";
                ++requestsSent;
            }
        }
        catch (exception &ex)
        {
            LOG(ERROR) << "Error: " << ex.what();
        }

        reactorThread.join();
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
        return 1;
    }
}

