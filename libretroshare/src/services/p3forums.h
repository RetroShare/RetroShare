#ifndef RS_P3_FORUMS_INTERFACE_H
#define RS_P3_FORUMS_INTERFACE_H

/*
 * libretroshare/src/services: p3forums.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008 by Robert Fernie.
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

#include "rsiface/rsforums.h"

class p3Forums: public RsForums 
{
	public:

	p3Forums();
virtual ~p3Forums();

/****************************************/
/********* rsForums Interface ***********/

virtual bool forumsChanged(std::list<std::string> &forumIds);

virtual bool getForumList(std::list<ForumInfo> &forumList);
virtual bool getForumThreadList(std::string fId, std::list<ThreadInfoSummary> &msgs);
virtual bool getForumThreadMsgList(std::string fId, std::string tId, std::list<ThreadInfoSummary> &msgs);
virtual bool getForumMessage(std::string fId, std::string mId, ForumMsgInfo &msg);

virtual	bool ForumMessageSend(ForumMsgInfo &info);

/****************************************/

	private:

void	loadDummyData();
std::list<ForumInfo> mForums;
bool 	mForumsChanged;
};


#endif
