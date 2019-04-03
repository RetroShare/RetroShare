/*******************************************************************************
 * libretroshare/src/util: retrodb.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Christopher Evi-Parker <retroshare@lunamutt.com>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RSSQLITE_H
#define RSSQLITE_H

#ifdef NO_SQLCIPHER
#include <sqlite3.h>
#else
#include <sqlcipher/sqlite3.h>
#endif

#include <string>
#include <set>
#include <list>
#include <map>
#include "rsdbbind.h"

#include "contentvalue.h"

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
    RetroDb(const std::string& dbPath, int flags, const std::string& key = "");

    /*!
     * closes db if it is not already closed
     */
    ~RetroDb();

    /*!
     * @return key used to encrypt database
     */
    std::string getKey() const;

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
     * Start transaction
     * @return true/false
     */
    bool beginTransaction();

    /*!
     * Commit transaction
     * @return true/false
     */
    bool commitTransaction();

    /*!
     * Rollback transaction
     * @return true/false
     */
    bool rollbackTransaction();

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

    /*!
     * Check if table exist in database
     * @param tableName table to check
     * @return true/false
     */
    bool tableExists(const std::string& tableName);

public:

    static const int OPEN_READONLY;
    static const int OPEN_READWRITE;
    static const int OPEN_READWRITE_CREATE;

private:

    bool execSQL_bind(const std::string &query, std::list<RetroBind*>& blobs);

    /*!
     * Build the "VALUE" part of an insertiong sql query
     * @param parameter contains place holder query
     * @param paramBindings
     */
    void buildInsertQueryValue(const std::map<std::string, uint8_t> keyMap, const ContentValue& cv,
            std::string& parameter, std::list<RetroBind*>& paramBindings);

    /*!
     * Build the "VALUE" part of an insertiong sql query
     * @param parameter contains place holder query
     * @param paramBindings
     */
    void buildUpdateQueryValue(const std::map<std::string, uint8_t> keyMap, const ContentValue& cv,
            std::string& parameter, std::list<RetroBind*>& paramBindings);

private:

    sqlite3* mDb;
    const std::string mKey;
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
     * @return -1 if cursor is in error, otherwise number of columns in result
     */
    int32_t columnCount() const ;

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

    template <class T>
    inline void getStringT(int columnIndex, T &str){
    	std::string temp;
    	getString(columnIndex, temp);
    	str = T(temp);
    }
private:
    sqlite3_stmt* mStmt;
};

#endif // RSSQLITE_H
