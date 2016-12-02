#include <algorithm>
#include <chrono>
#include <sqlite3.h>
#include <spatialite.h>

#include "easylogging++.h"
#include "spatialdb.hpp"

using namespace std;



namespace LocNet
{


const chrono::duration<uint32_t> ExpirationPeriod = chrono::hours(24);

//const string DatabaseFile = ":memory:"; // NOTE in-memory storage without a db file
//const string DatabaseFile = "file:locnet.sqlite"; // NOTE this may be any file URL
const string DatabaseFilePath = "locnet.sqlite";

const vector<string> DatabaseInitCommands = {
    "SELECT InitSpatialMetadata();",
    "CREATE TABLE IF NOT EXISTS nodes ( "
    "  id           TEXT PRIMARY KEY, "
    "  addressType  INT NOT NULL, "
    "  ipAddress    TEXT NOT NULL, "
    "  port         INT NOT NULL, "
    "  relationType INT NOT NULL, "
    "  roleType     INT NOT NULL, "
    "  location     POINT NOT NULL, "
    "  expiresAt    INT NOT NULL" // Unix timestamp
    ");",
};




NodeDbEntry::NodeDbEntry(const NodeDbEntry& other) :
    NodeInfo(other), _relationType(other._relationType), _roleType(other._roleType) {}

NodeDbEntry::NodeDbEntry( const NodeInfo& info,
                          NodeRelationType relationType, NodeContactRoleType roleType) :
    NodeInfo(info), _relationType(relationType), _roleType(roleType) {}

NodeDbEntry::NodeDbEntry( const NodeProfile& profile, const GpsLocation& location,
                          NodeRelationType relationType, NodeContactRoleType roleType ) :
    NodeInfo(profile, location), _relationType(relationType), _roleType(roleType) {}


NodeRelationType NodeDbEntry::relationType() const { return _relationType; }
NodeContactRoleType NodeDbEntry::roleType() const { return _roleType; }


bool NodeDbEntry::operator==(const NodeDbEntry& other) const
{
    return  NodeInfo::operator==(other) &&
            _relationType == other._relationType &&
            _roleType == other._roleType;
}




// NOTE SQLite works fine without this as sqlite3_open also calls init()
// struct StaticDatabaseInitializer {
//     StaticDatabaseInitializer() {
//         if ( sqlite3_initialize() != SQLITE_OK )
//             { throw new runtime_error("Failed to initialize SQLite environment"); }
//     }
// };
// 
// StaticDatabaseInitializer SqlLiteInitializer;



bool FileExist(const string &fileName)
{
    ifstream fileStream(fileName);
    return fileStream.good();
}


void ExecuteSql(sqlite3 *dbHandle, const string &sql)
{
    char *errorMessage;
    scope_exit freeMsg( [&errorMessage] { sqlite3_free(errorMessage); } );
    int execResult = sqlite3_exec( dbHandle, sql.c_str(), nullptr, nullptr, &errorMessage );
    if (execResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to execute command: " << sql;
        LOG(ERROR) << "Error was: " << errorMessage;
        throw runtime_error(errorMessage);
    }
}



// SpatiaLite initialization/shutdown sequence is documented here: https://groups.google.com/forum/#!msg/spatialite-users/83SOajOJ2JU/sgi5fuYAVVkJ
SpatiaLiteDatabase::SpatiaLiteDatabase(const GpsLocation& nodeLocation) :
    _myLocation(nodeLocation), _dbHandle(nullptr)
{
    _spatialiteConnection = spatialite_alloc_connection();
    
    bool creatingDb = ! FileExist(DatabaseFilePath);
    
    // TODO check if SQLITE_OPEN_NOMUTEX multithread mode is really what we need here
    int openResult = sqlite3_open_v2 ( DatabaseFilePath.c_str(), &_dbHandle,
         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_URI, nullptr);
    scope_error closeDbOnError( [this] { sqlite3_close(_dbHandle); } );
    if (openResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to open existing SpatiaLite database file " << DatabaseFilePath;
        throw runtime_error("Failed to open SpatiaLite database");
    }
    
//     LOG(INFO) << "SQLite version: " << sqlite3_libversion();
//     LOG(INFO) << "SpatiaLite version: " << spatialite_version();
    
    spatialite_init_ex(_dbHandle, _spatialiteConnection, 0);
    scope_error cleanupOnError( [this] { spatialite_cleanup_ex(_spatialiteConnection); } );
    
    if (creatingDb)
    {
        LOG(INFO) << "No database found, generating SpatiaLite database: " << DatabaseFilePath;
        for (const string &command : DatabaseInitCommands)
        {
            ExecuteSql(_dbHandle, command);
        }
        LOG(INFO) << "Database initialized";
    }
}


SpatiaLiteDatabase::~SpatiaLiteDatabase()
{
    sqlite3_close (_dbHandle);
    spatialite_cleanup_ex(_spatialiteConnection);
    // TODO there is no free cache function in current version despite description
    // spatialite_free_internal_cache();
    // TODO is this needed?
    //spatialite_shutdown();
}



//     char **results;
//     int rows;
//     int columns;
//     char *errorMessage;
//     int execResult = sqlite3_get_table( _dbHandle, queryStr.c_str(), &results, &rows, &columns, &errorMessage );
//     if (execResult != SQLITE_OK)
//     {
//         scope_exit freeMsg( [errorMessage] { sqlite3_free(errorMessage); } );
//         LOG(ERROR) << "Failed to query neighbours by distance: " << errorMessage;
//         throw runtime_error(errorMessage);
//     }
//     
//     scope_exit freeMsg( [results] { sqlite3_free_table(results); } );



string LocationPointSql(const GpsLocation &location)
{
    // NOTE int-based numbers, no string or user input, so no SQL injection vulnerability
    return string( "MakePoint(" +
        to_string( location.longitude() ) + "," + 
        to_string( location.latitude()  ) + ")" );
}



Distance SpatiaLiteDatabase::GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const
{
    sqlite3_stmt *statement;
    string queryStr(
        "SELECT Distance(" +
            LocationPointSql(one) + "," +
            LocationPointSql(other) + "," +
            "1" // Needed for GPS distance, without this SpatiaLite calculates only Euclidean distance
        ") / 1000 AS dist_km;" );
            
    int prepResult = sqlite3_prepare_v2( _dbHandle, queryStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw runtime_error("Failed to prepare statement for distance measurement");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    if ( sqlite3_step(statement) != SQLITE_ROW )
    {
        LOG(ERROR) << "Failed to run distance query";
        throw runtime_error("Failed to run distance query");
    }
    
    double result = sqlite3_column_double(statement, 0);
    return result;
}



void SpatiaLiteDatabase::Store(const NodeDbEntry &node)
{
    sqlite3_stmt *statement;
    string insertStr(
        "INSERT INTO nodes "
        "(id, addressType, ipAddress, port, relationType, roleType, expiresAt, location) VALUES "
        "(?, ?, ?, ?, ?, ?, ?, " + LocationPointSql( node.location() ) + ")" );
    int prepResult = sqlite3_prepare_v2( _dbHandle, insertStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << insertStr;
        throw runtime_error("Failed to prepare statement for storing node entry");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    time_t expiresAt = chrono::system_clock::to_time_t( chrono::system_clock::now() + ExpirationPeriod );
    const NetworkInterface &contact = node.profile().contact();
    if ( sqlite3_bind_text( statement, 1, node.profile().id().c_str(), -1, SQLITE_STATIC ) != SQLITE_OK ||
         sqlite3_bind_int(  statement, 2, static_cast<int>( contact.addressType() ) )      != SQLITE_OK ||
         sqlite3_bind_text( statement, 3, contact.address().c_str(), -1, SQLITE_STATIC )   != SQLITE_OK ||
         sqlite3_bind_int(  statement, 4, contact.port() )                                 != SQLITE_OK ||
         sqlite3_bind_int(  statement, 5, static_cast<int>( node.relationType() ) )        != SQLITE_OK ||
         sqlite3_bind_int(  statement, 6, static_cast<int>( node.roleType() ) )            != SQLITE_OK ||
         sqlite3_bind_int(  statement, 7, expiresAt )                                      != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind node store statement params";
        throw runtime_error("Failed to bind node store statement params");
    }
    
    int execResult = sqlite3_step(statement);
    if (execResult != SQLITE_DONE)
    {
        LOG(ERROR) << "Failed to run node store statement, error code: " << execResult;
        throw runtime_error("Failed to run node store statement");
    }
}



//shared_ptr<NodeDbEntry> SpatiaLiteDatabase::Load(const NodeId& nodeId) const
shared_ptr<NodeDbEntry> SpatiaLiteDatabase::Load(const NodeId&) const
{
    // TODO
    return shared_ptr<NodeDbEntry>();
}



//bool SpatiaLiteDatabase::Update(const NodeDbEntry& node)
void SpatiaLiteDatabase::Update(const NodeDbEntry&)
{
    // TODO
}



void SpatiaLiteDatabase::Remove(const NodeId&) // nodeId
{
    // TODO
}



void SpatiaLiteDatabase::ExpireOldNodes()
{
    // NOTE int-based number, no string or user input, so no SQL injection vulnerability
    time_t now = chrono::system_clock::to_time_t( chrono::system_clock::now() );
    string expireStr(
        "DELETE FROM nodes "
        "WHERE expiresAt <= " + to_string(now) );
    ExecuteSql(_dbHandle, expireStr);
}



size_t SpatiaLiteDatabase::GetColleagueNodeCount() const
{
    // TODO
    return 0;
}



vector<NodeInfo> ListNodesByDistance(sqlite3 *dbHandle, const GpsLocation &location,
    const string &whereCondition = "", const string orderBy = "", const string &limit = "")
{
    sqlite3_stmt *statement;
    string queryStr =
        "SELECT id, addressType, ipAddress, port, X(location), Y(location), "
            "Distance(location, " + LocationPointSql(location) + ", 1) / 1000 AS dist_km "
        "FROM nodes " +
        whereCondition + " " +
        orderBy + " " +
        limit;
    
    LOG(DEBUG) << "Running query: " << queryStr;
    
    int prepResult = sqlite3_prepare_v2( dbHandle, queryStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw runtime_error("Failed to prepare statement to get neighbourhood");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    vector<NodeInfo> result;
    while ( sqlite3_step(statement) == SQLITE_ROW )
    {
        const uint8_t *idPtr     = sqlite3_column_text  (statement, 0);
        int            addrType  = sqlite3_column_int   (statement, 1);
        const uint8_t *ipAddrPtr = sqlite3_column_text  (statement, 2);
        int            port      = sqlite3_column_int   (statement, 3);
        double         longitude = sqlite3_column_double(statement, 4);
        double         latitude  = sqlite3_column_double(statement, 5);
        
        NetworkInterface contact( static_cast<AddressType>(addrType),
            reinterpret_cast<const char*>(ipAddrPtr), static_cast<TcpPort>(port) );
        GpsLocation location(latitude, longitude);
        result.emplace_back( NodeProfile( reinterpret_cast<const char*>(idPtr), contact ), location );
    }
    
    return result;
}



vector<NodeInfo> SpatiaLiteDatabase::GetNeighbourNodesByDistance() const
{
    return ListNodesByDistance(_dbHandle, _myLocation,
        "WHERE relationType = " + to_string( static_cast<int>(NodeRelationType::Neighbour) ),
        "ORDER BY dist_km" );
}



//vector<NodeInfo> SpatiaLiteDatabase::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
vector<NodeInfo> SpatiaLiteDatabase::GetRandomNodes(size_t, Neighbours) const
{
    return ListNodesByDistance(_dbHandle, _myLocation, "",
        "ORDER BY RANDOM()" );
}



vector<NodeInfo> SpatiaLiteDatabase::GetClosestNodesByDistance(
    const GpsLocation& location, Distance radiusKm, size_t maxNodeCount, Neighbours filter) const
{
    string whereCondition = "WHERE (dist_km IS NULL OR dist_km <= " + to_string(radiusKm) + ")";
    if (filter == Neighbours::Excluded)
    {
        whereCondition += " AND relationType != " +
            to_string( static_cast<int>(NodeRelationType::Neighbour) );
    }
    
    return ListNodesByDistance(_dbHandle, location,
        whereCondition,
        "ORDER BY dist_km "
        "LIMIT " + to_string(maxNodeCount) );
}



} // namespace LocNet
