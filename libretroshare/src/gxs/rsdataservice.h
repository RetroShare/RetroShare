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
     * Retrieves latest version of msgs for a service
     * @param msgIds ids of messagesto retrieve
     * @param msg result of msg retrieval
     * @param cache whether to store retrieval in memory for faster later retrieval
     * @return error code
     */
    int retrieveMsgs(const std::string& grpId, std::map<std::string, RsGxsMsg*> msg, bool cache);

    /*!
     * Retrieves latest version of groups for a service
     * @param grpId the Id of the groups to retrieve
     * @param grp results of retrieval
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    int retrieveGrps(std::set<std::string, RsGxsGroup*>& grp, bool cache);

    /*!
     * Retrieves all the versions of a group
     * @param grpId the id of the group to get versions for
     * @param cache whether to store the result in memory
     * @return errCode
     */
    int retrieveGrpVersions(const std::string &grpId, std::set<RsNxsGrp *> &grp, bool cache);

    /*!
     * retrieves all the versions of a message for a group
     * @param grpId the id of the group message belongs to
     * @param msgId the id of the message to get versions for
     * @return errCode
     */
    int retrieveMsgVersions(const std::string &grpId, const std::string msgId, std::set<RsNxsMsg *> grp, bool cache);

    /*!
     * @param msgId the id of the message to retrieve
     * @return NULL if message does not exist, or pointer to grp if found
     */
    RsNxsMsg* retrieveMsgVersion(const RsGxsMsgId& msgId);

    /*!
     * @param grpId the id of the group to retrieve
     * @return NULL if group does not exist or pointer to grp if found
     */
    RsNxsGrp* retrieveGrpVersion(const RsGxsGrpId& grpId);

    /*!
     * allows for more complex queries specific to the service
     * @param search generally stores parameters needed for query
     * @param msgId is set with msg ids which satisfy the gxs search
     * @return error code
     */
    int searchMsgs(RsGxsSearch* search, std::list<RsGxsSrchResMsgCtx*>& result);

    /*!
     * allows for more complex queries specific to the associated service
     * @param search generally stores parameters needed for query
     * @param msgId is set with msg ids which satisfy the gxs search
     * @return error code
     */
    int searchGrps(RsGxsSearch* search, std::list<RsGxsSrchResGrpCtx*>& result);

    /*!
     * @return the cache size set for this RsGeneralDataService in bytes
     */
    uint32_t cacheSize() const;

    /*!
     * @param size size of cache to set in bytes
     */
    virtual int setCacheSize(uint32_t size);

    /*!
     * Stores a list signed messages into data store
     * @param msg list of signed messages to store
     * @return error code
     */
    int storeMessage(std::set<RsNxsMsg*>& msg);

    /*!
     * Stores a list of groups in data store
     * @param msg list of messages
     * @return error code
     */
    int storeGroup(std::set<RsNxsGrp*>& grp);

private:


    RsNxsMsg* getMessage(RetroCursor& c);
    RsNxsGrp* getGroup(RetroCursor& c);

private:


    RetroDb* mDb;
    std::list<std::string> msgColumns;
    std::list<std::string> grpColumns;

    std::string mServiceDir, mDbName;
    uint16_t mServType;
};

#endif // RSDATASERVICE_H
