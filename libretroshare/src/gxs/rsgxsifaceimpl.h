#ifndef RSGXSIFACEIMPL_H
#define RSGXSIFACEIMPL_H

#include "gxs/rsgenexchange.h"

/*!
 * The simple idea of this class is to implement the simple interface functions
 * of gen exchange
 */
class RsGxsIfaceImpl
{
public:

    RsGxsIfaceImpl(RsGenExchange* gxs);


    /*!
     * @return true if a change has occured
     */
    bool updated();

public:

    /** Requests **/

    /*!
     *
     * @param grpIds
     */
    void groupsChanged(std::list<RsGxsGroupId>& grpIds);

    /*!
     *
     * @param msgs
     */
    void msgsChanged(std::map<RsGxsGroupId,
                             std::vector<RsGxsMessageId> >& msgs);

    RsTokenService* getTokenService();

    /* Generic Lists */

    /*!
     *
     * @param token token to be redeemed for this request
     * @param groupIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getGroupList(const uint32_t &token,
                              std::list<RsGxsGroupId> &groupIds);

    /*!
     * @param token token to be redeemed for this request
     * @param msgIds the ids return for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgList(const uint32_t &token,
                            GxsMsgIdResult& msgIds);

    /* Generic Summary */

    /*!
     * @param token token to be redeemed for group summary request
     * @param groupInfo the ids returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getGroupSummary(const uint32_t &token,
                                 std::list<RsGroupMetaData> &groupInfo);

    /*!
     * @param token token to be redeemed for message summary request
     * @param msgInfo the message metadata returned for given request token
     * @return false if request token is invalid, check token status for error report
     */
    bool getMsgSummary(const uint32_t &token,
                               GxsMsgMetaMap &msgInfo);

    /*!
     * subscribes to group, and returns token which can be used
     * to be acknowledged to get group Id
     * @param token token to redeem for acknowledgement
     * @param grpId the id of the group to subscribe to
     */
    virtual bool subscribeToAlbum(uint32_t& token, const RsGxsGroupId& grpId, bool subscribe);

    /*!
     * This allows the client service to acknowledge that their msgs has
     * been created/modified and retrieve the create/modified msg ids
     * @param token the token related to modification/create request
     * @param msgIds map of grpid->msgIds of message created/modified
     * @return true if token exists false otherwise
     */
    bool acknowledgeMsg(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId);

    /*!
     * This allows the client service to acknowledge that their grps has
     * been created/modified and retrieve the create/modified grp ids
     * @param token the token related to modification/create request
     * @param msgIds vector of ids of groups created/modified
     * @return true if token exists false otherwise
     */
    bool acknowledgeGrp(const uint32_t& token, RsGxsGroupId& grpId);



private:

    RsGenExchange* mGxs;
};

#endif // RSGXSIFACEIMPL_H
