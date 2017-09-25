#include <fstream>

#include <catch.hpp>
#include <easylogging++.h>

#include "testimpls.hpp"

using namespace std;
using namespace LocNet;


const char PATH_SEPARATOR =
#ifdef _WIN32
    '\\'
#else
    '/'
#endif
;



struct Settlement
{
    Settlement(const string &name, GpsCoordinate latitude, GpsCoordinate longitude, uint32_t population) :
        name(name), location(latitude, longitude), population(population) {}
    
    string      name;
    GpsLocation location;
    uint32_t    population;
};


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



const size_t SEED_NODE_COUNT            = 5;
const size_t NEIGHBOURHOOD_TARGET_SIZE  = 50;
const size_t TOTAL_NODE_COUNT           = 500;

SCENARIO("Conceptual correctness of the algorithm organizing the global network", "[concept]")
{
    GIVEN("A map of the biggest cities")
    {
        vector<Settlement> settlements( LoadWorldCitiesCSV() );

        REQUIRE( settlements.size() > 100 ); // No point of running this test on just a few nodes
        
        vector< shared_ptr<TestConfig> > nodeConfigs;
        for (auto &settlement : settlements)
        {
            //cout << settlement.name << "\t" << settlement.location << "\t" << settlement.population << endl;
            
            NodeInfo nodeInfo( settlement.name, settlement.location,
                NodeContact(settlement.name, 8888, 9999), NodeInfo::Services() );
            shared_ptr<TestConfig> config( new TestConfig(nodeInfo) );
            nodeConfigs.push_back(config);
            
            if (nodeConfigs.size() >= TOTAL_NODE_COUNT)
                { break; }
        }
        
        // Dedicate the first cities as seed nodes
        vector<NetworkEndpoint> seedNodes;
//        seedNodes.emplace_back( nodeConfigs[0]->myNodeInfo().contact().nodeEndpoint() );
        
        shared_ptr<NodeRegistry> proxyFactory( new NodeRegistry() );
        for (auto config : nodeConfigs)
        {
            if ( seedNodes.size() < SEED_NODE_COUNT )
                { seedNodes.push_back( config->myNodeInfo().contact().nodeEndpoint() ); }
            
            config->_seedNodes = seedNodes;
            config->_neighbourhoodTargetSize = NEIGHBOURHOOD_TARGET_SIZE;
            
            // NOTE only 64 in-memory SpatiaLite instances are allowed, we'd have to use temporary DBs for more instances
            shared_ptr<ISpatialDatabase> spatialDb( new InMemorySpatialDatabase( config->myNodeInfo() ) );
                // new SpatiaLiteDatabase( config->myNodeInfo(), SpatiaLiteDatabase::IN_MEMORY_DB, chrono::seconds(1) ) );
            shared_ptr<Node> node = Node::Create(config, spatialDb, proxyFactory);
            proxyFactory->Register(node);
            
            node->EnsureMapFilled();
            
            cout << proxyFactory->nodes().size() << " - " << node->GetNodeInfo() << endl
                 << "  node/seed(s) map size " << node->GetNodeCount() << "/";
            for (auto seedEndpoint : seedNodes)
            {
                shared_ptr<Node> seed = proxyFactory->nodes().at( seedEndpoint.address() );
                cout << seed->GetNodeCount() << " ";
            }
            cout << ", neighbours " << node->GetNeighbourNodesByDistance().size() << endl;
        }
        
        THEN("It works fine") {
            //REQUIRE(false);
        }
    }
}
