/*
 * libretroshare/src/gxp: gxp.h
 *
 * General Exchange Protocol interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#ifndef RS_GMXP_H
#define RS_GMXP_H


/************************************************************************
 * GMXP: General Message Exchange Protocol.
 *
 * The existing experiences of Forums & Channels have highlighted some
 * significant limitations of the Cache-Based exchange system. Based
 * on this initial understandings - an improved transport system will
 * be design to provide a generalised message exchange foundation, 
 * which can be used to build new services... 
 *
 *
 * Key Properties:
 *
 * 1) Message independent: Should be able to be used for Forums, Channels
 *   Twitter, Photos, Task Tracking, Link Cloud, etc.
 * 2) Easy to Use: Specify(Msg, Permissions, KeyId) only.
 * 3) Efficient Network Transport. (in common with GIXP)
 * 4) Efficient Cache System (in common with GIXP).
 * 5) Uses Groups Feature (in common with GIXP).
 * 6) Search Protocols. ( might need overloading at higher level).
 *
 *****/


/******
 * NOTES:
 * 1) Based on Forum/Channel Groups.
 * 2) Will likely need to extend to handle info for other services.
 * 3) Perhaps a more generalised class like gxmp::msg would be best with extra data.
 *
 */

class gxmp::group
{
        gxp::id grpId;
        uint32_t grpType;       /* FORUM, CHANNEL, TWITTER, etc */

        uint32_t     timestamp;
        uint32_t     grpFlags;
        std::string  grpName;
        std::string  grpDesc;
        std::string  grpCategory;

        RsTlvImage   grpPixmap;

        gpp::permissions grpPermissions;

        gxip::keyref     adminKey;
        gxip::keyrefset  publishKeys;

        gxip::signature adminSignature;
};


/******
 * NOTES:
 * 1) This represents the base of the Unpacked Msgs, it will be overloaded by all classes
 * that want to use the service. It is difficult to go from gxmp::msg => gxmp::signedmsg
 * so data should be stored in the signedmsg format.
 * 2) All services will fundamentally use data types derived from this.
 * 3) This packet is only serialised once at post time, typically it is deserialised by all nodes.
 */
class gmxp::link
{
	uint32_t linktype;
	gxp::id  msgId;
}

class gmxp::msg
{
        gxp::id groupId;
        gxp::id msgId;

        gxp::id parentId;  /* for full threading */
        gxp::id threadId;  /* top of thread Id */

        gxp::id origMsgId;  /* if a replacement msg, otherwise == msgId */
        gxp::id replacingMsgId;  /* if a replacement msg, otherwise == NULL (for ordering) */

        uint32_t msgtype;          /* FORUM, CHANNEL, EVENT, COMMENT, VOTE, etc */
        uint32_t flags;         /* Is this needed? */
        uint32_t timestamp;

	// New extensions - put these in the generic message, so we can handle search / linking for all services.
	std::list<std::string> hashtags;
	std::list<gmxp::link>  linked;

        gpp::permissions msgPermissions; 

        gxip::signset signatures; // should this be a set, or a singleton?
};

class gmxp::group: public gxp::group
{
	???
};


/******
 * NOTES:
 * 1) This class will be based on p3distrib - which does most of this stuff already!
 * 2) There are lots of virtual functions - which can be overloaded to customise behaviour.
 *    for clarity - these have not been shown here.
 *
 * 3) We need to extend this class to add search functionality... so you can discover new
 *     stuff from your friends. This will need to be an overloaded functionality as the 
 *     search will be service specific.
 */


/* General Interface class which is extended by specific versions.
 *
 * This provides most of the generic search, and access functions.
 * As we are going to end up with diamond style double inheritance.
 * This function needs to be pure virtual.. so there is no disambiugation issues.
 */

class rsGmxp
{
	/* create content */
       std::string createGroup(std::wstring name, std::wstring desc, uint32_t flags, unsigned char *pngImageData, uint32_t imageSize);
       std::string publishMsg(RsDistribMsg *msg, bool personalSign);

	/* indicate interest in info */
       bool    subscribeToGroup(const std::string &grpId, bool subscribe);

	/* search messages (TO DEFINE) */

	/* extract messages (From p3Distrib Existing Methods) */

	bool    getAllGroupList(std::list<std::string> &grpids);
        bool    getSubscribedGroupList(std::list<std::string> &grpids);
        bool    getPublishGroupList(std::list<std::string> &grpids);
        void    getPopularGroupList(uint32_t popMin, uint32_t popMax, std::list<std::string> &grpids);

        bool    getAllMsgList(const std::string& grpId, std::list<std::string> &msgIds);
        bool    getParentMsgList(const std::string& grpId, 
				const std::string& pId, std::list<std::string> &msgIds);
        bool    getTimePeriodMsgList(const std::string& grpId, uint32_t timeMin,
                                uint32_t timeMax, std::list<std::string> &msgIds);

        GroupInfo *locked_getGroupInfo(const std::string& grpId);
        RsDistribMsg *locked_getGroupMsg(const std::string& grpId, const std::string& msgId);

        void getGrpListPubKeyAvailable(std::list<std::string>& grpList);

};


class p3gmxp
{
	p3gmxp(int serviceType, serialiser *);

	/* create content */
       std::string createGroup(std::wstring name, std::wstring desc, uint32_t flags, unsigned char *pngImageData, uint32_t imageSize);
       std::string publishMsg(RsDistribMsg *msg, bool personalSign);

	/* indicate interest in info */
       bool    subscribeToGroup(const std::string &grpId, bool subscribe);

	/* search messages (TO DEFINE) */

	/* extract messages (From p3Distrib Existing Methods) */

	bool    getAllGroupList(std::list<std::string> &grpids);
        bool    getSubscribedGroupList(std::list<std::string> &grpids);
        bool    getPublishGroupList(std::list<std::string> &grpids);
        void    getPopularGroupList(uint32_t popMin, uint32_t popMax, std::list<std::string> &grpids);

        bool    getAllMsgList(const std::string& grpId, std::list<std::string> &msgIds);
        bool    getParentMsgList(const std::string& grpId, 
				const std::string& pId, std::list<std::string> &msgIds);
        bool    getTimePeriodMsgList(const std::string& grpId, uint32_t timeMin,
                                uint32_t timeMax, std::list<std::string> &msgIds);

        GroupInfo *locked_getGroupInfo(const std::string& grpId);
        RsDistribMsg *locked_getGroupMsg(const std::string& grpId, const std::string& msgId);

        void getGrpListPubKeyAvailable(std::list<std::string>& grpList);

};


#endif /* RS_GMXP_H */


