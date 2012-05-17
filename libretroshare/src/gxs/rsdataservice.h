#ifndef RSDATASERVICE_H
#define RSDATASERVICE_H

#include "rsgds.h"

#include "util/retrodb.h"

class RsDataService : public RsGeneralDataService
{
public:

    RsDataService(const std::string& workingDir, const std::string& dbName, uint16_t serviceType);
    virtual ~RsDataService();

    /*!
     * Retrieves signed message
     * @param msgIds ids of messagesto retrieve
     * @param msg result of msg retrieval
     * @param cache whether to store retrieval in memory for faster later retrieval
     * @return error code
     */
    int retrieveMsgs(const std::list<std::string>& msgIds, std::set<RsGxsMsg*> msg, bool cache);

    /*!
     * Retrieves a group item by grpId
     * @param grpId the Id of the groups to retrieve
     * @param grp results of retrieval
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    int retrieveGrps(const std::list<std::string>& grpId, std::set<RsGxsGroup*>& grp, bool cache);

    /*!
     * @param grpId the id of the group to get versions for
     * @param cache whether to store the result in memory
     * @param errCode
     */
    int retrieveGrpVersions(const std::string& grpId, std::set<RsGxsGroup*>& grp, bool cache);

    /*!
     * @param msgId the id of the message to get versions for
     * @param cache whether to store the result in memory
     * @param errCode
     */
    int retrieveMsgVersions(const std::string& grpId, std::set<RsGxsGroup*> grp, bool cache);


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
     * @return the cache size set for this RsGeneralDataService
     */
    uint32_t cacheSize() const;


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
};

#endif // RSDATASERVICE_H
