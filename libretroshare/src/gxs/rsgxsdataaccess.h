#ifndef RSGXSDATAACCESS_H
#define RSGXSDATAACCESS_H

#include "rstokenservice.h"
#include "rsgds.h"

class RsGxsDataAccess : public RsTokenService
{
public:
    RsGxsDataAccess(RsGeneralDataService* ds, RsSerialType* serviceSerialiser);
    virtual ~RsGxsDataAccess() { return ;}

public:

    /** start: From RsTokenService **/

    bool requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
    bool requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
    bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds);
    bool requestGroupSubscribe(uint32_t &token, const std::string &groupId, bool subscribe);

    /* Poll */
    uint32_t requestStatus(const uint32_t token);

    /* Cancel Request */
    bool cancelRequest(const uint32_t &token);

    /** end : From RsTokenService **/

public:

    void processRequests();

    /*!
     * Retrieve group list for a given token
     * @param token request token to be redeemed
     * @param groupIds
     * @param msgIds
     * @return false if token cannot be redeemed, if false you may have tried to redeem when not ready
     */
    bool getGroupList(const uint32_t &token, std::list<std::string> &groupIds);

    /*!
     *
     * @param token request token to be redeemed
     * @param msgIds
     */
    bool getMsgList(const uint32_t &token, std::list<std::string> &msgIds);


    /*!
     * @param token request token to be redeemed
     * @param groupInfo
     */
    bool getGroupSummary(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);

    /*!
     *
     * @param token request token to be redeemed
     * @param msgInfo
     */
    bool getMsgSummary(const uint32_t &token, std::list<RsMsgMetaData> &msgInfo);

    /*!
     *
     * @param token request token to be redeemed
     * @param grpItem
     */
    bool getGroupData(const uint32_t &token, RsGxsGrpItem* grpItem);

    /*!
     *
     * @param token request token to be redeemed
     * @param msgItems
     * @return false if
     */
    bool getMsgData(const uint32_t &token, std::vector<RsGxsMsgItem*> msgItems);

private:

    /** helper functions to implement token service **/

    bool generateToken(uint32_t &token);
    bool storeRequest(const uint32_t &token, const uint32_t &ansType, const RsTokReqOptions &opts,
                      const uint32_t &type, const std::list<std::string> &ids);
    bool clearRequest(const uint32_t &token);

    bool updateRequestStatus(const uint32_t &token, const uint32_t &status);
    bool updateRequestInList(const uint32_t &token, std::list<std::string> ids);
    bool updateRequestOutList(const uint32_t &token, std::list<std::string> ids);
    bool checkRequestStatus(const uint32_t &token, uint32_t &status, uint32_t &reqtype, uint32_t &anstype, time_t &ts);

            // special ones for testing (not in final design)
    bool tokenList(std::list<uint32_t> &tokens);
    bool popRequestInList(const uint32_t &token, std::string &id);
    bool popRequestOutList(const uint32_t &token, std::string &id);


    virtual bool getGroupList(uint32_t &token, const RsTokReqOptions &opts,
                              const std::list<std::string> &groupIds, std::list<std::string> &outGroupIds);

    virtual bool getMsgList(uint32_t &token, const RsTokReqOptions &opts,
                            const std::list<std::string> &groupIds, std::list<std::string> &outMsgIds);

    virtual bool getMsgRelatedList(uint32_t &token, const RsTokReqOptions &opts,
                                   const std::list<std::string> &msgIds, std::list<std::string> &outMsgIds);


private:


};

#endif // RSGXSDATAACCESS_H
