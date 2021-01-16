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
#include "retroshare/rsgxscommon.h"


/* The Main Interface Class - for information about your Peers */
class RsWire;
extern RsWire *rsWire;

class RsWireGroup;
typedef std::shared_ptr<RsWireGroup> RsWireGroupSPtr;
typedef std::shared_ptr<const RsWireGroup> RsWireGroupConstSPtr;

class RsWireGroup: public RsGxsGenericGroupData
{
	public:
	RsWireGroup();

	std::string mTagline;
	std::string mLocation;

	// Images max size should be enforced.
	RsGxsImage  mHeadshot; // max size?
	RsGxsImage  mMasthead; // max size?

	// Unserialised stuff ---------------------

	// These are this groups top-level msgs.
	uint32_t mGroupPulses;
	uint32_t mGroupRepublishes;
	uint32_t mGroupLikes;
	uint32_t mGroupReplies;
	// how do we handle these. TODO
	// uint32_t mGroupFollowing;
	// uint32_t mGroupFollowers;

	// These are this groups REF / RESPONSE msgs from others.
	uint32_t mRefMentions; // TODO how to handle this?
	uint32_t mRefRepublishes;
	uint32_t mRefLikes;
	uint32_t mRefReplies;
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
 * - mPulseMode = WIRE_PULSE_TYPE_RESPONSE | WIRE_PULSE_TYPE_REPLY
 * - Ref fields refer to Parent (InReplyTo) Msg.
 *
 * Reply Reference, is Child Msg of Parent Msg, on Parent Publisher's Group.
 * - mPulseMode = WIRE_PULSE_TYPE_REFERENCE | WIRE_PULSE_TYPE_REPLY
 * - Ref fields refer to Reply Msg.
 * - NB: This Msg requires Parent Msg for complete info, while other two are self-contained.
 *
 * Additionally need to sort out additional relationships.
 * - Mentions.
 * - Followers.
 * - Following.
 ***********************************************************************/

#define  WIRE_PULSE_TYPE_ORIGINAL            (0x0001)
#define  WIRE_PULSE_TYPE_RESPONSE            (0x0002)
#define  WIRE_PULSE_TYPE_REFERENCE           (0x0004)

#define  WIRE_PULSE_RESPONSE_MASK            (0x0f00)
#define  WIRE_PULSE_TYPE_REPLY               (0x0100)
#define  WIRE_PULSE_TYPE_REPUBLISH           (0x0200)
#define  WIRE_PULSE_TYPE_LIKE                (0x0400)

#define  WIRE_PULSE_SENTIMENT_NO_SENTIMENT   (0x0000)
#define  WIRE_PULSE_SENTIMENT_POSITIVE       (0x0001)
#define  WIRE_PULSE_SENTIMENT_NEUTRAL        (0x0002)
#define  WIRE_PULSE_SENTIMENT_NEGATIVE       (0x0003)

class RsWirePulse;

typedef std::shared_ptr<RsWirePulse> RsWirePulseSPtr;
typedef std::shared_ptr<const RsWirePulse> RsWirePulseConstSPtr;

class RsWirePulse
{
	public:

	RsMsgMetaData mMeta;

	// Store actual Pulse here.
	std::string mPulseText;

	uint32_t mPulseType;
	uint32_t mSentiment; // sentiment can be asserted at any point.

	// These Ref to the related (parent or reply) if reply (RESPONSE set)
	// Mode                             RESPONSE          REFERENCE
	RsGxsGroupId   mRefGroupId;    //   PARENT_GrpId      REPLY_GrpId
	std::string    mRefGroupName;  //   PARENT_GrpName    REPLY_GrpName
	RsGxsMessageId mRefOrigMsgId;  //   PARENT_OrigMsgId  REPLY_OrigMsgId
	RsGxsId        mRefAuthorId;   //   PARENT_AuthorId   REPLY_AuthorId
	rstime_t       mRefPublishTs;  //   PARENT_PublishTs  REPLY_PublishTs
	std::string    mRefPulseText;  //   PARENT_PulseText  REPLY_PulseText
	uint32_t       mRefImageCount; //   PARENT_#Images    REPLY_#Images

	// Additional Fields for version 2.
	// Images, need to enforce 20k limit?
	RsGxsImage mImage1;
	RsGxsImage mImage2;
	RsGxsImage mImage3;
	RsGxsImage mImage4;

	// Below Here is not serialised.
	// They are additional fields linking pulses together or parsing elements of msg.

	// functions.
	uint32_t ImageCount();

	// can't have self referencial list, so need to use pointers.
	// using SharedPointers to automatically cleanup.

	// Pointer to WireGroups
	// mRefGroupPtr is opportunistically filled in, but will often be empty.
	RsWireGroupSPtr mRefGroupPtr; //  ORIG: N/A, RESP: Parent, REF: Reply Group
	RsWireGroupSPtr mGroupPtr;    //  ORIG: Own, RESP: Own,    REF: Parent Group

	// These are the direct children of this message
	// split into likes, replies and retweets.
	std::list<RsWirePulseSPtr> mReplies;
	std::list<RsWirePulseSPtr> mLikes;
	std::list<RsWirePulseSPtr> mRepublishes;

	// parsed from msg.
	// do we need references..?
	std::list<std::string> mHashTags;
	std::list<std::string> mMentions;
	std::list<std::string> mUrls;
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

	// Blocking Interfaces.
virtual bool createGroup(RsWireGroup &group) = 0;
virtual bool updateGroup(uint32_t &token, RsWireGroup &group) = 0;
virtual bool getGroups(const std::list<RsGxsGroupId> grpIds,
				std::vector<RsWireGroup> &groups) = 0;

	// New Blocking Interfaces.
	// Plan to migrate all GUI calls to these, and remove old interfaces above.
	// These are not single requests, but return data graphs for display.
virtual bool createOriginalPulse(const RsGxsGroupId &grpId, RsWirePulseSPtr pPulse) = 0;
virtual bool createReplyPulse(RsGxsGroupId grpId, RsGxsMessageId msgId,
				RsGxsGroupId replyWith, uint32_t reply_type,
				RsWirePulseSPtr pPulse) = 0;


	// Provide Individual Group Details for display.
virtual bool getWireGroup(const RsGxsGroupId &groupId, RsWireGroupSPtr &grp) = 0;
virtual bool getWirePulse(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, RsWirePulseSPtr &pPulse) = 0;

	// Provide list of pulses associated with groups.
virtual bool getPulsesForGroups(const std::list<RsGxsGroupId> &groupIds,
				std::list<RsWirePulseSPtr> &pulsePtrs) = 0;

	// Provide pulse, and associated replies / like etc.
virtual bool getPulseFocus(const RsGxsGroupId &groupId, const RsGxsMessageId &msgId,
				int type, RsWirePulseSPtr &pPulse) = 0;

	virtual bool editWire(RsWireGroup& wire) = 0;

};

#endif
