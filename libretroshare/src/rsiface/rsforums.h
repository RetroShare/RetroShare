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

#include "rsiface/rstypes.h"

#define RS_FORUM_PUBLIC		0x0001   /* anyone can publish */
#define RS_FORUM_PRIVATE	0x0002   /* anyone with key can publish */
#define RS_FORUM_ENCRYPTED	0x0004   /* need admin key */

#define RS_FORUM_ADMIN		0x0100   /* anyone can publish */
#define RS_FORUM_SUBSCRIBED	0x0200   /* anyone can publish */


class ForumInfo 
{
	public:
	ForumInfo() {}
	std::string forumId;
	std::wstring forumName;
	std::wstring forumDesc;

	uint32_t forumFlags;
	uint32_t pop;

	time_t lastPost;
};

class ForumMsgInfo 
{
	public:
	ForumMsgInfo() {}
	std::string forumId;
	std::string threadId;
	std::string msgId;

	unsigned int msgflags;

	std::wstring title;
	std::wstring msg;
	time_t ts;
};


class ThreadInfoSummary 
{
	public:
	ThreadInfoSummary() {}

	std::string msgId;
	std::string srcId;

	uint32_t msgflags;

	std::wstring title;
	int count; /* file count     */
	time_t ts;

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

virtual bool getForumList(std::list<ForumInfo> &forumList) = 0;
virtual bool getForumThreadList(std::string fId, std::list<ThreadInfoSummary> &msgs) = 0;
virtual bool getForumThreadMsgList(std::string fId, std::string tId, std::list<ThreadInfoSummary> &msgs) = 0;
virtual bool getForumMessage(std::string fId, std::string mId, ForumMsgInfo &msg) = 0;

virtual	bool ForumMessageSend(ForumMsgInfo &info)                 = 0;

/****************************************/

};


#endif
