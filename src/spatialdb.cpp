#include <algorithm>
#include <sqlite3.h>
#include <spatialite.h>

#include "easylogging++.h"
#include "spatialdb.hpp"

using namespace std;



namespace LocNet
{


//const string DatabaseFile = ":memory:"; // NOTE in-memory storage without a db file
const string DatabaseFile = "file:locnet.sqlite"; // NOTE this may be any file URL

const vector<string> DatabaseInitCommands = {
    "CREATE TABLE IF NOT EXISTS nodes ( "
    "  id       VARCHAR(255) PRIMARY KEY, "
    "  location BLOB NOT NULL, "
    "  ipaddr   VARCHAR(255) NOT NULL, "
    "  port     INT NOT NULL "
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




SpatiaLiteDatabase::SpatiaLiteDatabase(const GpsLocation& nodeLocation) :
    _myLocation(nodeLocation), _dbHandle(nullptr)
{
    // TODO check if SQLITE_OPEN_NOMUTEX multithread mode is really what we need here
    int openResult = sqlite3_open_v2 ( DatabaseFile.c_str(), &_dbHandle,
         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_URI, nullptr);
    if (openResult != SQLITE_OK)
    {
        sqlite3_close (_dbHandle);
        LOG(ERROR) << "Failed to open SQLite database file " << DatabaseFile;
        throw runtime_error("Failed to open SQLite database");
    }
    
    LOG(INFO) << "SQLite version: " << sqlite3_libversion();
    LOG(INFO) << "SpatiaLite version: " << spatialite_version();
    
    for (const string &command : DatabaseInitCommands)
    {
        ExecuteSql(command);
    }
}


SpatiaLiteDatabase::~SpatiaLiteDatabase()
{
    sqlite3_close (_dbHandle);
}



void SpatiaLiteDatabase::ExecuteSql(const string& sql)
{
    char *errorMessage;
    int execResult = sqlite3_exec(_dbHandle, sql.c_str(), nullptr, nullptr, &errorMessage);
    if (execResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to execute command: " << sql;
        LOG(ERROR) << "Error was: " << errorMessage;
        runtime_error ex(errorMessage);
        sqlite3_free(errorMessage);
        throw ex;
    }
}



void SpatiaLiteDatabase::QuerySql(const string& sql)
{
    char **results;
    int rows;
    int columns;
    char *errorMessage;
    int execResult = sqlite3_get_table(_dbHandle, sql.c_str(), &results, &rows, &columns, &errorMessage);
    if (execResult != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to execute command: " << sql;
        LOG(ERROR) << "Error was: " << errorMessage;
        runtime_error ex(errorMessage);
        sqlite3_free(errorMessage);
        throw ex;
    }
    sqlite3_free_table(results);
}



//Distance SpatiaLiteDatabase::GetDistanceKm(const GpsLocation& one, const GpsLocation& other) const
Distance SpatiaLiteDatabase::GetDistanceKm(const GpsLocation&, const GpsLocation&) const
{
    // TODO
    return 0;
}



//bool SpatiaLiteDatabase::Store(const NodeDbEntry& node)
bool SpatiaLiteDatabase::Store(const NodeDbEntry&)
{
    // TODO
    return false;
}



//shared_ptr<NodeDbEntry> SpatiaLiteDatabase::Load(const NodeId& nodeId) const
shared_ptr<NodeDbEntry> SpatiaLiteDatabase::Load(const NodeId&) const
{
    // TODO
    return shared_ptr<NodeDbEntry>();
}



//bool SpatiaLiteDatabase::Update(const NodeDbEntry& node)
bool SpatiaLiteDatabase::Update(const NodeDbEntry&)
{
    // TODO
    return false;
}



//bool SpatiaLiteDatabase::Remove(const NodeId& nodeId)
bool SpatiaLiteDatabase::Remove(const NodeId&)
{
    // TODO
    return false;
}



void SpatiaLiteDatabase::ExpireOldNodes()
{
    // TODO
}



size_t SpatiaLiteDatabase::GetColleagueNodeCount() const
{
    // TODO
    return 0;
}



vector<NodeInfo> SpatiaLiteDatabase::GetNeighbourNodesByDistance() const
{
    // TODO
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
