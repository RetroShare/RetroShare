#ifndef RSDATASERVICE_H
#define RSDATASERVICE_H

#include "rsgds.h"

#include "util/retrodb.h"

class RsDataService : public RsGeneralDataService
{
public:

    RsDataService(const std::string& serviceDir, const std::string& dbName, uint16_t serviceType, RsGxsSearchModule* mod = NULL);
    virtual ~RsDataService();

    /*!
     * Retrieves all msgs
     * @param reqIds requested msg ids (grpId,msgId), leave msg list empty to get all msgs for the grp
     * @param msg result of msg retrieval
     * @param cache whether to store results of this retrieval in memory for faster later retrieval
     * @return error code
     */
    int retrieveNxsMsgs(const GxsMsgReq& reqIds, GxsMsgResult& msg, bool cache);

    /*!
     * Retrieves all groups stored (most current versions only)
     * @param grp retrieved groups
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    int retrieveNxsGrps(std::map<std::string, RsNxsGrp*>& grp, bool cache);

    /*!
     * Retrieves meta data of all groups stored (most current versions only)
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    int retrieveGxsGrpMetaData(std::map<std::string, RsGxsGrpMetaData*>& grp);

    /*!
     * Retrieves meta data of all groups stored (most current versions only)
     * @param grpIds grpIds for which to retrieve meta data
     * @param msgMeta meta data result as map of grpIds to array of metadata for that grpId
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    int retrieveGxsMsgMetaData(const std::vector<std::string>& grpIds, GxsMsgMetaResult& msgMeta);

    /*!
     * remove msgs in data store
     * @param grpId group Id of message to be removed
     * @param msgIds ids of messages to be removed
     * @return error code
     */
    int removeMsgs(const std::string grpId, const std::vector<std::string>& msgIds);

    /*!
     * remove groups in data store listed in grpIds param
     * @param grpIds ids of groups to be removed
     * @return error code
     */
    int removeGroups(const std::vector<std::string>& grpIds);

    /*!
     * @return the cache size set for this RsGeneralDataService in bytes
     */
    uint32_t cacheSize() const;

    /*!
     * @param size size of cache to set in bytes
     */
    int setCacheSize(uint32_t size);

    /*!
     * Stores a list of signed messages into data store
     * @param msg map of message and decoded meta data information
     * @return error code
     */
    int storeMessage(std::map<RsNxsMsg*, RsGxsMsgMetaData*>& msg);

    /*!
     * Stores a list of groups in data store
     * @param grp map of group and decoded meta data
     * @return error code
     */
    int storeGroup(std::map<RsNxsGrp*, RsGxsGrpMetaData*>& grp);

    /*!
     * @param metaData The meta data item to update
     * @return error code
     */
    int updateMessageMetaData(MsgLocMetaData* metaData);

    /*!
     * @param metaData The meta data item to update
     * @return error code
     */
    int updateGroupMetaData(GrpLocMetaData* meta);


    /*!
     * Completely clear out data stored in
     * and returns this to a state
     * as it was when first constructed
     * @return error code
     */
    int resetDataStore();

private:

    /*!
     * Retrieves all the msg results from a cursor
     * @param c cursor to result set
     * @param msgs messages retrieved from cursor are stored here
     */
    void retrieveMessages(RetroCursor* c, std::vector<RsNxsMsg*>& msgs);

    /*!
     * extracts a msg meta item from a cursor at its
     * current position
     */
    RsGxsMsgMetaData* getMsgMeta(RetroCursor& c);

    /*!
     * extracts a grp meta item from a cursor at its
     * current position
     */
    RsGxsGrpMetaData* getGrpMeta(RetroCursor& c);

    /*!
     * extracts a msg item from a cursor at its
     * current position
     */
    RsNxsMsg* getMessage(RetroCursor& c);

    /*!
     * extracts a grp item from a cursor at its
     * current position
     */
    RsNxsGrp* getGroup(RetroCursor& c);

    /*!
     * Creates an sql database and its associated file
     * also creates the message and groups table
     */
    void initialise();

private:


    RetroDb* mDb;

    std::list<std::string> msgColumns;
    std::list<std::string> msgMetaColumns;

    std::list<std::string> grpColumns;
    std::list<std::string> grpMetaColumns;

    std::string mServiceDir, mDbName;
    uint16_t mServType;
};

#endif // RSDATASERVICE_H
