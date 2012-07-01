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

#define RSPOSTED_MSG_POST		1
#define RSPOSTED_MSG_VOTE		1
#define RSPOSTED_MSG_COMMENT		1


class RsPostedPost: public RsPostedMsg
{
	public:
	RsPostedPost(): RsPostedMsg(RSPOSTED_MSG_POST) { return; }
};


class RsPostedVote: public RsPostedMsg
{
	public:
	RsPostedVote(): RsPostedMsg(RSPOSTED_MSG_VOTE) { return; }
};


class RsPostedComment: public RsPostedMsg
{
	public:
	RsPostedComment(): RsPostedMsg(RSPOSTED_MSG_COMMENT) { return; }
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

	/* changed? */
virtual bool updated() = 0;

	/* Specific Service Data */
virtual bool getGroup(const uint32_t &token, RsPostedGroup &group) = 0;
virtual bool getPost(const uint32_t &token, RsPostedPost &post) = 0;
virtual bool getComment(const uint32_t &token, RsPostedComment &comment) = 0;

/* details are updated in album - to choose Album ID, and storage path */
virtual bool submitGroup(RsPostedGroup &group, bool isNew) = 0;
virtual bool submitPost(RsPostedPost &post, bool isNew) = 0;
virtual bool submitVote(RsPostedVote &vote, bool isNew) = 0;
virtual bool submitComment(RsPostedComment &comment, bool isNew) = 0;

};


#endif
