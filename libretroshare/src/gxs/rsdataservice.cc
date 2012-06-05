#include <fstream>

#include "rsdataservice.h"

#define MSG_TABLE_NAME std::string("MESSAGES")
#define GRP_TABLE_NAME std::string("GROUPS")


// generic
#define KEY_NXS_FILE std::string("nxsFile")
#define KEY_NXS_FILE_OFFSET std::string("fileOffset")
#define KEY_NXS_FILE_LEN std::string("nxsFileLen")
#define KEY_NXS_IDENTITY std::string("identity")
#define KEY_GRP_ID std::string("grpId")
#define KEY_IDENTITY_SIGN std::string("idSign")
#define KEY_TIME_STAMP std::string("timeStamp")
#define KEY_NXS_FLAGS std::string("flags")

// grp table columns
#define KEY_ADMIN_SIGN std::string("adminSign")
#define KEY_KEY_SET std::string("keySet")



// msg table columns
#define KEY_PUBLISH_SIGN std::string("publishSign")
#define KEY_MSG_ID std::string("msgId")


// grp col numbers
#define COL_GRP_ID 0
#define COL_ADMIN_SIGN 1
#define COL_KEY_SET 9

// msg col numbers
#define COL_PUBLISH_SIGN 1
#define COL_MSG_ID 9

// generic col numbers
#define COL_NXS_FILE 2
#define COL_NXS_FILE_OFFSET 3
#define COL_NXS_FILE_LEN 4
#define COL_TIME_STAMP 5
#define COL_NXS_FLAGS 6
#define COL_IDENTITY_SIGN 7
#define COL_IDENTITY 8


#define RS_DATA_SERVICE_DEBUG

RsDataService::RsDataService(const std::string &serviceDir, const std::string &dbName, uint16_t serviceType,
                             RsGxsSearchModule *mod)
    : mServiceDir(serviceDir), mDbName(mServiceDir + "/" + dbName), mServType(serviceType){

    initialise();

    msgColumns.push_back(KEY_GRP_ID); msgColumns.push_back(KEY_PUBLISH_SIGN);  msgColumns.push_back(KEY_NXS_FILE);
    msgColumns.push_back(KEY_NXS_FILE_OFFSET); msgColumns.push_back(KEY_NXS_FILE_LEN); msgColumns.push_back(KEY_TIME_STAMP);
    msgColumns.push_back(KEY_NXS_FLAGS); msgColumns.push_back(KEY_IDENTITY_SIGN); msgColumns.push_back(KEY_NXS_IDENTITY);
    msgColumns.push_back(KEY_MSG_ID);

    grpColumns.push_back(KEY_GRP_ID); grpColumns.push_back(KEY_ADMIN_SIGN); grpColumns.push_back(KEY_NXS_FILE);
    grpColumns.push_back(KEY_NXS_FILE_OFFSET);  grpColumns.push_back(KEY_NXS_FILE_LEN); grpColumns.push_back(KEY_TIME_STAMP);
    grpColumns.push_back(KEY_NXS_FLAGS); grpColumns.push_back(KEY_IDENTITY_SIGN);
    grpColumns.push_back(KEY_NXS_IDENTITY); grpColumns.push_back(KEY_KEY_SET);
}

RsDataService::~RsDataService(){
    mDb->closeDb();
    delete mDb;
}

void RsDataService::initialise(){

    // initialise database
    mDb = new RetroDb(mDbName, RetroDb::OPEN_READWRITE_CREATE);

    // create table for msgs
    mDb->execSQL("CREATE TABLE " + MSG_TABLE_NAME + "(" + KEY_MSG_ID
                 + " TEXT," + KEY_GRP_ID +  " TEXT," + KEY_NXS_FLAGS + " INT,"
                  + KEY_TIME_STAMP + " INT," + KEY_PUBLISH_SIGN + " BLOB," + KEY_NXS_IDENTITY + " TEXT,"
                 + KEY_IDENTITY_SIGN + " BLOB," + KEY_NXS_FILE + " TEXT,"+ KEY_NXS_FILE_OFFSET + " INT,"
                 + KEY_NXS_FILE_LEN+ " INT);");

    // create table for grps
    mDb->execSQL("CREATE TABLE " + GRP_TABLE_NAME + "(" + KEY_GRP_ID +
                 " TEXT," + KEY_TIME_STAMP + " INT," +
                 KEY_ADMIN_SIGN + " BLOB," + " BLOB," + KEY_NXS_FILE +
                  " TEXT," + KEY_NXS_FILE_OFFSET + " INT," + KEY_KEY_SET + " BLOB," + KEY_NXS_FILE_LEN + " INT,"
                  + KEY_NXS_IDENTITY + " TEXT," + KEY_NXS_FLAGS + " INT," + KEY_IDENTITY_SIGN + " BLOB);");

}

RsNxsGrp* RsDataService::getGroup(RetroCursor &c){

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
    c.getString(COL_GRP_ID, grp->grpId);
    ok &= !grp->grpId.empty();

    // identity if any
    c.getString(COL_IDENTITY, grp->identity);
    grp->timeStamp = c.getInt32(COL_TIME_STAMP);

    if(!( grp->identity.empty() ) && ok){
        offset = 0;
        data = (char*)c.getData(COL_IDENTITY_SIGN, data_len);
        if(data){

            grp->idSign.GetTlv(data, data_len, &offset);
        }
    }

    offset = 0;
    data = (char*)c.getData(COL_ADMIN_SIGN, data_len);
    if(data){
        grp->adminSign.GetTlv(data, data_len, &offset);
    }



    grp->grpFlag = c.getInt32(COL_NXS_FLAGS);

    offset = 0; data = NULL; data_len = 0;

    data = (char*)c.getData(COL_KEY_SET, data_len);
    if(data){
        ok &= grp->keys.GetTlv(data, data_len, &offset);
    }

    std::string grpFile;
    c.getString(COL_NXS_FILE, grpFile);
    ok &= !grpFile.empty();
    /* now retrieve grp data from file */

    if(ok){

        data_len = c.getInt32(COL_NXS_FILE_LEN);
        offset = c.getInt32(COL_NXS_FILE_OFFSET);

        char grp_data[data_len];
        std::ifstream istrm(grpFile.c_str(), std::ios::binary);
        istrm.seekg(offset, std::ios::beg);
        istrm.read(grp_data, data_len);

        istrm.close();
        offset = 0;
        ok &= grp->grp.GetTlv(grp_data, data_len, &offset);
    }

    if(ok)
        return grp;
    else
        delete grp;

    return NULL;
}


RsNxsMsg* RsDataService::getMessage(RetroCursor &c){

    RsNxsMsg* msg = new RsNxsMsg(mServType);

    bool ok = true;
    uint32_t data_len = 0,
    offset = 0;
    char* data = NULL;
    c.getString(COL_GRP_ID, msg->grpId);
    c.getString(COL_MSG_ID, msg->msgId);

    ok &= (!msg->grpId.empty()) && (!msg->msgId.empty());

    c.getString(COL_IDENTITY, msg->identity);

    if(!msg->identity.empty()){
        offset = 0;
        data = (char*)c.getData(COL_IDENTITY_SIGN, data_len);
        msg->idSign.GetTlv(data, data_len, &offset);
    }

    msg->msgFlag = c.getInt32(COL_NXS_FLAGS);
    msg->timeStamp = c.getInt32(COL_TIME_STAMP);

    offset = 0; data_len = 0;
    if(ok){

        data = (char*)c.getData(COL_PUBLISH_SIGN, data_len);
        if(data)
            msg->publishSign.GetTlv(data, data_len, &offset);

    }

    std::string msgFile;
    c.getString(COL_NXS_FILE, msgFile);
    offset = c.getInt32(COL_NXS_FILE_OFFSET);
    data_len = c.getInt32(COL_NXS_FILE_LEN);
    ok &= !msgFile.empty();
    /* now retrieve grp data from file */

    if(ok){

        char msg_data[data_len];
        std::ifstream istrm(msgFile.c_str(), std::ios::binary);
        istrm.seekg(offset, std::ios::beg);
        istrm.read(msg_data, data_len);

        istrm.close();
        offset = 0;
        ok &= msg->msg.GetTlv(msg_data, data_len, &offset);
    }

    if(ok)
        return msg;
    else
        delete msg;

    return NULL;
}

int RsDataService::storeMessage(std::set<RsNxsMsg *> &msg){


    std::set<RsNxsMsg*>::iterator sit = msg.begin();

    mDb->execSQL("BEGIN;");


    for(; sit != msg.end(); sit++){

        RsNxsMsg* msgPtr = *sit;
        std::string msgFile = mServiceDir + "/" + msgPtr->grpId + "-msgs";
        std::fstream ostrm(msgFile.c_str(), std::ios::binary | std::ios::app | std::ios::out);
        ostrm.seekg(0, std::ios::end); // go to end to append
        uint32_t offset = ostrm.tellg(); // get fill offset

        ContentValue cv;
        cv.put(KEY_NXS_FILE_OFFSET, (int32_t)offset);
        cv.put(KEY_NXS_FILE, msgFile);
        cv.put(KEY_NXS_FILE_LEN, (int32_t)msgPtr->msg.TlvSize());
        cv.put(KEY_MSG_ID, msgPtr->msgId);
        cv.put(KEY_GRP_ID, msgPtr->grpId);
        char pubSignData[msgPtr->publishSign.TlvSize()];
        offset = 0;
        msgPtr->publishSign.SetTlv(pubSignData, msgPtr->publishSign.TlvSize(), &offset);
        cv.put(KEY_PUBLISH_SIGN, msgPtr->publishSign.TlvSize(), pubSignData);


        if(! (msgPtr->identity.empty()) ){
            char idSignData[msgPtr->idSign.TlvSize()];
            offset = 0;
            msgPtr->idSign.SetTlv(idSignData, msgPtr->idSign.TlvSize(), &offset);
            cv.put(KEY_IDENTITY_SIGN, msgPtr->idSign.TlvSize(), idSignData);
            cv.put(KEY_NXS_IDENTITY, msgPtr->identity);
        }

        cv.put(KEY_NXS_FLAGS, (int32_t) msgPtr->msgFlag);
        cv.put(KEY_TIME_STAMP, (int32_t) msgPtr->timeStamp);

        offset = 0;
        char msgData[msgPtr->msg.TlvSize()];
        msgPtr->msg.SetTlv(msgData, msgPtr->msg.TlvSize(), &offset);
        ostrm.write(msgData, msgPtr->msg.TlvSize());
        ostrm.close();


        mDb->sqlInsert(MSG_TABLE_NAME, "", cv);
    }

    return mDb->execSQL("COMMIT;");;
}


int RsDataService::storeGroup(std::set<RsNxsGrp *> &grp){


    std::set<RsNxsGrp*>::iterator sit = grp.begin();

    mDb->execSQL("BEGIN;");

    for(; sit != grp.end(); sit++){

        RsNxsGrp* grpPtr = *sit;

        std::string grpFile = mServiceDir + "/" + grpPtr->grpId;
        std::fstream ostrm(grpFile.c_str(), std::ios::binary | std::ios::app | std::ios::out);
        ostrm.seekg(0, std::ios::end); // go to end to append
        uint32_t offset = ostrm.tellg(); // get fill offset

        ContentValue cv;
        cv.put(KEY_NXS_FILE_OFFSET, (int32_t)offset);
        cv.put(KEY_NXS_FILE_LEN, (int32_t)grpPtr->grp.TlvSize());
        cv.put(KEY_NXS_FILE, grpFile);
        cv.put(KEY_GRP_ID, grpPtr->grpId);
        cv.put(KEY_NXS_FLAGS, (int32_t)grpPtr->grpFlag);
        cv.put(KEY_TIME_STAMP, (int32_t)grpPtr->timeStamp);

        if(! (grpPtr->identity.empty()) ){
            cv.put(KEY_NXS_IDENTITY, grpPtr->identity);

            char idSignData[grpPtr->idSign.TlvSize()];
            offset = 0;
            grpPtr->idSign.SetTlv(idSignData, grpPtr->idSign.TlvSize(), &offset);
            cv.put(KEY_IDENTITY_SIGN, grpPtr->idSign.TlvSize(), idSignData);
            std::string wat(idSignData, grpPtr->idSign.TlvSize());
            std::cerr << wat << std::endl;
        }

        char adminSignData[grpPtr->adminSign.TlvSize()];
        offset = 0;
        grpPtr->adminSign.SetTlv(adminSignData, grpPtr->adminSign.TlvSize(), &offset);
        cv.put(KEY_ADMIN_SIGN, grpPtr->adminSign.TlvSize(), adminSignData);

        offset = 0;
        char keySetData[grpPtr->keys.TlvSize()];
        grpPtr->keys.SetTlv(keySetData, grpPtr->keys.TlvSize(), &offset);
        cv.put(KEY_KEY_SET, grpPtr->keys.TlvSize(), keySetData);

        offset = 0;
        char grpData[grpPtr->grp.TlvSize()];
        grpPtr->grp.SetTlv(grpData, grpPtr->grp.TlvSize(), &offset);
        ostrm.write(grpData, grpPtr->grp.TlvSize());
        ostrm.close();

        mDb->sqlInsert(GRP_TABLE_NAME, "", cv);
    }

    return mDb->execSQL("COMMIT;");
}

int RsDataService::retrieveGrps(std::map<std::string, RsNxsGrp*> &grp, bool cache){

    RetroCursor* c = mDb->sqlQuery(GRP_TABLE_NAME, grpColumns, "", "");

    if(c){

        bool valid = c->moveToFirst();

        while(valid){
            RsNxsGrp* g = getGroup(*c);

            // only add the latest grp info
            if(g){
                bool exists = grp.find(g->grpId) != grp.end();
                if(exists){

                    if(grp[g->grpId]->timeStamp < g->timeStamp){
                        delete grp[g->grpId];
                        grp[g->grpId] = g;
                    }else{
                        delete g;
                    }
                }else{
                    grp[g->grpId] = g;
                }
            }
            valid = c->moveToNext();
        }

        delete c;
        return 1;
    }else{
        return 0;
    }
}

int RsDataService::retrieveMsgs(const std::string &grpId, std::map<std::string, RsNxsMsg *>& msg, bool cache){

    RetroCursor* c = mDb->sqlQuery(MSG_TABLE_NAME, msgColumns, KEY_GRP_ID+ "='" + grpId + "'", "");

    if(c){

        bool valid = c->moveToFirst();
        while(valid){
            RsNxsMsg* m = getMessage(*c);

            if(m){
                // only add the latest grp info
                bool exists = msg.find(m->msgId) != msg.end();
                if(exists){

                    if(msg[m->msgId]->timeStamp < m->timeStamp){
                        delete msg[m->msgId];
                        msg[m->msgId] = m;
                    }else{
                        delete m;
                    }
                }else{
                    msg[m->msgId] = m;
                }


            }
            valid = c->moveToNext();
        }

        delete c;
        return 1;
    }else{
        return 0;
    }
}

int RsDataService::retrieveMsgVersions(const std::string &grpId, const std::string& msgId,
                                       std::set<RsNxsMsg *>& msg, bool cache){


    std::string selection = KEY_GRP_ID + "='" + grpId + "' and " + KEY_MSG_ID + "='" + msgId + "'";
    RetroCursor* c = mDb->sqlQuery(MSG_TABLE_NAME, msgColumns, selection, "");


    if(c){

        bool valid = c->moveToFirst();
        while(valid){
            RsNxsMsg* m = getMessage(*c);

            if(m)
                msg.insert(m);

            valid = c->moveToNext();
        }

        delete c;
        return 1;
    }else{
        return 0;
    }

}

int RsDataService::retrieveGrpVersions(const std::string &grpId, std::set<RsNxsGrp *> &grp, bool cache){

    std::string selection = KEY_GRP_ID + "='" + grpId + "'";
    RetroCursor* c = mDb->sqlQuery(GRP_TABLE_NAME, grpColumns, selection, "");

    if(c){

        bool valid = c->moveToFirst();
        while(valid){
            RsNxsGrp* g = getGroup(*c);

            if(g)
                grp.insert(g);

            valid = c->moveToNext();
        }

        delete c;
        return 1;
    }else{
        return 0;
    }
}

RsNxsGrp* RsDataService::retrieveGrpVersion(const RsGxsGrpId &grpId){

    std::set<RsNxsGrp*> grps;
    retrieveGrpVersions(grpId.grpId, grps, false);
    RsNxsGrp* grp = NULL;

    if(!grps.empty()){

        // find grp with comparable sign
        std::set<RsNxsGrp*>::iterator sit = grps.begin();

        for(; sit != grps.end(); sit++){
            grp = *sit;
            if(!memcmp(grp->adminSign.signData.bin_data, grpId.adminSign.signData.bin_data,
                      grpId.adminSign.signData.bin_len)){
                break;
            }
            grp = NULL;
        }

        if(grp){
            grps.erase(grp);
            // release memory for non matching grps
            for(sit = grps.begin(); sit != grps.end(); sit++)
                delete *sit;
        }

    }

    return grp;
}

RsNxsMsg* RsDataService::retrieveMsgVersion(const RsGxsMsgId &msgId){

    std::set<RsNxsMsg*> msgs;
    retrieveMsgVersions(msgId.grpId, msgId.msgId, msgs, false);
    RsNxsMsg* msg = NULL;

    if(!msgs.empty()){

        std::set<RsNxsMsg*>::iterator sit = msgs.begin();

        for(; sit != msgs.end(); sit++){

            msg = *sit;
            if(0 == memcmp(msg->idSign.signData.bin_data, msgId.idSign.signData.bin_data,
                       msg->idSign.signData.bin_len))
                break;

            msg = NULL;
        }

        if(msg){
            msgs.erase(msg);

            for(sit = msgs.begin(); sit != msgs.end(); sit++)
                delete *sit;
        }
    }


    return msg;
}

int RsDataService::resetDataStore(){

#ifdef RS_DATA_SERVICE_DEBUG
    std::cerr << "resetDataStore() " << std::endl;
#endif

    std::map<std::string, RsNxsGrp*> grps;
    retrieveGrps(grps, false);
    std::map<std::string, RsNxsGrp*>::iterator mit
            = grps.begin();

    for(; mit != grps.end(); mit++){
        std::string file = mServiceDir + "/" + mit->first;
        std::string msgFile = file + "-msgs";
        remove(file.c_str());
        remove(msgFile.c_str());
    }

    mDb->closeDb();
    remove(mDbName.c_str());

    initialise();

    return 1;
}

int RsDataService::removeGroups(const std::list<RsGxsGrpId> &grpIds){

    return 0;
}

int RsDataService::removeMsgs(const std::list<RsGxsMsgId> &msgIds){
    return 0;
}

uint32_t RsDataService::cacheSize() const {
    return 0;
}

int RsDataService::setCacheSize(uint32_t size) {
    return 0;
}

int RsDataService::searchGrps(RsGxsSearch *search, std::list<RsGxsSrchResGrpCtx *> &result) {

    return 0;
}

int RsDataService::searchMsgs(RsGxsSearch *search, std::list<RsGxsSrchResMsgCtx *> &result) {
    return 0;
}
