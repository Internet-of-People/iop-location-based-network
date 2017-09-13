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
    GIVEN("A network")
    {
        // CSV input is in the same file as the test executable
        const string &execPath = Config::Instance().execPath();
        int pos = execPath.rfind(PATH_SEPARATOR);
        string execDir( execPath.substr(0, pos+1) );
        
        ifstream citiesCsvFile( execDir + PATH_SEPARATOR + "worldcities.csv");
        REQUIRE( citiesCsvFile.is_open() );

        string line;
        while( getline(citiesCsvFile, line) )
        {
            // TODO
        }
        
        THEN("It works fine") {
            //REQUIRE(false);
        }
    }
}
