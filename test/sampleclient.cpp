#include <iostream>

#include "easylogging++.h"
#include "asio.hpp"
#include "network.hpp"
#include "IopLocNet.pb.h"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace asio::ip;



const size_t MessageSizeOffset = 1;
const size_t MessageHeaderSize = 5;

uint32_t GetMessageSizeFromHeader(const char *bytes)
{
    const uint8_t *data = reinterpret_cast<const uint8_t*>(bytes);
    return data[MessageSizeOffset + 0] +
          (data[MessageSizeOffset + 1] << 8) +
          (data[MessageSizeOffset + 2] << 16) +
          (data[MessageSizeOffset + 3] << 24);
}


iop::locnet::MessageWithHeader ReceiveMessage(tcp::iostream &conn)
{
    string messageBytes(MessageHeaderSize, 0);
    conn.read( &messageBytes[0], MessageHeaderSize );
    uint32_t bodySize = GetMessageSizeFromHeader( &messageBytes[0] );
    messageBytes.reserve(MessageHeaderSize + bodySize);
    conn.read( &messageBytes[0] + MessageHeaderSize, bodySize );

    iop::locnet::MessageWithHeader message;
    message.ParseFromString(messageBytes);
    return message;
}



int main()
{
    try
    {
        LOG(INFO) << "Connecting to location based network";
        tcp::iostream conn("127.0.0.1", "4567");
        if( !conn ) {
            throw std::runtime_error("Could not connect to server.");
        }

        LOG(INFO) << "Sending getneighbournodes request";
        iop::locnet::MessageWithHeader requestMsg;
        requestMsg.mutable_body()->mutable_request()->mutable_localservice()->mutable_getneighbournodes();
        requestMsg.mutable_body()->mutable_request()->set_version("1");
        requestMsg.set_header(1);
        requestMsg.set_header( requestMsg.ByteSize() - LocNet::IProtoBufNetworkSession::MessageHeaderSize);
        string reqStr( requestMsg.SerializeAsString() );
        LOG(DEBUG) << "Message buffer size: " << reqStr.size();
        
        LOG(INFO) << "Message body length: " << requestMsg.header()
                  << ", version: " << requestMsg.body().request().version()
                  << ", local service operation " << requestMsg.body().request().localservice().LocalServiceRequestType_case();
        conn << reqStr;
        
        LOG(INFO) << "Message sent, reading response";
        iop::locnet::MessageWithHeader msgReceived( ReceiveMessage(conn) );
        LOG(INFO) << "Got " << msgReceived.body().response().client().getneighbournodes().nodes_size()
                  << " neighbours in the reponse";
        
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
        return 1;
    }
}
