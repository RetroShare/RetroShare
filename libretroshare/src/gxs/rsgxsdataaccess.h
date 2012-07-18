#ifndef RSGXSDATAACCESS_H
#define RSGXSDATAACCESS_H

#include "rstokenservice.h"
#include "rsgds.h"


class GxsRequest
{

public:
	uint32_t token;
	uint32_t reqTime;

	uint32_t ansType;
	uint32_t reqType;
	RsTokReqOptions Options;

	uint32_t status;
};

class GroupMetaReq : public GxsRequest
{

public:
	std::list<std::string> mGroupIds;
	std::list<RsGxsGrpMetaData*> mGroupMetaData;
};

class GroupIdReq : public GxsRequest
{

public:
	std::list<std::string> mGroupIds;
	std::list<std::string> mGroupIdResult;
};

class GroupDataReq : public GxsRequest
{

public:
	std::list<std::string> mGroupIds;
	std::list<RsNxsGrp*> mGroupData;
};


class MsgIdReq : public GxsRequest
{

public:

	GxsMsgReq mMsgIds;
	GxsMsgIdResult mMsgIdResult;
};

class MsgMetaReq : public GxsRequest
{

public:
	GxsMsgReq mMsgIds;
	GxsMsgMetaResult   mMsgMetaData;
};

class MsgDataReq : public GxsRequest
{

public:

	GxsMsgReq mMsgIds;
	GxsMsgDataResult mMsgData;
};



class RsGxsDataAccess : public RsTokenService
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
    bool requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);

    /*!
     * For requesting info on all messages of one or more groups
     * @param token
     * @param ansType
     * @param opts
     * @param groupIds
     * @return
     */
    bool requestMsgInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const GxsMsgReq&);

    /*!
     * More involved for request of particular information for msg
     * @param token set to value if , should be discarded if routine returns false
     * @param ansType
     * @param opts
     * @param msgIds
     * @return true if successful in placing request, false otherwise
     */
    bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts,
    		const GxsMsgReq &msgIds);

    bool requestGroupSubscribe(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::string &grpId);

    /* Poll */
    uint32_t requestStatus(const uint32_t token);

    /* Cancel Request */
    bool cancelRequest(const uint32_t &token);

    /** E: RsTokenService **/

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
    bool getMsgData(const uint32_t &token, GxsMsgDataResult& msgData);

private:

    /** helper functions to implement token service **/

    /*!
     * Assigns a token value to passed integer
     * @param token is assigned a unique token value
     */
    void generateToken(uint32_t &token);
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
    void setReq(GxsRequest* req,const uint32_t &token, const uint32_t& ansType, const RsTokReqOptions &opts) const;

    /*!
     * Remove request for request queue
     * Request is deleted
     * @param token the token associated to the request
     * @return true if token successfully cleared, false if token does not exist
     */
    bool clearRequest(const uint32_t &token);


    bool updateRequestStatus(const uint32_t &token, const uint32_t &status);

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

    /*!
     * Attempts to retrieve group id list from data store
     * @param req
     * @return false if unsuccessful, true otherwise
     */
    bool getGroupList(GroupIdReq* req);

    /*!
     * Attempts to retrieve msg id list from data store
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

private:

    RsGeneralDataService* mDataStore;
	uint32_t mNextToken;
	std::map<uint32_t, GxsRequest*> mRequests;

    RsMutex mDataMutex;


};

#endif // RSGXSDATAACCESS_H
