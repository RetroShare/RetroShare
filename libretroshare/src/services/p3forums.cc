/*
 * libretroshare/src/services: rsforums.cc
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

#include "services/p3forums.h"

std::ostream &operator<<(std::ostream &out, const ForumInfo &info)
{
	std::string name(info.forumName.begin(), info.forumName.end());
	std::string desc(info.forumDesc.begin(), info.forumDesc.end());

	out << "ForumInfo:";
	out << std::endl;
	out << "ForumId: " << info.forumId << std::endl;
	out << "ForumName: " << name << std::endl;
	out << "ForumDesc: " << desc << std::endl;
	out << "ForumFlags: " << info.forumFlags << std::endl;
	out << "Pop: " << info.pop << std::endl;
	out << "LastPost: " << info.lastPost << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const ThreadInfoSummary &info)
{
	out << "ThreadInfoSummary:";
	out << std::endl;
	//out << "ForumId: " << forumId << std::endl;
	//out << "ThreadId: " << threadId << std::endl;

	return out;
}

std::ostream &operator<<(std::ostream &out, const ForumMsgInfo &info)
{
	out << "ForumMsgInfo:";
	out << std::endl;
	//out << "ForumId: " << forumId << std::endl;
	//out << "ThreadId: " << threadId << std::endl;

	return out;
}


RsForums *rsForums = NULL;

p3Forums::p3Forums() 
	:mForumsChanged(false)
{ 
	loadDummyData();
	return; 
}

p3Forums::~p3Forums() 
{ 
	return; 
}

/****************************************/

bool p3Forums::forumsChanged(std::list<std::string> &forumIds)
{
	bool changed = mForumsChanged;
	mForumsChanged = false;
	return changed;
}


bool p3Forums::getForumList(std::list<ForumInfo> &forumList)
{
	std::list<ForumInfo>::iterator it;
	for(it = mForums.begin(); it != mForums.end(); it++)
	{
		forumList.push_back(*it);
	}
	return true;
}

bool p3Forums::getForumThreadList(std::string fId, std::list<ThreadInfoSummary> &msgs)
{
	return true;
}

bool p3Forums::getForumThreadMsgList(std::string fId, std::string tId, std::list<ThreadInfoSummary> &msgs)
{
	return true;
}

bool p3Forums::getForumMessage(std::string fId, std::string mId, ForumMsgInfo &msg)
{
	return true;
}

bool p3Forums::ForumMessageSend(ForumMsgInfo &info)
{
	return true;
}


/****************************************/

void    p3Forums::loadDummyData()
{
	ForumInfo fi;
	time_t now = time(NULL);

	fi.forumId = "FID1234";
	fi.forumName = L"Forum 1";
	fi.forumDesc = L"Forum 1";
	fi.forumFlags = RS_FORUM_ADMIN;
	fi.pop = 2;
	fi.lastPost = now - 123;

	mForums.push_back(fi);

	fi.forumId = "FID2345";
	fi.forumName = L"Forum 2";
	fi.forumDesc = L"Forum 2";
	fi.forumFlags = RS_FORUM_SUBSCRIBED;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);

	fi.forumId = "FID3456";
	fi.forumName = L"Forum 3";
	fi.forumDesc = L"Forum 3";
	fi.forumFlags = 0;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);

	fi.forumId = "FID4567";
	fi.forumName = L"Forum 4";
	fi.forumDesc = L"Forum 4";
	fi.forumFlags = 0;
	fi.pop = 5;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);
	fi.forumId = "FID5678";
	fi.forumName = L"Forum 5";
	fi.forumDesc = L"Forum 5";
	fi.forumFlags = 0;
	fi.pop = 1;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);
	fi.forumId = "FID6789";
	fi.forumName = L"Forum 6";
	fi.forumDesc = L"Forum 6";
	fi.forumFlags = 0;
	fi.pop = 2;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);
	fi.forumId = "FID7890";
	fi.forumName = L"Forum 7";
	fi.forumDesc = L"Forum 7";
	fi.forumFlags = 0;
	fi.pop = 4;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);
	fi.forumId = "FID8901";
	fi.forumName = L"Forum 8";
	fi.forumDesc = L"Forum 8";
	fi.forumFlags = 0;
	fi.pop = 3;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);
	fi.forumId = "FID9012";
	fi.forumName = L"Forum 9";
	fi.forumDesc = L"Forum 9";
	fi.forumFlags = 0;
	fi.pop = 2;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);

	fi.forumId = "FID9123";
	fi.forumName = L"Forum 10";
	fi.forumDesc = L"Forum 10";
	fi.forumFlags = 0;
	fi.pop = 1;
	fi.lastPost = now - 1234;

	mForums.push_back(fi);

	mForumsChanged = true;
}


