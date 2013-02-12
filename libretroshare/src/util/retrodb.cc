
/*
 * RetroShare : RetroDb functionality
 *
 * Copyright 2012 Christopher Evi-Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <iostream>
#include <sstream>
#include <memory.h>
#include <time.h>
#include <inttypes.h>

#include "retrodb.h"

//#define RETRODB_DEBUG



void free_blob(void* dat){

    char* c = (char*) dat;
    delete[] c;
    dat = NULL;

}

const int RetroDb::OPEN_READONLY = SQLITE_OPEN_READONLY;
const int RetroDb::OPEN_READWRITE = SQLITE_OPEN_READWRITE;
const int RetroDb::OPEN_READWRITE_CREATE = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

RetroDb::RetroDb(const std::string &dbPath, int flags) : mDb(NULL) {

    int rc = sqlite3_open_v2(dbPath.c_str(), &mDb, flags, NULL);

    if(rc){
        std::cerr << "Can't open database, Error code: " <<  sqlite3_errmsg(mDb)
                  << std::endl;
        sqlite3_close(mDb);
     }
}


RetroDb::~RetroDb(){

    if(mDb)
        sqlite3_close(mDb);
}

void RetroDb::closeDb(){

    int rc;

    if(mDb)
        rc  = sqlite3_close(mDb);

#ifdef RETRODB_DEBUG
    std::cerr << "RetroDb::closeDb(): Error code on close: " << rc << std::endl;
#endif

}

#define TIME_LIMIT 3

bool RetroDb::execSQL(const std::string &query){

    // prepare statement
    sqlite3_stmt* stm = NULL;

#ifdef RETRODB_DEBUG
    std::cerr << "Query: " << query << std::endl;
#endif

    int rc = sqlite3_prepare_v2(mDb, query.c_str(), query.length(), &stm, NULL);

    // check if there are any errors
    if(rc != SQLITE_OK){
        std::cerr << "RetroDb::execSQL(): Error preparing statement\n";
        std::cerr << "Error code: " <<  sqlite3_errmsg(mDb)
                  << std::endl;
        return false;
    }


    uint32_t delta = 3;
    time_t stamp = time(NULL), now = 0;
    bool timeOut = false, ok = false;

    while(!timeOut){

        rc = sqlite3_step(stm);

        if(rc == SQLITE_DONE){
            ok = true;
            break;
        }

        if(rc != SQLITE_BUSY){
            ok = false;
            break;
        }

        now = time(NULL);
        delta = stamp - now;

        if(delta > TIME_LIMIT){
            ok = false;
            timeOut = true;
        }
        // TODO add sleep so not to waste
        // precious cycles
    }

    if(!ok){

        if(rc == SQLITE_BUSY){
            std::cerr << "RetroDb::execSQL()\n" ;
            std::cerr << "SQL timed out!" << std::endl;
        }else{
            std::cerr << "RetroDb::execSQL(): Error executing statement (code: " << rc << ")\n";
            std::cerr << "Sqlite Error msg: " <<  sqlite3_errmsg(mDb)
                      << std::endl;
        }
    }

    // finalise statement or else db cannot be closed
    sqlite3_finalize(stm);
    return ok;
}

RetroCursor* RetroDb::sqlQuery(const std::string& tableName, const std::list<std::string>& columns,
                               const std::string& selection, const std::string& orderBy){

    if(tableName.empty() || columns.empty()){
        std::cerr << "RetroDb::sqlQuery(): No table or columns given" << std::endl;
        return NULL;
    }

    std::string columnSelection; // the column names to return
    sqlite3_stmt* stmt = NULL;
    std::list<std::string>::const_iterator it = columns.begin();

    for(; it != columns.end(); it++){
        columnSelection += *it;

        it++;
        if(it != columns.end())
            columnSelection += ",";
        it--;
    }

    // construct query
    // SELECT columnSelection FROM tableName WHERE selection
    std::string sqlQuery = "SELECT " + columnSelection + " FROM " +
                           tableName;

    // add selection clause if present
    if(!selection.empty())
        sqlQuery += " WHERE " + selection;


    // add 'order by' clause if present
    if(!orderBy.empty())
        sqlQuery += " ORDER BY " + orderBy + ";";
    else
        sqlQuery += ";";

#ifdef RETRODB_DEBUG
    std::cerr << "RetroDb::sqlQuery(): " << sqlQuery << std::endl;
#endif

    sqlite3_prepare_v2(mDb, sqlQuery.c_str(), sqlQuery.length(), &stmt, NULL);
    return (new RetroCursor(stmt));
}

bool RetroDb::isOpen() const {
    return (mDb==NULL ? false : true);
}

bool RetroDb::sqlInsert(const std::string &table, const std::string& nullColumnHack, const ContentValue &cv){

    std::map<std::string, uint8_t> keyTypeMap;
    cv.getKeyTypeMap(keyTypeMap);
    std::map<std::string, uint8_t>::iterator mit = keyTypeMap.begin();

    // build columns part of insertion
    std::string qColumns = table + "(";

    for(; mit != keyTypeMap.end(); mit++){

        qColumns += mit->first;

        mit++;

        // add comma if more columns left
        if(mit == keyTypeMap.end())
            qColumns += ")"; // close bracket if at end
        else
             qColumns += ",";

        mit--;
    }

    // build values part of insertion
    std::string qValues = "VALUES(";
    std::ostringstream oStrStream;
    uint32_t index = 0;
    std::list<RetroDbBlob> blobL;

    for(mit=keyTypeMap.begin(); mit!=keyTypeMap.end(); mit++){

        uint8_t type = mit->second;
        std::string key = mit->first;



        if(ContentValue::BOOL_TYPE == type)
            {
                bool value;
                cv.getAsBool(key, value);
                oStrStream << value;
                qValues += oStrStream.str();
            }
        else if( ContentValue::DOUBLE_TYPE == type)
            {
                double value;
                cv.getAsDouble(key, value);
                oStrStream << value;
                qValues += oStrStream.str();
            }
        else if( ContentValue::DATA_TYPE == type)
            {
                char* value;
                uint32_t len;
                cv.getAsData(key, len, value);
                RetroDbBlob b;
                b.data = value;
                b.length = len;
                b.index = ++index;
                blobL.push_back(b);
                qValues += "?"; // parameter
            }
        else if ( ContentValue::STRING_TYPE == type)
            {
                std::string value;
                cv.getAsString(key, value);
                qValues += "'" + value +"'";
            }
        else if ( ContentValue::INT32_TYPE == type)
            {
                int32_t value = 0;
                cv.getAsInt32(key, value);
                oStrStream << value;
                qValues += oStrStream.str();
            }
        else if( ContentValue::INT64_TYPE == type)
            {
                int64_t value = 0;
                cv.getAsInt64(key, value);
                oStrStream << value;
                qValues += oStrStream.str();
            }


        mit++;
        if(mit != keyTypeMap.end()){ // add comma if more columns left
            qValues += ",";
        }
        else{ // at end close brackets
             qValues += ");";
         }
        mit--;

        // clear stream strings
        oStrStream.str("");
    }


    // complete insertion query
    std::string sqlQuery = "INSERT INTO " + qColumns + " " + qValues;

#ifdef RETRODB_DEBUG
    std::cerr << "RetroDb::sqlInsert(): " << sqlQuery << std::endl;
#endif

    // execute query
    execSQL_bind_blobs(sqlQuery, blobL);
    return true;
}

bool RetroDb::execSQL_bind_blobs(const std::string &query, std::list<RetroDbBlob> &blobs){

    // prepare statement
    sqlite3_stmt* stm = NULL;

#ifdef RETRODB_DEBUG
    std::cerr << "Query: " << query << std::endl;
#endif

    int rc = sqlite3_prepare_v2(mDb, query.c_str(), query.length(), &stm, NULL);

    // check if there are any errors
    if(rc != SQLITE_OK){
        std::cerr << "RetroDb::execSQL(): Error preparing statement\n";
        std::cerr << "Error code: " <<  sqlite3_errmsg(mDb)
                  << std::endl;
        return false;
    }

    std::list<RetroDbBlob>::iterator lit = blobs.begin();

    for(; lit != blobs.end(); lit++){
        const RetroDbBlob& b = *lit;
        sqlite3_bind_blob(stm, b.index, b.data, b.length, NULL);
    }

    uint32_t delta = 3;
    time_t stamp = time(NULL), now = 0;
    bool timeOut = false, ok = false;

    while(!timeOut){

        rc = sqlite3_step(stm);

        if(rc == SQLITE_DONE){
            ok = true;
            break;
        }

        if(rc != SQLITE_BUSY){
            ok = false;
            break;
        }

        now = time(NULL);
        delta = stamp - now;

        if(delta > TIME_LIMIT){
            ok = false;
            timeOut = true;
        }
        // TODO add sleep so not to waste
        // precious cycles
    }

    if(!ok){

        if(rc == SQLITE_BUSY){
            std::cerr << "RetroDb::execSQL()\n" ;
            std::cerr << "SQL timed out!" << std::endl;
        }else{
            std::cerr << "RetroDb::execSQL(): Error executing statement (code: " << rc << ")\n";
            std::cerr << "Sqlite Error msg: " <<  sqlite3_errmsg(mDb)
                      << std::endl;
        }
    }

    // finalise statement or else db cannot be closed
    sqlite3_finalize(stm);
    return ok;
}

bool RetroDb::sqlDelete(const std::string &tableName, const std::string &whereClause, const std::string &whereArgs){

    std::string sqlQuery = "DELETE FROM " + tableName;

    if(!whereClause.empty()){
        sqlQuery += " WHERE " + whereClause + ";";
    }else
        sqlQuery += ";";

    return execSQL(sqlQuery);
}


bool RetroDb::sqlUpdate(const std::string &tableName, std::string whereClause, const ContentValue& cv){

    std::string sqlQuery = "UPDATE " + tableName + " SET ";


    std::map<std::string, uint8_t> keyTypeMap;
    std::map<std::string, uint8_t>::iterator mit;
    cv.getKeyTypeMap(keyTypeMap);

    // build SET part of update
    std::string qValues = "";
    std::ostringstream oStrStream;

    for(mit=keyTypeMap.begin(); mit!=keyTypeMap.end(); mit++){

        uint8_t type = mit->second;
        std::string key = mit->first;

        if( ContentValue::BOOL_TYPE == type)
            {
                bool value;
                cv.getAsBool(key, value);
                oStrStream << value;
                qValues += key + "='" + oStrStream.str();
            }
        else if( ContentValue::DOUBLE_TYPE == type)
            {
                double value;
                cv.getAsDouble(key, value);
                oStrStream << value;
                qValues += key + "='" + oStrStream.str();
            }
        else if( ContentValue::DATA_TYPE == type)
            {
                char* value;
                uint32_t len;
                cv.getAsData(key, len, value);
                oStrStream.write(value, len);
                qValues += key + "='" + oStrStream.str() + "' ";
            }
        else if( ContentValue::STRING_TYPE == type)
            {
                std::string value;
                cv.getAsString(key, value);
                qValues += key + "='" + value + "' ";
            }
        else if( ContentValue::INT32_TYPE == type)
            {
                int32_t value;
                cv.getAsInt32(key, value);
                oStrStream << value;
                qValues += key + "='" + oStrStream.str() + "' ";
            }
        else if( ContentValue::INT64_TYPE == type)
            {
                int64_t value;
                cv.getAsInt64(key, value);
                oStrStream << value;
                qValues += key + "='" + oStrStream.str() + "' ";
            }

        mit++;
        if(mit != keyTypeMap.end()){ // add comma if more columns left
            qValues += ",";
        }
        mit--;

        // clear stream strings
        oStrStream.str("");
    }

    if(qValues.empty())
        return false;
    else
        sqlQuery += qValues;

    // complete update
    if(!whereClause.empty()){
        sqlQuery += " WHERE " +  whereClause + ";";
    }
    else{
        sqlQuery += ";";
    }

    // execute query
    return execSQL(sqlQuery);
}





/********************** RetroCursor ************************/

RetroCursor::RetroCursor(sqlite3_stmt *stmt)
    : mStmt(NULL), mCount(0), mPosCounter(0) {

     open(stmt);
}

RetroCursor::~RetroCursor(){

    // finalise statement
    if(mStmt){
        sqlite3_finalize(mStmt);
    }
}

bool RetroCursor::moveToFirst(){

#ifdef RETRODB_DEBUG
    std::cerr << "RetroCursor::moveToFirst()\n";
#endif

    if(!isOpen())
        return false;

    // reset statement
    int rc = sqlite3_reset(mStmt);

    if(rc != SQLITE_OK){

#ifdef RETRODB_DEBUG
    std::cerr << "Error code: " << rc << std::endl;
#endif

        return false;
    }

    rc = sqlite3_step(mStmt);

    if(rc == SQLITE_ROW){
        mPosCounter = 0;
        return true;
    }

#ifdef RETRODB_DEBUG
    std::cerr << "Error code: " << rc << std::endl;
#endif

    return false;
}

bool RetroCursor::moveToLast(){

#ifdef RETRODB_DEBUG
    std::cerr << "RetroCursor::moveToLast()\n";
#endif

    if(!isOpen())
        return -1;

    // go to begining
    int rc = sqlite3_reset(mStmt);

    if(rc != SQLITE_OK)
        return false;

    rc = sqlite3_step(mStmt);

    while(rc == SQLITE_ROW){
        rc = sqlite3_step(mStmt);
    }

    if(rc != SQLITE_DONE){
        std::cerr << "Error executing statement (code: " << rc << ")\n"
                  << std::endl;
        return false;

    }else{
        mPosCounter = mCount;
        return true;
    }
}

int RetroCursor::getResultCount() const {

    if(isOpen())
        return mCount;
    else
        return -1;
}

bool RetroCursor::isOpen() const {
    return !(mStmt == NULL);
}

bool RetroCursor::close(){

    if(!isOpen())
        return false;


    int rc = sqlite3_finalize(mStmt);
    mStmt = NULL;
    mPosCounter = 0;
    mCount = 0;

    return (rc == SQLITE_OK);
}

bool RetroCursor::open(sqlite3_stmt *stm){

#ifdef RETRODB_DEBUG
    std::cerr << "RetroCursor::open() \n";
#endif

    if(isOpen())
        close();

    mStmt = stm;

    // ensure statement is valid
    int rc = sqlite3_reset(mStmt);

    if(rc == SQLITE_OK){

        while((rc = sqlite3_step(mStmt)) == SQLITE_ROW)
            mCount++;

        sqlite3_reset(mStmt);

        return true;
    }
    else{
        std::cerr << "Error Opening cursor (code: " << rc << ")\n";
        close();
        return false;
    }

}

bool RetroCursor::moveToNext(){

#ifdef RETRODB_DEBUG
    std::cerr << "RetroCursor::moveToNext()\n";
#endif

    if(!isOpen())
        return false;

    int rc = sqlite3_step(mStmt);

    if(rc == SQLITE_ROW){
        mPosCounter++;
        return true;

    }else if(rc == SQLITE_DONE){ // no more results
        return false;
    }
    else if(rc == SQLITE_BUSY){ // should not enter here
        std::cerr << "RetroDb::moveToNext()\n" ;
        std::cerr << "Busy!, possible multiple accesses to Db" << std::endl
                  << "serious error";

        return false;

    }else{
        std::cerr << "Error executing statement (code: " << rc << ")\n";
        return false;
    }
}


int32_t RetroCursor::getPosition() const {

    if(isOpen())
        return mPosCounter;
    else
        return -1;
}


int32_t RetroCursor::getInt32(int columnIndex){
    return sqlite3_column_int(mStmt, columnIndex);
}

int64_t RetroCursor::getInt64(int columnIndex){
    return sqlite3_column_int64(mStmt, columnIndex);
}

bool RetroCursor::getBool(int columnIndex){
    return sqlite3_column_int(mStmt, columnIndex);
}

double RetroCursor::getDouble(int columnIndex){
    return sqlite3_column_double(mStmt, columnIndex);
}

void RetroCursor::getString(int columnIndex, std::string &str){
    char* raw_str = (char*)sqlite3_column_text(mStmt, columnIndex);
    if(raw_str != NULL)
        str.assign(raw_str);
}

const void* RetroCursor::getData(int columnIndex, uint32_t &datSize){

    const void* val = sqlite3_column_blob(mStmt, columnIndex);
    datSize = sqlite3_column_bytes(mStmt, columnIndex);

    return val;
}


