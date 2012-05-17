#include "rsdataservice.h"

#define MSG_TABLE_NAME std::string("MESSAGES")
#define GRP_TABLE_NAME std::string("GROUPS")


// generic
#define NXS_FILE std::string("msgFile")
#define NXS_FILE_OFFSET std::string("fileOffset")
#define NXS_LEN std::string("msgLen")
#define NXS_IDENTITY std::string("identity")
#define GRP_ID std::string("grpId")
#define IDENTITY_SIGN std::string("idSign")
#define TIME_STAMP std::string("timeStamp")
#define NXS_FLAGS std::string("flags")

// grp table columns
#define ADMIN_SIGN std::string("adminSign")
#define PUB_PUBLISH_KEY std::string("pubPublishhKey")
#define PUB_ADMIN_KEY std::string("pubAdminKey")
#define PRIV_ADMIN_KEY std::string("privAdminKey")
#define PRIV_PUBLISH_KEY std::string("privPublishKey")
#define GRP_FILE std::string("grpFile")


// msg table columns
#define PUBLISH_SIGN std::string("publishSign")
#define MSG_ID std::string("msgId")


// grp col numbers
#define COL_GRP_ID 1
#define COL_ADMIN_SIGN 2
#define COL_PUB_PUBLISH_KEY 10
#define COL_PUB_ADMIN_KEY 11
#define COL_PRIV_ADMIN_KEY 12
#define COL_PRIV_PUBLISH_KEY 13


// msg col numbers
#define COL_MSG_ID 1
#define COL_PUBLISH_SIGN 2

// generic col numbers
#define COL_NXS_FILE 3
#define COL_NXS_FILE_OFFSET 4
#define COL_NXS_LEN 5
#define COL_TIME_STAMP 6
#define COL_FLAGS 7
#define COL_IDENTITY_SIGN 8
#define COL_IDENTITY 9


RsDataService::RsDataService(const std::string &workingDir, const std::string &dbNamem, uint16_t serviceType)
{


    // initialise database
    remove("RsFileGdp_DataBase");
    mDb = new RetroDb(dbName, RetroDb::OPEN_READWRITE_CREATE);

    // create table for msgs
    mDb->execSQL("CREATE TABLE " + MSG_TABLE_NAME + "(" + MSG_ID
                 + " TEXT PRIMARY KEY ASC," + GRP_ID +  " TEXT," + FLAGS + " INT,"
                  + TIME_STAMP + " INT," + PUBLISH_SIGN + " BLOB," + NXS_IDENTITY + " TEXT,"
                 + IDENTITY_SIGN + " BLOB," + NXS_FILE + " TEXT,"+ NXS_FILE_OFFSET + " INT,"
                 + NXS_LEN+ " INT);");

    // create table for grps
    mDb->execSQL("CREATE TABLE " + GRP_TABLE_NAME + "(" + GRP_ID +
                 " TEXT PRIMARY KEY ASC," + TIME_STAMP + " INT," +
                 ADMIN_SIGN + " BLOB," + PUB_ADMIN_KEY + " BLOB,"
                 + PUB_PUBLISH_KEY + " BLOB," + PRIV_ADMIN_KEY +
                 " BLOB," + PRIV_PUBLISH_KEY + " BLOB," + NXS_FILE +
                  " TEXT," + NXS_FILE_OFFSET + " INT," + NXS_LEN + " INT,"
                  + NXS_IDENTITY + " TEXT," + IDENTITY_SIGN + " BLOB);");


    msgColumns.push_back(MSG_ID); msgColumns.push_back(PUBLISH_SIGN);  msgColumns.push_back(NXS_FILE);
    msgColumns.push_back(NXS_FILE_OFFSET); msgColumns.push_back(NXS_LEN); msgColumns.push_back(TIME_STAMP);
    msgColumns.push_back(NXS_FLAGS); msgColumns.push_back(IDENTITY); msgColumns.push_back(IDENTITY_SIGN);

    grpColumns.push_back(GRP_ID); grpColumns.push_back(ADMIN_SIGN); grpColumns.push_back(NXS_FILE);
    grpColumns.push_back(NXS_FILE_OFFSET); grpColumns.push_back(NXS_LEN); grpColumns.push_back(TIME_STAMP);
    grpColumns.push_back(NXS_FLAGS); grpColumns.push_back(IDENTITY); grpColumns.push_back(IDENTITY_SIGN);
    grpColumns.push_back(PUB_PUBLISH_KEY); grpColumns.push_back(PUB_ADMIN_KEY); grpColumns.push_back(PRIV_ADMIN_KEY);
    grpColumns.push_back(PRIV_PUBLISH_KEY); grpColumns.push_back(GRP_FILE);
}




RsNxsGrp* RsDataService::getGroup(RetroCursor &c){

    RsNxsGrp* grp = new RsNxsGrp(mServType);
    bool ok = true;

    std::ifstream istrm;
    uint32_t len, offset;
    char* data = NULL;
    uint32_t data_len = 0;
    uint32_t offset = 0;
    std::string grpId;
    uint32_t timeStamp;
    RsTlvBinaryData grpData;
    RsTlvSecurityKey adminKey, pubKey, privAdminKey, privPubKey;
    std::string identity;


    c.getString(COL_GRP_ID, grpId);
    ok &= !grpId.empty();
    c.getString(COL_IDENTITY, identity);

    timeStamp = c.getInt64(COL_TIME_STAMP);

    /* get keys */
    grp->keys.groupId = grpId;

    data = c.getData(COL_PUB_ADMIN_KEY, data_len);
    ok &= data;
    if(data){
        adminKey.GetTlv(data, data_len, &offset);
        grp->keys.keys.insert(adminKey);
    }
    offset = 0;

    data = c.getData(COL_PUB_PUBLISH_KEY, data_len);
    ok &= data;
    if(data){
        pubKey.GetTlv(data, data_len, &offset);
        grp->keys.keys.insert(pubKey);
    }
    offset = 0;

    data = c.getData(COL_PRIV_ADMIN_KEY, data_len);
    if(data){
        privAdminKey.GetTlv(COL_PRIV_ADMIN_KEY, data_len, &offset);
        grp->keys.keys.insert(privAdminKey);
    }
    offset = 0;

    data = c.getData(COL_PRIV_PUBLISH_KEY);
    if(data){
        privPubKey.GetTlv(data, data_len, &offset);
        grp->keys.keys.insert(privPubKey);
    }

    if(ok)
        return grp;
    else
        delete grp;

    return NULL;
}
