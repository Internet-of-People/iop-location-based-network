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
    "  ipaddr       TEXT NOT NULL, "
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
            ExecuteSql(command);
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



void SpatiaLiteDatabase::ExecuteSql(const string& sql)
{
    char *errorMessage;
    scope_exit freeMsg( [&errorMessage] { sqlite3_free(errorMessage); } );
    int execResult = sqlite3_exec( _dbHandle, sql.c_str(), nullptr, nullptr, &errorMessage );
    if (execResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to execute command: " << sql;
        LOG(ERROR) << "Error was: " << errorMessage;
        throw runtime_error(errorMessage);
    }
}



Distance SpatiaLiteDatabase::GetDistanceKm(const GpsLocation &one, const GpsLocation &other) const
{
    sqlite3_stmt *statement;
    const char *queryStr =
        "SELECT Distance("
            "MakePoint(?, ?),"
            "MakePoint(?, ?),"
            "1" // Needed for GPS distance, without this SpatiaLite calculates only Euclidean distance
        ") / 1000 AS dist_km;";
    int prepResult = sqlite3_prepare_v2( _dbHandle, queryStr, -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw runtime_error("Failed to prepare statement for distance measurement");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    if ( sqlite3_bind_double( statement, 1, one.longitude() )   != SQLITE_OK ||
         sqlite3_bind_double( statement, 2, one.latitude() )    != SQLITE_OK ||
         sqlite3_bind_double( statement, 3, other.longitude() ) != SQLITE_OK ||
         sqlite3_bind_double( statement, 4, other.latitude() )  != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind distance query params";
        throw runtime_error("Failed to bind distance query params");
    }
    
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
    const char *insertStr =
        "INSERT INTO nodes "
        "(id, ipaddr, port, relationType, roleType, expiresAt, location) VALUES "
        "(?, ?, ?, ?, ?, ?, MakePoint(?, ?))";
    int prepResult = sqlite3_prepare_v2( _dbHandle, insertStr, -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << insertStr;
        throw runtime_error("Failed to prepare statement for storing node entry");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    time_t expiresAt = chrono::system_clock::to_time_t( chrono::system_clock::now() + ExpirationPeriod );
    const NetworkInterface &contact = node.profile().contact();
    if ( sqlite3_bind_text(   statement, 1, node.profile().id().c_str(), -1, SQLITE_STATIC ) != SQLITE_OK ||
            sqlite3_bind_text(   statement, 2, contact.address().c_str(), -1, SQLITE_STATIC )   != SQLITE_OK ||
            sqlite3_bind_int(    statement, 3, contact.port() )                                 != SQLITE_OK ||
            sqlite3_bind_int(    statement, 4, static_cast<int>( node.relationType() ) )        != SQLITE_OK ||
            sqlite3_bind_int(    statement, 5, static_cast<int>( node.roleType() ) )            != SQLITE_OK ||
            sqlite3_bind_int(    statement, 6, expiresAt )                                      != SQLITE_OK ||
            sqlite3_bind_double( statement, 7, node.location().longitude() )                    != SQLITE_OK ||
            sqlite3_bind_double( statement, 8, node.location().latitude() )                     != SQLITE_OK )
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
    time_t now = chrono::system_clock::to_time_t( chrono::system_clock::now() );
    string expireStr(
        "DELETE FROM nodes "
        "WHERE expiresAt <= " + to_string(now) ); // NOTE int-based number, no string or user input, so no SQL injection vulnerability
    ExecuteSql(expireStr);
}



size_t SpatiaLiteDatabase::GetColleagueNodeCount() const
{
    // TODO
    return 0;
}



vector<NodeInfo> SpatiaLiteDatabase::GetNeighbourNodesByDistance() const
{
    // TODO
    
//     char **results;
//     int rows;
//     int columns;
//     char *errorMessage;
//     int execResult = sqlite3_get_table( _dbHandle, sql.c_str(), &results, &rows, &columns, &errorMessage );
//     if (execResult != SQLITE_OK)
//     {
//         LOG(ERROR) << "Failed to execute command: " << sql;
//         LOG(ERROR) << "Error was: " << errorMessage;
//         runtime_error ex(errorMessage);
//         sqlite3_free(errorMessage);
//         throw ex;
//     }
//     sqlite3_free_table(results);
    
    return {};
}



//vector<NodeInfo> SpatiaLiteDatabase::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
vector<NodeInfo> SpatiaLiteDatabase::GetRandomNodes(size_t, Neighbours) const
{
    // TODO
    return {};
}



//vector<NodeInfo> SpatiaLiteDatabase::GetClosestNodesByDistance(const GpsLocation& position, Distance radiusKm, size_t maxNodeCount, Neighbours filter) const
vector<NodeInfo> SpatiaLiteDatabase::GetClosestNodesByDistance(const GpsLocation&, Distance, size_t, Neighbours) const
{
    // TODO
    return {};
}



} // namespace LocNet
