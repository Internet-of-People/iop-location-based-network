#include <easylogging++.h>
#include <ezOptionParser.hpp>

#include "config.hpp"

using namespace std;



namespace LocNet
{


unique_ptr<Config> Config::_instance(nullptr);

const Config& Config::Instance()
{
    if (! _instance)
    {
        LOG(ERROR) << "Implementation error: config was not properly initialized. Call Config::Init()!";
        throw runtime_error("Config is uninitialized");
    }
    return *_instance;
}


bool Config::Init(int argc, const char* argv[])
{
    _instance.reset( new EzParserConfig() );
    return _instance->Initialize(argc, argv);
}




static const char *DEFAULT_CONFIG_FILE = "iop-locnet.cfg";
static const char *OPTNAME_HELP        = "--help";
static const char *OPTNAME_CONFIGFILE  = "--configfile";
static const char *OPTNAME_NODEID      = "--nodeid";
static const char *OPTNAME_HOSTNAME    = "--hostname";
static const char *OPTNAME_PORT        = "--port";
static const char *OPTNAME_LATITUDE    = "--latitude";
static const char *OPTNAME_LONGITUDE   = "--longitude";



bool EzParserConfig::Initialize(int argc, const char *argv[])
{
    ez::ezOptionParser _optParser;
    
    _optParser.overview = "Internet of People : Location Based Network";
    _optParser.syntax   = "iop-locnetd [OPTIONS]";
//     _optParser.example  = "iop-locnetd -h\n\n";
//     _optParser.footer   = "iop-locnetd v0.0.1\n";
    
    _optParser.add(
        "",             // Default value
        false,          // Is this a mandatory option?
        0,              // Number of expected values
        0,              // Delimiter character if expecting multiple values
        "Show usage",   // Help message for this option
        "-h", OPTNAME_HELP // Flag names
    );
    
    _optParser.add(DEFAULT_CONFIG_FILE, false, 1, 0,
        "Config filename to also load options from", "-c", OPTNAME_CONFIGFILE);
    _optParser.add("", true, 1, 0,
        "Public node id", "-i", OPTNAME_NODEID);
    _optParser.add("", false, 1, 0,
        "IP address (either v4 or v6) used for serving other nodes and clients", "-h", OPTNAME_HOSTNAME);
    _optParser.add("", true, 1, 0,
        "TCP port number used for serving other nodes and clients", "-p", OPTNAME_PORT);
    _optParser.add("", true, 1, 0, "Latitude part from GPS location of this server", "-y", OPTNAME_LATITUDE);
    _optParser.add("", true, 1, 0, "Longitude part form GPS location of this server", "-x", OPTNAME_LONGITUDE);
    
    
    _optParser.parse(argc, argv);
    
    string filename;
    _optParser.get(OPTNAME_CONFIGFILE)->getString(filename);
    if ( _optParser.importFile( filename.c_str() ) )
         { cout << "Processed config file " << filename << endl; }
    else { cout << "No config file found with name " << filename << ", using command line values only" << endl; }
    
    vector<string> badOptions;
    bool requiredPassed = _optParser.gotRequired(badOptions);
    if(! requiredPassed)
    {
        for(size_t idx = 0; idx < badOptions.size(); ++idx)
            { cerr << "Missing required option " << badOptions[idx] << endl; }
    }
    
    bool valueCountPassed = _optParser.gotExpected(badOptions);
//     if(! valueCountPassed)
//     {
//         for(size_t idx = 0; idx < badOptions.size(); ++idx)
//             { LOG(ERROR) << "Wrong number of values for option " << badOptions[idx]; }
//     }
    
    if ( _optParser.isSet(OPTNAME_HELP) || ! requiredPassed || ! valueCountPassed )
    {
        string usage;
        _optParser.getUsage(usage);
        cerr << endl << usage << endl;
        return false;
    }
    
    _optParser.get(OPTNAME_NODEID)->getString(_id);
    _optParser.get(OPTNAME_HOSTNAME)->getString(_ipAddr);
    _optParser.get(OPTNAME_LATITUDE)->getFloat(_latitude);
    _optParser.get(OPTNAME_LONGITUDE)->getFloat(_longitude);
    
    unsigned long port;
    _optParser.get(OPTNAME_PORT)->getULong(port);
    _port = port;
    
//     cout << "----- Pretty print" << endl;
//     string pretty;
//     _optParser.prettyPrint(pretty);
//     cout << pretty;
//     cout << "-----" << endl;
    
    return true;
}



NodeInfo EzParserConfig::myNodeInfo() const
{
    // TODO differentiate between ipv4 and v6 addresses
    return NodeInfo(
        NodeProfile(_id, NetworkInterface(AddressType::Ipv4, _ipAddr, _port) ),
        GpsLocation(_latitude, _longitude) );
}



}