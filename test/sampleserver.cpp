#include <iostream>
#include <csignal>

#include <easylogging++.h>

#include "config.hpp"
#include "network.hpp"
#include "testimpls.hpp"
#include "testdata.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



function<void(int)> mySignalHandlerFunc;

void signalHandler(int signal)
    { mySignalHandlerFunc(signal); }


int main()
{
    try
    {
        Config::InitForTest();
        
        LOG(INFO) << "Initializing server";
        shared_ptr<ISpatialDatabase> geodb( new SpatiaLiteDatabase( TestData::NodeBudapest,
            SpatiaLiteDatabase::IN_MEMORY_DB, chrono::hours(1) ) );
        geodb->Store(TestData::EntryKecskemet);
        geodb->Store(TestData::EntryLondon);
        geodb->Store(TestData::EntryNewYork);
        geodb->Store(TestData::EntryWien);
        geodb->Store(TestData::EntryCapeTown);
        
// TODO move this commented code section to unit testing
//        LOG(INFO) << "Distance: " << geodb->GetDistanceKm(TestData::Budapest, TestData::Kecskemet);
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
        shared_ptr<Node> node( new Node(geodb, connectionFactory) );
        node->EnsureMapFilled();
        
        const NodeContact &BudapestNodeContact( TestData::NodeBudapest.contact() );
        
        shared_ptr<IProtoBufRequestDispatcherFactory> nodeDispatcherFactory(
            new StaticDispatcherFactory( shared_ptr<IProtoBufRequestDispatcher>(
                new IncomingNodeRequestDispatcher(node) ) ) );
        shared_ptr<IProtoBufRequestDispatcherFactory> clientDispatcherFactory(
            new StaticDispatcherFactory( shared_ptr<IProtoBufRequestDispatcher>(
                new IncomingClientRequestDispatcher(node) ) ) );
        
        ProtoBufDispatchingTcpServer nodeTcpServer(
            BudapestNodeContact.nodePort(), nodeDispatcherFactory );
        ProtoBufDispatchingTcpServer clientTcpServer(
            BudapestNodeContact.clientPort(), clientDispatcherFactory );
        
        bool ShutdownRequested = false;

        mySignalHandlerFunc = [&ShutdownRequested, &nodeTcpServer, &clientTcpServer] (int)
        {
            ShutdownRequested = true;
            IoService::Instance().Shutdown();
        };
        
        std::signal(SIGINT,  signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        while (! ShutdownRequested)
            { IoService::Instance().Server().run_one(); }
        
        LOG(INFO) << "Finished successfully";
        return 0;
    }
    catch (exception &e)
    {
        LOG(ERROR) << "Failed with exception: " << e.what();
    }
}
