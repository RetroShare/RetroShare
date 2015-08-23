
/*
 * libretroshare/src/gxs: rsdataservice.cc
 *
 * Data Access, interface for RetroShare.
 *
 * Copyright 2011-2011 by Evi-Parker Christopher
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

/*****
 * #define RS_DATA_SERVICE_DEBUG       1
 * #define RS_DATA_SERVICE_DEBUG_TIME  1
 ****/

#include <fstream>
#include <util/rsdir.h>
#include <algorithm>

#ifdef RS_DATA_SERVICE_DEBUG_TIME
#include <util/rsscopetimer.h>
#endif

#include "rsdataservice.h"
#include "util/rsstring.h"

#define MSG_TABLE_NAME std::string("MESSAGES")
#define GRP_TABLE_NAME std::string("GROUPS")
#define DATABASE_RELEASE_TABLE_NAME std::string("DATABASE_RELEASE")

#define GRP_LAST_POST_UPDATE_TRIGGER std::string("LAST_POST_UPDATE")

#define MSG_INDEX_GRPID std::string("INDEX_MESSAGES_GRPID")

// generic
#define KEY_NXS_DATA std::string("nxsData")
#define KEY_NXS_DATA_LEN std::string("nxsDataLen")
#define KEY_NXS_IDENTITY std::string("identity")
#define KEY_GRP_ID std::string("grpId")
#define KEY_ORIG_GRP_ID std::string("origGrpId")
#define KEY_PARENT_GRP_ID std::string("parentGrpId")
#define KEY_SIGN_SET std::string("signSet")
#define KEY_TIME_STAMP std::string("timeStamp")
#define KEY_NXS_FLAGS std::string("flags")
#define KEY_NXS_META std::string("meta")
#define KEY_NXS_SERV_STRING std::string("serv_str")
#define KEY_NXS_HASH std::string("hash")
#define KEY_RECV_TS std::string("recv_time_stamp")

// remove later
#define KEY_NXS_FILE_OLD std::string("nxsFile")
#define KEY_NXS_FILE_OFFSET_OLD std::string("fileOffset")
#define KEY_NXS_FILE_LEN_OLD std::string("nxsFileLen")

// grp table columns
#define KEY_KEY_SET std::string("keySet")
#define KEY_GRP_NAME std::string("grpName")
#define KEY_GRP_SIGN_FLAGS std::string("signFlags")
#define KEY_GRP_CIRCLE_ID std::string("circleId")
#define KEY_GRP_CIRCLE_TYPE std::string("circleType")
#define KEY_GRP_INTERNAL_CIRCLE std::string("internalCircle")
#define KEY_GRP_ORIGINATOR std::string("originator")
#define KEY_GRP_AUTHEN_FLAGS std::string("authenFlags")

// grp local
#define KEY_GRP_SUBCR_FLAG std::string("subscribeFlag")
#define KEY_GRP_POP std::string("popularity")
#define KEY_MSG_COUNT std::string("msgCount")
#define KEY_GRP_STATUS std::string("grpStatus")
#define KEY_GRP_LAST_POST std::string("lastPost")
#define KEY_GRP_REP_CUTOFF std::string("rep_cutoff")


// msg table columns
#define KEY_MSG_ID std::string("msgId")
#define KEY_ORIG_MSG_ID std::string("origMsgId")
#define KEY_MSG_PARENT_ID std::string("parentId")
#define KEY_MSG_THREAD_ID std::string("threadId")
#define KEY_MSG_NAME std::string("msgName")

// msg local
#define KEY_MSG_STATUS std::string("msgStatus")
#define KEY_CHILD_TS std::string("childTs")

// database release columns
#define KEY_DATABASE_RELEASE_ID std::string("id")
#define KEY_DATABASE_RELEASE_ID_VALUE 1
#define KEY_DATABASE_RELEASE std::string("release")



/*** actual data col numbers ***/

// generic
#define COL_ACT_GROUP_ID 0
#define COL_NXS_DATA 1
#define COL_NXS_DATA_LEN 2
#define COL_META_DATA 3
#define COL_ACT_MSG_ID 4

/*** meta column numbers ***/

// grp col numbers

#define COL_KEY_SET 6
#define COL_GRP_SUBCR_FLAG 7
#define COL_GRP_POP 8
#define COL_MSG_COUNT 9
#define COL_GRP_STATUS 10
#define COL_GRP_NAME 11
#define COL_GRP_LAST_POST 12
#define COL_ORIG_GRP_ID 13
#define COL_GRP_SERV_STRING 14
#define COL_GRP_SIGN_FLAGS 15
#define COL_GRP_CIRCLE_ID 16
#define COL_GRP_CIRCL_TYPE 17
#define COL_GRP_INTERN_CIRCLE 18
#define COL_GRP_ORIGINATOR 19
#define COL_GRP_AUTHEN_FLAGS 20
#define COL_PARENT_GRP_ID 21
#define COL_GRP_RECV_TS 22
#define COL_GRP_REP_CUTOFF 23
#define COL_GRP_DATA_LEN 24


// msg col numbers
#define COL_MSG_ID 6
#define COL_ORIG_MSG_ID 7
#define COL_MSG_STATUS 8
#define COL_CHILD_TS 9
#define COL_PARENT_ID 10
#define COL_THREAD_ID 11
#define COL_MSG_NAME 12
#define COL_MSG_SERV_STRING 13
#define COL_MSG_RECV_TS 14
#define COL_MSG_DATA_LEN 15

// generic meta shared col numbers
#define COL_GRP_ID 0
#define COL_TIME_STAMP 1
#define COL_NXS_FLAGS 2
#define COL_SIGN_SET 3
#define COL_IDENTITY 4
#define COL_HASH 5

const std::string RsGeneralDataService::GRP_META_SERV_STRING = KEY_NXS_SERV_STRING;
const std::string RsGeneralDataService::GRP_META_STATUS = KEY_GRP_STATUS;
const std::string RsGeneralDataService::GRP_META_SUBSCRIBE_FLAG = KEY_GRP_SUBCR_FLAG;
const std::string RsGeneralDataService::GRP_META_CUTOFF_LEVEL = KEY_GRP_REP_CUTOFF;

const std::string RsGeneralDataService::MSG_META_SERV_STRING = KEY_NXS_SERV_STRING;
const std::string RsGeneralDataService::MSG_META_STATUS = KEY_MSG_STATUS;

const uint32_t RsGeneralDataService::GXS_MAX_ITEM_SIZE = 1572864; // 1.5 Mbytes

RsDataService::RsDataService(const std::string &serviceDir, const std::string &dbName, uint16_t serviceType,
                             RsGxsSearchModule * /* mod */, const std::string& key)
    : RsGeneralDataService(), mDbMutex("RsDataService"), mServiceDir(serviceDir), mDbName(dbName), mDbPath(mServiceDir + "/" + dbName), mServType(serviceType), mDb(NULL)
{
    bool isNewDatabase = !RsDirUtil::fileExists(mDbPath);

    mDb = new RetroDb(mDbPath, RetroDb::OPEN_READWRITE_CREATE, key);

    initialise(isNewDatabase);

    // for retrieving msg meta
    msgMetaColumns.push_back(KEY_GRP_ID); msgMetaColumns.push_back(KEY_TIME_STAMP); msgMetaColumns.push_back(KEY_NXS_FLAGS);
    msgMetaColumns.push_back(KEY_SIGN_SET); msgMetaColumns.push_back(KEY_NXS_IDENTITY); msgMetaColumns.push_back(KEY_NXS_HASH);
    msgMetaColumns.push_back(KEY_MSG_ID); msgMetaColumns.push_back(KEY_ORIG_MSG_ID); msgMetaColumns.push_back(KEY_MSG_STATUS);
    msgMetaColumns.push_back(KEY_CHILD_TS); msgMetaColumns.push_back(KEY_MSG_PARENT_ID); msgMetaColumns.push_back(KEY_MSG_THREAD_ID);
    msgMetaColumns.push_back(KEY_MSG_NAME); msgMetaColumns.push_back(KEY_NXS_SERV_STRING); msgMetaColumns.push_back(KEY_RECV_TS);
    msgMetaColumns.push_back(KEY_NXS_DATA_LEN);

    // for retrieving actual data
    msgColumns.push_back(KEY_GRP_ID);  msgColumns.push_back(KEY_NXS_DATA); msgColumns.push_back(KEY_NXS_DATA_LEN);
    msgColumns.push_back(KEY_NXS_META); msgColumns.push_back(KEY_MSG_ID);

    // for retrieving grp meta data
    grpMetaColumns.push_back(KEY_GRP_ID);  grpMetaColumns.push_back(KEY_TIME_STAMP); grpMetaColumns.push_back(KEY_NXS_FLAGS);
    grpMetaColumns.push_back(KEY_SIGN_SET); grpMetaColumns.push_back(KEY_NXS_IDENTITY); grpMetaColumns.push_back(KEY_NXS_HASH);
    grpMetaColumns.push_back(KEY_KEY_SET); grpMetaColumns.push_back(KEY_GRP_SUBCR_FLAG); grpMetaColumns.push_back(KEY_GRP_POP);
    grpMetaColumns.push_back(KEY_MSG_COUNT); grpMetaColumns.push_back(KEY_GRP_STATUS); grpMetaColumns.push_back(KEY_GRP_NAME);
    grpMetaColumns.push_back(KEY_GRP_LAST_POST); grpMetaColumns.push_back(KEY_ORIG_GRP_ID); grpMetaColumns.push_back(KEY_NXS_SERV_STRING);
    grpMetaColumns.push_back(KEY_GRP_SIGN_FLAGS); grpMetaColumns.push_back(KEY_GRP_CIRCLE_ID); grpMetaColumns.push_back(KEY_GRP_CIRCLE_TYPE);
    grpMetaColumns.push_back(KEY_GRP_INTERNAL_CIRCLE); grpMetaColumns.push_back(KEY_GRP_ORIGINATOR);
    grpMetaColumns.push_back(KEY_GRP_AUTHEN_FLAGS); grpMetaColumns.push_back(KEY_PARENT_GRP_ID); grpMetaColumns.push_back(KEY_RECV_TS);
    grpMetaColumns.push_back(KEY_GRP_REP_CUTOFF); grpMetaColumns.push_back(KEY_NXS_DATA_LEN);

    // for retrieving actual grp data
    grpColumns.push_back(KEY_GRP_ID); grpColumns.push_back(KEY_NXS_DATA); grpColumns.push_back(KEY_NXS_DATA_LEN);
    grpColumns.push_back(KEY_NXS_META);

    grpIdColumn.push_back(KEY_GRP_ID);

    mMsgIdColumn.push_back(KEY_MSG_ID);
}

RsDataService::~RsDataService(){

#ifdef RS_DATA_SERVICE_DEBUG
	std::cerr << "RsDataService::~RsDataService()";
	std::cerr << std::endl;
#endif

    mDb->closeDb();
    delete mDb;
}

static bool moveDataFromFileToDatabase(RetroDb *db, const std::string serviceDir, const std::string &tableName, const std::string &keyId, std::list<std::string> &files)
{
    bool ok = true;

    // Move message data
    std::list<std::string> columns;
    columns.push_back(keyId);
    columns.push_back(KEY_NXS_FILE_OLD);
    columns.push_back(KEY_NXS_FILE_OFFSET_OLD);
    columns.push_back(KEY_NXS_FILE_LEN_OLD);

    RetroCursor* c = db->sqlQuery(tableName, columns, "", "");

    if (c)
    {
        bool valid = c->moveToFirst();

        while (ok && valid){
            std::string dataFile;
            c->getString(1, dataFile);

            if (!dataFile.empty()) {
                bool fileOk = true;

                // first try to find the file in the service dir
                if (RsDirUtil::fileExists(serviceDir + "/" + dataFile)) {
                    dataFile.insert(0, serviceDir + "/");
                } else if (RsDirUtil::fileExists(dataFile)) {
                    // use old way for backward compatibility
                    //TODO: can be removed later
                } else {
                    fileOk = false;

                    std::cerr << "moveDataFromFileToDatabase() cannot find file " << dataFile;
                    std::cerr << std::endl;
                }

                if (fileOk) {
                    std::string id;
                    c->getString(0, id);

                    uint32_t offset = c->getInt32(2);
                    uint32_t data_len = c->getInt32(3);

                    char* data = new char[data_len];
                    std::ifstream istrm(dataFile.c_str(), std::ios::binary);
                    istrm.seekg(offset, std::ios::beg);
                    istrm.read(data, data_len);
                    istrm.close();

                    ContentValue cv;
                    // insert new columns
                    cv.put(KEY_NXS_DATA, data_len, data);
                    cv.put(KEY_NXS_DATA_LEN, (int32_t) data_len);
                    // clear old columns
                    cv.put(KEY_NXS_FILE_OLD, "");
                    cv.put(KEY_NXS_FILE_OFFSET_OLD, 0);
                    cv.put(KEY_NXS_FILE_LEN_OLD, 0);

                    ok = db->sqlUpdate(tableName, keyId + "='" + id + "'", cv);
                    delete[] data;

                    if (std::find(files.begin(), files.end(), dataFile) == files.end()) {
                        files.push_back(dataFile);
                    }
                }
            }

            valid = c->moveToNext();
        }

        delete c;
    }

    return ok;
}

void RsDataService::initialise(bool isNewDatabase)
{
    const int databaseRelease = 1;
    int currentDatabaseRelease = 0;
    bool ok = true;

    RsStackMutex stack(mDbMutex);

    // initialise database

    if (isNewDatabase || !mDb->tableExists(DATABASE_RELEASE_TABLE_NAME)) {
        // create table for database release
        mDb->execSQL("CREATE TABLE " + DATABASE_RELEASE_TABLE_NAME + "(" +
                     KEY_DATABASE_RELEASE_ID + " INT PRIMARY KEY," +
                     KEY_DATABASE_RELEASE + " INT);");
    }

    if (isNewDatabase) {
        // create table for msg data
        mDb->execSQL("CREATE TABLE " + MSG_TABLE_NAME + "(" +
                     KEY_MSG_ID + " TEXT PRIMARY KEY," +
                     KEY_GRP_ID +  " TEXT," +
                     KEY_NXS_FLAGS + " INT,"  +
                     KEY_ORIG_MSG_ID +  " TEXT," +
                     KEY_TIME_STAMP + " INT," +
                     KEY_NXS_IDENTITY + " TEXT," +
                     KEY_SIGN_SET + " BLOB," +
                     KEY_NXS_DATA + " BLOB,"+
                     KEY_NXS_DATA_LEN + " INT," +
                     KEY_MSG_STATUS + " INT," +
                     KEY_CHILD_TS + " INT," +
                     KEY_NXS_META + " BLOB," +
                     KEY_MSG_THREAD_ID + " TEXT," +
                     KEY_MSG_PARENT_ID + " TEXT,"+
                     KEY_MSG_NAME + " TEXT," +
                     KEY_NXS_SERV_STRING + " TEXT," +
                     KEY_NXS_HASH + " TEXT," +
                     KEY_RECV_TS + " INT);");

        // create table for grp data
        mDb->execSQL("CREATE TABLE " + GRP_TABLE_NAME + "(" +
                     KEY_GRP_ID + " TEXT PRIMARY KEY," +
                     KEY_TIME_STAMP + " INT," +
                     KEY_NXS_DATA + " BLOB," +
                     KEY_NXS_DATA_LEN + " INT," +
                     KEY_KEY_SET + " BLOB," +
                     KEY_NXS_META + " BLOB," +
                     KEY_GRP_NAME + " TEXT," +
                     KEY_GRP_LAST_POST + " INT," +
                     KEY_GRP_POP + " INT," +
                     KEY_MSG_COUNT + " INT," +
                     KEY_GRP_SUBCR_FLAG + " INT," +
                     KEY_GRP_STATUS + " INT," +
                     KEY_NXS_IDENTITY + " TEXT," +
                     KEY_ORIG_GRP_ID + " TEXT," +
                     KEY_NXS_SERV_STRING + " TEXT," +
                     KEY_NXS_FLAGS + " INT," +
                     KEY_GRP_AUTHEN_FLAGS + " INT," +
                     KEY_GRP_SIGN_FLAGS + " INT," +
                     KEY_GRP_CIRCLE_ID + " TEXT," +
                     KEY_GRP_CIRCLE_TYPE + " INT," +
                     KEY_GRP_INTERNAL_CIRCLE + " TEXT," +
                     KEY_GRP_ORIGINATOR + " TEXT," +
                     KEY_NXS_HASH + " TEXT," +
                     KEY_RECV_TS + " INT," +
                     KEY_PARENT_GRP_ID + " TEXT," +
                     KEY_GRP_REP_CUTOFF + " INT," +
                     KEY_SIGN_SET + " BLOB);");

        mDb->execSQL("CREATE TRIGGER " + GRP_LAST_POST_UPDATE_TRIGGER +
                " INSERT ON " + MSG_TABLE_NAME +
                std::string(" BEGIN ") +
                " UPDATE " + GRP_TABLE_NAME + " SET " + KEY_GRP_LAST_POST + "= new."
                + KEY_RECV_TS + " WHERE " + KEY_GRP_ID + "=new." + KEY_GRP_ID + ";"
                + std::string("END;"));

        mDb->execSQL("CREATE INDEX " + MSG_INDEX_GRPID + " ON " + MSG_TABLE_NAME + "(" + KEY_GRP_ID +  ");");

        // Insert release, no need to upgrade
        ContentValue cv;
        cv.put(KEY_DATABASE_RELEASE_ID, KEY_DATABASE_RELEASE_ID_VALUE);
        cv.put(KEY_DATABASE_RELEASE, databaseRelease);
        mDb->sqlInsert(DATABASE_RELEASE_TABLE_NAME, "", cv);

        currentDatabaseRelease = databaseRelease;
    } else {
        // check release

        {
            // try to select the release
            std::list<std::string> columns;
            columns.push_back(KEY_DATABASE_RELEASE);

            std::string where;
            rs_sprintf(where, "%s=%d", KEY_DATABASE_RELEASE_ID.c_str(), KEY_DATABASE_RELEASE_ID_VALUE);

            RetroCursor* c = mDb->sqlQuery(DATABASE_RELEASE_TABLE_NAME, columns, where, "");
            if (c) {
                ok = c->moveToFirst();

                if (ok) {
                    currentDatabaseRelease = c->getInt32(0);
                }
                delete c;

                if (!ok) {
                    // No record found ... insert the record
                    ContentValue cv;
                    cv.put(KEY_DATABASE_RELEASE_ID, KEY_DATABASE_RELEASE_ID_VALUE);
                    cv.put(KEY_DATABASE_RELEASE, currentDatabaseRelease);
                    ok = mDb->sqlInsert(DATABASE_RELEASE_TABLE_NAME, "", cv);
                }
            } else {
                ok = false;
            }
        }

        // Release 1
        int newRelease = 1;
        if (ok && currentDatabaseRelease < newRelease) {
            // Update database
            std::list<std::string> files;

            ok = startReleaseUpdate(newRelease);

            // Move data in files into database
            ok = ok && mDb->execSQL("ALTER TABLE " + GRP_TABLE_NAME + " ADD COLUMN " + KEY_NXS_DATA + " BLOB;");
            ok = ok && mDb->execSQL("ALTER TABLE " + GRP_TABLE_NAME + " ADD COLUMN " + KEY_NXS_DATA_LEN + " INT;");
            ok = ok && mDb->execSQL("ALTER TABLE " + MSG_TABLE_NAME + " ADD COLUMN " + KEY_NXS_DATA + " BLOB;");
            ok = ok && mDb->execSQL("ALTER TABLE " + MSG_TABLE_NAME + " ADD COLUMN " + KEY_NXS_DATA_LEN + " INT;");

            ok = ok && moveDataFromFileToDatabase(mDb, mServiceDir, GRP_TABLE_NAME, KEY_GRP_ID, files);
            ok = ok && moveDataFromFileToDatabase(mDb, mServiceDir, MSG_TABLE_NAME, KEY_MSG_ID, files);

// SQLite doesn't support DROP COLUMN
//            ok = ok && mDb->execSQL("ALTER TABLE " + GRP_TABLE_NAME + " DROP COLUMN " + KEY_NXS_FILE_OLD + ";");
//            ok = ok && mDb->execSQL("ALTER TABLE " + GRP_TABLE_NAME + " DROP COLUMN " + KEY_NXS_FILE_OFFSET_OLD + ";");
//            ok = ok && mDb->execSQL("ALTER TABLE " + GRP_TABLE_NAME + " DROP COLUMN " + KEY_NXS_FILE_LEN_OLD + ";");
//            ok = ok && mDb->execSQL("ALTER TABLE " + MSG_TABLE_NAME + " DROP COLUMN " + KEY_NXS_FILE_OLD + ";");
//            ok = ok && mDb->execSQL("ALTER TABLE " + MSG_TABLE_NAME + " DROP COLUMN " + KEY_NXS_FILE_OFFSET_OLD + ";");
//            ok = ok && mDb->execSQL("ALTER TABLE " + MSG_TABLE_NAME + " DROP COLUMN " + KEY_NXS_FILE_LEN_OLD + ";");

            ok = finishReleaseUpdate(newRelease, ok);
            if (ok) {
                // Remove transfered files
                std::list<std::string>::const_iterator file;
                for (file = files.begin(); file != files.end(); ++file) {
                    remove(file->c_str());
                }
                currentDatabaseRelease = newRelease;
            }
        }
    }

    if (ok) {
        std::cerr << "Database " << mDbName << " release " << currentDatabaseRelease << " successfully initialised." << std::endl;
    } else {
        std::cerr << "Database " << mDbName << " initialisation failed." << std::endl;
    }
}

bool RsDataService::startReleaseUpdate(int release)
{
    // Update database
    std::cerr << "Database " << mDbName << " update to release " << release << "." << std::endl;

    return mDb->beginTransaction();
}

bool RsDataService::finishReleaseUpdate(int release, bool result)
{
    if (result) {
        std::string where;
        rs_sprintf(where, "%s=%d", KEY_DATABASE_RELEASE_ID.c_str(), KEY_DATABASE_RELEASE_ID_VALUE);

        ContentValue cv;
        cv.put(KEY_DATABASE_RELEASE, release);
        result = mDb->sqlUpdate(DATABASE_RELEASE_TABLE_NAME, where, cv);
    }

    if (result) {
        result = mDb->commitTransaction();
    } else {
        result = mDb->rollbackTransaction();
    }

    if (result) {
        std::cerr << "Database " << mDbName << " successfully updated to release " << release << "." << std::endl;
    } else {
        std::cerr << "Database " << mDbName << " update to release " << release << "failed." << std::endl;
    }

    return result;
}

RsGxsGrpMetaData* RsDataService::locked_getGrpMeta(RetroCursor &c)
{
#ifdef RS_DATA_SERVICE_DEBUG
	std::cerr << "RsDataService::locked_getGrpMeta()";
	std::cerr << std::endl;
#endif

    RsGxsGrpMetaData* grpMeta = new RsGxsGrpMetaData();

    bool ok = true;

    // for extracting raw data
    uint32_t offset = 0;
    char* data = NULL;
    uint32_t data_len = 0;

    // grpId
    std::string tempId;
    c.getString(COL_GRP_ID, tempId);
    grpMeta->mGroupId = RsGxsGroupId(tempId);
    c.getString(COL_IDENTITY, tempId);
    grpMeta->mAuthorId = RsGxsId(tempId);

    c.getString(COL_GRP_NAME, grpMeta->mGroupName);
    c.getString(COL_ORIG_GRP_ID, tempId);
    grpMeta->mOrigGrpId = RsGxsGroupId(tempId);
    c.getString(COL_GRP_SERV_STRING, grpMeta->mServiceString);
    std::string temp;
    c.getString(COL_HASH, temp);
    grpMeta->mHash = RsFileHash(temp);
    grpMeta->mReputationCutOff = c.getInt32(COL_GRP_REP_CUTOFF);
    grpMeta->mSignFlags = c.getInt32(COL_GRP_SIGN_FLAGS);

    grpMeta->mPublishTs = c.getInt32(COL_TIME_STAMP);
    grpMeta->mGroupFlags = c.getInt32(COL_NXS_FLAGS);
    grpMeta->mGrpSize = c.getInt32(COL_GRP_DATA_LEN);

    offset = 0;


    offset = 0; data = NULL; data_len = 0;
    data = (char*)c.getData(COL_KEY_SET, data_len);

    if(data)
        ok &= grpMeta->keys.GetTlv(data, data_len, &offset);
	 else
		 grpMeta->keys.TlvClear() ;

    // local meta
    grpMeta->mSubscribeFlags = c.getInt32(COL_GRP_SUBCR_FLAG);
    grpMeta->mPop = c.getInt32(COL_GRP_POP);
    grpMeta->mVisibleMsgCount = c.getInt32(COL_MSG_COUNT);
    grpMeta->mLastPost = c.getInt32(COL_GRP_LAST_POST);
    grpMeta->mGroupStatus = c.getInt32(COL_GRP_STATUS);

    c.getString(COL_GRP_CIRCLE_ID, tempId);
    grpMeta->mCircleId = RsGxsCircleId(tempId);
    grpMeta->mCircleType = c.getInt32(COL_GRP_CIRCL_TYPE);
    c.getString(COL_GRP_INTERN_CIRCLE, tempId);
    grpMeta->mInternalCircle = RsGxsCircleId(tempId);

    std::string s ; c.getString(COL_GRP_ORIGINATOR, s) ;
    grpMeta->mOriginator = RsPeerId(s);
    grpMeta->mAuthenFlags = c.getInt32(COL_GRP_AUTHEN_FLAGS);
    grpMeta->mRecvTS = c.getInt32(COL_GRP_RECV_TS);


    c.getString(COL_PARENT_GRP_ID, tempId);
    grpMeta->mParentGrpId = RsGxsGroupId(tempId);

    if(ok)
        return grpMeta;
    else
        delete grpMeta;

    return NULL;
}

RsNxsGrp* RsDataService::locked_getGroup(RetroCursor &c)
{
    /*!
     * grpId, pub admin and pub publish key
     * necessary for successful group
     */
    RsNxsGrp* grp = new RsNxsGrp(mServType);
    bool ok = true;

    // for manipulating raw data
    uint32_t offset = 0;
    char* data = NULL;
    uint32_t data_len = 0;

    // grpId
    c.getStringT<RsGxsGroupId>(COL_ACT_GROUP_ID, grp->grpId);
    ok &= !grp->grpId.isNull();

    offset = 0; data_len = 0;
    if(ok){

        data = (char*)c.getData(COL_META_DATA, data_len);
        if(data)
            grp->meta.GetTlv(data, data_len, &offset);
    }

    /* now retrieve grp data */
    offset = 0; data_len = 0;
    if(ok){
        data = (char*)c.getData(COL_NXS_DATA, data_len);
        if(data)
            ok &= grp->grp.GetTlv(data, data_len, &offset);
    }

    if(ok)
        return grp;
    else
        delete grp;

    return NULL;
}

RsGxsMsgMetaData* RsDataService::locked_getMsgMeta(RetroCursor &c)
{

    RsGxsMsgMetaData* msgMeta = new RsGxsMsgMetaData();

    bool ok = true;
    uint32_t data_len = 0,
    offset = 0;
    char* data = NULL;


    std::string gId;
    c.getString(COL_GRP_ID, gId);
    msgMeta->mGroupId = RsGxsGroupId(gId);
    std::string temp;
    c.getString(COL_MSG_ID, temp);
    msgMeta->mMsgId = RsGxsMessageId(temp);
    // without these, a msg is meaningless
    ok &= (!msgMeta->mGroupId.isNull()) && (!msgMeta->mMsgId.isNull());

    c.getString(COL_ORIG_MSG_ID, temp);
    msgMeta->mOrigMsgId = RsGxsMessageId(temp);
    c.getString(COL_IDENTITY, temp);
    msgMeta->mAuthorId = RsGxsId(temp);
    c.getString(COL_MSG_NAME, msgMeta->mMsgName);
    c.getString(COL_MSG_SERV_STRING, msgMeta->mServiceString);

    c.getString(COL_HASH, temp);
    msgMeta->mHash = RsFileHash(temp);
    msgMeta->recvTS = c.getInt32(COL_MSG_RECV_TS);

    offset = 0;
    data = (char*)c.getData(COL_SIGN_SET, data_len);
    msgMeta->signSet.GetTlv(data, data_len, &offset);
    msgMeta->mMsgSize = c.getInt32(COL_MSG_DATA_LEN);

    msgMeta->mMsgFlags = c.getInt32(COL_NXS_FLAGS);
    msgMeta->mPublishTs = c.getInt32(COL_TIME_STAMP);

    offset = 0; data_len = 0;

    // thread and parent id
    c.getString(COL_THREAD_ID, temp);
    msgMeta->mThreadId = RsGxsMessageId(temp);
    c.getString(COL_PARENT_ID, temp);
    msgMeta->mParentId = RsGxsMessageId(temp);

    // local meta
    msgMeta->mMsgStatus = c.getInt32(COL_MSG_STATUS);
    msgMeta->mChildTs = c.getInt32(COL_CHILD_TS);

    if(ok)
        return msgMeta;
    else
        delete msgMeta;

    return NULL;
}



RsNxsMsg* RsDataService::locked_getMessage(RetroCursor &c)
{

    RsNxsMsg* msg = new RsNxsMsg(mServType);

    bool ok = true;
    uint32_t data_len = 0,
    offset = 0;
    char* data = NULL;
    c.getStringT<RsGxsGroupId>(COL_ACT_GROUP_ID, msg->grpId);
    std::string temp;
    c.getString(COL_ACT_MSG_ID, temp);
    msg->msgId = RsGxsMessageId(temp);

    ok &= (!msg->grpId.isNull()) && (!msg->msgId.isNull());

    offset = 0; data_len = 0;
    if(ok){

        data = (char*)c.getData(COL_META_DATA, data_len);
        if(data)
            msg->meta.GetTlv(data, data_len, &offset);
    }

    /* now retrieve msg data */
    offset = 0; data_len = 0;
    if(ok){
        data = (char*)c.getData(COL_NXS_DATA, data_len);
        if(data)
            ok &= msg->msg.GetTlv(data, data_len, &offset);
    }

    if(ok)
        return msg;
    else
        delete msg;

    return NULL;
}

int RsDataService::storeMessage(std::map<RsNxsMsg *, RsGxsMsgMetaData *> &msg)
{

    RsStackMutex stack(mDbMutex);

    std::map<RsNxsMsg*, RsGxsMsgMetaData* >::iterator mit = msg.begin();

    // start a transaction
    mDb->beginTransaction();

    for(; mit != msg.end(); ++mit){

        RsNxsMsg* msgPtr = mit->first;
        RsGxsMsgMetaData* msgMetaPtr = mit->second;

#ifdef RS_DATA_SERVICE_DEBUG 
	std::cerr << "RsDataService::storeMessage() ";
	std::cerr << " GroupId: " << msgMetaPtr->mGroupId.toStdString();
	std::cerr << " MessageId: " << msgMetaPtr->mMsgId.toStdString();
	std::cerr << std::endl;
#endif

        // skip msg item if size if greater than
        if(!validSize(msgPtr))
	{
		std::cerr << "RsDataService::storeMessage() ERROR invalid size";
		std::cerr << std::endl;
		continue;
	}

        ContentValue cv;

        uint32_t dataLen = msgPtr->msg.TlvSize();
        char msgData[dataLen];
        uint32_t offset = 0;
        msgPtr->msg.SetTlv(msgData, dataLen, &offset);
        cv.put(KEY_NXS_DATA, dataLen, msgData);

        cv.put(KEY_NXS_DATA_LEN, (int32_t)dataLen);
        cv.put(KEY_MSG_ID, msgMetaPtr->mMsgId.toStdString());
        cv.put(KEY_GRP_ID, msgMetaPtr->mGroupId.toStdString());
        cv.put(KEY_NXS_SERV_STRING, msgMetaPtr->mServiceString);
        cv.put(KEY_NXS_HASH, msgMetaPtr->mHash.toStdString());
        cv.put(KEY_RECV_TS, (int32_t)msgMetaPtr->recvTS);


        char signSetData[msgMetaPtr->signSet.TlvSize()];
        offset = 0;
        msgMetaPtr->signSet.SetTlv(signSetData, msgMetaPtr->signSet.TlvSize(), &offset);
        cv.put(KEY_SIGN_SET, msgMetaPtr->signSet.TlvSize(), signSetData);
        cv.put(KEY_NXS_IDENTITY, msgMetaPtr->mAuthorId.toStdString());


        cv.put(KEY_NXS_FLAGS, (int32_t) msgMetaPtr->mMsgFlags);
        cv.put(KEY_TIME_STAMP, (int32_t) msgMetaPtr->mPublishTs);

        offset = 0;
        char metaData[msgPtr->meta.TlvSize()];
        msgPtr->meta.SetTlv(metaData, msgPtr->meta.TlvSize(), &offset);
        cv.put(KEY_NXS_META, msgPtr->meta.TlvSize(), metaData);

        cv.put(KEY_MSG_PARENT_ID, msgMetaPtr->mParentId.toStdString());
        cv.put(KEY_MSG_THREAD_ID, msgMetaPtr->mThreadId.toStdString());
        cv.put(KEY_ORIG_MSG_ID, msgMetaPtr->mOrigMsgId.toStdString());
        cv.put(KEY_MSG_NAME, msgMetaPtr->mMsgName);

        // now local meta
        cv.put(KEY_MSG_STATUS, (int32_t)msgMetaPtr->mMsgStatus);
        cv.put(KEY_CHILD_TS, (int32_t)msgMetaPtr->mChildTs);

        if (!mDb->sqlInsert(MSG_TABLE_NAME, "", cv))
	{
		std::cerr << "RsDataService::storeMessage() sqlInsert Failed";
		std::cerr << std::endl;
		std::cerr << "\t For GroupId: " << msgMetaPtr->mGroupId.toStdString();
		std::cerr << std::endl;
		std::cerr << "\t & MessageId: " << msgMetaPtr->mMsgId.toStdString();
		std::cerr << std::endl;
	}
    }

    // finish transaction
    bool ret = mDb->commitTransaction();

    for(mit = msg.begin(); mit != msg.end(); ++mit)
    {
    	//TODO: API encourages aliasing, remove this abomination
    	if(mit->second != mit->first->metaData)
    		delete mit->second;

    	delete mit->first;
    	;
    }

    return ret;
}

bool RsDataService::validSize(RsNxsMsg* msg) const
{
	if((msg->msg.TlvSize() + msg->meta.TlvSize()) <= GXS_MAX_ITEM_SIZE) return true;

	return false;
}


int RsDataService::storeGroup(std::map<RsNxsGrp *, RsGxsGrpMetaData *> &grp)
{

    RsStackMutex stack(mDbMutex);

    std::map<RsNxsGrp*, RsGxsGrpMetaData* >::iterator sit = grp.begin();

    // begin transaction
    mDb->beginTransaction();

    for(; sit != grp.end(); ++sit)
    {

        RsNxsGrp* grpPtr = sit->first;
        RsGxsGrpMetaData* grpMetaPtr = sit->second;

        // if data is larger than max item size do not add
        if(!validSize(grpPtr)) continue;

#ifdef RS_DATA_SERVICE_DEBUG 
	std::cerr << "RsDataService::storeGroup() GrpId: " << grpPtr->grpId.toStdString();
	std::cerr << " CircleType: " << (uint32_t) grpMetaPtr->mCircleType;
	std::cerr << " CircleId: " << grpMetaPtr->mCircleId.toStdString();
	std::cerr << std::endl;
#endif

        /*!
         * STORE data, data len,
         * grpId, flags, publish time stamp, identity,
         * id signature, admin signatue, key set, last posting ts
         * and meta data
         **/
        ContentValue cv;

        uint32_t dataLen = grpPtr->grp.TlvSize();
        char grpData[dataLen];
        uint32_t offset = 0;
        grpPtr->grp.SetTlv(grpData, dataLen, &offset);
        cv.put(KEY_NXS_DATA, dataLen, grpData);

        cv.put(KEY_NXS_DATA_LEN, (int32_t) dataLen);
        cv.put(KEY_GRP_ID, grpPtr->grpId.toStdString());
        cv.put(KEY_GRP_NAME, grpMetaPtr->mGroupName);
        cv.put(KEY_ORIG_GRP_ID, grpMetaPtr->mOrigGrpId.toStdString());
        cv.put(KEY_NXS_SERV_STRING, grpMetaPtr->mServiceString);
        cv.put(KEY_NXS_FLAGS, (int32_t)grpMetaPtr->mGroupFlags);
        cv.put(KEY_TIME_STAMP, (int32_t)grpMetaPtr->mPublishTs);
        cv.put(KEY_GRP_SIGN_FLAGS, (int32_t)grpMetaPtr->mSignFlags);
        cv.put(KEY_GRP_CIRCLE_ID, grpMetaPtr->mCircleId.toStdString());
        cv.put(KEY_GRP_CIRCLE_TYPE, (int32_t)grpMetaPtr->mCircleType);
        cv.put(KEY_GRP_INTERNAL_CIRCLE, grpMetaPtr->mInternalCircle.toStdString());
        cv.put(KEY_GRP_ORIGINATOR, grpMetaPtr->mOriginator.toStdString());
        cv.put(KEY_GRP_AUTHEN_FLAGS, (int32_t)grpMetaPtr->mAuthenFlags);
        cv.put(KEY_PARENT_GRP_ID, grpMetaPtr->mParentGrpId.toStdString());
        cv.put(KEY_NXS_HASH, grpMetaPtr->mHash.toStdString());
        cv.put(KEY_RECV_TS, (int32_t)grpMetaPtr->mRecvTS);
        cv.put(KEY_GRP_REP_CUTOFF, (int32_t)grpMetaPtr->mReputationCutOff);
        cv.put(KEY_NXS_IDENTITY, grpMetaPtr->mAuthorId.toStdString());

        offset = 0;
        char keySetData[grpMetaPtr->keys.TlvSize()];
        grpMetaPtr->keys.SetTlv(keySetData, grpMetaPtr->keys.TlvSize(), &offset);
        cv.put(KEY_KEY_SET, grpMetaPtr->keys.TlvSize(), keySetData);

        offset = 0;
        char metaData[grpPtr->meta.TlvSize()];
        grpPtr->meta.SetTlv(metaData, grpPtr->meta.TlvSize(), &offset);
        cv.put(KEY_NXS_META, grpPtr->meta.TlvSize(), metaData);

        // local meta data
        cv.put(KEY_GRP_SUBCR_FLAG, (int32_t)grpMetaPtr->mSubscribeFlags);
        cv.put(KEY_GRP_POP, (int32_t)grpMetaPtr->mPop);
        cv.put(KEY_MSG_COUNT, (int32_t)grpMetaPtr->mVisibleMsgCount);
        cv.put(KEY_GRP_STATUS, (int32_t)grpMetaPtr->mGroupStatus);
        cv.put(KEY_GRP_LAST_POST, (int32_t)grpMetaPtr->mLastPost);

        if (!mDb->sqlInsert(GRP_TABLE_NAME, "", cv))
	{
		std::cerr << "RsDataService::storeGroup() sqlInsert Failed";
		std::cerr << std::endl;
		std::cerr << "\t For GroupId: " << grpMetaPtr->mGroupId.toStdString();
		std::cerr << std::endl;
	}
    }
    // finish transaction
    bool ret = mDb->commitTransaction();

    for(sit = grp.begin(); sit != grp.end(); ++sit)
    {
	//TODO: API encourages aliasing, remove this abomination
			if(sit->second != sit->first->metaData)
				delete sit->second;
    	delete sit->first;

    }

    return ret;
}

int RsDataService::updateGroup(std::map<RsNxsGrp *, RsGxsGrpMetaData *> &grp)
{

    RsStackMutex stack(mDbMutex);

    std::map<RsNxsGrp*, RsGxsGrpMetaData* >::iterator sit = grp.begin();

    // begin transaction
    mDb->beginTransaction();

    for(; sit != grp.end(); ++sit)
    {

        RsNxsGrp* grpPtr = sit->first;
        RsGxsGrpMetaData* grpMetaPtr = sit->second;

        // if data is larger than max item size do not add
        if(!validSize(grpPtr)) continue;

        /*!
         * STORE data, data len,
         * grpId, flags, publish time stamp, identity,
         * id signature, admin signatue, key set, last posting ts
         * and meta data
         **/
        ContentValue cv;
        uint32_t dataLen = grpPtr->grp.TlvSize();
        char grpData[dataLen];
        uint32_t offset = 0;
        grpPtr->grp.SetTlv(grpData, dataLen, &offset);
        cv.put(KEY_NXS_DATA, dataLen, grpData);

        cv.put(KEY_NXS_DATA_LEN, (int32_t) dataLen);
        cv.put(KEY_GRP_ID, grpPtr->grpId.toStdString());
        cv.put(KEY_GRP_NAME, grpMetaPtr->mGroupName);
        cv.put(KEY_ORIG_GRP_ID, grpMetaPtr->mOrigGrpId.toStdString());
        cv.put(KEY_NXS_SERV_STRING, grpMetaPtr->mServiceString);
        cv.put(KEY_NXS_FLAGS, (int32_t)grpMetaPtr->mGroupFlags);
        cv.put(KEY_TIME_STAMP, (int32_t)grpMetaPtr->mPublishTs);
        cv.put(KEY_GRP_SIGN_FLAGS, (int32_t)grpMetaPtr->mSignFlags);
        cv.put(KEY_GRP_CIRCLE_ID, grpMetaPtr->mCircleId.toStdString());
        cv.put(KEY_GRP_CIRCLE_TYPE, (int32_t)grpMetaPtr->mCircleType);
        cv.put(KEY_GRP_INTERNAL_CIRCLE, grpMetaPtr->mInternalCircle.toStdString());
        cv.put(KEY_GRP_ORIGINATOR, grpMetaPtr->mOriginator.toStdString());
        cv.put(KEY_GRP_AUTHEN_FLAGS, (int32_t)grpMetaPtr->mAuthenFlags);
        cv.put(KEY_NXS_HASH, grpMetaPtr->mHash.toStdString());
        cv.put(KEY_NXS_IDENTITY, grpMetaPtr->mAuthorId.toStdString());

        offset = 0;
        char keySetData[grpMetaPtr->keys.TlvSize()];
        grpMetaPtr->keys.SetTlv(keySetData, grpMetaPtr->keys.TlvSize(), &offset);
        cv.put(KEY_KEY_SET, grpMetaPtr->keys.TlvSize(), keySetData);

        offset = 0;
        char metaData[grpPtr->meta.TlvSize()];
        grpPtr->meta.SetTlv(metaData, grpPtr->meta.TlvSize(), &offset);
        cv.put(KEY_NXS_META, grpPtr->meta.TlvSize(), metaData);

        // local meta data
        cv.put(KEY_GRP_SUBCR_FLAG, (int32_t)grpMetaPtr->mSubscribeFlags);
        cv.put(KEY_GRP_POP, (int32_t)grpMetaPtr->mPop);
        cv.put(KEY_MSG_COUNT, (int32_t)grpMetaPtr->mVisibleMsgCount);
        cv.put(KEY_GRP_STATUS, (int32_t)grpMetaPtr->mGroupStatus);
        cv.put(KEY_GRP_LAST_POST, (int32_t)grpMetaPtr->mLastPost);

        mDb->sqlUpdate(GRP_TABLE_NAME, "grpId='" + grpPtr->grpId.toStdString() + "'", cv);
    }
    // finish transaction
    bool ret = mDb->commitTransaction();

    for(sit = grp.begin(); sit != grp.end(); ++sit)
    {
	//TODO: API encourages aliasing, remove this abomination
			if(sit->second != sit->first->metaData)
				delete sit->second;
    	delete sit->first;

    }

    return ret;
}

int RsDataService::updateGroupKeys(const RsGxsGroupId& grpId,const RsTlvSecurityKeySet& keys,uint32_t subscribe_flags)
{
    RsStackMutex stack(mDbMutex);

    // begin transaction
    mDb->beginTransaction();

    /*!
     * STORE key set
     **/

    ContentValue cv;
        //cv.put(KEY_NXS_FLAGS, (int32_t)grpMetaPtr->mGroupFlags); ?

    uint32_t offset = 0;
    char keySetData[keys.TlvSize()];
    keys.SetTlv(keySetData, keys.TlvSize(), &offset);
    cv.put(KEY_KEY_SET, keys.TlvSize(), keySetData);
    cv.put(KEY_GRP_SUBCR_FLAG, (int32_t)subscribe_flags);

    mDb->sqlUpdate(GRP_TABLE_NAME, "grpId='" + grpId.toStdString() + "'", cv);

    // finish transaction
    return  mDb->commitTransaction();
}

bool RsDataService::validSize(RsNxsGrp* grp) const
{
	if((grp->grp.TlvSize() + grp->meta.TlvSize()) <= GXS_MAX_ITEM_SIZE) return true;
	return false;
}

int RsDataService::retrieveNxsGrps(std::map<RsGxsGroupId, RsNxsGrp *> &grp, bool withMeta, bool /* cache */)
{
#ifdef RS_DATA_SERVICE_DEBUG_TIME
    RsScopeTimer timer("");
    int resultCount = 0;
    int requestedGroups = grp.size();
#endif

    if(grp.empty()){

        RsStackMutex stack(mDbMutex);
        RetroCursor* c = mDb->sqlQuery(GRP_TABLE_NAME, grpColumns, "", "");

        if(c)
        {
                std::vector<RsNxsGrp*> grps;

                locked_retrieveGroups(c, grps);
                std::vector<RsNxsGrp*>::iterator vit = grps.begin();

#ifdef RS_DATA_SERVICE_DEBUG_TIME
                resultCount = grps.size();
#endif

                for(; vit != grps.end(); ++vit)
                {
                        grp[(*vit)->grpId] = *vit;
                }

                delete c;
        }

    }else{

        RsStackMutex stack(mDbMutex);
        std::map<RsGxsGroupId, RsNxsGrp *>::iterator mit = grp.begin();

        std::list<RsGxsGroupId> toRemove;

        for(; mit != grp.end(); ++mit)
        {
            const RsGxsGroupId& grpId = mit->first;
            RetroCursor* c = mDb->sqlQuery(GRP_TABLE_NAME, grpColumns, "grpId='" + grpId.toStdString() + "'", "");

            if(c)
            {
                std::vector<RsNxsGrp*> grps;
                locked_retrieveGroups(c, grps);

                if(!grps.empty())
                {
                        RsNxsGrp* ng = grps.front();
                        grp[ng->grpId] = ng;

#ifdef RS_DATA_SERVICE_DEBUG_TIME
                        ++resultCount;
#endif
                }else{
                        toRemove.push_back(grpId);
                }

                delete c;
            }
        }

        std::list<RsGxsGroupId>::iterator grpIdIt;
        for (grpIdIt = toRemove.begin(); grpIdIt != toRemove.end(); ++grpIdIt)
        {
            grp.erase(*grpIdIt);
        }
    }

#ifdef RS_DATA_SERVICE_DEBUG_TIME
    std::cerr << "RsDataService::retrieveNxsGrps() " << mDbName << ", Requests: " << requestedGroups << ", Results: " << resultCount << ", Time: " << timer.duration() << std::endl;
#endif

    if(withMeta && !grp.empty())
    {
        std::map<RsGxsGroupId, RsGxsGrpMetaData*> metaMap;
        std::map<RsGxsGroupId, RsNxsGrp *>::iterator mit = grp.begin();
        for(; mit != grp.end(); ++mit)
            metaMap.insert(std::make_pair(mit->first, (RsGxsGrpMetaData*)(NULL)));

        retrieveGxsGrpMetaData(metaMap);

        mit = grp.begin();
        for(; mit != grp.end(); ++mit)
        {
            RsNxsGrp* grpPtr = grp[mit->first];
            grpPtr->metaData = metaMap[mit->first];

#ifdef RS_DATA_SERVICE_DEBUG 
	    std::cerr << "RsDataService::retrieveNxsGrps() GrpId: " << mit->first.toStdString();
	    std::cerr << " CircleType: " << (uint32_t) grpPtr->metaData->mCircleType;
	    std::cerr << " CircleId: " << grpPtr->metaData->mCircleId.toStdString();
	    std::cerr << std::endl;
#endif
        }

#ifdef RS_DATA_SERVICE_DEBUG_TIME
        std::cerr << "RsDataService::retrieveNxsGrps() " << mDbName << ", Time with meta: " << timer.duration() << std::endl;
#endif
    }

    return 1;
}

void RsDataService::locked_retrieveGroups(RetroCursor* c, std::vector<RsNxsGrp*>& grps){

    if(c){
        bool valid = c->moveToFirst();

        while(valid){
            RsNxsGrp* g = locked_getGroup(*c);

            // only add the latest grp info
            if(g)
            {
                grps.push_back(g);
            }
            valid = c->moveToNext();
        }
    }
}

int RsDataService::retrieveNxsMsgs(const GxsMsgReq &reqIds, GxsMsgResult &msg, bool /* cache */, bool withMeta)
{
#ifdef RS_DATA_SERVICE_DEBUG_TIME
    RsScopeTimer timer("");
    int resultCount = 0;
#endif

    GxsMsgReq::const_iterator mit = reqIds.begin();

    GxsMsgReq metaReqIds;// collects metaReqIds if needed

    for(; mit != reqIds.end(); ++mit)
    {

        const RsGxsGroupId& grpId = mit->first;

        // if vector empty then request all messages
        const std::vector<RsGxsMessageId>& msgIdV = mit->second;
        std::vector<RsNxsMsg*> msgSet;

        if(msgIdV.empty()){

            RsStackMutex stack(mDbMutex);

            RetroCursor* c = mDb->sqlQuery(MSG_TABLE_NAME, msgColumns, KEY_GRP_ID+ "='" + grpId.toStdString() + "'", "");

            if(c)
            {
                locked_retrieveMessages(c, msgSet);

#ifdef RS_DATA_SERVICE_DEBUG_TIME
                resultCount += msgSet.size();
#endif
            }

            delete c;
        }else{

            // request each grp
            std::vector<RsGxsMessageId>::const_iterator sit = msgIdV.begin();

            for(; sit!=msgIdV.end();++sit){
                const RsGxsMessageId& msgId = *sit;

                RsStackMutex stack(mDbMutex);

                RetroCursor* c = mDb->sqlQuery(MSG_TABLE_NAME, msgColumns, KEY_GRP_ID+ "='" + grpId.toStdString()
                                               + "' AND " + KEY_MSG_ID + "='" + msgId.toStdString() + "'", "");

                if(c)
                {
                    locked_retrieveMessages(c, msgSet);

#ifdef RS_DATA_SERVICE_DEBUG_TIME
                    resultCount += c->getResultCount();
#endif
                }

                delete c;
            }
        }

        msg[grpId] = msgSet;

        if(withMeta)
        {
            std::vector<RsGxsMessageId> msgIds;

            std::vector<RsNxsMsg*>::iterator lit = msgSet.begin(),
            lit_end = msgSet.end();

            for(; lit != lit_end; ++lit)
                msgIds.push_back( (*lit)->msgId );

            metaReqIds[grpId] = msgIds;
        }
        msgSet.clear();
    }

#ifdef RS_DATA_SERVICE_DEBUG_TIME
    std::cerr << "RsDataService::retrieveNxsMsgs() " << mDbName << ", Requests: " << reqIds.size() << ", Results: " << resultCount << ", Time: " << timer.duration() << std::endl;
#endif

    // tres expensive !?
    if(withMeta)
    {

        GxsMsgMetaResult metaResult;

        // request with meta ids so there is no chance of
        // a mem leak being left over
        retrieveGxsMsgMetaData(metaReqIds, metaResult);

        GxsMsgResult::iterator mit2 = msg.begin(), mit2_end = msg.end();

        for(; mit2 != mit2_end; ++mit2)
        {
            const RsGxsGroupId& grpId = mit2->first;
            std::vector<RsNxsMsg*>& msgV = msg[grpId];
            std::vector<RsNxsMsg*>::iterator lit = msgV.begin(),
            lit_end = msgV.end();

            // as retrieval only attempts to retrieve what was found this elimiates chance
            // of a memory fault as all are assigned
            for(; lit != lit_end; ++lit)
            {
                std::vector<RsGxsMsgMetaData*>& msgMetaV = metaResult[grpId];
                std::vector<RsGxsMsgMetaData*>::iterator meta_lit = msgMetaV.begin();
                RsNxsMsg* msgPtr = *lit;
                for(; meta_lit != msgMetaV.end(); )
                {
                    RsGxsMsgMetaData* meta = *meta_lit;
                    if(meta->mMsgId == msgPtr->msgId)
                    {
                        msgPtr->metaData = meta;
                        meta_lit = msgMetaV.erase(meta_lit);
                    }else{
                        ++meta_lit;
                    }
                }
            }

            std::vector<RsGxsMsgMetaData*>& msgMetaV = metaResult[grpId];
            std::vector<RsGxsMsgMetaData*>::iterator meta_lit;

            // clean up just in case, should not go in here
            for(meta_lit = msgMetaV.begin(); meta_lit !=
                           msgMetaV.end(); )
            {
                RsGxsMsgMetaData* meta = *meta_lit;
                delete meta;
                meta_lit = msgMetaV.erase(meta_lit);
            }
        }

#ifdef RS_DATA_SERVICE_DEBUG_TIME
        std::cerr << "RsDataService::retrieveNxsMsgs() " << mDbName << ", Time with meta: " << timer.duration() << std::endl;
#endif
    }

    return 1;
}

void RsDataService::locked_retrieveMessages(RetroCursor *c, std::vector<RsNxsMsg *> &msgs)
{
    bool valid = c->moveToFirst();
    while(valid){
        RsNxsMsg* m = locked_getMessage(*c);

        if(m){
            msgs.push_back(m);
        }

        valid = c->moveToNext();
    }
    return;
}

int RsDataService::retrieveGxsMsgMetaData(const GxsMsgReq& reqIds, GxsMsgMetaResult &msgMeta)
{
    RsStackMutex stack(mDbMutex);

#ifdef RS_DATA_SERVICE_DEBUG_TIME
    RsScopeTimer timer("");
    int resultCount = 0;
#endif

    GxsMsgReq::const_iterator mit = reqIds.begin();

    for(; mit != reqIds.end(); ++mit)
    {

        const RsGxsGroupId& grpId = mit->first;

        // if vector empty then request all messages
        const std::vector<RsGxsMessageId>& msgIdV = mit->second;
        std::vector<RsGxsMsgMetaData*> metaSet;

        if(msgIdV.empty()){
            RetroCursor* c = mDb->sqlQuery(MSG_TABLE_NAME, msgMetaColumns, KEY_GRP_ID+ "='" + grpId.toStdString() + "'", "");

            if (c)
            {
                locked_retrieveMsgMeta(c, metaSet);

#ifdef RS_DATA_SERVICE_DEBUG_TIME
                resultCount += metaSet.size();
#endif
            }
        }else{

            // request each grp
            std::vector<RsGxsMessageId>::const_iterator sit = msgIdV.begin();

            for(; sit!=msgIdV.end(); ++sit){
                const RsGxsMessageId& msgId = *sit;
                RetroCursor* c = mDb->sqlQuery(MSG_TABLE_NAME, msgMetaColumns, KEY_GRP_ID+ "='" + grpId.toStdString()
                                               + "' AND " + KEY_MSG_ID + "='" + msgId.toStdString() + "'", "");

                if (c)
                {
                    locked_retrieveMsgMeta(c, metaSet);

#ifdef RS_DATA_SERVICE_DEBUG_TIME
                    resultCount += c->getResultCount();
#endif
                }
            }
        }

        msgMeta[grpId] = metaSet;
    }

#ifdef RS_DATA_SERVICE_DEBUG_TIME
    std::cerr << "RsDataService::retrieveGxsMsgMetaData() " << mDbName << ", Requests: " << reqIds.size() << ", Results: " << resultCount << ", Time: " << timer.duration() << std::endl;
#endif

    return 1;
}

void RsDataService::locked_retrieveMsgMeta(RetroCursor *c, std::vector<RsGxsMsgMetaData *> &msgMeta)
{

    if(c)
    {
        bool valid = c->moveToFirst();
        while(valid){
            RsGxsMsgMetaData* m = locked_getMsgMeta(*c);

            if(m != NULL)
                msgMeta.push_back(m);

            valid = c->moveToNext();
        }
        delete c;
    }
}

int RsDataService::retrieveGxsGrpMetaData(std::map<RsGxsGroupId, RsGxsGrpMetaData *>& grp)
{
#ifdef RS_DATA_SERVICE_DEBUG
	std::cerr << "RsDataService::retrieveGxsGrpMetaData()";
	std::cerr << std::endl;
#endif

    RsStackMutex stack(mDbMutex);

#ifdef RS_DATA_SERVICE_DEBUG_TIME
    RsScopeTimer timer("");
#endif

    if(grp.empty()){

#ifdef RS_DATA_SERVICE_DEBUG
	std::cerr << "RsDataService::retrieveGxsGrpMetaData() retrieving all";
	std::cerr << std::endl;
#endif

        RetroCursor* c = mDb->sqlQuery(GRP_TABLE_NAME, grpMetaColumns, "", "");

        if(c)
        {
            bool valid = c->moveToFirst();

            while(valid)
            {
                RsGxsGrpMetaData* g = locked_getGrpMeta(*c);
                if(g)
                {
                    grp[g->mGroupId] = g;
                }
                valid = c->moveToNext();
            }
            delete c;
        }

    }else
    {
        std::map<RsGxsGroupId, RsGxsGrpMetaData *>::iterator mit = grp.begin();

          for(; mit != grp.end(); ++mit)
          {
              const RsGxsGroupId& grpId = mit->first;
              RetroCursor* c = mDb->sqlQuery(GRP_TABLE_NAME, grpMetaColumns, "grpId='" + grpId.toStdString() + "'", "");

              if(c)
              {
                  bool valid = c->moveToFirst();

                  while(valid)
                  {
                      RsGxsGrpMetaData* g = locked_getGrpMeta(*c);

                      if(g)
                      {
                          grp[g->mGroupId] = g;
                      }
                      valid = c->moveToNext();
                  }
                  delete c;
              }

          }

      }

#ifdef RS_DATA_SERVICE_DEBUG_TIME
    std::cerr << "RsDataService::retrieveGxsGrpMetaData() " << mDbName << ", Time: " << timer.duration() << std::endl;
#endif

    return 1;
}

int RsDataService::resetDataStore()
{

#ifdef RS_DATA_SERVICE_DEBUG
    std::cerr << "resetDataStore() " << std::endl;
#endif

    std::map<RsGxsGroupId, RsNxsGrp*> grps;

    retrieveNxsGrps(grps, false, false);
    std::map<RsGxsGroupId, RsNxsGrp*>::iterator mit
            = grps.begin();

    {
        RsStackMutex stack(mDbMutex);

        // remove all grp msgs files from service dir
        for(; mit != grps.end(); ++mit){
            std::string file = mServiceDir + "/" + mit->first.toStdString();
            std::string msgFile = file + "-msgs";
            remove(file.c_str()); // remove group file
            remove(msgFile.c_str()); // and remove messages file
            delete mit->second;
        }

        mDb->execSQL("DROP INDEX " + MSG_INDEX_GRPID);
        mDb->execSQL("DROP TABLE " + DATABASE_RELEASE_TABLE_NAME);
        mDb->execSQL("DROP TABLE " + MSG_TABLE_NAME);
        mDb->execSQL("DROP TABLE " + GRP_TABLE_NAME);
        mDb->execSQL("DROP TRIGGER " + GRP_LAST_POST_UPDATE_TRIGGER);
    }

    // recreate database
    initialise(true);

    return 1;
}

int RsDataService::updateGroupMetaData(GrpLocMetaData &meta)
{
	RsStackMutex stack(mDbMutex);
    RsGxsGroupId& grpId = meta.grpId;
    return mDb->sqlUpdate(GRP_TABLE_NAME,  KEY_GRP_ID+ "='" + grpId.toStdString() + "'", meta.val) ? 1 : 0;
}

int RsDataService::updateMessageMetaData(MsgLocMetaData &metaData)
{
	RsStackMutex stack(mDbMutex);
    RsGxsGroupId& grpId = metaData.msgId.first;
    RsGxsMessageId& msgId = metaData.msgId.second;
    return mDb->sqlUpdate(MSG_TABLE_NAME,  KEY_GRP_ID+ "='" + grpId.toStdString()
                          + "' AND " + KEY_MSG_ID + "='" + msgId.toStdString() + "'", metaData.val) ? 1 : 0;
}

int RsDataService::removeMsgs(const GxsMsgReq& msgIds)
{
	RsStackMutex stack(mDbMutex);

	GxsMsgReq::const_iterator mit = msgIds.begin();

	for(; mit != msgIds.end(); ++mit)
	{
		const std::vector<RsGxsMessageId>& msgIdV = mit->second;
		const RsGxsGroupId& grpId = mit->first;

		// delete messages
		GxsMsgReq msgsToDelete;
		msgsToDelete[grpId] = msgIdV;
		locked_removeMessageEntries(msgsToDelete);
	}

    return 1;
}

int RsDataService::removeGroups(const std::vector<RsGxsGroupId> &grpIds)
{

	RsStackMutex stack(mDbMutex);

	locked_removeGroupEntries(grpIds);

    return 1;
}

int RsDataService::retrieveGroupIds(std::vector<RsGxsGroupId> &grpIds)
{
    RsStackMutex stack(mDbMutex);

#ifdef RS_DATA_SERVICE_DEBUG_TIME
	RsScopeTimer timer("");
	int resultCount = 0;
#endif

	RetroCursor* c = mDb->sqlQuery(GRP_TABLE_NAME, grpIdColumn, "", "");

	if(c)
	{
		bool valid = c->moveToFirst();

		while(valid)
		{
			std::string grpId;
			c->getString(0, grpId);
			grpIds.push_back(RsGxsGroupId(grpId));
			valid = c->moveToNext();

#ifdef RS_DATA_SERVICE_DEBUG_TIME
			++resultCount;
#endif
		}
		delete c;
	}else
	{
		return 0;
	}

#ifdef RS_DATA_SERVICE_DEBUG_TIME
	std::cerr << "RsDataService::retrieveGroupIds() " << mDbName << ", Results: " << resultCount << ", Time: " << timer.duration() << std::endl;
#endif

	return 1;
}

int RsDataService::retrieveMsgIds(const RsGxsGroupId& grpId, RsGxsMessageId::std_vector& msgIds)
{
#ifdef RS_DATA_SERVICE_DEBUG_TIME
	RsScopeTimer timer("");
	int resultCount = 0;
#endif

	RetroCursor* c = mDb->sqlQuery(MSG_TABLE_NAME, mMsgIdColumn, KEY_GRP_ID+ "='" + grpId.toStdString() + "'", "");

	if(c)
	{
		bool valid = c->moveToFirst();

		while(valid)
		{
			std::string msgId;
            c->getString(0, msgId);

            if(c->columnCount() != 1)
            std::cerr << "(EE) ********* not retrieving all columns!!" << std::endl;

			msgIds.push_back(RsGxsMessageId(msgId));
			valid = c->moveToNext();

#ifdef RS_DATA_SERVICE_DEBUG_TIME
			++resultCount;
#endif
		}
		delete c;
	}else
	{
		return 0;
	}

#ifdef RS_DATA_SERVICE_DEBUG_TIME
	std::cerr << "RsDataService::retrieveNxsGrps() " << mDbName << ", Results: " << resultCount << ", Time: " << timer.duration() << std::endl;
#endif

	return 1;

}

bool RsDataService::locked_removeMessageEntries(const GxsMsgReq& msgIds)
{
    // start a transaction
    bool ret = mDb->beginTransaction();

    GxsMsgReq::const_iterator mit = msgIds.begin();

    for(; mit != msgIds.end(); ++mit)
    {
    	const RsGxsGroupId& grpId = mit->first;
    	const std::vector<RsGxsMessageId>& msgsV = mit->second;
    	std::vector<RsGxsMessageId>::const_iterator vit = msgsV.begin();

    	for(; vit != msgsV.end(); ++vit)
    	{
    		const RsGxsMessageId& msgId = *vit;
    		mDb->sqlDelete(MSG_TABLE_NAME, KEY_GRP_ID+ "='" + grpId.toStdString()
                    + "' AND " + KEY_MSG_ID + "='" + msgId.toStdString() + "'", "");
    	}
    }

    ret &= mDb->commitTransaction();

    return ret;
}

bool RsDataService::locked_removeGroupEntries(const std::vector<RsGxsGroupId>& grpIds)
{
    // start a transaction
    bool ret = mDb->beginTransaction();

    std::vector<RsGxsGroupId>::const_iterator vit = grpIds.begin();

    for(; vit != grpIds.end(); ++vit)
    {

		const RsGxsGroupId& grpId = *vit;
		mDb->sqlDelete(GRP_TABLE_NAME, KEY_GRP_ID+ "='" + grpId.toStdString() + "'", "");
    }

    ret &= mDb->commitTransaction();

    return ret;
}
uint32_t RsDataService::cacheSize() const {
    return 0;
}

int RsDataService::setCacheSize(uint32_t /* size */)
{
    return 0;
}

