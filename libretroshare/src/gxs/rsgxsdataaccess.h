/*******************************************************************************
 * libretroshare/src/gxs: rsgxsdataaccess.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Christopher Evi-Parker, Robert Fernie                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RSGXSDATAACCESS_H
#define RSGXSDATAACCESS_H

#include <queue>
#include "retroshare/rstokenservice.h"
#include "rsgxsrequesttypes.h"
#include "rsgds.h"


typedef std::map< RsGxsGroupId, std::map<RsGxsMessageId, RsGxsMsgMetaData*> > MsgMetaFilter;
typedef std::map< RsGxsGroupId, RsGxsGrpMetaData* > GrpMetaFilter;

bool operator<(const std::pair<uint32_t,GxsRequest*>& p1,const std::pair<uint32_t,GxsRequest*>& p2);

class RsGxsDataAccess : public RsTokenService
{
public:
    explicit RsGxsDataAccess(RsGeneralDataService* ds);
    virtual ~RsGxsDataAccess() ;

public:

	/** S: RsTokenService
	 * TODO: cleanup
	 * In the following methods @param uint32_t ansType is of no use, it is
	 * deprecated and should be removed as soon as possible as it is cause of
	 * many confusions, instead use const RsTokReqOptions::mReqType &opts to
	 * specify the kind of data you are interested in.
	 * Most of the methods use const uint32_t &token as param type change it to
	 * uint32_t
	 */

    /*!
     * Use this to request group related information
     * @param token The token returned for the request, store this value to pool for request completion
	 * @param ansType @deprecated unused @see S: RsTokenService notice
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds group id to request info for
     * @return
     */
    bool requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<RsGxsGroupId> &groupIds) override;

    /*!
     * Use this to request all group related info
     * @param token The token returned for the request, store this value to pool for request completion
	 * @param ansType @deprecated unused @see S: RsTokenService notice
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @return
     */
    bool requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts) override;

    /*!
     * Use this to get msg information (id, meta, or data), store token value to poll for request completion
     * @param token The token returned for the request
	 * @param ansType @deprecated unused @see S: RsTokenService notice
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds The ids of the groups to get, second entry of map empty to query for all msgs
     * @return true if request successful false otherwise
     */
    bool requestMsgInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const GxsMsgReq& msgIds) override;

    /*!
     * Use this to get message information (id, meta, or data), store token value to poll for request completion
     * @param token The token returned for the request
	 * @param ansType @deprecated unused @see S: RsTokenService notice
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds The ids of the groups to get, this retrieve all the msgs info for each grpId in list, if group Id list is empty \n
     * all messages for all groups are retrieved
     * @return true if request successful false otherwise
     */
    bool requestMsgInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<RsGxsGroupId>& grpIds) override;

    /*!
     * For requesting msgs related to a given msg id within a group
     * @param token The token returned for the request
	 * @param ansType @deprecated unused @see S: RsTokenService notice
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds The ids of the groups to get, second entry of map empty to query for all msgs
     * @return true if request successful false otherwise
     */
    bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::vector<RsGxsGrpMsgIdPair> &msgIds) override;

    /*!
     * This request statistics on amount of data held
     * number of groups
     * number of groups subscribed
     * number of messages
     * size of db store
     * total size of messages
     * total size of groups
     * @param token
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     */
    void requestServiceStatistic(uint32_t& token, const RsTokReqOptions &opts) override;

	/*!
	 * To request statistic on a group
	 * @param token set to value to be redeemed to get statistic
	 * @param grpId the id of the group
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
	 */
    void requestGroupStatistic(uint32_t& token, const RsGxsGroupId& grpId, const RsTokReqOptions &opts) override;

    /* Poll */
	GxsRequestStatus requestStatus(uint32_t token);

    /* Cancel Request */
    bool cancelRequest(const uint32_t &token);


    /** E: RsTokenService **/

public:

    /*!
     * This adds a groups to the gxs data base, this is a blocking call \n
     * If function returns successfully DataAccess can be queried for grp
     * @param grp the group to add, responsibility grp passed lies with callee
     * @return false if group cound not be added
     */
    bool addGroupData(RsNxsGrp* grp);

    /*!
	 * This updates a groups in the gxs data base, this is a blocking call \n
	 * If function returns successfully DataAccess can be queried for grp
	 * @param grp the group to add, responsibility grp passed lies with callee
	 * @return false if group cound not be added
	 */
    bool updateGroupData(RsNxsGrp* grp);

    /*!
     * This adds a group to the gxs data base, this is a blocking call \n
     * Responsibility for msg still lies with callee \n
     * If function returns successfully DataAccess can be queried for msg
     * @param msg the msg to add
     * @return false if msg could not be added, true otherwise
     */
    bool addMsgData(RsNxsMsg* msg);

    /*!
     * This retrieves a group from the gxs data base, this is a blocking call \n
     * @param grp the group to add, memory ownership passed to the callee
     * @return false if group cound not be retrieved
     */
    bool getGroupData(const RsGxsGroupId& grpId,RsNxsGrp *& grp_data);


public:



    /*!
     * This must be called periodically to progress requests
     */
    void processRequests();

    /*!
     * @param token
     * @param grpStatistic
     * @return false if token cannot be redeemed
     */
    bool getGroupStatistic(const uint32_t &token, GxsGroupStatistic& grpStatistic);

    /*!
     * @param token
     * @param servStatistic
     * @return false if token cannot be redeemed
     */
    bool getServiceStatistic(const uint32_t &token, GxsServiceStatistic& servStatistic);


    /*!
     * Retrieve group list for a given token
     * @param token request token to be redeemed
     * @param groupIds
     * @param msgIds
     * @return false if token cannot be redeemed, if false you may have tried to redeem when not ready
     */
    bool getGroupList(const uint32_t &token, std::list<RsGxsGroupId> &groupIds);

    /*!
     *
     * @param token request token to be redeemed
     * @param msgIds
     */
    bool getMsgIdList(const uint32_t &token, GxsMsgIdResult &msgIds);


    /*!
     * Retrieve msg list for a given token for message related info
     * @param token token to be redeemed
     * @param msgIds a map of RsGxsGrpMsgIdPair -> msgList (vector)
     * @return false if could not redeem token
     */
    bool getMsgRelatedList(const uint32_t &token, MsgRelatedIdResult& msgIds);


    /*!
     * @param token request token to be redeemed
     * @param groupInfo
     */
    bool getGroupSummary(const uint32_t &token, std::list<const RsGxsGrpMetaData*>& groupInfo);

    /*!
     *
     * @param token request token to be redeemed
     * @param msgInfo
     */
    bool getMsgSummary(const uint32_t &token, GxsMsgMetaResult &msgInfo);


    /*!
     * Retrieve msg meta for a given token for message related info
     * @param token token to be redeemed
     * @param msgIds a map of RsGxsGrpMsgIdPair -> msgList (vector)
     * @return false if could not redeem token
     */
    bool getMsgRelatedSummary(const uint32_t &token, MsgRelatedMetaResult& msgMeta);

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

    /*!
     *
     * @param token request token to be redeemed
     * @param msgData
     * @return false if data cannot be found for token
     */
    bool getMsgRelatedData(const uint32_t &token, NxsMsgRelatedDataResult& msgData);

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
    GxsRequest* locked_retrieveCompetedRequest(const uint32_t& token);

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
	void setReq(GxsRequest* req, uint32_t token, uint32_t ansType, const RsTokReqOptions& opts) const;

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
	bool locked_updateRequestStatus(uint32_t token, GxsRequestStatus status);

    /*!
     * Use to query the status and other values of a given token
     * @param token the toke of the request to check for
     * @param status set to current status of request
     * @param reqtype set to request type of request
     * @param anstype set to to anstype of request
     * @param ts time stamp
     * @return false if token does not exist, true otherwise
     */
	bool checkRequestStatus( uint32_t token, GxsRequestStatus &status,
	                         uint32_t &reqtype, uint32_t &anstype, rstime_t &ts);

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
    void cleanseMsgMetaMap(GxsMsgMetaResult& result);

public:

    /*!
     * Assigns a token value to passed integer
     * The status of the token can still be queried from request status feature
     * @param token is assigned a unique token value
     */
    uint32_t generatePublicToken();

    /*!
     * Updates the status of associate token
     * @param token
     * @param status
     * @return false if token could not be found, true if token disposed of
     */
	bool updatePublicRequestStatus(uint32_t token, GxsRequestStatus status);

    /*!
     * This gets rid of a publicly issued token
     * @param token
     * @return false if token could not found, true if token disposed of
     */
	bool disposeOfPublicToken(uint32_t token);

private:

    /* These perform the actual blocking retrieval of data */

    /*!
     * Attempts to retrieve group id list from data store
     * @param req
     * @return false if unsuccessful, true otherwise
     */
    bool getGroupList(GroupIdReq* req);

    /*!
     * convenience function for filtering grpIds
     * @param grpIdsIn The ids to filter with opts
     * @param opts the filter options
     * @param grpIdsOut grpIdsIn filtered with opts
     */
    bool getGroupList(const std::list<RsGxsGroupId>& grpIdsIn, const RsTokReqOptions& opts, std::list<RsGxsGroupId>& grpIdsOut);

    /*!
     * Attempts to retrieve msg id list from data store
     * Computationally/CPU-Bandwidth expensive
     * @param req
     * @return false if unsuccessful, true otherwise
     */
    bool getMsgIdList(MsgIdReq* req);

    /*!
     * Attempts to retrieve msg Meta list from data store
     * Computationally/CPU-Bandwidth expensive
     *
     * @param msgIds List of message Ids for the Message Metas to retrieve
     * @param opts   GxsRequest options
     * @param result Map of Meta information for messages
     *
     */
	bool getMsgMetaDataList( const GxsMsgReq& msgIds, const RsTokReqOptions& opts, GxsMsgMetaResult& result );

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
     * Attempts to retrieve messages related to msgIds of associated equest
     * @param req Request object to satisfy
     * @return false if data cannot be found for token
     */
    bool getMsgRelatedInfo(MsgRelatedInfoReq* req);


    /*!
     *
     * Attempts to retrieve group statistic
     * @param req Request object to satisfy
     */
    bool getGroupStatistic(GroupStatisticRequest* req);

    /*!
     *
     * Attempts to retrieve group data in serialized format
     * @param req Request object to satisfy
     */
	bool getGroupSerializedData(GroupSerializedDataReq* req);

    /*!
     *
     * Attempts to service statistic
     * @param req request object to satisfy
     */
    bool getServiceStatistic(ServiceStatisticRequest* req);

    /*!
     * This filter msgs based of options supplied (at the moment just status masks)
     * @param msgIds The msgsIds to filter
     * @param opts the request options set by user
     * @param meta The accompanying meta information for msg, ids
     */
    void filterMsgIdList(GxsMsgIdResult& msgIds, const RsTokReqOptions& opts, const MsgMetaFilter& meta) const;

    /*!
     * This filter msgs based of options supplied (at the moment just status masks)
     * @param grpIds The group ids to filter
     * @param opts the request options containing mask set by user
     * @param meta The accompanying meta information for group ids
     */
    void filterGrpList(std::list<RsGxsGroupId>& msgIds, const RsTokReqOptions& opts, const GrpMetaFilter& meta) const;


    /*!
     * This applies the options to the meta to find out if the given message satisfies
     * them
     * @param opts options containing filters to check
     * @param meta meta containing currently defined options for msg
     * @return true if msg meta passes all options
     */
    bool checkMsgFilter(const RsTokReqOptions& opts, const RsGxsMsgMetaData* meta) const;

    /*!
     * This applies the options to the meta to find out if the given group satisfies
     * them
     * @param opts options containing filters to check
     * @param meta meta containing currently defined options for group
     * @return true if group meta passes all options
     */
    bool checkGrpFilter(const RsTokReqOptions& opts, const RsGxsGrpMetaData* meta) const;


    /*!
     * This is a filter method which applies the request options to the list of ids
     * requested
     * @param msgIds the msg ids for filter to be applied to
     * @param opts the options used to parameterise the id filter
     * @param msgIdsOut the left overs ids after filter is applied to msgIds
     */
    bool getMsgIdList(const GxsMsgReq& msgIds, const RsTokReqOptions& opts, GxsMsgReq& msgIdsOut);

private:
    bool locked_clearRequest(const uint32_t &token);

    RsGeneralDataService* mDataStore;

    RsMutex mDataMutex; /* protecting below */

    uint32_t mNextToken;
	std::map<uint32_t, GxsRequestStatus> mPublicToken;

    std::set<std::pair<uint32_t,GxsRequest*> > mRequestQueue;
    std::map<uint32_t, GxsRequest*> mCompletedRequests;
};

#endif // RSGXSDATAACCESS_H
