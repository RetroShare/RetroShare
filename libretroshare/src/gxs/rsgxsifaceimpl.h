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

    void groupsChanged(std::list<RsGxsGroupId>& grpIds);


    void msgsChanged(std::map<RsGxsGroupId,
                             std::vector<RsGxsMessageId> >& msgs);

    RsTokenService* getTokenService();

    bool getGroupList(const uint32_t &token,
                              std::list<RsGxsGroupId> &groupIds);
    bool getMsgList(const uint32_t &token,
                            GxsMsgIdResult& msgIds);

    /* Generic Summary */
    bool getGroupSummary(const uint32_t &token,
                                 std::list<RsGroupMetaData> &groupInfo);

    bool getMsgSummary(const uint32_t &token,
                               GxsMsgMetaMap &msgInfo);

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
