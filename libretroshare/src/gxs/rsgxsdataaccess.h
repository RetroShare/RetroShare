#ifndef RSGXSDATAACCESS_H
#define RSGXSDATAACCESS_H

/*
 * libretroshare/src/retroshare: rsgxsdataaccess.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie, Christopher Evi-Parker
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

#include "rstokenservice.h"
#include "rsgxsrequesttypes.h"
#include "rsgds.h"


typedef std::map< RsGxsGroupId, std::map<RsGxsMessageId, RsGxsMsgMetaData*> > MsgMetaFilter;

class RsGxsDataAccess : public RsTokenServiceV2
{
public:
    RsGxsDataAccess(RsGeneralDataService* ds);
    virtual ~RsGxsDataAccess() { return ;}

public:

    /** S: RsTokenService **/

    /*!
     *
     * @param token
     * @param ansType
     * @param opts
     * @param groupIds
     * @return
     */
    bool requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptionsV2 &opts, const std::list<RsGxsGroupId> &groupIds);

    /*!
     * For requesting info on all messages of one or more groups
     * @param token
     * @param ansType
     * @param opts
     * @param groupIds
     * @return
     */
    bool requestMsgInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptionsV2 &opts, const GxsMsgReq&);

    /*!
     * This sets the status of the message
     * @param msgId the message id to set status for
     * @param status status
     * @param statusMask the mask for the settings targetted
     * @return true if request made successfully, false otherwise
     */
    bool requestSetMessageStatus(uint32_t &token, const RsGxsGrpMsgIdPair &msgId,
    		const uint32_t status, const uint32_t statusMask);

    /*!
     *
     * @param token
     * @param grpId
     * @param status
     * @param statusMask
     * @return true if request made successfully, false otherwise
     */
    bool requestSetGroupStatus(uint32_t &token, const RsGxsGroupId &grpId, const uint32_t status,
    		const uint32_t statusMask);

    /*!
     * Use request status to find out if successfully set
     * @param groupId
     * @param subscribeFlags
     * @param subscribeMask
     * @return true if request made successfully, false otherwise
     */
    bool requestSetGroupSubscribeFlags(uint32_t& token, const RsGxsGroupId &groupId, uint32_t subscribeFlags,
    		uint32_t subscribeMask);


    /* Poll */
    uint32_t requestStatus(const uint32_t token);

    /* Cancel Request */
    bool cancelRequest(const uint32_t &token);

    /** E: RsTokenService **/


public:

    /*!
     * This adds a groups to the gxs data base, this is a blocking call
     * Responsibility for grp still lies with callee \n
     * If function returns successfully DataAccess can be queried for grp
     * @param grp the group to add, responsibility grp passed lies with callee
     * @return false if group cound not be added
     */
    bool addGroupData(RsNxsGrp* grp);

    /*!
     * This adds a group to the gxs data base, this is a blocking call \n
     * Responsibility for msg still lies with callee \n
     * If function returns successfully DataAccess can be queried for msg
     * @param msg the msg to add
     * @return false if msg could not be added, true otherwise
     */
    bool addMsgData(RsNxsMsg* msg);

public:

    /*!
     * This must be called periodically to progress requests
     */
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
    bool getMsgList(const uint32_t &token, GxsMsgIdResult &msgIds);


    /*!
     * @param token request token to be redeemed
     * @param groupInfo
     */
    bool getGroupSummary(const uint32_t &token, std::list<RsGxsGrpMetaData*> &groupInfo);

    /*!
     *
     * @param token request token to be redeemed
     * @param msgInfo
     */
    bool getMsgSummary(const uint32_t &token, GxsMsgMetaResult &msgInfo);

    /*!
     *
     * @param token request token to be redeemed
     * @param grpData
     */
    bool getGroupData(const uint32_t &token, std::list<RsNxsGrp*>& grpData);

    /*!
     *
     * @param token request token to be redeemed
     * @param msgData
     * @return false if data cannot be found for token
     */
    bool getMsgData(const uint32_t &token, NxsMsgDataResult& msgData);

private:

    /** helper functions to implement token service **/

    /*!
     * Assigns a token value to passed integer
     * @param token is assigned a unique token value
     */
    void generateToken(uint32_t &token);

    /*!
     *
     * @param token the value of the token for the request object handle wanted
     * @return the request associated to this token
     */
    GxsRequest* retrieveRequest(const uint32_t& token);

    /*!
     * Add a gxs request to queue
     * @param req gxs request to add
     */
    void storeRequest(GxsRequest* req);

    /*!
     * convenience function to setting members of request
     * @param req
     * @param token
     * @param ansType
     * @param opts
     */
    void setReq(GxsRequest* req,const uint32_t &token, const uint32_t& ansType, const RsTokReqOptionsV2 &opts) const;

    /*!
     * Remove request for request queue
     * Request is deleted
     * @param token the token associated to the request
     * @return true if token successfully cleared, false if token does not exist
     */
    bool clearRequest(const uint32_t &token);

    /*!
     * Updates the status flag of a request
     * @param token the token value of the request to set
     * @param status the status to set
     * @return
     */
    bool updateRequestStatus(const uint32_t &token, const uint32_t &status);

    /*!
     * Use to query the status and other values of a given token
     * @param token the toke of the request to check for
     * @param status set to current status of request
     * @param reqtype set to request type of request
     * @param anstype set to to anstype of request
     * @param ts time stamp
     * @return false if token does not exist, true otherwise
     */
    bool checkRequestStatus(const uint32_t &token, uint32_t &status, uint32_t &reqtype, uint32_t &anstype, time_t &ts);

            // special ones for testing (not in final design)
    /*!
     * Get list of active tokens of this token service
     * @param tokens sets to list of token contained in this tokenservice
     */
    void tokenList(std::list<uint32_t> &tokens);

    /*!
     * Convenience function to delete the ids
     * @param filter the meta filter to clean
     */
    void cleanseMetaFilter(MsgMetaFilter& filter);

private:

    /* These perform the actual blocking retrieval of data */

    /*!
     * Attempts to retrieve group id list from data store
     * @param req
     * @return false if unsuccessful, true otherwise
     */
    bool getGroupList(GroupIdReq* req);

    /*!
     * Attempts to retrieve msg id list from data store
     * Computationally/CPU-Bandwidth expensive
     * @param req
     * @return false if unsuccessful, true otherwise
     */
    bool getMsgList(MsgIdReq* req);


    /*!
     * Attempts to retrieve group meta data from data store
     * @param req
     * @return false if unsuccessful, true otherwise
     */
    bool getGroupSummary(GroupMetaReq* req);

    /*!
     * Attempts to retrieve msg meta data from data store
     * @param req
     * @return false if unsuccessful, true otherwise
     */
    bool getMsgSummary(MsgMetaReq* req);

    /*!
     * Attempts to retrieve group data from data store
     * @param req The request specifying data to retrieve
     * @return false if unsuccessful, true otherwise
     */
    bool getGroupData(GroupDataReq* req);

    /*!
     * Attempts to retrieve message data from data store
     * @param req The request specifying data to retrieve
     * @return false if unsuccessful, true otherwise
     */
    bool getMsgData(MsgDataReq* req);

    /*!
     * This filter msgs based of options supplied (at the moment just status masks)
     * @param msgIds The msgsIds to filter
     * @param opts the request options set by user
     * @param meta The accompanying meta information for msg, ids
     */
    void filterMsgList(GxsMsgIdResult& msgIds, const RsTokReqOptionsV2& opts, const MsgMetaFilter& meta) const;


    /*!
     * This applies the options to the meta to find out if the given message satisfies
     * them
     * @param opts options containing filters to check
     * @param meta meta containing currently defined options for msg
     * @return true if msg meta passes all options
     */
    bool checkMsgFilter(const RsTokReqOptionsV2& opts, const RsGxsMsgMetaData* meta) const;

private:

    RsGeneralDataService* mDataStore;
	uint32_t mNextToken;
	std::map<uint32_t, GxsRequest*> mRequests;

    RsMutex mDataMutex;


};

#endif // RSGXSDATAACCESS_H
