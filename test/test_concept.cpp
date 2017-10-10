#include <fstream>

#include <catch.hpp>
#include <easylogging++.h>

#include "testimpls.hpp"

using namespace std;
using namespace LocNet;



struct Settlement
{
    Settlement(const string &name, GpsCoordinate latitude, GpsCoordinate longitude, uint32_t population) :
        name(name), location(latitude, longitude), population(population) {}
    
    string      name;
    GpsLocation location;
    uint32_t    population;
};



const char PATH_SEPARATOR =
#ifdef _WIN32
    '\\'
#else
    '/'
#endif
;

vector<Settlement> LoadWorldCitiesCSV()
{
    // CSV input is in the same file as the test executable
    const string &execPath = TestConfig::ExecPath;
    int pos = execPath.rfind(PATH_SEPARATOR);
    string execDir( execPath.substr(0, pos+1) ); // if not found, pos will be -1, substr will be empty
    
    ifstream citiesCsvFile( execDir + PATH_SEPARATOR + "worldcities.csv");
    REQUIRE( citiesCsvFile.is_open() );

    // Skip first (header) line
    string csvLine;
    REQUIRE( getline(citiesCsvFile, csvLine) );
    
    // Process entries from CSV file
    vector<Settlement> settlements;
    while( getline(citiesCsvFile, csvLine) )
    {
        stringstream lineStream(csvLine);
        vector<string> tokens;
        
        string token;
        while ( lineStream.good() )
        {
            if (lineStream.peek() != '"')
            {
                // Read value until next comma
                if ( getline(lineStream, token, ',') )
                    { tokens.push_back(token); }
            }
            else
            {
                // Respect commas inside quotes
                getline(lineStream, token, '"');
                if ( getline(lineStream, token, '"') )
                {
                    tokens.push_back(token);
                    getline(lineStream, token, ','); // consume comma after quotes
                }
            }
        }
        
        //REQUIRE( tokens.size() == 9 ); // Unnecessarily creates thousands of assertions included in the reports
        const string &city       = tokens[1];
        GpsCoordinate latitude   = stof( tokens[2] );
        GpsCoordinate longitude  = stof( tokens[3] );
        uint32_t      population = stoul( tokens[4] );
        const string &country    = tokens[5];
        const string &region     = tokens[8].substr(0, tokens[8].size() - 1); // Cut \r
        
        string settlementName = city + "(" + country + "," + region + ")";
        settlements.emplace_back(settlementName, latitude, longitude, population);
    }
    
    return settlements;
}



struct TestCase
{
    size_t _maxNodeCount;
    size_t _seedCount;
    size_t _maxNeighbourCount;
    
    TestCase(size_t maxNodeCount, size_t seedCount, size_t maxNeighbourCount) :
        _maxNodeCount(maxNodeCount), _seedCount(seedCount), _maxNeighbourCount(maxNeighbourCount) {}
};

ostream& operator<<(ostream &out, const TestCase testCase)
    { return out << "test case (maxNodes: " << testCase._maxNodeCount << ", seeds: " << testCase._seedCount << ", neighbours: " << testCase._maxNeighbourCount << ")"; }


vector< shared_ptr<TestConfig> > createOneConfigByCity(
    const vector<Settlement> &settlements, const TestCase &testCase)
{
    vector< shared_ptr<TestConfig> > nodeConfigs;
    for (auto &settlement : settlements)
    {
        //cout << settlement.name << "\t" << settlement.location << "\t" << settlement.population << endl;
        
        NodeInfo nodeInfo( settlement.name, settlement.location,
            NodeContact(settlement.name, 8888, 9999), NodeInfo::Services() );
        shared_ptr<TestConfig> config( new TestConfig(nodeInfo) );
        nodeConfigs.push_back(config);
        
        if ( nodeConfigs.size() >= testCase._maxNodeCount )
            { break; }
    }
    return nodeConfigs;
}


vector< shared_ptr<TestConfig> > createConfigsByPopulation(
    const vector<Settlement> &settlements, const TestCase &testCase, size_t populationByNode)
{
    vector<Settlement> cities(settlements);
    
    vector< shared_ptr<TestConfig> > nodeConfigs;
    for (auto &city : cities)
    {
        size_t nodeUniqueIndex = 0;
        do
        {
            //cout << settlement.name << "\t" << settlement.location << "\t" << settlement.population << endl;
            
            ++nodeUniqueIndex;
            NodeInfo nodeInfo( city.name + "-" + to_string(nodeUniqueIndex), city.location,
                NodeContact(city.name + "-" + to_string(nodeUniqueIndex), 8888, 9999), NodeInfo::Services() );
            shared_ptr<TestConfig> config( new TestConfig(nodeInfo) );
            nodeConfigs.push_back(config);
            
            if (city.population > populationByNode)
                { city.population -= populationByNode; }
        }
        while ( city.population > populationByNode &&
                nodeConfigs.size() < testCase._maxNodeCount );
        
        if ( nodeConfigs.size() >= testCase._maxNodeCount )
            { break; }
    }
    return nodeConfigs;
}



void testNodes( const vector< shared_ptr<TestConfig> > &nodeConfigs,
                const TestCase &testCase )
{
    vector<NetworkEndpoint> seedNodes;
    shared_ptr<TestClock> testClock( new TestClock() );
    shared_ptr<NodeRegistry> proxyFactory( new NodeRegistry() );
    for (auto config : nodeConfigs)
    {
        // Dedicate the first cities as seed nodes
        bool isSeed = false;
        if ( seedNodes.size() < testCase._seedCount )
        {
            isSeed = true;
            seedNodes.push_back( config->myNodeInfo().contact().nodeEndpoint() );
        }
        
        config->_seedNodes = seedNodes;
        config->_neighbourhoodTargetSize = testCase._maxNeighbourCount;
        
        shared_ptr<ISpatialDatabase> spatialDb(
            new InMemorySpatialDatabase( config->myNodeInfo(), testClock, config->dbExpirationPeriod() ) );
            // NOTE only 64 in-memory SpatiaLite instances are allowed, use temporary DBs on disk for more
            // new SpatiaLiteDatabase( config->myNodeInfo(), SpatiaLiteDatabase::IN_MEMORY_DB, chrono::seconds(1) ) );
        shared_ptr<Node> node = Node::Create(config, spatialDb, proxyFactory);
        proxyFactory->Register(node);
     
//        cout << proxyFactory->nodes().size() << " - " << node->GetNodeInfo() << endl;
        
        node->EnsureMapFilled();
        
//         cout << "  node/seed(s) map size " << node->GetNodeCount() << "/";
//         for (auto seedEndpoint : seedNodes)
//         {
//             shared_ptr<Node> seed = proxyFactory->nodes().at( seedEndpoint.address() );
//             cout << seed->GetNodeCount() << " ";
//         }
//         cout << ", neighbours " << node->GetNeighbourNodesByDistance().size() << endl;
        
        if (! isSeed)
            { REQUIRE( node->GetNodeCount() > testCase._seedCount ); }
    }

    THEN("Nodes keep their node relations alive")
    {
        // Elapse some time but node relations must not expire yet
        testClock->elapse(TestConfig::DbExpirationPeriod / 3);
        for ( auto &entry : proxyFactory->nodes() )
            { entry.second->RenewNodeRelations(); }
        
        // Elapse more time to expire all entries that not were renewed
        testClock->elapse(TestConfig::DbExpirationPeriod);
        for ( auto &entry : proxyFactory->nodes() )
        {
            entry.second->ExpireOldNodes();
            REQUIRE( entry.second->GetNodeCount() > testCase._seedCount );
        }
    }
    
    // TODO somehow test if a splitted network can rejoin
}



SCENARIO("Conceptual correctness of the algorithm organizing the global network", "[concept]")
{
    GIVEN("A map of the biggest cities")
    {
        vector<Settlement> settlements( LoadWorldCitiesCSV() );

        vector<TestCase> testCases = {
            TestCase(   10,  1,   3),
            TestCase(   50,  2,  10),
            TestCase(  100,  3,  15),
            TestCase(  200,  4,  20),
//            TestCase(  500,  5,  30), // TODO fix this: Mariehamn(Aland,Finström) fails with this neighbourhood size
//            TestCase( 1000,  8,  40),
//            TestCase( 2000,  9,  50),
//            TestCase( 5000, 10,  60),
//            TestCase(10000, 15, 100),
        };

        for (auto const &testCase : testCases)
        {
            WHEN("Node GPS locations are unique for " + to_string(testCase._maxNodeCount) + " nodes")
            {
                cout // << endl << endl << endl << endl 
                     << "Running network simulation with unique nodes, " << testCase << endl;
                auto nodeConfigs = createOneConfigByCity(settlements, testCase);
                testNodes(nodeConfigs, testCase);
            }
            
            WHEN("Nodes with duplicate positions for " + to_string(testCase._maxNodeCount) + " nodes")
            {
                cout // << endl << endl << endl << endl 
                     << "Running network simulation with redundant nodes, " << testCase << endl;
                auto nodeConfigs = createConfigsByPopulation(settlements, testCase, 1000000);
                testNodes(nodeConfigs, testCase);
            }
        }
    }
}
