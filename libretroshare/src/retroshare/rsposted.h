#ifndef RETROSHARE_POSTED_GUI_INTERFACE_H
#define RETROSHARE_POSTED_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsposted.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2012 by Robert Fernie.
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
class RsPosted;
extern RsPosted *rsPosted;


class RsPostedGroup
{
	public:
	RsGroupMetaData mMeta;
	RsPostedGroup() { return; }
};

class RsPostedMsg
{
	public:
	RsPostedMsg(uint32_t t)
	:postedType(t) { return; }

	RsMsgMetaData mMeta;
	uint32_t postedType;
};

#define RSPOSTED_MSGTYPE_POST		0x0001
#define RSPOSTED_MSGTYPE_VOTE		0x0002
#define RSPOSTED_MSGTYPE_COMMENT	0x0004

#define RSPOSTED_PERIOD_YEAR		1
#define RSPOSTED_PERIOD_MONTH		2
#define RSPOSTED_PERIOD_WEEK		3
#define RSPOSTED_PERIOD_DAY		4
#define RSPOSTED_PERIOD_HOUR		5

#define RSPOSTED_VIEWMODE_LATEST	1
#define RSPOSTED_VIEWMODE_TOP		2
#define RSPOSTED_VIEWMODE_HOT		3
#define RSPOSTED_VIEWMODE_COMMENTS	4


class RsPostedPost: public RsPostedMsg
{
	public:
	RsPostedPost(): RsPostedMsg(RSPOSTED_MSGTYPE_POST) 
	{ 
		mMeta.mMsgFlags = RSPOSTED_MSGTYPE_POST;
		return; 
	}

	std::string mLink;
	std::string mNotes;
};


class RsPostedVote: public RsPostedMsg
{
	public:
	RsPostedVote(): RsPostedMsg(RSPOSTED_MSGTYPE_VOTE)
	{ 
		mMeta.mMsgFlags = RSPOSTED_MSGTYPE_VOTE;
		return; 
	}
};


class RsPostedComment: public RsPostedMsg
{
	public:
	RsPostedComment(): RsPostedMsg(RSPOSTED_MSGTYPE_COMMENT) 
	{ 
		mMeta.mMsgFlags = RSPOSTED_MSGTYPE_COMMENT;
		return; 
	}

	std::string mComment;
};


std::ostream &operator<<(std::ostream &out, const RsPostedGroup &group);
std::ostream &operator<<(std::ostream &out, const RsPostedPost &post);
std::ostream &operator<<(std::ostream &out, const RsPostedVote &vote);
std::ostream &operator<<(std::ostream &out, const RsPostedComment &comment);


class RsPosted: public RsTokenService
{
	public:

	RsPosted()  { return; }
virtual ~RsPosted() { return; }

	/* Specific Service Data */
virtual bool getGroup(const uint32_t &token, RsPostedGroup &group) = 0;
virtual bool getPost(const uint32_t &token, RsPostedPost &post) = 0;
virtual bool getComment(const uint32_t &token, RsPostedComment &comment) = 0;

virtual bool submitGroup(uint32_t &token, RsPostedGroup &group, bool isNew) = 0;
virtual bool submitPost(uint32_t &token, RsPostedPost &post, bool isNew) = 0;
virtual bool submitVote(uint32_t &token, RsPostedVote &vote, bool isNew) = 0;
virtual bool submitComment(uint32_t &token, RsPostedComment &comment, bool isNew) = 0;

	// Special Ranking Request.
virtual bool requestRanking(uint32_t &token, std::string groupId) = 0;
virtual bool getRankedPost(const uint32_t &token, RsPostedPost &post) = 0;

virtual bool extractPostedCache(const std::string &str, uint32_t &votes, uint32_t &comments) = 0;


	// Control Ranking Calculations.
virtual bool setViewMode(uint32_t mode) = 0;
virtual bool setViewPeriod(uint32_t period) = 0;
virtual bool setViewRange(uint32_t first, uint32_t count) = 0;

	// exposed for testing...
virtual float calcPostScore(const RsMsgMetaData &meta) = 0;

};


#endif
