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

const vector<string> DatabaseInitCommands = {
"BEGIN TRANSACTION;",
    "SELECT InitSpatialMetadata();",
    
    "CREATE TABLE IF NOT EXISTS metainfo ( "
    "  key   TEXT PRIMARY KEY, "
    "  value TEXT NOT NULL "
    ");"
    
    "INSERT OR IGNORE INTO metainfo (key, value) "
    "  VALUES ('version', '1');"
    
    "CREATE TABLE IF NOT EXISTS nodes ( "
    "  id           TEXT PRIMARY KEY, "
    "  ipAddress    TEXT NOT NULL, "
    "  nodePort     INT NOT NULL, "
    "  clientPort   INT NOT NULL, "
    "  relationType INT NOT NULL, "
    "  roleType     INT NOT NULL, "
    "  expiresAt    INT NOT NULL, " // Unix timestamp. NOTE consider implementing non expiring entries to have NULL here.
    "  location     POINT NOT NULL "
    ");"
    
    "CREATE TABLE IF NOT EXISTS services ( "
    "  nodeId       TEXT NOT NULL, "
    "  serviceType  INT NOT NULL, "
    "  port         INT NOT NULL, "
    "  data         BLOB, "
    "  PRIMARY KEY(nodeId, serviceType), "
    "  FOREIGN KEY(nodeId) REFERENCES nodes(id) "
    ");"
    
"END TRANSACTION;" };




NodeDbEntry::NodeDbEntry(const NodeDbEntry& other) :
    NodeInfo(other), _relationType(other._relationType), _roleType(other._roleType) {}

NodeDbEntry::NodeDbEntry( const NodeInfo& info,
                          NodeRelationType relationType, NodeContactRoleType roleType) :
    NodeInfo(info), _relationType(relationType), _roleType(roleType) {}


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
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Attempt to register listener instance null"); }
    
    if ( _listeners.find( listener->sessionId() ) != _listeners.end() )
    {
        LOG(DEBUG) << "Session already have a registered listener, ignore request to add new one";
        return;
    }
    
    _listeners[ listener->sessionId() ] = listener;
    listener->OnRegistered();
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
//             { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to initialize SQLite environment"); }
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
    char *errorMessage = nullptr;
    int execResult = sqlite3_exec( dbHandle, sql.c_str(), nullptr, nullptr, &errorMessage );
    scope_exit freeMsg( [&errorMessage] { sqlite3_free(errorMessage); } );
    if (execResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to execute command: " << sql;
        LOG(ERROR) << "Error was: " << errorMessage;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, errorMessage);
    }
}


// void QuerySql(sqlite3 *dbHandle, const string &queryStr)
// {
//     char **results;
//     int rows;
//     int columns;
//     char *errorMessage;
//     int execResult = sqlite3_get_table( dbHandle, queryStr.c_str(), &results, &rows, &columns, &errorMessage );
//     scope_error freeMsg( [errorMessage] { sqlite3_free(errorMessage); } );
//     if (execResult != SQLITE_OK)
//     {
//         LOG(ERROR) << "Failed to query neighbours by distance: " << errorMessage;
//         throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, errorMessage);
//     }
//     
//     scope_exit freeMsg( [results] { sqlite3_free_table(results); } );
// }



string LocationPointSql(const GpsLocation &location)
{
    // NOTE int-based numbers, no string or user input, so no SQL injection vulnerability
    return string( "MakePoint(" +
        to_string( location.longitude() ) + "," + 
        to_string( location.latitude()  ) + ")" );
}



vector<NodeDbEntry> SpatiaLiteDatabase::QueryEntries(const GpsLocation &fromLocation,
    const string &whereCondition, const string orderBy, const string &limit) const
{
    sqlite3_stmt *statement;
    string queryStr =
        "SELECT id, ipAddress, nodePort, clientPort, X(location), Y(location), "
            "relationType, roleType, expiresAt, "
            "Distance(location, " + LocationPointSql(fromLocation) + ", 1) / 1000 AS dist_km "
        "FROM nodes " +
        whereCondition + " " +
        orderBy + " " +
        limit;
    
    //LOG(DEBUG) << "Running query: " << queryStr;
    
    int prepResult = sqlite3_prepare_v2( _dbHandle, queryStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement to get neighbourhood");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    vector<NodeDbEntry> result;
    while ( sqlite3_step(statement) == SQLITE_ROW )
    {
        const uint8_t *idPtr        = sqlite3_column_text  (statement, 0);
        const uint8_t *ipAddrPtr    = sqlite3_column_text  (statement, 1);
        int            nodePort     = sqlite3_column_int   (statement, 2);
        int            clientPort   = sqlite3_column_int   (statement, 3);
        double         longitude    = sqlite3_column_double(statement, 4);
        double         latitude     = sqlite3_column_double(statement, 5);
        int            relationType = sqlite3_column_int   (statement, 6);
        int            roleType     = sqlite3_column_int   (statement, 7);
        //int            expiresAt    = sqlite3_column_int   (statement, 8);
        
        NodeContact contact( reinterpret_cast<const char*>(ipAddrPtr),
                             static_cast<TcpPort>(nodePort), static_cast<TcpPort>(clientPort) );
        string nodeId( reinterpret_cast<const char*>(idPtr) );
        NodeInfo::Services services = LoadServices(nodeId);
        NodeInfo info( nodeId, GpsLocation(latitude, longitude), contact, services );
        result.emplace_back( info,
            // TODO use some kind of checked conversion function from int to enums
            static_cast<NodeRelationType>(relationType),
            static_cast<NodeContactRoleType>(roleType) );
    }
    
    return result;
}


NodeDbEntry ThisNodeToDbEntry(const NodeInfo &thisNodeInfo)
    { return NodeDbEntry(thisNodeInfo, NodeRelationType::Self, NodeContactRoleType::Self); }


// SpatiaLite initialization/shutdown sequence is documented here:
// https://groups.google.com/forum/#!msg/spatialite-users/83SOajOJ2JU/sgi5fuYAVVkJ
SpatiaLiteDatabase::SpatiaLiteDatabase( const NodeInfo& myNodeInfo, const string &dbPath,
                                        chrono::duration<uint32_t> entryExpirationPeriod ) :
    _myNodeInfo(myNodeInfo), _dbHandle(nullptr), _entryExpirationPeriod(entryExpirationPeriod)
{
    _spatialiteConnection = spatialite_alloc_connection();
    
    bool creatingDb = ! FileExist(dbPath);
    
    // TODO SQLITE_OPEN_FULLMUTEX performs operations sequentially using a mutex.
    //      We might have to change to a more performant but more complicated model here.
    int openResult = sqlite3_open_v2 ( dbPath.c_str(), &_dbHandle,
         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_URI, nullptr); // nullptr: no vFS module to use
    if (openResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to open/create SpatiaLite database file " << dbPath;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to open SpatiaLite database");
    }
    scope_error closeDbOnError( [this] { sqlite3_close(_dbHandle); } );
    
#ifndef _WIN32
    spatialite_init_ex(_dbHandle, _spatialiteConnection, 0);
    scope_error cleanupOnError( [this] { spatialite_cleanup_ex(_spatialiteConnection); } );
#else
    sqlite3_enable_load_extension(_dbHandle, 1);
    sqlite3_load_extension(_dbHandle, "mod_spatialite", nullptr, nullptr);
#endif

    LOG(TRACE) << "SQLite version: " << sqlite3_libversion();
    LOG(TRACE) << "SpatiaLite version: " << spatialite_version();
    
    if (creatingDb)
    {
        LOG(INFO) << "No SpatiaLite database found, generating: " << dbPath;
        for (const string &command : DatabaseInitCommands)
            { ExecuteSql(_dbHandle, command); }
        LOG(INFO) << "Database initialized";
    }
    
    LOG(DEBUG) << "Updating node information in database";
    vector<NodeDbEntry> selfEntries = QueryEntries( _myNodeInfo.location(),
        "WHERE relationType = " + to_string( static_cast<uint32_t>(NodeRelationType::Self) ) );
    if ( selfEntries.size() > 1 )
        { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Multiple self instances found, database may have been tampered with."); }
    if ( ! selfEntries.empty() && selfEntries.front().id() != _myNodeInfo.id() )
        { throw LocationNetworkError(ErrorCode::ERROR_BAD_STATE, "Node id changed, database is invalidated. Delete database file " +
            dbPath + " to force signing up to the network with the new node id."); }
    
    if ( selfEntries.empty() )  { Store ( ThisNodeToDbEntry(_myNodeInfo), false ); }
    else                        { Update( ThisNodeToDbEntry(_myNodeInfo), false ); }
    LOG(DEBUG) << "Database ready with node count: " << GetNodeCount();
}


SpatiaLiteDatabase::~SpatiaLiteDatabase()
{
    sqlite3_close (_dbHandle);
#ifndef _WIN32
    spatialite_cleanup_ex(_spatialiteConnection);
#endif
    // TODO there is no free cache function in current version despite description
    // spatialite_free_internal_cache();
    // TODO is this needed?
    //spatialite_shutdown();
}


IChangeListenerRegistry& SpatiaLiteDatabase::changeListenerRegistry()
    { return _listenerRegistry; }




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
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement for distance measurement");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    if ( sqlite3_step(statement) != SQLITE_ROW )
    {
        LOG(ERROR) << "Failed to run distance query";
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to run distance query");
    }
    
    double result = sqlite3_column_double(statement, 0);
    return result;
}



// TODO now that we have services in a different table, probably all methods should change
// to use transactions where node and related service entries are updated together
NodeInfo::Services SpatiaLiteDatabase::LoadServices(const NodeId& nodeId) const
{
    sqlite3_stmt *statement;
    string queryStr =
        "SELECT serviceType, port, data "
        "FROM services WHERE nodeId=?";
    
    int prepResult = sqlite3_prepare_v2( _dbHandle, queryStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement to load services");
    }
    
    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    if ( sqlite3_bind_text( statement, 1, nodeId.c_str(), -1, SQLITE_STATIC ) != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind LoadServices query node id param";
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to bind LoadServices query node id param");
    }
    
    NodeInfo::Services services;
    while ( sqlite3_step(statement) == SQLITE_ROW )
    {
        int serviceType  = sqlite3_column_int  (statement, 0);
        int port         = sqlite3_column_int  (statement, 1);
        
        string data;
        if ( sqlite3_column_type(statement, 2) == SQLITE_BLOB )
        {
            int dataBytesCnt = sqlite3_column_bytes(statement, 2);
            const void *dataBytes  = sqlite3_column_blob (statement, 2);
            if ( dataBytes != nullptr && dataBytesCnt > 0 )
                { data = string( reinterpret_cast<const char*>(dataBytes), dataBytesCnt ); }
        }
        
        ServiceInfo service( static_cast<ServiceType>(serviceType), port, data );
        services[ service.type() ] = service;
    }
    
    return services;
}



void SpatiaLiteDatabase::StoreServices(const NodeId& nodeId, const NodeInfo::Services& services)
{
    RemoveServices(nodeId);
    
    sqlite3_stmt *statement;
    string insertStr(
        "INSERT INTO services "
        "(nodeId, serviceType, port, data) "
        "VALUES (?, ?, ?, ?)" );
    int prepResult = sqlite3_prepare_v2( _dbHandle, insertStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << insertStr;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement for storing node services");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    for (const auto &servicePair : services)
    {
        // TODO abstract bind checks away, probably with functions, or maybe macros
        const ServiceInfo &service = servicePair.second;
        const char *blobData = service.customData().empty() ? nullptr : service.customData().data();
        int blobSize = service.customData().size();
        if ( sqlite3_bind_text( statement, 1, nodeId.c_str(), -1, SQLITE_STATIC )  != SQLITE_OK ||
             sqlite3_bind_int(  statement, 2, static_cast<int>( service.type() ) ) != SQLITE_OK ||
             sqlite3_bind_int(  statement, 3, service.port() )                     != SQLITE_OK ||
             sqlite3_bind_blob( statement, 4, blobData, blobSize, SQLITE_STATIC )  != SQLITE_OK )
        {
            LOG(ERROR) << "Failed to bind store service statement params";
            throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to bind store service statement params");
        }
        
        int execResult = sqlite3_step(statement);
        if (execResult != SQLITE_DONE)
        {
            LOG(ERROR) << "Failed to run store service statement, error code: " << execResult;
            throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to run store service statement");
        }
        
        int resetResult = sqlite3_reset(statement);
        int clearResult = sqlite3_clear_bindings(statement);
        if (resetResult != SQLITE_OK || clearResult != SQLITE_OK)
        {
            LOG(ERROR) << "Failed to reuse store service statement, error code: " << resetResult;
            throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to reuse store service statement");
        }
    }
}


void SpatiaLiteDatabase::RemoveServices(const NodeId& nodeId)
{
    sqlite3_stmt *statement;
    string queryStr = "DELETE FROM services WHERE nodeId=?";
    
    int prepResult = sqlite3_prepare_v2( _dbHandle, queryStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement to remove services");
    }
    
    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    if ( sqlite3_bind_text( statement, 1, nodeId.c_str(), -1, SQLITE_STATIC ) != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind RemoveService query node id param";
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to bind remove service query node id param");
    }
    
    int execResult = sqlite3_step(statement);
    if (execResult != SQLITE_DONE)
    {
        LOG(ERROR) << "Failed to run node RemoveServices statement, error code: " << execResult;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to run remove services statement");
    }
}



// TODO ideally we would just call QueryEntries() here, but have to manually bind id param
//      to avoid SQL injection attacks. We could deduplicate at least some parts like result processing.
shared_ptr<NodeDbEntry> SpatiaLiteDatabase::Load(const NodeId& nodeId) const
{
    sqlite3_stmt *statement;
    string queryStr =
        "SELECT id, ipAddress, nodePort, clientPort, X(location), Y(location), "
               "relationType, roleType "
        "FROM nodes "
        "WHERE id=?";
    
    //LOG(DEBUG) << "Running query: " << queryStr;
    
    int prepResult = sqlite3_prepare_v2( _dbHandle, queryStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << queryStr;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement to load node");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    if ( sqlite3_bind_text( statement, 1, nodeId.c_str(), -1, SQLITE_STATIC ) != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind load query node id param";
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to bind load query node id param");
    }

    shared_ptr<NodeDbEntry> result;
    if ( sqlite3_step(statement) == SQLITE_ROW )
    {
        const uint8_t *idPtr        = sqlite3_column_text  (statement, 0);
        const uint8_t *ipAddrPtr    = sqlite3_column_text  (statement, 1);
        int            nodePort     = sqlite3_column_int   (statement, 2);
        int            clientPort   = sqlite3_column_int   (statement, 3);
        double         longitude    = sqlite3_column_double(statement, 4);
        double         latitude     = sqlite3_column_double(statement, 5);
        int            relationType = sqlite3_column_int   (statement, 6);
        int            roleType     = sqlite3_column_int   (statement, 7);
        
        // TODO create enums through checked "enum constructor" method
        NodeContact contact( reinterpret_cast<const char*>(ipAddrPtr),
                             static_cast<TcpPort>(nodePort), static_cast<TcpPort>(clientPort) );
        
        NodeInfo::Services services = LoadServices(nodeId);
        result.reset( new NodeDbEntry(
            NodeInfo( reinterpret_cast<const char*>(idPtr), GpsLocation(latitude, longitude), contact, services ),
            static_cast<NodeRelationType>(relationType), static_cast<NodeContactRoleType>(roleType) ) );
    }
    
    return result;
}



// TODO reduce SpatiaLite boilerplate in general as much as possible. Currently it's very repetitive.
void SpatiaLiteDatabase::Store(const NodeDbEntry &node, bool expires)
{
    sqlite3_stmt *statement;
    string insertStr(
        "INSERT INTO nodes "
        "(id, ipAddress, nodePort, clientPort, relationType, roleType, expiresAt, location) VALUES "
        "(?, ?, ?, ?, ?, ?, ?, " + LocationPointSql( node.location() ) + ")" );
    int prepResult = sqlite3_prepare_v2( _dbHandle, insertStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << insertStr;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement for storing node entry");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    time_t expiresAt = expires ?
        chrono::system_clock::to_time_t( chrono::system_clock::now() + _entryExpirationPeriod ) :
        numeric_limits<time_t>::max();
    const NodeContact &contact = node.contact();
    // TODO abstract long bind checks away, probably with functions, or maybe macros
    if ( sqlite3_bind_text( statement, 1, node.id().c_str(), -1, SQLITE_STATIC )        != SQLITE_OK ||
         sqlite3_bind_text( statement, 2, contact.address().c_str(), -1, SQLITE_STATIC )!= SQLITE_OK ||
         sqlite3_bind_int(  statement, 3, contact.nodePort() )                          != SQLITE_OK ||
         sqlite3_bind_int(  statement, 4, contact.clientPort() )                        != SQLITE_OK ||
         sqlite3_bind_int(  statement, 5, static_cast<int>( node.relationType() ) )     != SQLITE_OK ||
         sqlite3_bind_int(  statement, 6, static_cast<int>( node.roleType() ) )         != SQLITE_OK ||
         sqlite3_bind_int(  statement, 7, expiresAt )                                   != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind node store statement params";
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to bind node store statement params");
    }
    
    int execResult = sqlite3_step(statement);
    if (execResult != SQLITE_DONE)
    {
        LOG(ERROR) << "Failed to run node store statement, error code: " << execResult;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to run node store statement");
    }
    
    StoreServices( node.id(), node.services() );
    
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
        "  ipAddress=?, nodePort=?, clientPort=?, relationType=?, roleType=?, expiresAt=?, "
        "  location=" + LocationPointSql( node.location() ) + " "
        "WHERE id=?");
    int prepResult = sqlite3_prepare_v2( _dbHandle, insertStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << insertStr;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement for updating node entry");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    time_t expiresAt = expires ?
        chrono::system_clock::to_time_t( chrono::system_clock::now() + _entryExpirationPeriod ) :
        numeric_limits<time_t>::max();
    const NodeContact &contact = node.contact();
    if ( sqlite3_bind_text( statement, 1, contact.address().c_str(), -1, SQLITE_STATIC )!= SQLITE_OK ||
         sqlite3_bind_int(  statement, 2, contact.nodePort() )                          != SQLITE_OK ||
         sqlite3_bind_int(  statement, 3, contact.clientPort() )                        != SQLITE_OK ||
         sqlite3_bind_int(  statement, 4, static_cast<int>( node.relationType() ) )     != SQLITE_OK ||
         sqlite3_bind_int(  statement, 5, static_cast<int>( node.roleType() ) )         != SQLITE_OK ||
         sqlite3_bind_int(  statement, 6, expiresAt )                                   != SQLITE_OK ||
         sqlite3_bind_text( statement, 7, node.id().c_str(), -1, SQLITE_STATIC )        != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind node store statement params";
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to bind node store statement params");
    }
    
    int execResult = sqlite3_step(statement);
    if (execResult != SQLITE_DONE)
    {
        LOG(ERROR) << "Failed to run node update statement, error code: " << execResult;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to run node update statement");
    }
    
    int affectedRows = sqlite3_changes(_dbHandle);
    if (affectedRows != 1)
    {
        LOG(ERROR) << "Affected row count for update should be 1, got : " << affectedRows;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Wrong affected row count for update");
    }
    
    StoreServices( node.id(), node.services() );
    
    // update cached self node info
    if ( node.relationType() == NodeRelationType::Self )
        { _myNodeInfo = node; }
    
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
        { throw LocationNetworkError(ErrorCode::ERROR_INVALID_VALUE, "Node to be removed is not present: " + nodeId); }
    if ( storedNode->relationType() == NodeRelationType::Self )
        { throw LocationNetworkError(ErrorCode::ERROR_INVALID_VALUE, "Attempt to delete self entry"); }
    
    RemoveServices(nodeId);
    
    sqlite3_stmt *statement;
    string insertStr(
        "DELETE FROM nodes "
        "WHERE id=?");
    int prepResult = sqlite3_prepare_v2( _dbHandle, insertStr.c_str(), -1, &statement, nullptr );
    if (prepResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to prepare statement: " << insertStr;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to prepare statement for deleting node entry");
    }

    scope_exit finalizeStmt( [&statement] { sqlite3_finalize(statement); } );
    
    if ( sqlite3_bind_text( statement, 1, nodeId.c_str(), -1, SQLITE_STATIC ) != SQLITE_OK )
    {
        LOG(ERROR) << "Failed to bind node delete statement id param";
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to bind node delete statement id param");
    }
    
    int execResult = sqlite3_step(statement);
    if (execResult != SQLITE_DONE)
    {
        LOG(ERROR) << "Failed to run node delete statement, error code: " << execResult;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Failed to run node delete statement");
    }
    
    int affectedRows = sqlite3_changes(_dbHandle);
    if (affectedRows != 1)
    {
        LOG(ERROR) << "Affected row count for delete should be 1, got : " << affectedRows;
        throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Wrong affected row count for delete");
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
    string expiredCondition(
        "WHERE expiresAt <= " + to_string(now) + " AND " +
            "relationType != " + to_string( static_cast<uint32_t>(NodeRelationType::Self) ) );
    
    vector<NodeDbEntry> expiredEntries = QueryEntries( _myNodeInfo.location(), expiredCondition );
    
    for (const auto &entry : expiredEntries)
    {
        Remove( entry.id() );
        for ( auto listenerEntry : _listenerRegistry.listeners() )
            { listenerEntry->RemovedNode(entry); }
    }
}



vector<NodeDbEntry> SpatiaLiteDatabase::GetNodes(NodeContactRoleType roleType)
{
    return QueryEntries( _myNodeInfo.location(),
        "WHERE roleType = " + to_string( static_cast<int>(roleType) ) );
}



size_t SpatiaLiteDatabase::GetNodeCount() const
{
    // NOTE this would be better done by SELECT COUNT(*) but that would need a lot more boilerplate code again
    vector<NodeDbEntry> nodes( QueryEntries( _myNodeInfo.location() ) );
    return nodes.size();
}



vector<NodeDbEntry> SpatiaLiteDatabase::GetNeighbourNodesByDistance() const
{
    return QueryEntries( _myNodeInfo.location(),
        "WHERE relationType = " + to_string( static_cast<int>(NodeRelationType::Neighbour) ),
        "ORDER BY dist_km" );
}



vector<NodeDbEntry> SpatiaLiteDatabase::GetRandomNodes(size_t maxNodeCount, Neighbours filter) const
{
    string whereCondition = filter == Neighbours::Included ? "" :
        "WHERE relationType = " + to_string( static_cast<int>(NodeRelationType::Colleague) );
    return QueryEntries( _myNodeInfo.location(), whereCondition,
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
    
    return QueryEntries(location,
        whereCondition,
        "ORDER BY dist_km",
        "LIMIT " + to_string(maxNodeCount) );
}



NodeDbEntry SpatiaLiteDatabase::ThisNode() const
{
    return ThisNodeToDbEntry(_myNodeInfo);
//     string whereCondition = "WHERE relationType = " + to_string( static_cast<int>(NodeRelationType::Colleague) );
//     vector<NodeDbEntry> result = QueryEntries( _myNodeInfo.location(), whereCondition );
//     if ( result.empty() )
//         { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Self node information is not found"); }
//     if ( result.size() > 1 )
//         { throw LocationNetworkError(ErrorCode::ERROR_INTERNAL, "Multiple self node info entries found"); }
//     return result.front();
}



} // namespace LocNet
