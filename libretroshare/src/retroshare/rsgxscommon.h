#ifndef RETROSHARE_GXS_COMMON_OBJS_INTERFACE_H
#define RETROSHARE_GXS_COMMON_OBJS_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsgxscommong.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "rsgxsifacetypes.h"

class RsGxsFile
{
	public:
	RsGxsFile();
	std::string mName;
	std::string mHash;
	uint64_t    mSize;
	std::string mPath;
};

class RsGxsImage
{
	public:
	RsGxsImage();
	uint8_t *mData;
	uint32_t mSize;
};

#define GXS_VOTE_DOWN 	0x0001
#define GXS_VOTE_UP	0x0002

class RsGxsVote
{
	public:
	RsGxsVote();
	RsMsgMetaData mMeta;
	uint32_t mVoteType;
};

class RsGxsComment
{
	public:
	RsGxsComment();
	RsMsgMetaData mMeta;
	std::string mComment;

	// below is calculated.
	uint32_t mUpVotes;
	uint32_t mDownVotes;
	double   score; 

	// This is filled in if detailed Comment Data is called.
	std::list<RsGxsVote> votes;
};


class RsGxsCommentService
{
	public:

	RsGxsCommentService() { return; }
virtual ~RsGxsCommentService() { return; }

virtual bool getCommentData(const uint32_t &token, std::vector<RsGxsComment> &comments) = 0;
virtual bool getRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &comments) = 0;

//virtual bool getDetailedCommentData(const uint32_t &token, std::vector<RsGxsComment> &comments);

virtual bool createComment(uint32_t &token, RsGxsComment &comment) = 0;
virtual bool createVote(uint32_t &token, RsGxsVote &vote) = 0;

virtual bool acknowledgeComment(const uint32_t& token, std::pair<RsGxsGroupId, RsGxsMessageId>& msgId) = 0;

};



#endif

