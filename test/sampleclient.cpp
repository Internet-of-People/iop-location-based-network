#include <iostream>
#include <string>

#include "easylogging++.h"
#include "asio.hpp"
#include "network.hpp"
#include "IopLocNet.pb.h"
#include "testdata.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace asio::ip;



const size_t MessageSizeOffset = 1;
const size_t MessageHeaderSize = 5;

uint32_t GetMessageSizeFromHeader(const char *bytes)
{
    // Adapt big endian value from network to local format
    const uint8_t *data = reinterpret_cast<const uint8_t*>(bytes);
    return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
}


iop::locnet::MessageWithHeader* ReceiveMessage(tcp::iostream &conn)
{
    // Allocate a buffer for the message header and read it
    string messageBytes(MessageHeaderSize, 0);
    conn.read( &messageBytes[0], MessageHeaderSize );
    
    // Extract message size from the header to know how many bytes to read
    uint32_t bodySize = GetMessageSizeFromHeader( &messageBytes[MessageSizeOffset] );
    
    // Extend buffer to fit remaining message size and read it
    messageBytes.resize(MessageHeaderSize + bodySize, 0);
    LOG(DEBUG) << "Message buffer size: " << messageBytes.size();
    conn.read( &messageBytes[0] + MessageHeaderSize, bodySize );

    // Deserialize message from receive buffer
    unique_ptr<iop::locnet::MessageWithHeader> message( new iop::locnet::MessageWithHeader() );
    message->ParseFromString(messageBytes);
    return message.release();
}



int main()
{
    try
    {
        LOG(INFO) << "Connecting to location based network";
        const LocNet::NetworkInterface &BudapestNodeContact( LocNet::TestData::NodeBudapest.profile().contacts().front() );
        tcp::iostream conn( BudapestNodeContact.address(), to_string( BudapestNodeContact.port() ) );
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
        unique_ptr<iop::locnet::MessageWithHeader> msgReceived( ReceiveMessage(conn) );
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
