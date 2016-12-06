#include <iostream>
#include <csignal>

#include "easylogging++.h"
#include "locnet.hpp"
#include "network.hpp"
#include "testimpls.hpp"
#include "testdata.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



bool ShutdownRequested = false;

void ShutdownSignalHandler(int)
{
    ShutdownRequested = true;
}


int main()
{
    std::signal(SIGINT,  ShutdownSignalHandler);
    std::signal(SIGTERM, ShutdownSignalHandler);
    
    try
    {
        LOG(INFO) << "Initializing server";
//        shared_ptr<ISpatialDatabase> geodb( new InMemorySpatialDatabase(TestData::Budapest) );
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase(TestData::Budapest) );
        
// TODO move this commented code section to unit testing
//        LOG(INFO) << "Distance: " << geodb->GetDistanceKm(TestData::Budapest, TestData::Kecskemet);
//         geodb->Store(TestData::EntryKecskemet);
//         geodb->Store(TestData::EntryLondon);
//         geodb->Store(TestData::EntryNewYork);
//         geodb->Store(TestData::EntryWien);
//         geodb->Store(TestData::EntryCapeTown);
//         size_t nodeCount = geodb->GetNodeCount();
//         LOG(INFO) << "Node count: " << nodeCount;
//         vector<NodeInfo> neighbours = geodb->GetNeighbourNodesByDistance();
//         LOG(INFO) << "Neighbours: " << neighbours.size();
//         for (const NodeInfo &node : neighbours)
//             { LOG(DEBUG) << "  Neighbour Id: " << node.profile().id(); }
//         LOG(DEBUG) << ( neighbours[0] == TestData::NodeKecskemet );
//         LOG(DEBUG) << ( neighbours[1] == TestData::NodeWien );
//         geodb->ExpireOldNodes();
//         vector<NodeInfo> nodes = geodb->GetClosestNodesByDistance(
//             TestData::NewYork, 20000, 100, Neighbours::Included);
//         for (const NodeInfo &node : nodes)
//             { LOG(DEBUG) << "  Close node Id: " << node.profile().id(); }
//         shared_ptr<NodeDbEntry> entry = geodb->Load( TestData::NodeKecskemet.profile().id() );
//         LOG(DEBUG) << (entry ? *entry == TestData::EntryKecskemet : false);
//         
//         NodeDbEntry testEntry( NodeInfo( NodeProfile(
//             "TestId", NetworkInterface(AddressType::Ipv4, "127.0.0.1", 1234) ), GpsLocation(1.0, 2.0) ),
//             NodeRelationType::Colleague, NodeContactRoleType::Initiator);
//         geodb->Store(testEntry);
//         shared_ptr<NodeDbEntry> loadedEntry( geodb->Load( testEntry.profile().id() ) );
//         LOG(DEBUG) << (loadedEntry ? *loadedEntry == testEntry : false);
//         NodeDbEntry updatedTestEntry(
//             NodeInfo( testEntry.profile(), GpsLocation(3.0, 4.0) ),
//             NodeRelationType::Neighbour, NodeContactRoleType::Acceptor);
//         geodb->Update(updatedTestEntry);
//         loadedEntry = geodb->Load( testEntry.profile().id() );
//         LOG(DEBUG) << (loadedEntry ? *loadedEntry == updatedTestEntry : false);
//         LOG(DEBUG) << (geodb->GetNodeCount() == nodeCount + 1);
//         geodb->Remove( testEntry.profile().id() );
//         LOG(DEBUG) << (geodb->GetNodeCount() == nodeCount);
//         vector<NodeDbEntry> entries = geodb->GetNodes(NodeContactRoleType::Initiator);
//         for (const NodeDbEntry &entry : entries)
//             { LOG(DEBUG) << "  Entry id: " << entry.profile().id(); }
        
        shared_ptr<INodeConnectionFactory> connectionFactory(
            new DummyNodeConnectionFactory() );
        Node node( TestData::NodeBudapest, geodb, connectionFactory );
        IncomingRequestDispatcher dispatcher(node);
        
        const LocNet::NetworkInterface &BudapestNodeContact(
            LocNet::TestData::NodeBudapest.profile().contact() );
        TcpNetwork network(BudapestNodeContact);
        
        while (! ShutdownRequested)
        {
            // TODO CTRL-C should break this loop immediately instead of after finishing next request
            ProtoBufSyncTcpSession session(network);
            
            LOG(INFO) << "Reading request";
            shared_ptr<iop::locnet::MessageWithHeader> requestMsg( session.ReceiveMessage() );
            
            LOG(INFO) << "Serving request";
            unique_ptr<iop::locnet::Response> response( dispatcher.Dispatch( requestMsg->body().request() ) );
            
            LOG(INFO) << "Sending response";
            iop::locnet::MessageWithHeader responseMsg;
            responseMsg.mutable_body()->set_allocated_response( response.release() );
            session.SendMessage(responseMsg);
            
            session.Close();
        }
        
        LOG(INFO) << "Finished successfully";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
