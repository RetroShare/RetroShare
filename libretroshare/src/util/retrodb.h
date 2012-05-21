#ifndef RSSQLITE_H
#define RSSQLITE_H

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

#include "sqlite3.h"

#include <string>
#include <set>
#include <list>
#include <map>


class ContentValue;
class RetroCursor;

/*!
 * RetroDb provide a means for Retroshare's core and \n
 * services to maintain an easy to use random access file via a database \n
 * It models itself after android's sqlite functionality \n
 * This is essentially unashamedly a clone of Android's SQLiteDatabase interface
 */
class RetroDb
{
public:

    /*!
     * @param dbPath path to data base file
     * @param flags determine where to open read only or read/write
     */
    RetroDb(const std::string& dbPath, int flags);

    /*!
     * closes db if it is not already closed
     */
    ~RetroDb();

    /*!
     * opens sqlite data base
     * @param dbPath
     * @param flags
     * @return false if failed to open, true otherwise
     */
    bool openDb(const std::string& dbPath, int flags = OPEN_READONLY);

    /*!
     * close the database
     */
    void closeDb();

    /*!
     *
     * @return false if database is not open, true otherwise
     */
    bool isOpen() const;

    /* modifying db */
public:



    /*!
     * To a make query which do not return a result \n
     * below are the type of queries this method should be used for \n
     * ALTER TABLE \n
     * CREATE or DROP table / trigger / view / index / virtual table \n
     * REINDEX \n
     * RELEASE \n
     * SAVEPOINT \n
     * PRAGMA that returns no data \n
     * @param query SQL query
     * @return false if there was an sqlite error, true otherwise
     */
    bool execSQL(const std::string& query);

    /*!
     * inserts a row in a database table
     * @param table table you want to insert content values into
     * @param nullColumnHack  SQL doesn't allow inserting a completely \n
     *        empty row without naming at least one column name
     * @param cv hold entries to insert
     * @return true if insertion successful, false otherwise
     */
    bool sqlInsert(const std::string& table,const  std::string& nullColumnHack, const ContentValue& cv);

    /*!
     * update row in a database table
     * @param tableName the table on which to apply the UPDATE
     * @param whereClause formatted as where statement without 'WHERE' itself
     * @param cv Values used to replace current values in accessed record
     * @return true if update was successful, false otherwise
     */
    bool sqlUpdate(const std::string& tableName, const std::string whereClause, const ContentValue& cv);

    /*!
     * Query the given table, returning a Cursor over the result set
     * @param tableName the table name
     * @param columns list columns that should be returned and their order (the list's order)
     * @param selection  A filter declaring which rows to return, formatted as \n
     *        an SQL WHERE clause (excluding the WHERE itself). Passing null will \n
     *        return all rows for the given table.
     * @param order the rows, formatted as an SQL ORDER BY clause (excluding the ORDER BY itself)
     * @return cursor over result set, this allocated resource should be free'd after use \n
     *         column order is in list order.
     */
    RetroCursor* sqlQuery(const std::string& tableName, const std::list<std::string>& columns,
                          const std::string& selection, const std::string& orderBy);

    /*!
     * delete row in an sql table
     * @param tableName the table on which to apply the DELETE
     * @param whereClause formatted as where statement without 'WHERE' itself
     * @return false
     */
    bool sqlDelete(const std::string& tableName, const std::string& whereClause, const std::string& whereArgs);

    /*!
     * TODO
     * defragment database, should be done on databases if many modifications have occured
     */
    void vacuum();

public:

    static const int OPEN_READONLY;
    static const int OPEN_READWRITE;
    static const int OPEN_READWRITE_CREATE;

private:

    class RetroDbBlob{

    public:

        char* data;
        uint32_t length;
        uint32_t index;
    };

    bool execSQL_bind_blobs(const std::string &query, std::list<RetroDbBlob>& blobs);

private:

    sqlite3* mDb;

};

/*!
 * Exposes result set from retrodb query
 */
class RetroCursor {

public:

    /*!
     * Initialises a null cursor
     * @warning cursor takes ownership of statement passed to it
     */
    RetroCursor(sqlite3_stmt*);

    ~RetroCursor();

    /*!
     * move to first row of results
     * @return false if no results
     */
    bool moveToFirst();

    /*!
     * move to next row of results
     * @return false if no further results
     */
    bool moveToNext();

    /*!
     * move to last row of results
     * @return false if no result, true otherwise
     */
    bool moveToLast();

    /*!
     * gets current position of cursor
     * @return current position of cursor
     */
    int32_t getPosition() const;

    /* data retrieval */

    /*!
     * @return true if cursor is open and active, false otherwise
     */
    bool isOpen() const;

    /*!
     * cursor is closed, statement used to open cursor is deleted
     * @return false if error on close (was already closed, error occured)
     */
    bool close();

    /*!
     *
     * @return -1 if cursor is in error, otherwise number of rows in result
     */
    int32_t getResultCount() const;

    /*!
     * Current statement is closed and discarded (finalised)
     * before actual opening occurs
     * @param stm statement to open cursor on
     * @return true if cursor is successfully opened
     */
    bool open(sqlite3_stmt* stm);

public:


    /*!
     * Returns the value of the requested column as a String.
     * @param columnIndex the zero-based index of the target column.
     * @return  the value of the column as 32 bit integer
     */
    int32_t getInt32(int columnIndex);

    /*!
     * Returns the value of the requested column as a String.
     * @param columnIndex the zero-based index of the target column.
     * @return the value of the column as 64 bit integer
     */
    int64_t getInt64(int columnIndex);

    /*!
     * Returns the value of the requested column as a String.
     * @param columnIndex the zero-based index of the target column.
     * @return  the value of the column as 64 bit float
     */
    double getDouble(int columnIndex);

    /*!
     * Returns the value of the requested column as a String.
     * @param columnIndex the zero-based index of the target column.
     * @return  the value of the column as bool
     */
    bool getBool(int columnIndex);

    /*!
     * Returns the value of the requested column as a String.
     * @param columnIndex the zero-based index of the target column.
     * @return the value of the column as a string
     */
    void getString(int columnIndex, std::string& str);

    /*!
     * Returns the value of the requested column as a String.
     * data returned must be copied, as it is freed after RetroDb
     * is closed or destroyed
     * @param columnIndex the zero-based index of the target column.
     * @return  the value of the column as pointer to raw data
     */
    const void* getData(int columnIndex, uint32_t& datSize);


private:

    sqlite3_stmt* mStmt;
    int mCount; /// number of results
    int mPosCounter;
};


/*!
 * @brief Convenience container for making additions to databases
 * This class provides a means of holding column values to insert into a database
 */
class ContentValue {

public:

    static const uint8_t INT32_TYPE;
    static const uint8_t INT64_TYPE;
    static const uint8_t DOUBLE_TYPE;
    static const uint8_t STRING_TYPE;
    static const uint8_t DATA_TYPE;
    static const uint8_t BOOL_TYPE;

    ContentValue();

    /*!
     * copy constructor that copys the key value set from another \n
     * ContentValue object to this one
     * makes a deep copy of raw data
     * @param from ContentValue instance to copy key value set from
     */
    ContentValue(ContentValue& from);

    /*!
     *
     *
     *
     */
    ~ContentValue();

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     * @warning cast string literals explicitly as string, observed string literal \n
     *          being casted to bool instead e.g. string("hello") rather than "hello"
     */
    void put(const std::string& key, const std::string& value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, bool value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, int64_t value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, int32_t value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, double value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, uint32_t len, const char* value);


    /*!
     * get value as 32 bit signed integer
     * @param key the value to get
     */
    bool getAsInt32(const std::string& key, int32_t& value) const;

    /*!
     * get value as 64 bit signed integer
     * @param key the value to get
     */
    bool getAsInt64(const std::string& key, int64_t& value) const;

    /*!
     * get value as bool
     * @param key the value to get
     */
    bool getAsBool(const std::string& key, bool& value) const;

    /*!
     * get as value double
     * @param key the value to get
     */
    bool getAsDouble(const std::string& key, double& value) const;

    /*!
     * get as value as string
     * @param key the value to get
     * @param value the data retrieved
     */
    bool getAsString(const std::string& key, std::string& value) const;

    /*!
     * get as value as raw data
     * @warning Deep copy of data reference should be made, if this instance ContentValue \n
     *          is destroyed pointer returned (value) is pointing to invalid memory
     * @param key the value to get
     */
    bool getAsData(const std::string&, uint32_t& len, char*& value) const;

    /*!
     * @param keySet the is set with key to type pairs contained in the ContentValue instance
     */
    void getKeyTypeMap(std::map<std::string, uint8_t>& keySet) const;

    /*!
     * @param key the key of the key value pair to remove
     * @return true if key was found and removed, false otherwise
     */
    bool removeKeyValue(const std::string& key);

    /*!
     * clears this data structure of all its key value pairs held
     */
    void clear();

private:

    /*!
     * release memory resource associated with mKvData
     */
    void clearData();

private:

    std::map<std::string, int32_t> mKvInt32;
    std::map<std::string, int64_t> mKvInt64;
    std::map<std::string, double> mKvDouble;
    std::map<std::string, std::string> mKvString;
    std::map<std::string, std::pair<uint32_t, char*> > mKvData;
    std::map<std::string, bool> mKvBool;

    std::map<std::string, uint8_t> mKvSet;

};


#endif // RSSQLITE_H
