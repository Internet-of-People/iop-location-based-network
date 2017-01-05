#include <cstdlib>

#include <easylogging++.h>
#include <ezOptionParser.hpp>

#include "config.hpp"

using namespace std;



namespace LocNet
{



static const uint16_t VERSION_MAJOR = 0;
static const uint16_t VERSION_MINOR = 0;
static const uint16_t VERSION_PATCH = 1;

static const string LOCNET_VERSION = to_string(VERSION_MAJOR) + "." +
    to_string(VERSION_MINOR) + "." + to_string(VERSION_PATCH);


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


const string& Config::version() const
    { return LOCNET_VERSION; }



#ifdef WIN32

#include <windows.h>
#include <shlobj.h>
#define MAX_PATH 1024

string GetWindowsDirectory(int folderId = CSIDL_APPDATA, bool createDir = true)
{
    char path[MAX_PATH] = "";
    if ( SHGetSpecialFolderPathA(NULL, path, folderId, createDir) )
        { return path; }

    throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to get Windows user app directory");
}

#else

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

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
#ifdef WIN32
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




static const TcpPort DefaultPort        = 16980;
static const string DEFAULT_PORT        = to_string(DefaultPort);

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
static const char *OPTNAME_PORT         = "--port";
static const char *OPTNAME_LATITUDE     = "--latitude";
static const char *OPTNAME_LONGITUDE    = "--longitude";
static const char *OPTNAME_SEEDNODE     = "--seednode";

static const char *OPTNAME_DBPATH       = "--dbpath";
static const char *OPTNAME_LOGPATH      = "--logpath";


const chrono::duration<uint32_t> EzParserConfig::_dbMaintenancePeriod = chrono::hours(6);
const chrono::duration<uint32_t> EzParserConfig::_dbExpirationPeriod = chrono::hours(24);
const chrono::duration<uint32_t> EzParserConfig::_discoveryPeriod = chrono::hours(1);

static const vector<NetworkInterface> DefaultSeedNodes {
    NetworkInterface("ham4.fermat.cloud", DefaultPort),
    NetworkInterface("ham5.fermat.cloud", DefaultPort),
    NetworkInterface("ham6.fermat.cloud", DefaultPort),
    NetworkInterface("ham7.fermat.cloud", DefaultPort),
};


bool EzParserConfig::Initialize(int argc, const char *argv[])
{
    ez::ezOptionParser _optParser;
    
    _optParser.overview = "Internet of People : Location Based Network";
    _optParser.syntax   = "iop-locnetd [--option value]";
//     _optParser.example  = "iop-locnetd -h\n\n";
//     _optParser.footer   = "iop-locnetd v0.0.1\n";
    
    // Set up all option details
    _optParser.add(
        "",             // Default value
        false,          // Is this a mandatory option?
        0,              // Number of expected values
        0,              // Delimiter character if expecting multiple values
        "Show usage",   // Help message for this option
        OPTNAME_HELP, "-h" // Flag names
    );
    
    _optParser.add("", false, 0, 0, "Print version information.", OPTNAME_VERSION);
    _optParser.add(DEFAULT_CONFIG_FILE.c_str(), false, 1, 0, ( "Path to config file to load options from. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_CONFIG_FILE ).c_str(), OPTNAME_CONFIGFILE, "-c");
    _optParser.add("", true, 1, 0, "Public node id, should be an SHA256 hash "
        "of the public key of this node", OPTNAME_NODEID, "-i");
    _optParser.add("", true, 1, 0, "Host address (either DNS, ipv4 or v6) to be used externally "
        "by other nodes or clients to connect to this node", OPTNAME_HOST, "-h");
    _optParser.add(DEFAULT_PORT.c_str(), false, 1, 0, ( "TCP port number for connecting to this node. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_PORT ).c_str(), OPTNAME_PORT, "-p");
    _optParser.add("", true, 1, 0, "GPS latitude of this server "
        "as real number from range (-90,90)", OPTNAME_LATITUDE, "-y");
    _optParser.add("", true, 1, 0, "GPS longitude of this server "
        "as real number from range (-180,180)", OPTNAME_LONGITUDE, "-x");
    _optParser.add("", false, 1, 0, "Host name of seed node to be used instead of default seeds. "
        "You can repeat this option to define multiple custom seed nodes.", OPTNAME_SEEDNODE, "-s");
    
    _optParser.add(DEFAULT_LOGPATH.c_str(), false, 1, 0, ( "TCP port number for connecting to this node. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_LOGPATH ).c_str(), OPTNAME_LOGPATH, "-p");
    _optParser.add(DEFAULT_DBPATH.c_str(), false, 1, 0, ( "Path to log file. " +
        DESC_OPTIONAL_DEFAULT + DEFAULT_DBPATH ).c_str(), OPTNAME_DBPATH, "-d");
    
    // Perform parsing, first from command line ...
    _optParser.parse(argc, argv);
    
    // ... then from config file if present (will not overwrite existing values)
    string filename;
    _optParser.get(OPTNAME_CONFIGFILE)->getString(filename);
    if ( _optParser.importFile( filename.c_str() ) )
         { cout << "Processed config file " << filename << endl; }
    else { cout << "Config file '" << filename << "' not found, using command line values only" << endl; }
    
    // Check for missing mandatory options
    vector<string> badOptions;
    bool validateRequiredPassed = _optParser.gotRequired(badOptions);
    if( ! validateRequiredPassed &&
        ! ( _optParser.isSet(OPTNAME_HELP) || _optParser.isSet(OPTNAME_VERSION) ) )
    {
        for(size_t idx = 0; idx < badOptions.size(); ++idx)
            { cerr << "Missing required option " << badOptions[idx] << endl; }
    }
   
// Could also check for option value counts but we currently don't use format --option val1,val2,val3
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
    
    // Fetch extracted option values from parser
    _versionRequested = _optParser.isSet(OPTNAME_VERSION);
    _optParser.get(OPTNAME_NODEID)->getString(_id);
    _optParser.get(OPTNAME_HOST)->getString(_ipAddr);
    _optParser.get(OPTNAME_LATITUDE)->getFloat(_latitude);
    _optParser.get(OPTNAME_LONGITUDE)->getFloat(_longitude);
    _optParser.get(OPTNAME_LOGPATH)->getString(_logPath);
    _optParser.get(OPTNAME_DBPATH)->getString(_dbPath);
    
    unsigned long port;
    _optParser.get(OPTNAME_PORT)->getULong(port);
    _port = port;
        
    _myNodeInfo.reset( new NodeInfo(
        NodeProfile(_id, NetworkInterface(_ipAddr, _port) ),
        GpsLocation(_latitude, _longitude) ) );
    
    vector<vector<string>> seedOptionVectors;
    _optParser.get(OPTNAME_SEEDNODE)->getMultiStrings(seedOptionVectors);
    for (auto const &seedVector : seedOptionVectors)
        { _seedNodes.push_back( NetworkInterface(seedVector[0], DefaultPort) ); }
    
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

const vector<NetworkInterface>& EzParserConfig::seedNodes() const
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