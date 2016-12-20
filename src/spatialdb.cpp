#include <algorithm>
#include <chrono>
#include <limits>

#include <easylogging++.h>
#include <sqlite3.h>
#include <spatialite.h>

#include "spatialdb.hpp"

using namespace std;



namespace LocNet
{


const string SpatiaLiteDatabase::IN_MEMORY_DB = ":memory:";

const vector<string> SpatiaLiteDatabase::DatabaseInitCommands = {
    "BEGIN TRANSACTION;",
    "SELECT InitSpatialMetadata();",
    "CREATE TABLE IF NOT EXISTS metainfo ( "
    "  key   TEXT PRIMARY KEY, "
    "  value TEXT NOT NULL "
    ");"
    "INSERT INTO metainfo (key, value) "
    "  VALUES ('version', '1');"
    "CREATE TABLE IF NOT EXISTS nodes ( "
    "  id           TEXT PRIMARY KEY, "
    "  addressType  INT NOT NULL, "
    "  ipAddress    TEXT NOT NULL, "
    "  port         INT NOT NULL, "
    "  relationType INT NOT NULL, "
    "  roleType     INT NOT NULL, "
    "  expiresAt    INT NOT NULL, " // Unix timestamp. NOTE consider implementing non expiring entries to have NULL here.
    "  location     POINT NOT NULL "
    ");"
    "END TRANSACTION;"
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



void ThreadSafeChangeListenerRegistry::AddListener(shared_ptr<IChangeListener> listener)
{
    lock_guard<mutex> lock(_mutex);
    if (listener == nullptr)
        { throw runtime_error("Attempt to register invalid listener"); }
        
    _listeners[ listener->sessionId() ] = listener;
    LOG(DEBUG) << "Registered ChangeListener for session " << listener->sessionId();
}


void ThreadSafeChangeListenerRegistry::RemoveListener(const SessionId& sessionId)
{
    lock_guard<mutex> lock(_mutex);
    _listeners.erase(sessionId);
    LOG(DEBUG) << "Deregistered ChangeListener for session " << sessionId;
}


vector<shared_ptr<IChangeListener>> ThreadSafeChangeListenerRegistry::listeners() const
{
    lock_guard<mutex> lock(_mutex);
    vector<shared_ptr<IChangeListener>> result;
    for (const auto &listenerEntry : _listeners)
        { result.push_back(listenerEntry.second); }
    //LOG(DEBUG) << "Listener count " << result.size();
    return result;
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



void SpatiaLiteDatabase::ExecuteSql(sqlite3 *dbHandle, const string &sql)
{
    char *errorMessage = nullptr;
    int execResult = sqlite3_exec( dbHandle, sql.c_str(), nullptr, nullptr, &errorMessage );
    scope_exit freeMsg( [&errorMessage] { sqlite3_free(errorMessage); } );
    if (execResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to execute command: " << sql;
        LOG(ERROR) << "Error was: " << errorMessage;
        throw runtime_error(errorMessage);
    }
}



string LocationPointSql(const GpsLocation &location)
{
    // NOTE int-based numbers, no string or user input, so no SQL injection vulnerability
    return string( "MakePoint(" +
        to_string( location.longitude() ) + "," + 
        to_string( location.latitude()  ) + ")" );
}



vector<NodeDbEntry> QueryEntries(sqlite3 *dbHandle, const GpsLocation &fromLocation,
    const string &whereCondition = "", const string orderBy = "", const string &limit = "")
{
    sqlite3_stmt *statement;
    string queryStr =
        "SELECT id, addressType, ipAddress, port, X(location), Y(location), "
            "relationType, roleType, expiresAt, "
            "Distance(location, " + LocationPointSql(fromLocation) + ", 1) / 1000 AS dist_km "
        "FROM nodes " +
        whereCondition + " " +
        orderBy + " " +
        limit;
    
    //LOG(DEBUG) << "Running query: " << queryStr;
    
    int prepResult = sqlite3_prepare_v2( dbHandle, queryStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw runtime_error("Failed to prepare statement to get neighbourhood");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    vector<NodeDbEntry> result;
    while ( sqlite3_step(statement) == SQLITE_ROW )
    {
        const uint8_t *idPtr        = sqlite3_column_text  (statement, 0);
        int            addrType     = sqlite3_column_int   (statement, 1);
        const uint8_t *ipAddrPtr    = sqlite3_column_text  (statement, 2);
        int            port         = sqlite3_column_int   (statement, 3);
        double         longitude    = sqlite3_column_double(statement, 4);
        double         latitude     = sqlite3_column_double(statement, 5);
        int            relationType = sqlite3_column_int   (statement, 6);
        int            roleType     = sqlite3_column_int   (statement, 7);
        //int            expiresAt    = sqlite3_column_int   (statement, 8);
        
        NetworkInterface contact( static_cast<AddressType>(addrType),
            reinterpret_cast<const char*>(ipAddrPtr), static_cast<TcpPort>(port) );
        NodeInfo info( NodeProfile( reinterpret_cast<const char*>(idPtr), contact ),
                       GpsLocation(latitude, longitude) );
        result.emplace_back( info,
            // TODO use some kind of checked conversion function from int to enums
            static_cast<NodeRelationType>(relationType),
            static_cast<NodeContactRoleType>(roleType) );
    }
    
    return result;
}



// SpatiaLite initialization/shutdown sequence is documented here:
// https://groups.google.com/forum/#!msg/spatialite-users/83SOajOJ2JU/sgi5fuYAVVkJ
SpatiaLiteDatabase::SpatiaLiteDatabase( const NodeInfo& myNodeInfo, const string &dbPath,
                                        chrono::duration<uint32_t> entryExpirationPeriod ) :
    _myLocation( myNodeInfo.location() ), _dbHandle(nullptr),
    _entryExpirationPeriod(entryExpirationPeriod)
{
    _spatialiteConnection = spatialite_alloc_connection();
    
    bool creatingDb = ! FileExist(dbPath);
    
    // NOTE SQLITE_OPEN_FULLMUTEX performs operations sequentially using a mutex.
    //      We might have to change to a more performant but more complicated model here.
    int openResult = sqlite3_open_v2 ( dbPath.c_str(), &_dbHandle,
         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_URI, nullptr); // null: no vFS module to use
    if (openResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to open/create SpatiaLite database file " << dbPath;
        throw runtime_error("Failed to open SpatiaLite database");
    }
    scope_error closeDbOnError( [this] { sqlite3_close(_dbHandle); } );
    
//     LOG(INFO) << "SQLite version: " << sqlite3_libversion();
//     LOG(INFO) << "SpatiaLite version: " << spatialite_version();
    
    spatialite_init_ex(_dbHandle, _spatialiteConnection, 0);
    scope_error cleanupOnError( [this] { spatialite_cleanup_ex(_spatialiteConnection); } );
    
    if (creatingDb)
    {
        LOG(INFO) << "No SpatiaLite database found, generating: " << dbPath;
        for (const string &command : DatabaseInitCommands)
            { ExecuteSql(_dbHandle, command); }
        LOG(INFO) << "Database initialized";
    }
    
    LOG(DEBUG) << "Updating node information in database";
    vector<NodeDbEntry> selfEntries = QueryEntries( _dbHandle, _myLocation,
        "WHERE relationType = " + to_string( static_cast<uint32_t>(NodeRelationType::Self) ) );
    if ( selfEntries.size() > 1 )
        { throw runtime_error("Multiple self instances found, database may have been tampered with."); }
    if ( ! selfEntries.empty() && selfEntries.front().profile().id() != myNodeInfo.profile().id() )
        { throw runtime_error("Node id changed, database is invalidated. Delete database file " +
            dbPath + " to force signing up to the network with the new node id."); }
    
    NodeDbEntry myNodeDbEntry(myNodeInfo, NodeRelationType::Self, NodeContactRoleType::Acceptor);
    if ( selfEntries.empty() )  { Store (myNodeDbEntry, false ); }
    else                        { Update(myNodeDbEntry, false ); }
    LOG(DEBUG) << "Database ready with node count: " << GetNodeCount();
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


IChangeListenerRegistry& SpatiaLiteDatabase::changeListenerRegistry()
    { return _listenerRegistry; }




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



// TODO ideally we would just call QueryEntries() here, but have to manually bind id param
//      to avoid SQL injection attacks. We could deduplicate at least some parts like result processing.
shared_ptr<NodeDbEntry> SpatiaLiteDatabase::Load(const NodeId& nodeId) const
{
    sqlite3_stmt *statement;
    string queryStr =
        "SELECT id, addressType, ipAddress, port, X(location), Y(location), "
               "relationType, roleType "
        "FROM nodes "
        "WHERE id=?";
    
    //LOG(DEBUG) << "Running query: " << queryStr;
    
    int prepResult = sqlite3_prepare_v2( _dbHandle, queryStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw runtime_error("Failed to prepare statement to load node");
    }
    
    if ( sqlite3_bind_text( statement, 1, nodeId.c_str(), -1, SQLITE_STATIC ) != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind load query node id param";
        throw runtime_error("Failed to bind load query node id param");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    shared_ptr<NodeDbEntry> result;
    if ( sqlite3_step(statement) == SQLITE_ROW )
    {
        const uint8_t *idPtr        = sqlite3_column_text  (statement, 0);
        int            addrType     = sqlite3_column_int   (statement, 1);
        const uint8_t *ipAddrPtr    = sqlite3_column_text  (statement, 2);
        int            port         = sqlite3_column_int   (statement, 3);
        double         longitude    = sqlite3_column_double(statement, 4);
        double         latitude     = sqlite3_column_double(statement, 5);
        int            relationType = sqlite3_column_int   (statement, 6);
        int            roleType     = sqlite3_column_int   (statement, 7);
        
        NetworkInterface contact( static_cast<AddressType>(addrType),
            reinterpret_cast<const char*>(ipAddrPtr), static_cast<TcpPort>(port) );
        GpsLocation location(latitude, longitude);
        // TODO create enums through checked "enum constructor" method
        result.reset( new NodeDbEntry(
            NodeInfo( NodeProfile( reinterpret_cast<const char*>(idPtr), contact ), location ),
            static_cast<NodeRelationType>(relationType), static_cast<NodeContactRoleType>(roleType) ) );
    }
    return result;
}



void SpatiaLiteDatabase::Store(const NodeDbEntry &node, bool expires)
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
    
    time_t expiresAt = expires ?
        chrono::system_clock::to_time_t( chrono::system_clock::now() + _entryExpirationPeriod ) :
        numeric_limits<time_t>::max();
    const NetworkInterface &contact = node.profile().contact();
    // TODO abstract bind checks away, probably with functions, or maybe macros
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
    
    for ( auto listenerEntry : _listenerRegistry.listeners() )
    {
        // if ( auto listener = listenerEntry.lock() )
            { listenerEntry->AddedNode(node); }
    }
}



void SpatiaLiteDatabase::Update(const NodeDbEntry& node, bool expires)
{
    sqlite3_stmt *statement;
    string insertStr(
        "UPDATE nodes SET "
        "  addressType=?, ipAddress=?, port=?, relationType=?, roleType=?, expiresAt=?, "
        "  location=" + LocationPointSql( node.location() ) + " "
        "WHERE id=?");
    int prepResult = sqlite3_prepare_v2( _dbHandle, insertStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << insertStr;
        throw runtime_error("Failed to prepare statement for updating node entry");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    time_t expiresAt = expires ?
        chrono::system_clock::to_time_t( chrono::system_clock::now() + _entryExpirationPeriod ) :
        numeric_limits<time_t>::max();
    const NetworkInterface &contact = node.profile().contact();
    if ( sqlite3_bind_int(  statement, 1, static_cast<int>( contact.addressType() ) )      != SQLITE_OK ||
         sqlite3_bind_text( statement, 2, contact.address().c_str(), -1, SQLITE_STATIC )   != SQLITE_OK ||
         sqlite3_bind_int(  statement, 3, contact.port() )                                 != SQLITE_OK ||
         sqlite3_bind_int(  statement, 4, static_cast<int>( node.relationType() ) )        != SQLITE_OK ||
         sqlite3_bind_int(  statement, 5, static_cast<int>( node.roleType() ) )            != SQLITE_OK ||
         sqlite3_bind_int(  statement, 6, expiresAt )                                      != SQLITE_OK ||
         sqlite3_bind_text( statement, 7, node.profile().id().c_str(), -1, SQLITE_STATIC ) != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind node store statement params";
        throw runtime_error("Failed to bind node store statement params");
    }
    
    int execResult = sqlite3_step(statement);
    if (execResult != SQLITE_DONE)
    {
        LOG(ERROR) << "Failed to run node update statement, error code: " << execResult;
        throw runtime_error("Failed to run node update statement");
    }
    
    int affectedRows = sqlite3_changes(_dbHandle);
    if (affectedRows != 1)
    {
        LOG(ERROR) << "Affected row count for update should be 1, got : " << affectedRows;
        throw runtime_error("Wrong affected row count for update");
    }
    
    for ( auto listenerEntry : _listenerRegistry.listeners() )
    {
        // if ( auto listener = listenerEntry.lock() )
            { listenerEntry->UpdatedNode(node); }
    }
}



void SpatiaLiteDatabase::Remove(const NodeId &nodeId)
{
    shared_ptr<NodeDbEntry> storedNode = Load(nodeId);
    if (storedNode == nullptr)
        { throw runtime_error("Node to be removed is not present: " + nodeId); }
    
    sqlite3_stmt *statement;
    string insertStr(
        "DELETE FROM nodes "
        "WHERE id=?");
    int prepResult = sqlite3_prepare_v2( _dbHandle, insertStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << insertStr;
        throw runtime_error("Failed to prepare statement for deleting node entry");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    if ( sqlite3_bind_text( statement, 1, nodeId.c_str(), -1, SQLITE_STATIC ) != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind node delete statement id param";
        throw runtime_error("Failed to bind node delete statement id param");
    }
    
    int execResult = sqlite3_step(statement);
    if (execResult != SQLITE_DONE)
    {
        LOG(ERROR) << "Failed to run node delete statement, error code: " << execResult;
        throw runtime_error("Failed to run node delete statement");
    }
    
    int affectedRows = sqlite3_changes(_dbHandle);
    if (affectedRows != 1)
    {
        LOG(ERROR) << "Affected row count for delete should be 1, got : " << affectedRows;
        throw runtime_error("Wrong affected row count for delete");
    }
    
    for ( auto listenerEntry : _listenerRegistry.listeners() )
    {
        // if ( auto listener = listenerEntry.lock() )
            { listenerEntry->RemovedNode(*storedNode); }
    }
}



void SpatiaLiteDatabase::ExpireOldNodes()
{
    // NOTE int-based number, no string or user input, so no SQL injection vulnerability
    time_t now = chrono::system_clock::to_time_t( chrono::system_clock::now() );
    string expiredCondition( "WHERE expiresAt <= " + to_string(now) );
    
    vector<NodeDbEntry> expiredEntries = QueryEntries(_dbHandle, _myLocation, expiredCondition);
    
    string expireStr(
        "DELETE FROM nodes " +
        expiredCondition);
    ExecuteSql(_dbHandle, expireStr);
    
    for ( auto listenerEntry : _listenerRegistry.listeners() )
    {
        // if ( auto listener = listenerEntry.lock() )
        {
            for (const auto &entry : expiredEntries)
                { listenerEntry->RemovedNode(entry); }
        }
    }
}



vector<NodeDbEntry> SpatiaLiteDatabase::GetNodes(NodeContactRoleType roleType)
{
    return QueryEntries(_dbHandle, _myLocation,
        "WHERE roleType = " + to_string( static_cast<int>(roleType) ) );
}



size_t SpatiaLiteDatabase::GetNodeCount() const
{
    vector<NodeDbEntry> nodes( QueryEntries(_dbHandle, _myLocation) );
    return nodes.size();
}



vector<NodeDbEntry> SpatiaLiteDatabase::GetNeighbourNodesByDistance() const
{
    return QueryEntries(_dbHandle, _myLocation,
        "WHERE relationType = " + to_string( static_cast<int>(NodeRelationType::Neighbour) ),
        "ORDER BY dist_km" );
}



vector<NodeDbEntry> SpatiaLiteDatabase::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
{
    string whereCondition = filter == Neighbours::Included ? "" :
        "WHERE relationType = " + to_string( static_cast<int>(NodeRelationType::Colleague) );
    return QueryEntries(_dbHandle, _myLocation, whereCondition,
        "ORDER BY RANDOM()", "LIMIT " + to_string(maxNodeCount) );
}



vector<NodeDbEntry> SpatiaLiteDatabase::GetClosestNodesByDistance(
    const GpsLocation& location, Distance radiusKm, size_t maxNodeCount, Neighbours filter) const
{
    string whereCondition = "WHERE (dist_km IS NULL OR dist_km <= " + to_string(radiusKm) + ")";
    if (filter == Neighbours::Excluded)
    {
        whereCondition += " AND relationType = " +
            to_string( static_cast<int>(NodeRelationType::Colleague) );
    }
    
    return QueryEntries(_dbHandle, location,
        whereCondition,
        "ORDER BY dist_km",
        "LIMIT " + to_string(maxNodeCount) );
}



} // namespace LocNet
