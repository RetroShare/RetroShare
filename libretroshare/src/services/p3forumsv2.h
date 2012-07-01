/*
 * libretroshare/src/services: p3forumsv2.h
 *
 * Wiki interface for RetroShare.
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

#ifndef P3_FORUMSV2_SERVICE_HEADER
#define P3_FORUMSV2_SERVICE_HEADER

#include "services/p3gxsservice.h"

#include "retroshare/rsforumsv2.h"

#include <map>
#include <string>

/* 
 * Wiki Service
 *
 * This is an example service for the new cache system.
 * For the moment, it will only hold data passed to it from the GUI.
 * and spew that back when asked....
 *
 * We are doing it like this - so we can check the required interface functionality.
 *
 * Expect it won't take long before it'll be properly linked into the backend!
 *
 * This will be transformed into a Plugin Service, once the basics have been worked out.
 *
 */

class ForumDataProxy: public GxsDataProxy
{
        public:

        bool getForumGroup(const std::string &id, RsForumV2Group &group);
        bool getForumMsg(const std::string &id, RsForumV2Msg &msg);

        bool addForumGroup(const RsForumV2Group &group);
        bool addForumMsg(const RsForumV2Msg &msg);

        /* These Functions must be overloaded to complete the service */
virtual bool convertGroupToMetaData(void *groupData, RsGroupMetaData &meta);
virtual bool convertMsgToMetaData(void *msgData, RsMsgMetaData &meta);

};





class p3ForumsV2: public p3GxsDataService, public RsForumsV2
{
	public:

	p3ForumsV2(uint16_t type);

virtual int	tick();

	public:


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
virtual bool getGroupData(const uint32_t &token, RsForumV2Group &group);
virtual bool getMsgData(const uint32_t &token, RsForumV2Msg &msg);

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


virtual bool createGroup(RsForumV2Group &group);
virtual bool createMsg(RsForumV2Msg &msg);


	private:

std::string genRandomId();
bool 	generateDummyData();

	ForumDataProxy *mForumProxy;

	RsMutex mForumMtx;

	/***** below here is locked *****/

	bool mUpdated;




#if 0

        /* Data Requests */
virtual bool requestGroupList(     uint32_t &token, const RsTokReqOptions &opts);
virtual bool requestMsgList(       uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds);
virtual bool requestMsgRelatedList(uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &msgIds);

virtual bool requestGroupData(     uint32_t &token, const std::list<std::string> &groupIds);
virtual bool requestMsgData(       uint32_t &token, const std::list<std::string> &msgIds);

virtual bool getGroupList(const uint32_t &token, std::list<std::string> &groupIds);
virtual bool getMsgList(const uint32_t &token, std::list<std::string> &msgIds);

virtual bool getGroupData(const uint32_t &token, RsForumV2Group &group);
virtual bool getMsgData(const uint32_t &token, RsForumV2Msg &msg);

        /* Poll */
virtual uint32_t requestStatus(const uint32_t token);

virtual bool createGroup(RsForumV2Group &group);
virtual bool createPage(RsForumV2Msg &msg);


/************* Old Extern Interface *******/

virtual bool updated();
virtual bool getGroupList(std::list<std::string> &group);

virtual bool getGroup(const std::string &groupid, RsWikiGroup &group);
virtual bool getPage(const std::string &pageid, RsWikiPage &page);
virtual bool getPageVersions(const std::string &origPageId, std::list<std::string> &pages);
virtual bool getOrigPageList(const std::string &groupid, std::list<std::string> &pageIds);
virtual bool getLatestPage(const std::string &origPageId, std::string &page);

virtual bool createGroup(RsWikiGroup &group);
virtual bool createPage(RsWikiPage &page);


	private:

virtual bool InternalgetGroupList(std::list<std::string> &group);
virtual bool InternalgetGroup(const std::string &groupid, RsForumV2Group &group);
virtual bool InternalgetPage(const std::string &pageid, RsForumV2Msg &msg);
virtual bool InternalgetPageVersions(const std::string &origPageId, std::list<std::string> &pages);
virtual bool InternalgetOrigPageList(const std::string &groupid, std::list<std::string> &pageIds);
virtual bool InternalgetLatestPage(const std::string &origPageId, std::string &page);

std::string genRandomId();

	ForumDataProxy *mForumProxy;

	RsMutex mForumMtx;

	/***** below here is locked *****/

	bool mUpdated;

	std::map<std::string, std::list<std::string > > mGroupToOrigPages;
	std::map<std::string, std::list<std::string > > mOrigToPageVersions;
	std::map<std::string, std::string> mOrigPageToLatestPage;
	std::map<std::string, RsForumV2Group> mGroups;
	std::map<std::string, RsForumV2Msg> mPages;
#endif

};

#endif 
