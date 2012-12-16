#ifndef GENEXCHANGETESTSERVICE_H
#define GENEXCHANGETESTSERVICE_H

#include "gxs/rsgenexchange.h"
#include "gxs/rsgxsifaceimpl.h"
#include "rsdummyservices.h"

class GenExchangeTestService : public RsGenExchange
{
public:
    GenExchangeTestService(RsGeneralDataService* dataServ, RsNetworkExchangeService*, RsGixs* gixs, uint32_t authenPolicy);

    void notifyChanges(std::vector<RsGxsNotify*>& changes);

    void publishDummyGrp(uint32_t& token, RsDummyGrp* grp);
    void publishDummyMsg(uint32_t& token, RsDummyMsg* msg);


    /*!
     * Retrieve group list for a given token
     * @param token
     * @param groupIds
     * @return false if token cannot be redeemed, if false you may have tried to redeem when not ready
     */
    bool getGroupListTS(const uint32_t &token, std::list<RsGxsGroupId> &groupIds);

    /*!
     * Retrieve msg list for a given token sectioned by group Ids
     * @param token token to be redeemed
     * @param msgIds a map of grpId -> msgList (vector)
     */
    bool getMsgListTS(const uint32_t &token, GxsMsgIdResult &msgIds);


    /*!
     * retrieve group meta data associated to a request token
     * @param token
     * @param groupInfo
     */
    bool getGroupMetaTS(const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);

    /*!
     * retrieves message meta data associated to a request token
     * @param token token to be redeemed
     * @param msgInfo the meta data to be retrieved for token store here
     */
    bool getMsgMetaTS(const uint32_t &token, GxsMsgMetaMap &msgInfo);

    /*!
     * retrieves group data associated to a request token
     * @param token token to be redeemed for grpitem retrieval
     * @param grpItem the items to be retrieved for token are stored here
     */
    bool getGroupDataTS(const uint32_t &token, std::vector<RsGxsGrpItem*>& grpItem);

    /*!
     * retrieves message data associated to a request token
     * @param token token to be redeemed for message item retrieval
     * @param msgItems
     */
    bool getMsgDataTS(const uint32_t &token, GxsMsgDataMap& msgItems);

    /*!
     * Retrieve msg related list for a given token sectioned by group Ids
     * @param token token to be redeemed
     * @param msgIds a map of grpMsgIdPair -> msgList (vector)
     */
    bool getMsgRelatedListTS(const uint32_t &token, MsgRelatedIdResult &msgIds);

    /*!
     * retrieves msg related data msgItems as a map of msg-grpID pair to vector
     * of items
     * @param token token to be redeemed
     * @param msgItems map of msg items
     */
    bool getMsgRelatedDataTS(const uint32_t &token, GxsMsgRelatedDataMap& msgItems);


    void setGroupSubscribeFlagTS(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask);

    void setGroupStatusFlagTS(uint32_t& token, const RsGxsGroupId& grpId, const uint32_t& status, const uint32_t& mask);

    void setGroupServiceStringTS(uint32_t& token, const RsGxsGroupId& grpId, const std::string& servString);

    void setMsgStatusFlagTS(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const uint32_t& status, const uint32_t& mask);

    void setMsgServiceStringTS(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, const std::string& servString );

    void service_tick();

};

#endif // GENEXCHANGETESTSERVICE_H
