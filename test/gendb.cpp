#include <iostream>
#include <sqlite3.h>
#include <spatialite.h>

#include <easylogging++.h>

#include "spatialdb.hpp"

INITIALIZE_EASYLOGGINGPP

using namespace std;
using namespace LocNet;



int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " dbPath" << endl;
        return 1;
    }
    
    try
    {
        string dbPath( argv[1] );
        
        void *_spatialiteConnection = spatialite_alloc_connection();;
        
        sqlite3 *_dbHandle;
        int openResult = sqlite3_open_v2 ( dbPath.c_str(), &_dbHandle,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_URI, nullptr); // null: no vFS module to use
        scope_exit closeDb( [_dbHandle] { sqlite3_close(_dbHandle); } );
        if (openResult != SQLITE_OK)
        {
            cerr << "Failed to open/create SpatiaLite database file " << dbPath << endl;
            throw runtime_error("Failed to open SpatiaLite database");
        }
        
        spatialite_init_ex(_dbHandle, _spatialiteConnection, 0);
        scope_exit cleanupSpatialite( [_spatialiteConnection] { spatialite_cleanup_ex(_spatialiteConnection); } );
        
        cout << "Generating empty database, this may take a while..." << endl;
        for (const string &command : SpatiaLiteDatabase::DatabaseInitCommands)
            { SpatiaLiteDatabase::ExecuteSql(_dbHandle, command); }
        cout << "Finished successfully" << endl;
    }
    catch (exception &e)
    {
        cerr << "Failed with exception: " << e.what() << endl;
    }
}
