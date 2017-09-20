#define CATCH_CONFIG_RUNNER
//#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <easylogging++.h>

#include "config.hpp"
#include "testimpls.hpp"

using namespace std;


INITIALIZE_EASYLOGGINGPP



int main( int argc, const char* const argv[] )
{
    // Disable logging to prevent flooding the console
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Filename, "test.log");
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToFile, "false");
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToStandardOutput, "false");
    
    LocNet::TestConfig::ExecPath = argv[0];
    
    Catch::Session session;
    try { return session.run( argc, argv ); }
    catch (exception &e)
        { cerr << "Caught exception: " << e.what() << endl; }
}
