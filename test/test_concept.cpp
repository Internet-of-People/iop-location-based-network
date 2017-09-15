#include <fstream>

#include <catch.hpp>
#include <easylogging++.h>

#include "config.hpp"
#include "testdata.hpp"
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


SCENARIO("Conceptual correctness of the algorithm organizing the global network", "[concept]")
{
    GIVEN("A map of the biggest cities")
    {
        // CSV input is in the same file as the test executable
        const string &execPath = Config::Instance().execPath();
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
        
        shared_ptr<NodeRegistry> proxyFactory = shared_ptr<NodeRegistry>( new NodeRegistry() );
        for (auto &settlement : settlements)
        {
            //cout << settlement.name << "\t" << settlement.location << "\t" << settlement.population << endl;

            NodeInfo nodeInfo( settlement.name, settlement.location,
                NodeContact(settlement.name, 8888, 9999), NodeInfo::Services() );
            shared_ptr<ISpatialDatabase> spatialDb = shared_ptr<ISpatialDatabase>(
                new InMemorySpatialDatabase(nodeInfo) );
            // TODO check if many in-memory SpatiaLite instances can be unique with acceptable memory requirement
            //    new SpatiaLiteDatabase(nodeInfo, SpatiaLiteDatabase::IN_MEMORY_DB, chrono::seconds(1) ) );
            shared_ptr<Node> node = Node::Create(spatialDb, proxyFactory);
            proxyFactory->Register(node);
        }
        
        for ( auto node : proxyFactory->nodes() )
        {
            auto const &nodeInfo = node.second->GetNodeInfo();
            cout << nodeInfo << ", map size: " << node.second->GetNodeCount() << endl;
        }
        
        THEN("It works fine") {
            //REQUIRE(false);
        }
    }
}
