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

#include "serialiser/rsgxsitems.h"

typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > GxsMsgReq;
typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > GxsMsgIdResult;
typedef std::map<RsGxsGroupId, std::vector<RsGxsMsgMetaData*> > GxsMsgMetaResult;
typedef std::map<RsGxsGroupId, std::vector<RsNxsMsg*> > NxsMsgDataResult;

#define GXS_REQUEST_TYPE_GROUP_DATA			0x00010000
#define GXS_REQUEST_TYPE_GROUP_META			0x00020000
#define GXS_REQUEST_TYPE_GROUP_IDS			0x00040000
#define GXS_REQUEST_TYPE_MSG_DATA			0x00080000
#define GXS_REQUEST_TYPE_MSG_META			0x00100000
#define GXS_REQUEST_TYPE_MSG_IDS 			0x00200000

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
	mBefore = 0; mAfter = 0; mReqType = 0;
}

uint32_t mOptions;

// Request specific matches with Group / Message Status.
// Should be usable with any Options... applied afterwards.
uint32_t mStatusFilter;
uint32_t mStatusMask;

uint32_t mReqType;

uint32_t mSubscribeFilter; // Only for Groups.

// Time range... again applied after Options.
time_t   mBefore;
time_t   mAfter;
};


/*!
 * A proxy class for requesting generic service data for GXS
 * This seperates the request mechanism from the actual retrieval of data
 */
class RsTokenService
{

public:

	static const uint8_t GXS_REQUEST_STATUS_FAILED;
	static const uint8_t GXS_REQUEST_STATUS_PENDING;
	static const uint8_t GXS_REQUEST_STATUS_PARTIAL;
	static const uint8_t GXS_REQUEST_STATUS_FINISHED_INCOMPLETE;
	static const uint8_t GXS_REQUEST_STATUS_COMPLETE;
	static const uint8_t GXS_REQUEST_STATUS_DONE;			 // ONCE ALL DATA RETRIEVED.

public:

    RsTokenService()  { return; }
    virtual ~RsTokenService() { return; }

        /* Data Requests */

    /*!
     * Use this to request group related information
     * @param token The token returned for the request, store this value to pool for request completion
     * @param ansType The type of result (e.g. group data, meta, ids)
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds group id to request info for. Leave empty to get info on all groups,
     * @return
     */
    virtual bool requestGroupInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<RsGxsGroupId> &groupIds) = 0;

    /*!
     * Use this to get msg related information, store this value to pole for request completion
     * @param token The token returned for the request
     * @param ansType The type of result wanted
     * @param opts Additional option that affect outcome of request. Please see specific services, for valid values
     * @param groupIds The ids of the groups to get, second entry of map empty to query for all msgs
     * @return
     */
    virtual bool requestMsgInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const GxsMsgReq& msgIds) = 0;



    /*!
     * This sets the status of the message
     * @param msgId the message id to set status for
     * @param status status
     * @param statusMask the mask for the settings targetted
     * @return true if request made successfully, false otherwise
     */
    virtual bool requestSetMessageStatus(uint32_t &token, const RsGxsGrpMsgIdPair &msgId,
    		const uint32_t status, const uint32_t statusMask) = 0;

    /*!
     * Set the status of a group given by group Id
     * @param token The token returned for this request
     * @param grpId The Id of the group to apply status change to
     * @param status The status to apply
     * @param statusMask The status mask (target particular type of status)
     * @return true if request made successfully, false otherwise
     */
    virtual bool requestSetGroupStatus(uint32_t &token, const RsGxsGroupId &grpId, const uint32_t status,
    		const uint32_t statusMask) = 0;

    /*!
     * Use request status to find out if successfully set
     * @param groupId
     * @param subscribeFlags
     * @param subscribeMask
     * @return true if request made successfully, false otherwise
     */
    virtual bool requestSetGroupSubscribeFlags(uint32_t& token, const RsGxsGroupId &groupId, uint32_t subscribeFlags, uint32_t subscribeMask) = 0;


    	// (FUTURE WORK).
    //virtual bool groupRestoreKeys(const std::string &groupId) = 0;
    //virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers) = 0;

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
