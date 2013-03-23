#ifndef RSTOKENSERVICE_H
#define RSTOKENSERVICE_H

/*
 * libretroshare/src/retroshare: rstokenservice.h
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

#include <inttypes.h>
#include <string>
#include <list>

//#include "gxs/rsgxs.h"
#include "retroshare/rsgxsifacetypes.h"

#define GXS_REQUEST_TYPE_GROUP_DATA			0x00010000
#define GXS_REQUEST_TYPE_GROUP_META			0x00020000
#define GXS_REQUEST_TYPE_GROUP_IDS			0x00040000
#define GXS_REQUEST_TYPE_MSG_DATA			0x00080000
#define GXS_REQUEST_TYPE_MSG_META			0x00100000
#define GXS_REQUEST_TYPE_MSG_IDS 			0x00200000

#define GXS_REQUEST_TYPE_MSG_RELATED_DATA		0x00400000
#define GXS_REQUEST_TYPE_MSG_RELATED_META		0x00800000
#define GXS_REQUEST_TYPE_MSG_RELATED_IDS 		0x01000000



// This bit will be filled out over time.
#define RS_TOKREQOPT_MSG_VERSIONS	0x0001		// MSGRELATED: Returns All MsgIds with OrigMsgId = MsgId.
#define RS_TOKREQOPT_MSG_ORIGMSG	0x0002		// MSGLIST: All Unique OrigMsgIds in a Group.
#define RS_TOKREQOPT_MSG_LATEST		0x0004		// MSGLIST: All Latest MsgIds in Group. MSGRELATED: Latest MsgIds for Input Msgs.

#define RS_TOKREQOPT_MSG_THREAD		0x0010		// MSGRELATED: All Msgs in Thread. MSGLIST: All Unique Thread Ids in Group.
#define RS_TOKREQOPT_MSG_PARENT		0x0020		// MSGRELATED: All Children Msgs.

#define RS_TOKREQOPT_MSG_AUTHOR		0x0040		// MSGLIST: Messages from this AuthorId

// Read Status.
#define RS_TOKREQOPT_READ		0x0001
#define RS_TOKREQOPT_UNREAD		0x0002

#define RS_TOKREQ_ANSTYPE_LIST		0x0001
#define RS_TOKREQ_ANSTYPE_SUMMARY	0x0002
#define RS_TOKREQ_ANSTYPE_DATA		0x0003
#define RS_TOKREQ_ANSTYPE_ACK           0x0004


/*!
 * This class provides useful generic support for GXS style services.
 * I expect much of this will be incorporated into the base GXS.
 */
class RsTokReqOptions
{
public:
RsTokReqOptions()
{
	mOptions = 0;
        mStatusFilter = 0; mStatusMask = 0; mSubscribeFilter = 0;
        mSubscribeMask = 0;
        mMsgFlagMask = 0; mMsgFlagFilter = 0;
	mBefore = 0; mAfter = 0; mReqType = 0;
}

uint32_t mOptions;

// Request specific matches with Group / Message Status.
// Should be usable with any Options... applied afterwards.
uint32_t mStatusFilter;
uint32_t mStatusMask;

// use
uint32_t mMsgFlagMask, mMsgFlagFilter;

uint32_t mReqType;

uint32_t mSubscribeFilter, mSubscribeMask; // Only for Groups.

// Time range... again applied after Options.
time_t   mBefore;
time_t   mAfter;
};

std::ostream &operator<<(std::ostream &out, const RsGroupMetaData &meta);
std::ostream &operator<<(std::ostream &out, const RsMsgMetaData &meta);

/*!
 * A proxy class for requesting generic service data for GXS
 * This seperates the request mechanism from the actual retrieval of data
 */
class RsTokenService
{

public:

        static const uint8_t GXS_REQUEST_V2_STATUS_FAILED;
        static const uint8_t GXS_REQUEST_V2_STATUS_PENDING;
        static const uint8_t GXS_REQUEST_V2_STATUS_PARTIAL;
        static const uint8_t GXS_REQUEST_V2_STATUS_FINISHED_INCOMPLETE;
        static const uint8_t GXS_REQUEST_V2_STATUS_COMPLETE;
        static const uint8_t GXS_REQUEST_V2_STATUS_DONE;			 // ONCE ALL DATA RETRIEVED.

public:

    RsTokenService()  { return; }
    virtual ~RsTokenService() { return; }

    /* Data Requests */

    /*!
     * Use this to request group related information
     * @param token The token returned for the request, store this value to pool for request completion
     * @param ansType The type of result (e.g. group data, meta, ids)
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds group id to request info for
     * @return
     */
    virtual bool requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<RsGxsGroupId> &groupIds) = 0;

    /*!
     * Use this to request all group related info
     * @param token The token returned for the request, store this value to pool for request completion
     * @param ansType The type of result (e.g. group data, meta, ids)
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @return
     */
    virtual bool requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts) = 0;

    /*!
     * Use this to get msg related information, store this value to pole for request completion
     * @param token The token returned for the request
     * @param ansType The type of result wanted
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds The ids of the groups to get, second entry of map empty to query for all msgs
     * @return true if request successful false otherwise
     */
    virtual bool requestMsgInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const GxsMsgReq& msgIds) = 0;

    /*!
     * Use this to get msg related information, store this value to pole for request completion
     * @param token The token returned for the request
     * @param ansType The type of result wanted
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds The ids of the groups to get, this retrieves all the msgs info for each grpId in list
     * @return true if request successful false otherwise
     */
    virtual bool requestMsgInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<RsGxsGroupId>& grpIds) = 0;

    /*!
     * For requesting msgs related to a given msg id within a group
     * @param token The token returned for the request
     * @param ansType The type of result wanted
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds The ids of the groups to get, second entry of map empty to query for all msgs
     * @return true if request successful false otherwise
     */
    virtual bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::vector<RsGxsGrpMsgIdPair>& msgIds) = 0;


    /* Poll */

    /*!
     * Request the status of ongoing request.
     * Please use this for polling as much cheaper
     * than polling the specific service as they might
     * not return intermediate status information
     * @param token value of token to check status for
     * @return the current status of request
     */
    virtual uint32_t requestStatus(const uint32_t token) = 0;

    /*!
     * This request statistics on amount of data held
     * number of groups
     * number of groups subscribed
     * number of messages
     * size of db store
     * total size of messages
     * total size of groups
     * @param token
     */
    virtual void requestServiceStatistic(const uint32_t& token) = 0;

	/*!
	 * To request statistic on a group
	 * @param token set to value to be redeemed to get statistic
	 * @param grpId the id of the group
	 */
	virtual void requestGroupStatistic(const uint32_t& token, const RsGxsGroupId& grpId) = 0;


        /* Cancel Request */

    /*!
     * If this function returns false, it may be that the request has completed
     * already. Useful for very expensive request. This is a blocking operation
     * @param token the token of the request to cancel
     * @return false if unusuccessful in cancelling request, true if successful
     */
    virtual bool cancelRequest(const uint32_t &token) = 0;



};

#endif // RSTOKENSERVICE_H
