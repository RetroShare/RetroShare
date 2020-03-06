/*******************************************************************************
 * libretroshare/src/retroshare: rsturtle.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2020 by Robert Fernie <retroshare@lunamutt.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RETROSHARE_WIRE_GUI_INTERFACE_H
#define RETROSHARE_WIRE_GUI_INTERFACE_H

#include <inttypes.h>
#include <string>
#include <list>

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"


/* The Main Interface Class - for information about your Peers */
class RsWire;
extern RsWire *rsWire;

class RsWireGroup
{
	public:

	RsGroupMetaData mMeta;
	std::string mDescription;
};



/***********************************************************************
 * RsWire - is intended to be a Twitter clone - but fully decentralised.
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

/************************************************************************
 * Pulse comes in three flavours.
 *
 *
 * Original Msg Pulse
 * - Spontaneous msg, on your own group.
 * - mPulseType = WIRE_PULSE_TYPE_ORIGINAL_MSG
 * - Ref fields are empty.
 *
 * Reply to a Pulse (i.e Retweet), has two parts.
 * as we want the retweet to reference the original, and the original to know about reply.
 * This info will be duplicated in two msgs - but allow data to spread easier.
 *
 * Reply Msg Pulse, will be Top-Level Msg on Publisher's Group.
 * - mPulseMode = WIRE_PULSE_TYPE_REPLY_MSG
 * - Ref fields refer to Parent (InReplyTo) Msg.
 *
 * Reply Reference, is Child Msg of Parent Msg, on Parent Publisher's Group.
 * - mPulseMode = WIRE_PULSE_TYPE_REPLY_REFERENCE
 * - Ref fields refer to Reply Msg.
 * - NB: This Msg requires Parent Msg for complete info, while other two are self-contained.
 ***********************************************************************/

#define  WIRE_PULSE_TYPE_ORIGINAL_MSG        (0x0001)
#define  WIRE_PULSE_TYPE_REPLY_MSG           (0x0002)
#define  WIRE_PULSE_TYPE_REPLY_REFERENCE     (0x0004)

#define  WIRE_PULSE_TYPE_SENTIMENT_POSITIVE  (0x0010)
#define  WIRE_PULSE_TYPE_SENTIMENT_NEUTRAL   (0x0020)
#define  WIRE_PULSE_TYPE_SENTIMENT_NEGATIVE  (0x0040)

class RsWirePulse
{
	public:

	RsMsgMetaData mMeta;

	// Store actual Pulse here.
	std::string mPulseText;

	uint32_t mPulseType;

	// These Ref to the related (parent or reply) if reply (MODE_REPLY_MSG set)
	// Mode                            REPLY_MSG only    REPLY_REFERENCE
	RsGxsGroupId   mRefGroupId;   //   PARENT_GrpId      REPLY_GrpId
	std::string    mRefGroupName; //   PARENT_GrpName    REPLY_GrpName
	RsGxsMessageId mRefOrigMsgId; //   PARENT_OrigMsgId  REPLY_OrigMsgId
	RsGxsId        mRefAuthorId;  //   PARENT_AuthorId   REPLY_AuthorId
	rstime_t       mRefPublishTs; //   PARENT_PublishTs  REPLY_PublishTs
	std::string    mRefPulseText; //   PARENT_PulseText  REPLY_PulseText

	// Open Question. Do we want these additional fields?
	// These can potentially be added at some point.
	//	std::list<std::string> mMentions;
	//	std::list<std::string> mHashTags;
	//	std::list<std::string> mUrls;
	//	RsWirePlace mPlace;
};


std::ostream &operator<<(std::ostream &out, const RsWireGroup &group);
std::ostream &operator<<(std::ostream &out, const RsWirePulse &pulse);


class RsWire: public RsGxsIfaceHelper
{
	public:

	explicit RsWire(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}
	virtual ~RsWire() {}

	/*!
	 * To acquire a handle to token service handler
	 * needed to make requests to the service
	 * @return handle to token service for this gxs service
	 */
	virtual RsTokenService* getTokenService() = 0;

	/* Specific Service Data */
virtual bool getGroupData(const uint32_t &token, std::vector<RsWireGroup> &groups) = 0;
virtual bool getPulseData(const uint32_t &token, std::vector<RsWirePulse> &pulses) = 0;

virtual bool createGroup(uint32_t &token, RsWireGroup &group) = 0;
virtual bool createPulse(uint32_t &token, RsWirePulse &pulse) = 0;

};

#endif
