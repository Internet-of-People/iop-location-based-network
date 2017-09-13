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
            
            for (auto token : tokens)
                { cout << token << "\t"; }
            cout << endl;
            REQUIRE( tokens.size() == 9 );
        }
        
        THEN("It works fine") {
            //REQUIRE(false);
        }
    }
}
