#include <cstdlib>

#ifdef _WIN32
  #include <windows.h>
  #include <shlobj.h>
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <pwd.h>
#endif

#include <easylogging++.h>
#include <ezOptionParser.hpp>

#include "config.hpp"

using namespace std;



namespace LocNet
{


static const uint16_t VERSION_MAJOR = 1;
static const uint16_t VERSION_MINOR = 0;
static const uint16_t VERSION_PATCH = 0;
static const string   RELEASE_STATE = "alpha";

static const string LOCNET_VERSION = to_string(VERSION_MAJOR) + "." +
    to_string(VERSION_MINOR) + "." + to_string(VERSION_PATCH) + "-" + RELEASE_STATE;

static const size_t NEIGHBOURHOOD_TARGET_SIZE = 50;

const chrono::duration<uint32_t> EzParserConfig::_dbMaintenancePeriod = chrono::hours(7);
const chrono::duration<uint32_t> EzParserConfig::_dbExpirationPeriod  = chrono::hours(24);
const chrono::duration<uint32_t> EzParserConfig::_discoveryPeriod     = chrono::minutes(5);



unique_ptr<Config> Config::_instance(nullptr);

const Config& Config::Instance()
{
    if (! _instance)
    {
        LOG(ERROR) << "Implementation error: config was not properly initialized. Call Config::Init()!";
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Config is uninitialized");
    }
    return *_instance;
}


bool Config::Init(int argc, const char* argv[])
{
    _instance.reset( new EzParserConfig() );
    return _instance->Initialize(argc, argv);
}


void Config::InitForTest()
{
    _instance.reset( new EzParserConfig() );
    _instance->_testMode = true;
    
    int argc = 7;
    const char *argv[] = {
        "test",
        "--nodeid", "TestNodeId",
        "--latitude", "0.0",
        "--longitude", "0.0",
    };
    _instance->Initialize(argc, argv);
}


Config::Config() {}

const string& Config::version() const
    { return LOCNET_VERSION; }

bool Config::isTestMode() const
    { return _testMode; }

size_t Config::neighbourhoodTargetSize() const
    { return isTestMode() ? 3 : NEIGHBOURHOOD_TARGET_SIZE; }



#ifdef _WIN32

string GetWindowsDirectory(int folderId = CSIDL_APPDATA, bool createDir = true)
{
    char path[MAX_PATH] = "";
    if ( SHGetSpecialFolderPathA(NULL, path, folderId, createDir) )
        { return path; }

    throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to get Windows user app directory");
}

#else

string GetPosixHomeDirectory()
{
    const char *home = getenv("HOME");
    if ( home == nullptr || strlen(home) == 0 )
        { home = getpwuid(getuid())->pw_dir; }
    return home;
}

#endif


static const string APPLICATION_DIRECTORY_RELATIVE_NAME = "iop-locnet";

// Windows < Vista: C:\Documents and Settings\Username\Application Data\iop-locnet
// Windows >= Vista: C:\Users\Username\AppData\Roaming\iop-locnet
// Mac: ~/Library/Application Support/iop-locnet
// Unix: ~/.iop-locnet
string GetApplicationDataDirectory()
{
#ifdef _WIN32
    string result = GetWindowsDirectory() + "\\" + APPLICATION_DIRECTORY_RELATIVE_NAME + "\\";
    if ( ! CreateDirectory( result.c_str(), nullptr ) && ERROR_ALREADY_EXISTS != GetLastError() )
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to create directory " + result); }
    return result;
#elif MAC_OSX
    string result = GetPosixHomeDirectory() + "/Library/Application Support/" + APPLICATION_DIRECTORY_RELATIVE_NAME + "/"; 
    system( ("mkdir -p " + result).c_str() );
    return result;
#else
    string result = GetPosixHomeDirectory() + "/." + APPLICATION_DIRECTORY_RELATIVE_NAME + "/";
    system( ("mkdir -p " + result).c_str() );
    return result;
#endif
}




static const TcpPort DefaultNodePort    = 16980;
static const TcpPort DefaultClientPort  = 16981;
static const TcpPort DefaultLocalPort   = 16982;
static const string DEFAULT_NODE_PORT   = to_string(DefaultNodePort);
static const string DEFAULT_CLIENT_PORT = to_string(DefaultClientPort);
static const string DEFAULT_LOCAL_PORT  = to_string(DefaultLocalPort);

static const string DESC_OPTIONAL_DEFAULT = "Optional, default value: ";
static const string DEFAULT_CONFIG_FILE = GetApplicationDataDirectory() + "iop-locnet.cfg";
static const string DEFAULT_DBPATH      = GetApplicationDataDirectory() + "locnet.sqlite";
static const string DEFAULT_LOGPATH     = GetApplicationDataDirectory() + "debug.log";
//const string DBFILE_PATH = ":memory:"; // NOTE in-memory storage without a db file
//const string DBFILE_PATH = "file:locnet.sqlite"; // NOTE this may be any file URL


static const char *OPTNAME_HELP         = "--help";
static const char *OPTNAME_VERSION      = "--version";
static const char *OPTNAME_CONFIGFILE   = "--configfile";
static const char *OPTNAME_NODEID       = "--nodeid";
static const char *OPTNAME_HOST         = "--host";
static const char *OPTNAME_NODE_PORT    = "--nodeport";
static const char *OPTNAME_CLIENT_PORT  = "--clientport";
static const char *OPTNAME_LOCAL_PORT   = "--localport";
static const char *OPTNAME_LATITUDE     = "--latitude";
static const char *OPTNAME_LONGITUDE    = "--longitude";
static const char *OPTNAME_SEEDNODE     = "--seednode";

static const char *OPTNAME_DBPATH       = "--dbpath";
static const char *OPTNAME_LOGPATH      = "--logpath";
static const char *OPTNAME_TESTMODE     = "--test";

static const vector<NetworkEndpoint> DefaultSeedNodes {
    NetworkEndpoint("ham4.fermat.cloud", DefaultNodePort),
    NetworkEndpoint("ham5.fermat.cloud", DefaultNodePort),
    NetworkEndpoint("ham6.fermat.cloud", DefaultNodePort),
    NetworkEndpoint("ham7.fermat.cloud", DefaultNodePort),
};


bool EzParserConfig::Initialize(int argc, const char *argv[])
{
    ez::ezOptionParser _optParser;
    
    _optParser.overview = "Internet of People : Location Based Network";
    _optParser.syntax   = "iop-locnetd [--option value]";
//     _optParser.example  = "iop-locnetd -h\n\n";
//     _optParser.footer   = "iop-locnetd version info"\n";
    
    // Set up all option details
    _optParser.add(
        "",             // Default value
        false,          // Is this a mandatory option?
        0,              // Number of expected values
        0,              // Delimiter character if expecting multiple values
        "Show usage",   // Help message for this option
        OPTNAME_HELP, "-h" // Flag names
    );
    
    _optParser.add("", false, 0, 0, "Print version information.", OPTNAME_VERSION, "-v");
    // TODO consider to remove --test from releases
    _optParser.add("", false, 0, 0, "This is not the option you're looking for, unless debugging a development version.", OPTNAME_TESTMODE);
    
    _optParser.add(DEFAULT_CONFIG_FILE.c_str(), false, 1, 0, ( "Path to config file to load options from. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_CONFIG_FILE ).c_str(), OPTNAME_CONFIGFILE);
    _optParser.add("", true, 1, 0, "Public node id, should be an SHA256 hash "
        "of the public key of this node", OPTNAME_NODEID);
    _optParser.add("", false, 1, 0, "Externally accessible IP address (ipv4 or v6) to be advertised "
        "for other nodes or clients. Required for seeds only, autodetected otherwise.", OPTNAME_HOST);
    _optParser.add(DEFAULT_NODE_PORT.c_str(), false, 1, 0, ( "TCP port to serve node to node communication. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_NODE_PORT ).c_str(), OPTNAME_NODE_PORT);
    _optParser.add(DEFAULT_CLIENT_PORT.c_str(), false, 1, 0, ( "TCP port to serve client (i.e. end user) queries. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_CLIENT_PORT ).c_str(), OPTNAME_CLIENT_PORT);
    _optParser.add(DEFAULT_LOCAL_PORT.c_str(), false, 1, 0, ( "TCP port to serve other IoP services running on this node. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_LOCAL_PORT ).c_str(), OPTNAME_LOCAL_PORT);
    _optParser.add("", true, 1, 0, "GPS latitude of this server "
        "as real number from range (-90,90)", OPTNAME_LATITUDE);
    _optParser.add("", true, 1, 0, "GPS longitude of this server "
        "as real number from range (-180,180)", OPTNAME_LONGITUDE);
    _optParser.add("", false, 1, 0, "Host name of seed node to be used instead of default seeds. "
        "You can repeat this option to define multiple custom seed nodes.", OPTNAME_SEEDNODE);
    
    _optParser.add(DEFAULT_LOGPATH.c_str(), false, 1, 0, ( "Path to log file. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_LOGPATH ).c_str(), OPTNAME_LOGPATH);
    _optParser.add(DEFAULT_DBPATH.c_str(), false, 1, 0, ( "Path to db file. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_DBPATH ).c_str(), OPTNAME_DBPATH);
    
    // Perform parsing, first from command line ...
    _optParser.parse(argc, argv);
    
    // ... then from config file if present (will not overwrite existing values)
    string filename;
    _optParser.get(OPTNAME_CONFIGFILE)->getString(filename);
    if ( _optParser.importFile( filename.c_str() ) )
        { cout << "Processed config file " << filename << endl; }
    else { if (! isTestMode() )
        { cout << "Config file '" << filename << "' not found, using command line values only" << endl; } }
    
    // Check for missing mandatory options
    vector<string> badOptions;
    bool validateRequiredPassed = _optParser.gotRequired(badOptions);
    if( ! validateRequiredPassed &&
        ! ( _optParser.isSet(OPTNAME_HELP) || _optParser.isSet(OPTNAME_VERSION) ) )
    {
        for(size_t idx = 0; idx < badOptions.size(); ++idx)
            { cerr << "Missing required option " << badOptions[idx] << endl; }
    }
   
//     // Can also check for option value counts but we currently don't use format --option val1,val2,val3
//     bool valueCountPassed = _optParser.gotExpected(badOptions);
//     if(! valueCountPassed)
//     {
//         for(size_t idx = 0; idx < badOptions.size(); ++idx)
//             { LOG(ERROR) << "Wrong number of values for option " << badOptions[idx]; }
//     }
    
    if ( ! _optParser.isSet(OPTNAME_VERSION) &&
         ( _optParser.isSet(OPTNAME_HELP) || ! validateRequiredPassed ) ) // || ! valueCountPassed ) )
    {
        string usage;
        _optParser.getUsage(usage);
        cerr << endl << usage;
        return false;
    }
    
    if ( _optParser.isSet(OPTNAME_TESTMODE) )
        { _testMode = true; }
    
    // Fetch extracted option values from parser
    _versionRequested = _optParser.isSet(OPTNAME_VERSION);
    _optParser.get(OPTNAME_NODEID)->getString(_nodeId);
    _optParser.get(OPTNAME_HOST)->getString(_ipAddr);
    _optParser.get(OPTNAME_LATITUDE)->getFloat(_latitude);
    _optParser.get(OPTNAME_LONGITUDE)->getFloat(_longitude);
    _optParser.get(OPTNAME_LOGPATH)->getString(_logPath);
    _optParser.get(OPTNAME_DBPATH)->getString(_dbPath);
    
    unsigned long nodePort;
    _optParser.get(OPTNAME_NODE_PORT)->getULong(nodePort);
    _nodePort = nodePort;
    
    unsigned long clientPort;
    _optParser.get(OPTNAME_CLIENT_PORT)->getULong(clientPort);
    _clientPort = clientPort;
    
    unsigned long localPort;
    _optParser.get(OPTNAME_LOCAL_PORT)->getULong(localPort);
    _localPort = localPort;
    
    _myNodeInfo.reset( new NodeInfo( _nodeId, GpsLocation(_latitude, _longitude),
        NodeContact(_ipAddr, _nodePort, _clientPort), {} ) );
    
    vector<vector<string>> seedOptionVectors;
    _optParser.get(OPTNAME_SEEDNODE)->getMultiStrings(seedOptionVectors);
    for (auto const &seedVector : seedOptionVectors)
        { _seedNodes.push_back( NetworkEndpoint(seedVector[0], DefaultNodePort) ); }
    
//     cout << "----- Pretty print" << endl;
//     string pretty;
//     _optParser.prettyPrint(pretty);
//     cout << pretty;
//     cout << "-----" << endl;
    
    return true;
}



bool EzParserConfig::versionRequested() const
    { return _versionRequested; }

const string& EzParserConfig::logPath() const
    { return _logPath; }    

const string& EzParserConfig::dbPath() const
    { return _dbPath; }

const NodeInfo& EzParserConfig::myNodeInfo() const
    { return *_myNodeInfo; }

const vector<NetworkEndpoint>& EzParserConfig::seedNodes() const
    { return _seedNodes.empty() ? DefaultSeedNodes : _seedNodes; }

TcpPort EzParserConfig::localServicePort() const
    { return _localPort; }
    
chrono::duration<uint32_t> EzParserConfig::dbMaintenancePeriod() const
    { return isTestMode() ? chrono::duration<uint32_t>(chrono::seconds(35)) : _dbMaintenancePeriod; }

chrono::duration<uint32_t> EzParserConfig::dbExpirationPeriod() const
    { return isTestMode() ? chrono::duration<uint32_t>(chrono::minutes(2)) : _dbExpirationPeriod; }

chrono::duration<uint32_t> EzParserConfig::discoveryPeriod() const
    { return isTestMode() ? chrono::duration<uint32_t>(chrono::seconds(15)) : _discoveryPeriod; }


}