
#include <iostream>
#include <memory.h>
#include <sstream>
#include "util/rstime.h"

#include "retrodb.h"
#include "utest.h"


#define INT32_KEY "day"
#define INT64_KEY "pub_key"
#define DATA_KEY "data"
#define DOUBLE_KEY "a_double"
#define STRING_KEY "a_string"
#define BOOL_KEY "a_bool"

#define DB_FILE_NAME "RetroDb"

static RetroDb* mDb = NULL;

void createRetroDb();
void destroyRetroDb();

INITTEST();
// mainly test ability to move up and down result set


void testTraverseResult();


int main() {

    testTraverseResult();

    FINALREPORT("RETRO CURSOR TEST");
    return 0;
}



void createRetroDb(){

    if(mDb)
        destroyRetroDb();


    remove(DB_FILE_NAME); // account for interrupted tests
    mDb = new RetroDb(DB_FILE_NAME, RetroDb::OPEN_READWRITE_CREATE);
}

void destroyRetroDb(){
    if(mDb == NULL)
        return;

    mDb->closeDb();
    delete mDb;
    mDb = NULL;
    int rc = remove(DB_FILE_NAME);
    std::cerr << "remove code: " << rc << std::endl;

    if(rc !=0){
        perror("Could not delete db: ");

    }
}


void testTraverseResult(){


    createRetroDb();

    bool statementExecuted = mDb->execSQL("CREATE TABLE retroDB(day INTEGER PRIMARY KEY ASC, pub_key INT, data BLOB, a_double REAL, a_string VARCHAR(255), a_bool INT);");
    CHECK(statementExecuted);

    ContentValue cv;

    cv.put(INT32_KEY, (int32_t)20);
    int64_t large_num = 32432242344423;
    cv.put(INT64_KEY, large_num);
    std::string str = "3dajaojaljfacjlaf£%£^%\"%\"%$";
    const char* data = str.data();
    int size = str.size();
    cv.put(DATA_KEY, size, data);
    cv.put(DOUBLE_KEY, 3.14);
    cv.put(STRING_KEY, "hello precious");
    cv.put(BOOL_KEY, false);

    bool insertExecuted = mDb->sqlInsert("retroDB", "", cv);
    cv.put(INT32_KEY, (int32_t)21);
    insertExecuted &= mDb->sqlInsert("retroDB", "", cv);
    cv.put(INT32_KEY, (int32_t)2);
    insertExecuted &= mDb->sqlInsert("retroDB", "", cv);
    cv.put(INT32_KEY, (int32_t)204);
    insertExecuted &= mDb->sqlInsert("retroDB", "", cv);
    cv.put(INT32_KEY, (int32_t)22);
    insertExecuted &= mDb->sqlInsert("retroDB", "", cv);

    CHECK(insertExecuted);

    std::list<std::string> columns;
    columns.push_back(INT32_KEY); columns.push_back(INT64_KEY); columns.push_back(DOUBLE_KEY);
    columns.push_back(DATA_KEY); columns.push_back(BOOL_KEY); columns.push_back(STRING_KEY);
    std::string orderBy = std::string(INT32_KEY) + " ASC";
    RetroCursor* cursor = mDb->sqlQuery("retroDB", columns, "", orderBy);

    CHECK(cursor->getResultCount() == 5);
    cursor->moveToFirst();
    CHECK(cursor->getPosition() == 0);

    cursor->moveToLast();
    CHECK(cursor->getPosition() == cursor->getResultCount());

    cursor->moveToFirst();

    CHECK(cursor->getInt32(0) == 2);
    cursor->moveToNext();
    cursor->moveToNext();

    CHECK(cursor->getInt32(0) == 21);

    delete cursor;

    REPORT("testTraverseResult()");
    destroyRetroDb();
}
