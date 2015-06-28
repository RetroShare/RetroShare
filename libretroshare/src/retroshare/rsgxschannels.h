#ifndef RETROSHARE_GXS_CHANNEL_GUI_INTERFACE_H
#define RETROSHARE_GXS_CHANNEL_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsgxschannel.h
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

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsgxscommon.h"



/* The Main Interface Class - for information about your Peers */
class RsGxsChannels;
extern RsGxsChannels *rsGxsChannels;


// These should be in rsgxscommon.h

class RsGxsChannelGroup
{
	public:
	RsGroupMetaData mMeta;
	std::string mDescription;
	RsGxsImage mImage;

	bool mAutoDownload;
};

class RsGxsChannelPost
{
public:
	RsGxsChannelPost() : mCount(0), mSize(0) {}

public:
	RsMsgMetaData mMeta;
	std::string mMsg;  // UTF8 encoded.

	std::list<RsGxsFile> mFiles;
	uint32_t mCount;   // auto calced.
	uint64_t mSize;    // auto calced.

	RsGxsImage mThumbnail;
};


std::ostream &operator<<(std::ostream &out, const RsGxsChannelGroup &group);
std::ostream &operator<<(std::ostream &out, const RsGxsChannelPost &post);

class RsGxsChannels: public RsGxsIfaceHelper, public RsGxsCommentService
{
	public:

	RsGxsChannels(RsGxsIface *gxs)
	:RsGxsIfaceHelper(gxs)  { return; }
virtual ~RsGxsChannels() { return; }

	/* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsChannelGroup> &groups) = 0;
virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChannelPost> &posts) = 0;

virtual bool getRelatedPosts(const uint32_t &token, std::vector<RsGxsChannelPost> &posts) = 0;

	/* From RsGxsCommentService */
//virtual bool getCommentData(const uint32_t &token, std::vector<RsGxsComment> &comments) = 0;
//virtual bool getRelatedComments(const uint32_t &token, std::vector<RsGxsComment> &comments) = 0;
//virtual bool createComment(uint32_t &token, RsGxsComment &comment) = 0;
//virtual bool createVote(uint32_t &token, RsGxsVote &vote) = 0;

        //////////////////////////////////////////////////////////////////////////////
virtual void setMessageReadStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;

virtual bool setChannelAutoDownload(const RsGxsGroupId &groupId, bool enabled) = 0;
virtual bool getChannelAutoDownload(const RsGxsGroupId &groupid) = 0;
//virtual void setChannelAutoDownload(uint32_t& token, const RsGxsGroupId& groupId, bool autoDownload) = 0;

//virtual bool setMessageStatus(const std::string &msgId, const uint32_t status, const uint32_t statusMask);
//virtual bool setGroupSubscribeFlags(const std::string &groupId, uint32_t subscribeFlags, uint32_t subscribeMask);

//virtual bool groupRestoreKeys(const std::string &groupId);
    virtual bool groupShareKeys(const RsGxsGroupId &groupId, std::set<RsPeerId>& peers)=0;

	// Overloaded subscribe fn.
virtual bool subscribeToGroup(uint32_t &token, const RsGxsGroupId &groupId, bool subscribe) = 0;

virtual bool createGroup(uint32_t &token, RsGxsChannelGroup &group) = 0;
virtual bool createPost(uint32_t &token, RsGxsChannelPost &post) = 0;

virtual bool updateGroup(uint32_t &token, RsGxsChannelGroup &group) = 0;

        // File Interface
virtual bool ExtraFileHash(const std::string &path, std::string filename) = 0;
virtual bool ExtraFileRemove(const RsFileHash &hash) = 0;


};



#endif
