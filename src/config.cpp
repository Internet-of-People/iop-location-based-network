#include <easylogging++.h>
#include <ezOptionParser.hpp>

#include "config.hpp"

using namespace std;



namespace LocNet
{



static const uint16_t VERSION_MAJOR = 0;
static const uint16_t VERSION_MINOR = 0;
static const uint16_t VERSION_PATCH = 1;
//static const string   VERSION_TAG   = "a3";

static const string LOCNET_VERSION =
    to_string(VERSION_MAJOR) + "." + to_string(VERSION_MINOR) + "." +
    to_string(VERSION_PATCH); // + "-" + VERSION_TAG;


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


const string& Config::version() const
    { return LOCNET_VERSION; }



static const TcpPort DefaultPort        = 16980;
static const string DEFAULT_PORT_STR    = to_string(DefaultPort);

static const string DESC_OPTIONAL_DEFAULT = "Optional, default value: ";
static const char *DEFAULT_PORT         = DEFAULT_PORT_STR.c_str();
static const char *DEFAULT_CONFIG_FILE  = "iop-locnet.cfg";
static const char *DEFAULT_DBPATH       = "locnet.sqlite";
//const string DBFILE_PATH = ":memory:"; // NOTE in-memory storage without a db file
//const string DBFILE_PATH = "file:locnet.sqlite"; // NOTE this may be any file URL


static const char *OPTNAME_HELP         = "--help";
static const char *OPTNAME_VERSION      = "--version";
static const char *OPTNAME_CONFIGFILE   = "--configfile";
static const char *OPTNAME_NODEID       = "--nodeid";
static const char *OPTNAME_HOST         = "--host";
static const char *OPTNAME_PORT         = "--port";
static const char *OPTNAME_LATITUDE     = "--latitude";
static const char *OPTNAME_LONGITUDE    = "--longitude";
static const char *OPTNAME_SEEDNODE     = "--seednode";

static const char *OPTNAME_DBPATH       = "--dbpath";


const chrono::duration<uint32_t> EzParserConfig::_dbMaintenancePeriod = chrono::hours(6);
const chrono::duration<uint32_t> EzParserConfig::_dbExpirationPeriod = chrono::hours(24);
const chrono::duration<uint32_t> EzParserConfig::_discoveryPeriod = chrono::hours(1);

static const vector<Address> DefaultSeedNodes {
    "ham4.fermat.cloud",
    "ham5.fermat.cloud",
    "ham6.fermat.cloud",
    "ham7.fermat.cloud",
//     NodeProfile( "Fermat1", NetworkInterface(AddressType::Ipv4, "104.155.51.239",  16980) ),
//     NodeProfile( "Fermat2", NetworkInterface(AddressType::Ipv4, "104.199.126.235", 16980) ),
//     NodeProfile( "Fermat3", NetworkInterface(AddressType::Ipv4, "130.211.120.237", 16980) ),
//     NodeProfile( "Fermat4", NetworkInterface(AddressType::Ipv4, "104.199.219.45",  16980) ),
//     NodeProfile( "Fermat5", NetworkInterface(AddressType::Ipv4, "104.196.57.34",   16980) ),
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
        OPTNAME_HELP, "-h" // Flag names
    );
    
    _optParser.add(DEFAULT_CONFIG_FILE, false, 0, 0, "Print version information.",
        OPTNAME_VERSION);
    _optParser.add(DEFAULT_CONFIG_FILE, false, 1, 0, ( "Path to config file to load options from. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_CONFIG_FILE ).c_str(), OPTNAME_CONFIGFILE, "-c");
    _optParser.add("", true, 1, 0, "Public node id, should be an SHA256 hash "
        "of the public key of this node", OPTNAME_NODEID, "-i");
    _optParser.add("", true, 1, 0, "Host address (either DNS, ipv4 or v6) to be used externally "
        "by other nodes or clients to connect to this node", OPTNAME_HOST, "-h");
    _optParser.add(DEFAULT_PORT, false, 1, 0, ( "TCP port number for connecting to this node. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_PORT ).c_str(), OPTNAME_PORT, "-p");
    _optParser.add("", true, 1, 0, "GPS latitude of this server "
        "as real number from range (-90,90)", OPTNAME_LATITUDE, "-y");
    _optParser.add("", true, 1, 0, "GPS longitude of this server "
        "as real number from range (-180,180)", OPTNAME_LONGITUDE, "-x");
    _optParser.add("", false, 1, 0, "Host name of seed node to be used instead of default seeds. "
        "You can repeat this option to define multiple custom seed nodes.", OPTNAME_SEEDNODE, "-s");
    
    _optParser.add(DEFAULT_DBPATH, false, 1, 0, ( "Path to node database. For SQLite, value ':memory:' "
        "creates in-memory database that will not be saved to disk. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_DBPATH ).c_str(), OPTNAME_DBPATH, "-d");
    
    
    _optParser.parse(argc, argv);
    
    string filename;
    _optParser.get(OPTNAME_CONFIGFILE)->getString(filename);
    if ( _optParser.importFile( filename.c_str() ) )
         { cout << "Processed config file " << filename << endl; }
    else { cout << "Config file '" << filename << "' not found, using command line values only" << endl; }
    
    vector<string> badOptions;
    bool validateRequiredPassed = _optParser.gotRequired(badOptions);
    if( ! validateRequiredPassed &&
        ! ( _optParser.isSet(OPTNAME_HELP) || _optParser.isSet(OPTNAME_VERSION) ) )
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
    
    if ( ! _optParser.isSet(OPTNAME_VERSION) &&
         ( _optParser.isSet(OPTNAME_HELP) || ! validateRequiredPassed || ! valueCountPassed ) )
    {
        string usage;
        _optParser.getUsage(usage);
        cerr << endl << usage << endl;
        return false;
    }
    
    _versionRequested = _optParser.isSet(OPTNAME_VERSION);
    _optParser.get(OPTNAME_NODEID)->getString(_id);
    _optParser.get(OPTNAME_HOST)->getString(_ipAddr);
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
    
    // TODO DNS entry also should be enabled as an address, how to handle this here?
    AddressType addrType = _ipAddr.find(':') == string::npos ?
        AddressType::Ipv4 : AddressType::Ipv6;
    _myNodeInfo.reset( new NodeInfo(
        NodeProfile(_id, NetworkInterface(addrType, _ipAddr, _port) ),
        GpsLocation(_latitude, _longitude) ) );
    
    vector<vector<string>> seedOptionVectors;
    _optParser.get(OPTNAME_SEEDNODE)->getMultiStrings(seedOptionVectors);
    for (auto const &seedVector : seedOptionVectors)
        { _seedNodes.push_back( seedVector[0] ); }
    
    return true;
}



bool EzParserConfig::versionRequested() const
    { return _versionRequested; }

const string& EzParserConfig::dbPath() const
    { return _dbPath; }

const NodeInfo& EzParserConfig::myNodeInfo() const
    { return *_myNodeInfo; }

const vector<Address>& EzParserConfig::seedNodes() const
    { return _seedNodes.empty() ? DefaultSeedNodes : _seedNodes; }

TcpPort EzParserConfig::defaultPort() const
    { return DefaultPort; }

chrono::duration< uint32_t > EzParserConfig::dbMaintenancePeriod() const
    { return _dbMaintenancePeriod; }

std::chrono::duration<uint32_t> EzParserConfig::dbExpirationPeriod() const
    { return _dbExpirationPeriod; }

std::chrono::duration<uint32_t> EzParserConfig::discoveryPeriod() const
    { return _discoveryPeriod; }


}