#ifndef RETROSHARE_WIRE_GUI_INTERFACE_H
#define RETROSHARE_WIRE_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rswire.h
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

#include <retroshare/rsidentityVEG.h>

/* The Main Interface Class - for information about your Peers */
class RsWireVEG;
extern RsWireVEG *rsWireVEG;

class RsWireGroupShare
{
	public:

	uint32_t mShareType;
	std::string mShareGroupId;
	std::string mPublishKey;
	uint32_t mCommentMode;
	uint32_t mResizeMode;
};

class RsWireGroup
{
	public:

	RsGroupMetaData mMeta;

	//std::string mGroupId;
	//std::string mName;

	std::string mDescription;
	std::string mCategory;

	std::string mHashTags;

	RsWireGroupShare mShareOptions;
};



/***********************************************************************
 * So pulses operate in the following modes.
 *
 * => Standard, a post to your own group.
 * => @User, gets duplicated on each user's group.
 * => RT, duplicated as child of original post.
 *
 * From Twitter: 
 *  twitter can be: embedded, replied to, favourited, unfavourited, 
 *    retweeted, unretweeted and deleted
 *
 * See: https://dev.twitter.com/docs/platform-objects
 *
 * Format of message: .... 
 *
 *  #HashTags.
 *  @68769381495134  => ID of Sender. 
 *  <http>
 *
 ***********************************************************************/

class RsWirePlace
{
	public:

	

};


class RsWirePulse
{
	public:

	RsMsgMetaData mMeta;

	//std::string mGroupId;
	//std::string mOrigPageId;
	//std::string mPrevId;
	//std::string mPageId;
	//std::string mName;

	std::string mPulse; // all the text is stored here.

	std::string mInReplyPulse;

	uint32_t mPulseFlags;

	std::list<std::string> mMentions;
	std::list<std::string> mHashTags;
	std::list<std::string> mUrls;

	RsWirePlace mPlace;
};





class RsWireVEG: public RsTokenServiceVEG
{
	public:

	RsWireVEG()  { return; }
virtual ~RsWireVEG() { return; }

	/* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, RsWireGroup &group) = 0;
virtual bool getMsgData(const uint32_t &token, RsWirePulse &pulse) = 0;

	/* Create Stuff */
virtual bool createGroup(uint32_t &token, RsWireGroup &group, bool isNew) = 0;
virtual bool createPulse(uint32_t &token, RsWirePulse &pulse, bool isNew) = 0;

};



#endif
