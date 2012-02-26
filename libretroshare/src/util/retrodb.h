#ifndef RSSQLITE_H
#define RSSQLITE_H

#include "sqlite3.h"

#include <string>
#include <set>
#include <map>


class ContentValue;
class RetroCursor;

/*!
 * The idea of RsDb is to provide a means for Retroshare core and \n
 * its services to maintain an easy to use random access file via a database \n
 * It models itself after android's sqlite functionality \n
 * This is essentially close of Androids SQLiteDatabase
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
     * @return false if we failed to open
     */
    bool openDb(const std::string& dbPath, int flags = OPEN_READONLY);

    /*!
     * close the database
     */
    void closeDb();

    /* modifying db */
public:

    /*!
     * To make queries which does not return a result
     * @param query SQL query
     */
    void execSQL(const std::string& query);

    /*!
     * inserts a row in a database table
     * @param table table you want to insert content values into
     * @param cv hold entries to insert
     * @return true if insertion successful, false otherwise
     */
    bool sqlInsert(std::string& table, std::string nullColumnHack, const ContentValue& cv);

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
     * @param columns columns that should be returned
     * @param selection  A filter declaring which rows to return, formatted as \n
     *        an SQL WHERE clause (excluding the WHERE itself). Passing null will \n
     *        return all rows for the given table.
     * @param order the rows, formatted as an SQL ORDER BY clause (excluding the ORDER BY itself)
     * @return cursor over result set
     */
    RetroCursor* sqlQuery(const std::string& tableName, const std::set<std::string>& columns,
                          const std::string& selection, const std::string& orderBy);

    /*!
     * delete row in an sql table
     * @param tableName the table on which to apply the DELETE
     * @param whereClause formatted as where statement without 'WHERE' itself
     * @return false
     */
    bool sqlDelete(const std::string& tableName, const std::string& whereClause);

    /*!
     * defragment database, should be done on databases if many modifications have occured
     */
    void vacuum();

    /*!
     * TODO: remove, do not use; for testing
     */
     sqlite3_stmt* getCurrStmt();

public:

    static const int OPEN_READONLY;
    static const int OPEN_READWRITE;

private:


    sqlite3* mDb;
    sqlite3_stmt* mCurrStmt; // TODO: remove

};

/*!
 * Exposes result set from retrodb query
 */
class RetroCursor {

public:

    /*!
     * Initialises a null cursor
     */
    RetroCursor(sqlite3_stmt*);

    /*!
     * move to first row of results
     * @return false if no result
     */
    bool moveToFirst();

    /*!
     * move to next row of results
     * @return false if no row to move next to
     */
    bool moveToNext();

    /*!
     * move to last row of results
     * @return false if no result
     */
    bool moveToLast();

    /*!
     * gets current position of cursor
     * @return current position of cursor
     */
    uint32_t getPosition();

    /* data retrieval */
public:


    /*!
     * Returns the value of the requested column as a String.
     * @param columnIndex the zero-based index of the target column.
     * @return  the value of the column as 32 bit integer
     */
    int32_t getInt(int columnIndex);

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
    std::string getString(int columnIndex);

    /*!
     * Returns the value of the requested column as a String.
     * @param columnIndex the zero-based index of the target column.
     * @return  the value of the column as pointer to raw data
     */
    void* getData(int columnIndex, uint32_t& datSize);


private:

    sqlite3_stmt* mStmt;
    int mCount;
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
    void put(const std::string& key, uint32_t len, char* value);


    /*!
     * get as value as
     * @param key the value to get
     */
    bool getAsInt32(const std::string& key, int32_t& value) const;

    /*!
     * get as value as 64 bit integer
     * @param key the value to get
     */
    bool getAsInt64(const std::string& key, int64_t& value) const;

    /*!
     * get as value as bool
     * @param key the value to get
     */
    bool getAsBool(const std::string& key, bool& value) const;

    /*!
     * get as value as double
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
