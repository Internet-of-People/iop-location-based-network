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



static const string DESC_OPTIONAL_DEFAULT = "Optional, default value: ";

static const char *DEFAULT_CONFIG_FILE  = "iop-locnet.cfg";
static const char *DEFAULT_PORT         = "6371";
static const char *DEFAULT_DBPATH       = "locnet.sqlite";
//const string DBFILE_PATH = ":memory:"; // NOTE in-memory storage without a db file
//const string DBFILE_PATH = "file:locnet.sqlite"; // NOTE this may be any file URL


static const char *OPTNAME_HELP         = "--help";
static const char *OPTNAME_CONFIGFILE   = "--configfile";
static const char *OPTNAME_NODEID       = "--nodeid";
static const char *OPTNAME_IPADDRESS    = "--ipaddress";
static const char *OPTNAME_PORT         = "--port";
static const char *OPTNAME_LATITUDE     = "--latitude";
static const char *OPTNAME_LONGITUDE    = "--longitude";

static const char *OPTNAME_DBPATH       = "--dbpath";



const vector<NodeProfile> EzParserConfig::_seedNodes {
    // TODO put some real seed nodes in here
    NodeProfile( "BudapestSeedNodeId", NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6371) ),
//  NodeProfile( NodeId("WienSeedNodeId"), NetworkInterface(AddressType::Ipv4, "127.0.0.1", 6372) ),
};



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
    
    _optParser.add(DEFAULT_CONFIG_FILE, false, 1, 0, ( "Path to config file to load options from. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_CONFIG_FILE ).c_str(), "-c", OPTNAME_CONFIGFILE);
    _optParser.add("", true, 1, 0, "Public node id, should be an SHA256 hash "
        "of the public key of this node", "-i", OPTNAME_NODEID);
    _optParser.add("", true, 1, 0, "IP address (either ipv4 or v6) of network interface "
        "that can be used to connect to this node", "-a", OPTNAME_IPADDRESS);
    _optParser.add(DEFAULT_PORT, false, 1, 0, ( "TCP port number for connecting to this node. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_PORT ).c_str(), "-p", OPTNAME_PORT);
    _optParser.add("", true, 1, 0, "GPS latitude of this server "
        "as real number from range (-90,90)", "-y", OPTNAME_LATITUDE);
    _optParser.add("", true, 1, 0, "GPS longitude of this server "
        "as real number from range (-180,180)", "-x", OPTNAME_LONGITUDE);
    
    _optParser.add(DEFAULT_DBPATH, false, 1, 0, ( "Path to node database. For SQLite, value ':memory:' "
        "creates in-memory database that will not be saved to disk. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_DBPATH ).c_str(), "-d", OPTNAME_DBPATH);
    
    
    _optParser.parse(argc, argv);
    
    string filename;
    _optParser.get(OPTNAME_CONFIGFILE)->getString(filename);
    if ( _optParser.importFile( filename.c_str() ) )
         { cout << "Processed config file " << filename << endl; }
    else { cout << "Config file '" << filename << "' not found, using command line values only" << endl; }
    
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
    _optParser.get(OPTNAME_IPADDRESS)->getString(_ipAddr);
    _optParser.get(OPTNAME_LATITUDE)->getFloat(_latitude);
    _optParser.get(OPTNAME_LONGITUDE)->getFloat(_longitude);
    _optParser.get(OPTNAME_DBPATH)->getString(_dbPath);
    
    unsigned long port;
    _optParser.get(OPTNAME_PORT)->getULong(port);
    _port = port;
    
//     cout << "----- Pretty print" << endl;
//     string pretty;
//     _optParser.prettyPrint(pretty);
//     cout << pretty;
//     cout << "-----" << endl;
    
    
    AddressType addrType = _ipAddr.find(':') == string::npos ?
        AddressType::Ipv4 : AddressType::Ipv6;
    _myNodeInfo.reset( new NodeInfo(
        NodeProfile(_id, NetworkInterface(addrType, _ipAddr, _port) ),
        GpsLocation(_latitude, _longitude) ) );
    
    return true;
}



const string& EzParserConfig::dbPath() const
    { return _dbPath; }

const NodeInfo& EzParserConfig::myNodeInfo() const
    { return *_myNodeInfo; }

const vector<NodeProfile>& EzParserConfig::seedNodes() const
    { return _seedNodes; }



}