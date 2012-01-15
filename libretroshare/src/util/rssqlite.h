#ifndef RSSQLITE_H
#define RSSQLITE_H

#include "sqlite3.h"

#include <string>
#include <set>

/*!
 * The idea of RsDb is to provide a means for Retroshare core and its services itself to maintain
 * easy to use random access files via a database
 * Especially for messages, rather than all data to memory
 * It models itself after android's sqlite functionality
 */
class RetroDb
{
public:

    /*!
     * @param dbPath path to data base file
     */
    RetroDb(const std::string dbPath);

    /*!
     * closes db if it is not already closed
     */
    ~RetroDb();

    /*!
     * opens an sqlite data base
     */
    bool openDb(const std::string dbPath);
    void closeDb();

    /* modifying db */
public:

    void execSQL(const std::string&);

    /*!
     * insert a row in a database table
     * @return true if insertion successful, false otherwise
     */
    bool sqlInsert(std::string& table, const ContentValue&);

    /*!
     * update row in a database table
     * @return true if update was successful, false otherwise
     */
    bool sqlUpdate(const std::string& tableName, const std::set<std::string>& columns, const std::string& query);

    /*!
     * Query the given table, returning a Cursor over the result set
     * @param tableName
     * @param columns
     * @param query
     * @return cursor over result set
     */
    RetroCursor* sqlQuery(const std::string& tableName, const std::set<std::string>& columns, const std::string& query);

    /*!
     * delete row in an sql table
     *
     */
    bool sqlDelete(const std::string& tableName, const std::string& query);

    /*!
     * defragment database, should be done on databases if many modifications have occured
     */
    void vacuum();

private:


    sqlite3* mDb;

};

/*!
 * Exposes results from retrodb query
 */
class RetroCursor {

public:

    RetroCursor();

    /*!
     * move to first row of result,
     * @return false if no result
     */
    bool moveToFirst();

    /*!
     * move to
     * @return false if no row to move next to
     */
    bool moveToNext();

    /*!
     * move to last row of result
     * @return false if no result
     */
    bool moveToLast();

    /* data retrieval */
public:

    int32_t getInt32(const std::string& );
    uint32_t getUint32(const std::string& );
    int64_t getInt64(const std::string& );
    uint64_t getUint64(const std::string& );
    bool getBool(const std::string& );
    std::string getString(const std::string& );
    void* getData(const std::string&, uint32_t& );
};

/*!
 * @brief Convenience class for making additions to databases
 *
 *
 *
 */
class ContentValue {

public:
    ContentValue();

    void put(const std::string&, uint32_t );
    void put(const std::string &, uint64_t);
    void put(const std::string& , const std::string& );
    void put(const std::string& , bool);
    void put(const std::string& , int64_t);
    void put(const std::string &, int32_t);
    void put(const std::string&, uint32_t len, void* data);
    void put(const std::string&, int64_t );


    int32_t getAsInt32(const std::string& );
    uint32_t getAsUint32(const std::string& );
    int64_t getAsInt64(const std::string& );
    uint64_t getAsUint64(const std::string& );
    bool getAsBool(const std::string& );
    std::string getAsString(const std::string& );
    void* getAsData(const std::string&, uint32_t& );

};


#endif // RSSQLITE_H
