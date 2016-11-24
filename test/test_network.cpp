#include <thread>

#include "asio.hpp"
#include "catch.hpp"
#include "easylogging++.h"
#include "network.hpp"
#include "testimpls.hpp"
#include "testdata.hpp"

using namespace std;
using namespace LocNet;
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
    conn.read( &messageBytes[0] + MessageHeaderSize, bodySize );

    // Deserialize message from receive buffer
    unique_ptr<iop::locnet::MessageWithHeader> message( new iop::locnet::MessageWithHeader() );
    message->ParseFromString(messageBytes);
    return message.release();
}



SCENARIO("TCP networking", "[network]")
{
    GIVEN("A configured Node and Tcp networking")
    {
        shared_ptr<ISpatialDatabase> geodb( new InMemorySpatialDatabase(TestData::Budapest) );
        geodb->Store(TestData::EntryKecskemet);
        geodb->Store(TestData::EntryLondon);
        geodb->Store(TestData::EntryNewYork);
        geodb->Store(TestData::EntryWien);
        geodb->Store(TestData::EntryCapeTown);
        
        shared_ptr<IRemoteNodeConnectionFactory> connectionFactory(
            new DummyRemoteNodeConnectionFactory() );
        Node node( TestData::NodeBudapest, geodb, connectionFactory );
        MessageDispatcher dispatcher(node);
        
        const LocNet::NetworkInterface &BudapestNodeContact(
            LocNet::TestData::NodeBudapest.profile().contacts().front() );
        TcpNetwork network(BudapestNodeContact);
        thread serverThread( [&dispatcher, &network]
        {
            SyncProtoBufNetworkSession session( network.acceptor() );
            shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session.ReceiveMessage() );

            unique_ptr<iop::locnet::Response> response( dispatcher.Dispatch( requestMsg->body().request() ) );
            
            iop::locnet::MessageWithHeader responseMsg;
            responseMsg.mutable_body()->set_allocated_response( response.release() );
            session.SendMessage(responseMsg);
        } );
        
        THEN("It serves clients") {
            tcp::iostream conn( BudapestNodeContact.address(), to_string( BudapestNodeContact.port() ) );
            if( !conn ) {
                throw std::runtime_error("Could not connect to server.");
            }

            iop::locnet::MessageWithHeader requestMsg;
            requestMsg.mutable_body()->mutable_request()->mutable_localservice()->mutable_getneighbournodes();
            requestMsg.mutable_body()->mutable_request()->set_version("1");
            requestMsg.set_header(1);
            requestMsg.set_header( requestMsg.ByteSize() - LocNet::IProtoBufNetworkSession::MessageHeaderSize);
            string reqStr( requestMsg.SerializeAsString() );

            conn << reqStr;
            
            unique_ptr<iop::locnet::MessageWithHeader> msgReceived( ReceiveMessage(conn) );
            const iop::locnet::GetNeighbourNodesByDistanceResponse &response =
                msgReceived->body().response().localservice().getneighbournodes();
            
            REQUIRE( response.nodes_size() == 2 );
            REQUIRE( Converter::FromProtoBuf( response.nodes(0) ) == TestData::NodeKecskemet );
            REQUIRE( Converter::FromProtoBuf( response.nodes(1) ) == TestData::NodeWien );
        }
        
        serverThread.join();
    }
}
