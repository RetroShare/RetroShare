/*
 * libretroshare/src/services: p3posted.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#ifndef P3_POSTED_SERVICE_HEADER
#define P3_POSTED_SERVICE_HEADER

#include "services/p3gxsservice.h"
#include "retroshare/rsposted.h"

#include <map>
#include <string>

/* 
 * Posted Service
 *
 */


class PostedDataProxy: public GxsDataProxy
{
	public:

	bool addGroup(const RsPostedGroup &group);
	bool addPost(const RsPostedPost &post);
	bool addVote(const RsPostedVote &vote);
	bool addComment(const RsPostedComment &comment);

	bool getGroup(const std::string &id, RsPostedGroup &group);
	bool getPost(const std::string &id, RsPostedPost &post);
	bool getVote(const std::string &id, RsPostedVote &vote);
	bool getComment(const std::string &id, RsPostedComment &comment);

        /* These Functions must be overloaded to complete the service */
virtual bool convertGroupToMetaData(void *groupData, RsGroupMetaData &meta);
virtual bool convertMsgToMetaData(void *groupData, RsMsgMetaData &meta);

};



class p3PostedService: public p3GxsDataService, public RsPosted
{
	public:

	p3PostedService(uint16_t type);

virtual int	tick();

	public:

// NEW INTERFACE.
/************* Extern Interface *******/

virtual bool updated();

       /* Data Requests */
virtual bool requestGroupInfo(     uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgInfo(       uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgRelatedInfo(uint32_t &token, uint32_t ansType, const RsTokReqOptions &opts, const std::list<std::string> &msgIds);

        /* Generic Lists */
virtual bool getGroupList(         const uint32_t &token, std::list<std::string> &groupIds);
virtual bool getMsgList(           const uint32_t &token, std::list<std::string> &msgIds);

        /* Generic Summary */
virtual bool getGroupSummary(      const uint32_t &token, std::list<RsGroupMetaData> &groupInfo);
virtual bool getMsgSummary(        const uint32_t &token, std::list<RsMsgMetaData> &msgInfo);

        /* Actual Data -> specific to Interface */
        /* Specific Service Data */
virtual bool getGroup(const uint32_t &token, RsPostedGroup &group);
virtual bool getPost(const uint32_t &token, RsPostedPost &post);
virtual bool getComment(const uint32_t &token, RsPostedComment &comment);


        /* Poll */
virtual uint32_t requestStatus(const uint32_t token);

        /* Cancel Request */
virtual bool cancelRequest(const uint32_t &token);

        //////////////////////////////////////////////////////////////////////////////
        /* Functions from Forums -> need to be implemented generically */
virtual bool groupsChanged(std::list<std::string> &groupIds);

        // Get Message Status - is retrived via MessageSummary.
virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);

        // 
virtual bool groupSubscribe(const std::string &groupId, bool subscribe);

virtual bool groupRestoreKeys(const std::string &groupId);
virtual bool groupShareKeys(const std::string &groupId, std::list<std::string>& peers);

/* details are updated in album - to choose Album ID, and storage path */
virtual bool submitGroup(RsPostedGroup &group, bool isNew);
virtual bool submitPost(RsPostedPost &post, bool isNew);
virtual bool submitVote(RsPostedVote &vote, bool isNew);
virtual bool submitComment(RsPostedComment &comment, bool isNew);



	private:

std::string genRandomId();
bool generateDummyData();

	PostedDataProxy *mPostedProxy;

	RsMutex mPostedMtx;
	bool mUpdated;


};

#endif 
