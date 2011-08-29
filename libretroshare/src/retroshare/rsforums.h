#ifndef RS_FORUM_GUI_INTERFACE_H
#define RS_FORUM_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsforums.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Robert Fernie.
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


#include <list>
#include <iostream>
#include <string>

#include "rstypes.h"
#include "rsdistrib.h" /* For FLAGS */

#define FORUM_MSG_STATUS_MASK           0x000f
#define FORUM_MSG_STATUS_READ           0x0001
#define FORUM_MSG_STATUS_UNREAD_BY_USER 0x0002

class ForumInfo 
{
	public:
		ForumInfo() 
		{
			forumFlags = 0 ;
			subscribeFlags = 0 ;
			pop = 0 ;
			lastPost = 0 ;
		}
		std::string forumId;
		std::wstring forumName;
		std::wstring forumDesc;

		uint32_t forumFlags;
		uint32_t subscribeFlags;

		uint32_t pop;

		time_t lastPost;
};

class ForumMsgInfo 
{
	public:
		ForumMsgInfo() 
		{
			msgflags = 0 ;
			ts = childTS = status = 0 ;
		}
		std::string forumId;
		std::string threadId;
		std::string parentId;
		std::string msgId;

		std::string srcId; /* if Authenticated -> signed here */

		unsigned int msgflags;

		std::wstring title;
		std::wstring msg;
		time_t ts;
		time_t childTS;
		uint32_t status;
};


class ThreadInfoSummary 
{
	public:
	ThreadInfoSummary() 
	{
		msgflags = 0 ;
		count = 0 ;
		ts = childTS = 0 ;
	}
	std::string forumId;
	std::string threadId;
	std::string parentId;
	std::string msgId;

	uint32_t msgflags;

	std::wstring title;
	std::wstring msg;
	int count; /* file count     */
	time_t ts;
	time_t childTS;
};

std::ostream &operator<<(std::ostream &out, const ForumInfo &info);
std::ostream &operator<<(std::ostream &out, const ThreadInfoSummary &info);
std::ostream &operator<<(std::ostream &out, const ForumMsgInfo &info);

class RsForums;
extern RsForums   *rsForums;

class RsForums 
{
	public:

	RsForums() { return; }
virtual ~RsForums() { return; }

/****************************************/

virtual bool forumsChanged(std::list<std::string> &forumIds) = 0;


virtual std::string createForum(const std::wstring &forumName, const std::wstring &forumDesc, uint32_t forumFlags) = 0;

virtual bool getForumInfo(const std::string &fId, ForumInfo &fi) = 0;

/*!
 * allows peers to change information for the forum:
 * can only change name and descriptions
 *
 */
virtual bool setForumInfo(const std::string &fId, ForumInfo &fi) = 0;

virtual bool getForumList(std::list<ForumInfo> &forumList) = 0;
virtual bool getForumThreadList(const std::string &fId, std::list<ThreadInfoSummary> &msgs) = 0;
virtual bool getForumThreadMsgList(const std::string &fId, const std::string &pId, std::list<ThreadInfoSummary> &msgs) = 0;
virtual bool getForumMessage(const std::string &fId, const std::string &mId, ForumMsgInfo &msg) = 0;
virtual bool setMessageStatus(const std::string& fId,const std::string& mId, const uint32_t status, const uint32_t statusMask) = 0;
virtual bool getMessageStatus(const std::string& fId, const std::string& mId, uint32_t& status) = 0;
virtual	bool ForumMessageSend(ForumMsgInfo &info) = 0;
virtual bool forumRestoreKeys(const std::string& fId) = 0;
virtual bool forumSubscribe(const std::string &fId, bool subscribe)	= 0;

/*!
 * shares keys with peers
 *@param fId the forum for which private publish keys will be shared
 *@param peers  list of peers to be sent keys
 *
 */
virtual bool forumShareKeys(std::string fId, std::list<std::string>& peers) = 0;

virtual	bool getMessageCount(const std::string &fId, unsigned int &newCount, unsigned int &unreadCount) = 0;

/****************************************/

};


#endif
