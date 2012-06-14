#ifndef RETROSHARE_WIKI_GUI_INTERFACE_H
#define RETROSHARE_WIKI_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rswiki.h
 *
 * RetroShare C++ Interface.
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

#include <inttypes.h>
#include <string>
#include <list>

#include <retroshare/rsidentity.h>

/* The Main Interface Class - for information about your Peers */
class RsWiki;
extern RsWiki *rsWiki;

class RsWikiGroupShare
{
	public:

	uint32_t mShareType;
	std::string mShareGroupId;
	std::string mPublishKey;
	uint32_t mCommentMode;
	uint32_t mResizeMode;
};

class RsWikiGroup
{
	public:

	RsGroupMetaData mMeta;

	//std::string mGroupId;
	//std::string mName;

	std::string mDescription;
	std::string mCategory;

	std::string mHashTags;

	RsWikiGroupShare mShareOptions;
};

class RsWikiPage
{
	public:

	RsMsgMetaData mMeta;

	// IN META DATA.
	//std::string mGroupId;
	//std::string mOrigPageId;
	//std::string mPageId;
	//std::string mName;

	// WE SHOULD SWITCH TO USING THREAD/PARENT IDS HERE....
	std::string mPrevId;

	std::string mPage; // all the text is stored here.

	std::string mHashTags;
};

class RsWiki: public RsTokenService
{
	public:

	RsWiki()  { return; }
virtual ~RsWiki() { return; }

	/* changed? */
virtual bool updated() = 0;

        /* Data Requests (from RsTokenService) */
//virtual bool requestGroupList(     uint32_t &token, const RsTokReqOptions &opts) = 0;
//virtual bool requestMsgList(       uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &groupIds) = 0;
//virtual bool requestMsgRelatedList(uint32_t &token, const RsTokReqOptions &opts, const std::list<std::string> &msgIds) = 0;

//virtual bool requestGroupData(     uint32_t &token, const std::list<std::string> &groupIds) = 0;
//virtual bool requestMsgData(       uint32_t &token, const std::list<std::string> &msgIds) = 0;

        /* Poll */
//virtual uint32_t requestStatus(const uint32_t token) = 0;

	/* Generic List Data */
virtual bool getGroupList(const uint32_t &token, std::list<std::string> &groupIds) = 0;
virtual bool getMsgList(const uint32_t &token, std::list<std::string> &msgIds) = 0;

	/* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, RsWikiGroup &group) = 0;
virtual bool getMsgData(const uint32_t &token, RsWikiPage &page) = 0;



/* details are updated in group - to choose GroupID */
virtual bool createGroup(RsWikiGroup &group) = 0;
virtual bool createPage(RsWikiPage &page) = 0;

#if 0
virtual bool getGroupList(std::list<std::string> &groups) = 0;
virtual bool getGroup(const std::string &groupid, RsWikiGroup &group) = 0;
virtual bool getPage(const std::string &pageid, RsWikiPage &page) = 0;
virtual bool getPageVersions(const std::string &origPageId, std::list<std::string> &pages) = 0;
virtual bool getOrigPageList(const std::string &groupid, std::list<std::string> &pageIds) = 0;
virtual bool getLatestPage(const std::string &origPageId, std::string &pageId) = 0;
#endif


};



#endif
