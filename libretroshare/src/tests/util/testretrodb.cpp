
#include <iostream>
#include <memory.h>
#include <sstream>
#include "util/rstime.h"

#include "retrodb.h"
#include "utest.h"


#define DAY_KEY "day"
#define PUB_KEY "pub_key"
#define DATA_KEY "data"
#define DOUBLE_KEY "a_double"

#define DB_FILE_NAME "RetroDb"

static RetroDb* mDb = NULL;

void createRetroDb();
void destroyRetroDb();

void testSqlExec();
void testSqlUpdate();
void testSqlInsert();
void testSqlDelete();
void testSqlQuery();
void testBinaryInsertion();

INITTEST();

int main(){

    testSqlExec();
    testBinaryInsertion();
    testSqlInsert();
    testSqlUpdate();
    testSqlDelete();
    testSqlQuery();

    FINALREPORT("RETRO DB TEST");
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

void testSqlExec(){

      createRetroDb();

    // create simple table
    bool statementExecuted = mDb->execSQL("CREATE TABLE retroDB(day INTEGER PRIMARY KEY ASC, pub_key, data);");
    statementExecuted &= mDb->execSQL("INSERT INTO retroDB(day, pub_key, data) VALUES(1,2,3);");
    statementExecuted &= mDb->execSQL("INSERT INTO retroDB(day, pub_key, data) VALUES(3,4525624,4524);");

    CHECK(statementExecuted);

    // now check if you can retrieve records
    std::list<std::string> columns;
    columns.push_back("day");
    columns.push_back("pub_key");
    columns.push_back("data");

    std::string selection, orderBy;
    RetroCursor* c = mDb->sqlQuery("retroDB", columns, selection, orderBy);


    CHECK(c->getResultCount() == 2);

    // got to first record
    c->moveToFirst();
    int32_t first =c->getInt32(0), second = c->getInt32(1),
    third = c->getInt32(2);


    CHECK(first == 1);
    CHECK(second == 2);
    CHECK(third == 3);

    // get 2nd record
    c->moveToNext();

    first =c->getInt32(0), second = c->getInt32(1),
    third = c->getInt32(2);

    CHECK(first == 3);
    CHECK(second == 4525624);
    CHECK(third == 4524);

    delete c;

    REPORT("testSqlExec()");
    destroyRetroDb();
}

void testBinaryInsertion(){

    createRetroDb();
    // create simple table
    bool statementExecuted = mDb->execSQL("CREATE TABLE retroDB(day INTEGER PRIMARY KEY ASC, data BLOB);");
    statementExecuted &= mDb->execSQL("INSERT INTO retroDB(day, data) VALUES(1, 'dafadfad%$£%^£%%\"$R\"$\"$\"');");

    // now check if you can retrieve records
    std::list<std::string> columns;
    columns.push_back("day");
    columns.push_back("data");

    std::string selection, orderBy;
    RetroCursor* c = mDb->sqlQuery("retroDB", columns, selection, orderBy);

    c->moveToFirst();
    int first = c->getInt32(0);
    uint32_t size;
    const char* data = (const char*)c->getData(1, size);
    std::string str = "dafadfad%$£%^£%%\"$R\"$\"$\"";
    const char* data_comp = str.data();
    bool binCompare = ok;

    for(int i=0; i < 24 ; i++)
        binCompare &= data[i] == data_comp[i];

    CHECK(first == 1);
    CHECK(binCompare);

    delete c;
    REPORT("testBinaryInsertion()");
    destroyRetroDb();
}




void testSqlUpdate(){

    createRetroDb();

    bool statementExecuted = mDb->execSQL("CREATE TABLE retroDB(day INTEGER PRIMARY KEY ASC, data BLOB, peerId VARCHAR(255), time INT);");
    CHECK(statementExecuted);

    std::string data = "dadJOOodaodaoro20r2-0r20002ri02fgi3t0***";
    ContentValue cv;

    cv.put("day", (int32_t)20);
    std::string peerid = "TheRetroSquad";
    cv.put("peerId", peerid);
    cv.put("data", data.size(), data.data());
    int64_t now = time(NULL);
    cv.put("time", now);
    bool insertExecuted = mDb->sqlInsert("retroDB", "", cv);

    CHECK(insertExecuted);

    // now check entry is fine

    std::list<std::string> columns;
    columns.push_back("day"); columns.push_back("peerId"); columns.push_back("data"); columns.push_back("time");

    RetroCursor* c = mDb->sqlQuery("retroDB", columns, "", "");

    CHECK(c->getResultCount() == 1);
    std::string result;
    c->moveToFirst();
    c->getString(1, result);

    CHECK(result == peerid);

    delete c;
    c = NULL;

    // now make an update and see if this is reflected

    int64_t now_plus = now+203;
    cv.put("time", now_plus);
    bool update = mDb->sqlUpdate("retroDB", "", cv);
    CHECK(update);
    c = mDb->sqlQuery("retroDB", columns, "", "");
    c->moveToFirst();
    int64_t now_plus_compare = c->getDouble(3);

    CHECK(now_plus_compare == now_plus);

    // now attempt an update which should find no valid record


    delete c;

    REPORT("testSqlUpdate()");
    destroyRetroDb();
}

void testSqlInsert(){

    createRetroDb();

    bool statementExecuted = mDb->execSQL("CREATE TABLE retroDB(day INTEGER PRIMARY KEY ASC, a_double DOUBLE, data BLOB);");
    CHECK(statementExecuted);

    ContentValue cv;

    cv.put("day", (int32_t)20);
    std::string str = "3dajaojaljfacjlaf£%£^%\"%\"%$";
    const char* data = str.data();
    int size = 27;
    cv.put(DATA_KEY, size, data);
    cv.put(DOUBLE_KEY, 3.14);
    bool insertExecuted = mDb->sqlInsert("retroDB", "", cv);

    std::list<std::string> columns;
    columns.push_back("day"); columns.push_back("a_double"); columns.push_back("data");
    RetroCursor* cursor = mDb->sqlQuery("retroDB", columns, "", "");

    CHECK(cursor != NULL);

    cursor->moveToFirst();

    CHECK(20 == cursor->getInt32(0));
    CHECK(3.14 == cursor->getDouble(1));

    CHECK(insertExecuted);

    delete cursor;
    FINALREPORT("testSqlInsert()");
    destroyRetroDb();
}

void testSqlQuery(){
    //  test ordering of data and selection clause


    createRetroDb();

  // create simple table
    bool statementExecuted = mDb->execSQL("CREATE TABLE retroDB(priority INTEGER PRIMARY KEY ASC, name VARCHAR(255), friends, games INTEGER);");
    statementExecuted &= mDb->execSQL("INSERT INTO retroDB(priority, name, friends, games) VALUES(4,'sammy',2,30);");
    statementExecuted &= mDb->execSQL("INSERT INTO retroDB(priority, name, friends, games) VALUES(2,'davy',6,9);");
    statementExecuted &= mDb->execSQL("INSERT INTO retroDB(priority, name, friends, games) VALUES(6,'pammy',2,4);");
    statementExecuted &= mDb->execSQL("INSERT INTO retroDB(priority, name, friends, games) VALUES(5,'tommy',3,4534);");
    statementExecuted &= mDb->execSQL("INSERT INTO retroDB(priority, name, friends, games) VALUES(9,'jonny',3,44);");

    CHECK(statementExecuted);

    std::list<std::string> columns;
    columns.push_back("name");
    std::string selection = "games <= 31";
    std::string orderBy = "priority DESC";


    // output should be by name: pammy, sammy and davy
    RetroCursor* cursor = mDb->sqlQuery("retroDB", columns,selection, orderBy);

    cursor->moveToFirst();
    std::string name;
    cursor->getString(0,name);
    CHECK(name == "pammy");

    cursor->moveToNext();
    cursor->getString(0,name);
    CHECK(name == "sammy");

    cursor->moveToNext();
    cursor->getString(0,name);
    CHECK(name == "davy");


    delete cursor;
    FINALREPORT("testSqlQuery()");
    destroyRetroDb();
    return;
}

void testSqlDelete(){

    createRetroDb();

    bool statementExecuted = mDb->execSQL("CREATE TABLE retroDB(day INTEGER PRIMARY KEY ASC, a_double DOUBLE, data BLOB);");
    CHECK(statementExecuted);

    ContentValue cv;

    cv.put("day", (int32_t)20);
    std::string str = "3dajaojaljfacjlaf£%£^%\"%\"%$";
    const char* data = str.data();
    uint32_t size = str.size();
    cv.put(DATA_KEY, size, data);
    cv.put(DOUBLE_KEY, 3.14);

    // insert to records
    bool insertExecuted = mDb->sqlInsert("retroDB", "", cv);
    cv.put("day", (int32_t(5)));
    CHECK(insertExecuted);
    mDb->sqlInsert("retroDB", "", cv);


    // now check that their are two records in the db
    std::list<std::string> columns;
    columns.push_back("day"); columns.push_back("a_double"); columns.push_back("data");
    RetroCursor* cursor = mDb->sqlQuery("retroDB", columns, "", "");
    CHECK(cursor->getResultCount() == 2);
    delete cursor;
    // now remove a record and search for the removed record, query should return no records
    mDb->sqlDelete("retroDB", "day=5", "");


    cursor = mDb->sqlQuery("retroDB", columns, "day=5", "");
    CHECK(cursor->getResultCount() == 0);
    delete cursor;

    // now check for the remaining record
    cursor = mDb->sqlQuery("retroDB", columns, "day=20", "");
    CHECK(cursor->getResultCount() == 1);
    cursor->moveToFirst();

    // verify there is no data corruption
    const char* data_comp1 = (const char*)cursor->getData(2, size);
    const char* data_comp2 = str.data();

    bool binCompare = true;

    for(int i=0; i < str.size() ; i++)
        binCompare &= data_comp1[i] == data_comp2[i];

    CHECK(binCompare);

    delete cursor;

    FINALREPORT("testSqlDelete()");
    destroyRetroDb();
}
