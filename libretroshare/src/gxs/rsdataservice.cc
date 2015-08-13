
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

#define MSG_TABLE_NAME std::string("MESSAGES")
#define GRP_TABLE_NAME std::string("GROUPS")

#define GRP_LAST_POST_UPDATE_TRIGGER std::string("LAST_POST_UPDATE")

#define MSG_INDEX_GRPID std::string("INDEX_MESSAGES_GRPID")

// generic
#define KEY_NXS_FILE std::string("nxsFile")
#define KEY_NXS_FILE_OFFSET std::string("fileOffset")
#define KEY_NXS_FILE_LEN std::string("nxsFileLen")
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




/*** actual data col numbers ***/

// generic
#define COL_ACT_GROUP_ID 0
#define COL_NXS_FILE 1
#define COL_NXS_FILE_OFFSET 2
#define COL_NXS_FILE_LEN 3
#define COL_META_DATA 4
#define COL_ACT_MSG_ID 5

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
    : RsGeneralDataService(), mDbMutex("RsDataService"), mServiceDir(serviceDir), mDbName(dbName), mDbPath(mServiceDir + "/" + dbName), mServType(serviceType),
    mDb( new RetroDb(mDbPath, RetroDb::OPEN_READWRITE_CREATE, key)) {

    initialise();

    // for retrieving msg meta
    msgMetaColumns.push_back(KEY_GRP_ID); msgMetaColumns.push_back(KEY_TIME_STAMP); msgMetaColumns.push_back(KEY_NXS_FLAGS);
    msgMetaColumns.push_back(KEY_SIGN_SET); msgMetaColumns.push_back(KEY_NXS_IDENTITY); msgMetaColumns.push_back(KEY_NXS_HASH);
    msgMetaColumns.push_back(KEY_MSG_ID); msgMetaColumns.push_back(KEY_ORIG_MSG_ID); msgMetaColumns.push_back(KEY_MSG_STATUS);
    msgMetaColumns.push_back(KEY_CHILD_TS); msgMetaColumns.push_back(KEY_MSG_PARENT_ID); msgMetaColumns.push_back(KEY_MSG_THREAD_ID);
    msgMetaColumns.push_back(KEY_MSG_NAME); msgMetaColumns.push_back(KEY_NXS_SERV_STRING); msgMetaColumns.push_back(KEY_RECV_TS);

    // for retrieving actual data
    msgColumns.push_back(KEY_GRP_ID);  msgColumns.push_back(KEY_NXS_FILE); msgColumns.push_back(KEY_NXS_FILE_OFFSET);
    msgColumns.push_back(KEY_NXS_FILE_LEN); msgColumns.push_back(KEY_NXS_META); msgColumns.push_back(KEY_MSG_ID);

    // for retrieving grp meta data
    grpMetaColumns.push_back(KEY_GRP_ID);  grpMetaColumns.push_back(KEY_TIME_STAMP); grpMetaColumns.push_back(KEY_NXS_FLAGS);
    grpMetaColumns.push_back(KEY_SIGN_SET); grpMetaColumns.push_back(KEY_NXS_IDENTITY); grpMetaColumns.push_back(KEY_NXS_HASH);
    grpMetaColumns.push_back(KEY_KEY_SET); grpMetaColumns.push_back(KEY_GRP_SUBCR_FLAG); grpMetaColumns.push_back(KEY_GRP_POP);
    grpMetaColumns.push_back(KEY_MSG_COUNT); grpMetaColumns.push_back(KEY_GRP_STATUS); grpMetaColumns.push_back(KEY_GRP_NAME);
    grpMetaColumns.push_back(KEY_GRP_LAST_POST); grpMetaColumns.push_back(KEY_ORIG_GRP_ID); grpMetaColumns.push_back(KEY_NXS_SERV_STRING);
    grpMetaColumns.push_back(KEY_GRP_SIGN_FLAGS); grpMetaColumns.push_back(KEY_GRP_CIRCLE_ID); grpMetaColumns.push_back(KEY_GRP_CIRCLE_TYPE);
    grpMetaColumns.push_back(KEY_GRP_INTERNAL_CIRCLE); grpMetaColumns.push_back(KEY_GRP_ORIGINATOR);
    grpMetaColumns.push_back(KEY_GRP_AUTHEN_FLAGS); grpMetaColumns.push_back(KEY_PARENT_GRP_ID); grpMetaColumns.push_back(KEY_RECV_TS);
    grpMetaColumns.push_back(KEY_GRP_REP_CUTOFF);
    

    // for retrieving actual grp data
    grpColumns.push_back(KEY_GRP_ID); grpColumns.push_back(KEY_NXS_FILE); grpColumns.push_back(KEY_NXS_FILE_OFFSET);
    grpColumns.push_back(KEY_NXS_FILE_LEN); grpColumns.push_back(KEY_NXS_META);

    // for retrieving msg offsets
    mMsgOffSetColumns.push_back(KEY_MSG_ID); mMsgOffSetColumns.push_back(KEY_NXS_FILE_OFFSET);
    mMsgOffSetColumns.push_back(KEY_NXS_FILE_LEN);

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

void RsDataService::initialise(){

    RsStackMutex stack(mDbMutex);

    // initialise database


    // create table for msg data
    mDb->execSQL("CREATE TABLE IF NOT EXISTS " + MSG_TABLE_NAME + "(" +
                 KEY_MSG_ID + " TEXT PRIMARY KEY," +
                 KEY_GRP_ID +  " TEXT," +
                 KEY_NXS_FLAGS + " INT,"  +
                 KEY_ORIG_MSG_ID +  " TEXT," +
                 KEY_TIME_STAMP + " INT," +
                 KEY_NXS_IDENTITY + " TEXT," +
                 KEY_SIGN_SET + " BLOB," +
                 KEY_NXS_FILE + " TEXT,"+
                 KEY_NXS_FILE_OFFSET + " INT," +
                 KEY_MSG_STATUS + " INT," +
                 KEY_CHILD_TS + " INT," +
                 KEY_NXS_META + " BLOB," +
                 KEY_MSG_THREAD_ID + " TEXT," +
                 KEY_MSG_PARENT_ID + " TEXT,"+
                 KEY_MSG_NAME + " TEXT," +
                 KEY_NXS_SERV_STRING + " TEXT," +
                 KEY_NXS_HASH + " TEXT," +
                 KEY_RECV_TS + " INT," +
                 KEY_NXS_FILE_LEN + " INT);");

    // create table for grp data
    mDb->execSQL("CREATE TABLE IF NOT EXISTS " + GRP_TABLE_NAME + "(" +
                 KEY_GRP_ID + " TEXT PRIMARY KEY," +
                 KEY_TIME_STAMP + " INT," +
                 KEY_NXS_FILE + " TEXT," +
                 KEY_NXS_FILE_OFFSET + " INT," +
                 KEY_KEY_SET + " BLOB," +
                 KEY_NXS_FILE_LEN + " INT," +
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

    mDb->execSQL("CREATE TRIGGER IF NOT EXISTS " + GRP_LAST_POST_UPDATE_TRIGGER +
    		" INSERT ON " + MSG_TABLE_NAME +
    		std::string(" BEGIN ") +
    		" UPDATE " + GRP_TABLE_NAME + " SET " + KEY_GRP_LAST_POST + "= new."
    		+ KEY_RECV_TS + " WHERE " + KEY_GRP_ID + "=new." + KEY_GRP_ID + ";"
    		+ std::string("END;"));

    mDb->execSQL("CREATE INDEX IF NOT EXISTS " + MSG_INDEX_GRPID + " ON " + MSG_TABLE_NAME + "(" + KEY_GRP_ID +  ");");
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
    grpMeta->mGrpSize = c.getInt32(COL_NXS_FILE_LEN);

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

    /* now retrieve grp data from file */
    std::string grpFile;
    c.getString(COL_NXS_FILE, grpFile);
    ok &= !grpFile.empty();

    if(ok){

        data_len = c.getInt32(COL_NXS_FILE_LEN);
        offset = c.getInt32(COL_NXS_FILE_OFFSET);

        // first try to find the file in the service dir
        if (RsDirUtil::fileExists(mServiceDir + "/" + grpFile)) {
            grpFile.insert(0, mServiceDir + "/");
        } else if (RsDirUtil::fileExists(grpFile)) {
            // use old way for backward compatibility
            //TODO: can be removed later
        } else {
            ok = false;

//#ifdef RS_DATA_SERVICE_DEBUG
            std::cerr << "RsDataService::locked_getGroup() cannot find group file " << grpFile;
            std::cerr << std::endl;
//#endif
        }

        if (ok) {
            char grp_data[data_len];
            std::ifstream istrm(grpFile.c_str(), std::ios::binary);
            istrm.seekg(offset, std::ios::beg);
            istrm.read(grp_data, data_len);

            istrm.close();
            offset = 0;
            ok &= grp->grp.GetTlv(grp_data, data_len, &offset);
        }
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
    msgMeta->mMsgSize = c.getInt32(COL_NXS_FILE_LEN);

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

    /* now retrieve grp data from file */
    std::string msgFile;
    c.getString(COL_NXS_FILE, msgFile);
    offset = c.getInt32(COL_NXS_FILE_OFFSET);
    data_len = c.getInt32(COL_NXS_FILE_LEN);
    ok &= !msgFile.empty();

    if(ok){

        // first try to find the file in the service dir
        if (RsDirUtil::fileExists(mServiceDir + "/" + msgFile)) {
            msgFile.insert(0, mServiceDir + "/");
        } else if (RsDirUtil::fileExists(msgFile)) {
            // use old way for backward compatibility
            //TODO: can be removed later
        } else {
            ok = false;

//#ifdef RS_DATA_SERVICE_DEBUG
            std::cerr << "RsDataService::locked_getMessage() cannot find message file " << msgFile;
            std::cerr << std::endl;
//#endif
        }

        if (ok) {
            char* msg_data = new char[data_len];
            std::ifstream istrm(msgFile.c_str(), std::ios::binary);
            istrm.seekg(offset, std::ios::beg);
            istrm.read(msg_data, data_len);

            istrm.close();
            offset = 0;
            ok &= msg->msg.GetTlv(msg_data, data_len, &offset);
            delete[] msg_data;
        }
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
    mDb->execSQL("BEGIN;");

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

        // create or access file in binary
        std::string filename = msgPtr->grpId.toStdString() + "-msgs";
        std::string msgFile = mServiceDir + "/" + filename;
        std::fstream ostrm(msgFile.c_str(), std::ios::binary | std::ios::app | std::ios::out);
        ostrm.seekg(0, std::ios::end); // go to end to append
        uint32_t offset = ostrm.tellg(); // get fill offset

        ContentValue cv;

        cv.put(KEY_NXS_FILE_OFFSET, (int32_t)offset);
        cv.put(KEY_NXS_FILE, filename);
        cv.put(KEY_NXS_FILE_LEN, (int32_t)msgPtr->msg.TlvSize());
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

        offset = 0;
        char* msgData = new char[msgPtr->msg.TlvSize()];
        msgPtr->msg.SetTlv(msgData, msgPtr->msg.TlvSize(), &offset);
        ostrm.write(msgData, msgPtr->msg.TlvSize());
        ostrm.close();
        delete[] msgData;

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
    bool ret = mDb->execSQL("COMMIT;");

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
    mDb->execSQL("BEGIN;");

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

        std::string filename = grpPtr->grpId.toStdString();
        std::string grpFile = mServiceDir + "/" + filename;
        std::fstream ostrm(grpFile.c_str(), std::ios::binary | std::ios::app | std::ios::out);
        ostrm.seekg(0, std::ios::end); // go to end to append
        uint32_t offset = ostrm.tellg(); // get fill offset

        /*!
         * STORE file offset, file length, file name,
         * grpId, flags, publish time stamp, identity,
         * id signature, admin signatue, key set, last posting ts
         * and meta data
         **/
        ContentValue cv;
        cv.put(KEY_NXS_FILE_OFFSET, (int32_t)offset);
        cv.put(KEY_NXS_FILE_LEN, (int32_t)grpPtr->grp.TlvSize());
        cv.put(KEY_NXS_FILE, filename);
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

        offset = 0;
        char grpData[grpPtr->grp.TlvSize()];
        grpPtr->grp.SetTlv(grpData, grpPtr->grp.TlvSize(), &offset);
        ostrm.write(grpData, grpPtr->grp.TlvSize());
        ostrm.close();

        if (!mDb->sqlInsert(GRP_TABLE_NAME, "", cv))
	{
		std::cerr << "RsDataService::storeGroup() sqlInsert Failed";
		std::cerr << std::endl;
		std::cerr << "\t For GroupId: " << grpMetaPtr->mGroupId.toStdString();
		std::cerr << std::endl;
	}
    }
    // finish transaction
    bool ret = mDb->execSQL("COMMIT;");

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
    mDb->execSQL("BEGIN;");

    for(; sit != grp.end(); ++sit)
    {

        RsNxsGrp* grpPtr = sit->first;
        RsGxsGrpMetaData* grpMetaPtr = sit->second;

        // if data is larger than max item size do not add
        if(!validSize(grpPtr)) continue;

        std::string filename = grpPtr->grpId.toStdString();
        std::string grpFile = mServiceDir + "/" + filename;
        std::ofstream ostrm(grpFile.c_str(), std::ios::binary | std::ios::trunc);
        uint32_t offset = 0; // get file offset

        /*!
         * STORE file offset, file length, file name,
         * grpId, flags, publish time stamp, identity,
         * id signature, admin signatue, key set, last posting ts
         * and meta data
         **/
        ContentValue cv;
        cv.put(KEY_NXS_FILE_OFFSET, (int32_t)offset);
        cv.put(KEY_NXS_FILE_LEN, (int32_t)grpPtr->grp.TlvSize());
        cv.put(KEY_NXS_FILE, filename);
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

        offset = 0;
        char grpData[grpPtr->grp.TlvSize()];
        grpPtr->grp.SetTlv(grpData, grpPtr->grp.TlvSize(), &offset);
        ostrm.write(grpData, grpPtr->grp.TlvSize());
        ostrm.close();

        mDb->sqlUpdate(GRP_TABLE_NAME, "grpId='" + grpPtr->grpId.toStdString() + "'", cv);
    }
    // finish transaction
    bool ret = mDb->execSQL("COMMIT;");

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
    mDb->execSQL("BEGIN;");

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
    return  mDb->execSQL("COMMIT;");
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
    std::cerr << "RsDataService::retrieveNxsMsgs() " << mDbName << ", Requests: " << reqIds.size() << "Results: " << resultCount << ", Time: " << timer.duration() << std::endl;
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
        mDb->execSQL("DROP TABLE " + MSG_TABLE_NAME);
        mDb->execSQL("DROP TABLE " + GRP_TABLE_NAME);
        mDb->execSQL("DROP TRIGGER " + GRP_LAST_POST_UPDATE_TRIGGER);
    }

    // recreate database
    initialise();

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

MsgOffset offSetAccum(const MsgOffset& x, const MsgOffset& y)
{
	MsgOffset m;
	m.msgLen = y.msgLen + x.msgLen;
	return m;
}

int RsDataService::removeMsgs(const GxsMsgReq& msgIds)
{
	RsStackMutex stack(mDbMutex);

	// for each group
	// get for all msgs their offsets and lengths
	// for message not contained in msg id vector
	// store their data file segments in buffer
	// then recalculate the retained messages'
	// new offsets, update db with new offsets
	// replace old msg file with new file
	// remove messages that were not retained from
	// db

	GxsMsgReq::const_iterator mit = msgIds.begin();


	for(; mit != msgIds.end(); ++mit)
	{
		MsgUpdates updates;
		const std::vector<RsGxsMessageId>& msgIdV = mit->second;
		const RsGxsGroupId& grpId = mit->first;

		GxsMsgReq reqIds;
		reqIds.insert(std::make_pair(grpId, std::vector<RsGxsMessageId>() ));

		// can get offsets for each file
		std::vector<MsgOffset> msgOffsets;
		locked_getMessageOffsets(grpId, msgOffsets);

		std::string oldFileName = mServiceDir + "/" + grpId.toStdString() + "-msgs";
		std::string newFileName = mServiceDir + "/" + grpId.toStdString() + "-msgs-temp";
		std::ifstream in(oldFileName.c_str(), std::ios::binary);
		std::vector<char> dataBuff, newBuffer;

		std::vector<MsgOffset>::iterator vit = msgOffsets.begin();

        uint32_t maxSize = 0;// größe aller msgs, newbuf könnte aber kleiner sein, weil msgs weggehen
		for(; vit != msgOffsets.end(); ++vit)
			maxSize += vit->msgLen;

		// may be preferable to determine file len reality
		// from file? corrupt db?
        dataBuff.reserve(maxSize);// dataBuff.resize(maxSize);
        newBuffer.reserve(maxSize);// newBuffer.resize(maxSize);

		dataBuff.insert(dataBuff.end(),
				std::istreambuf_iterator<char>(in),
				std::istreambuf_iterator<char>());

		in.close();
        uint32_t newOffset = 0;// am anfang der liste ist offset=0, jetzt gehen wir die msgs liste durch
		for(std::vector<MsgOffset>::size_type i = 0; i < msgOffsets.size(); ++i)
		{
			const MsgOffset& m = msgOffsets[i];

            //uint32_t newOffset = 0;//hier ist es zu spät, offset muss hochgezählt werden
			if(std::find(msgIdV.begin(), msgIdV.end(), m.msgId) == msgIdV.end())
			{
				MsgUpdate up;

				uint32_t msgLen = m.msgLen;

				up.msgId = m.msgId;
				up.cv.put(KEY_NXS_FILE_OFFSET, (int32_t)newOffset);

				newBuffer.insert(newBuffer.end(), dataBuff.begin()+m.msgOffset,
						dataBuff.begin()+m.msgOffset+m.msgLen);

				newOffset += msgLen;

				up.cv.put(KEY_NXS_FILE_LEN, (int32_t)msgLen);

				// add msg update
				updates[grpId].push_back(up);
			}
		}

		std::ofstream out(newFileName.c_str(), std::ios::binary);

		std::copy(newBuffer.begin(), newBuffer.end(),
				std::ostreambuf_iterator<char>(out));

		out.close();

		// now update the new positions in db
		locked_updateMessageEntries(updates);

		// then delete removed messages
		GxsMsgReq msgsToDelete;
		msgsToDelete[grpId] = msgIdV;
		locked_removeMessageEntries(msgsToDelete);

		// now replace old file location with new file
		remove(oldFileName.c_str());
		RsDirUtil::renameFile(newFileName, oldFileName);
	}

    return 1;
}

int RsDataService::removeGroups(const std::vector<RsGxsGroupId> &grpIds)
{

	RsStackMutex stack(mDbMutex);

	// the grp id is the group file name
	// first remove file then remove group
	// from db

	std::vector<RsGxsGroupId>::const_iterator vit = grpIds.begin();
	for(; vit != grpIds.end(); ++vit)
	{
		const std::string grpFileName = mServiceDir + "/" + (*vit).toStdString();
		remove(grpFileName.c_str());
	}

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

bool RsDataService::locked_updateMessageEntries(const MsgUpdates& updates)
{
    // start a transaction
    bool ret = mDb->execSQL("BEGIN;");

    MsgUpdates::const_iterator mit = updates.begin();

    for(; mit != updates.end(); ++mit)
    {

    	const RsGxsGroupId& grpId = mit->first;
    	const std::vector<MsgUpdate>& updateV = mit->second;
    	std::vector<MsgUpdate>::const_iterator vit = updateV.begin();

    	for(; vit != updateV.end(); ++vit)
    	{
    		const MsgUpdate& update = *vit;
    		mDb->sqlUpdate(MSG_TABLE_NAME, KEY_GRP_ID+ "='" + grpId.toStdString()
                    + "' AND " + KEY_MSG_ID + "='" + update.msgId.toStdString() + "'", update.cv);
    	}
    }

    ret &= mDb->execSQL("COMMIT;");

    return ret;
}

bool RsDataService::locked_removeMessageEntries(const GxsMsgReq& msgIds)
{
    // start a transaction
    bool ret = mDb->execSQL("BEGIN;");

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

    ret &= mDb->execSQL("COMMIT;");

    return ret;
}

bool RsDataService::locked_removeGroupEntries(const std::vector<RsGxsGroupId>& grpIds)
{
    // start a transaction
    bool ret = mDb->execSQL("BEGIN;");

    std::vector<RsGxsGroupId>::const_iterator vit = grpIds.begin();

    for(; vit != grpIds.end(); ++vit)
    {

		const RsGxsGroupId& grpId = *vit;
		mDb->sqlDelete(GRP_TABLE_NAME, KEY_GRP_ID+ "='" + grpId.toStdString() + "'", "");
    }

    ret &= mDb->execSQL("COMMIT;");

    return ret;
}
void RsDataService::locked_getMessageOffsets(const RsGxsGroupId& grpId, std::vector<MsgOffset>& offsets)
{

	RetroCursor* c = mDb->sqlQuery(MSG_TABLE_NAME, mMsgOffSetColumns, KEY_GRP_ID+ "='" + grpId.toStdString() + "'", "");

    if(c)
    {
        bool valid = c->moveToFirst();

        while(valid)
        {
        	RsGxsMessageId msgId;
        	int32_t msgLen;
        	int32_t msgOffSet;
        	std::string temp;
            c->getString(0, temp);
            msgId = RsGxsMessageId(temp);
            msgOffSet = c->getInt32(1);
            msgLen = c->getInt32(2);

            MsgOffset offset;
            offset.msgId = msgId;
            offset.msgLen = msgLen;
            offset.msgOffset = msgOffSet;
            offsets.push_back(offset);

            valid = c->moveToNext();
        }
        delete c;
    }
}

uint32_t RsDataService::cacheSize() const {
    return 0;
}

int RsDataService::setCacheSize(uint32_t /* size */)
{
    return 0;
}

