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
 * 1) This represents the base of the Unpacked Msgs, it will be overloaded by all classes
 * that want to use the service. It is difficult to go from gxmp::msg => gxmp::signedmsg
 * so data should be stored in the signedmsg format.
 * 2) All services will fundamentally use data types derived from this.
 * 3) This packet is only serialised once at post time, typically it is deserialised be all nodes.
 */

class gmxp::msg
{
        gxp::id groupId;
        gxp::id msgId;

        gxp::id parentId;  /* for full threading */
        gxp::id threadId;  /* top of thread Id */

        gxp::id origMsgId;  /* if a replacement msg, otherwise == msgId */
        gxp::id replacingMsgId;  /* if a replacement msg, otherwise == NULL (for ordering) */

        uint32_t type;          /* FORUM, CHANNEL, EVENT, COMMENT, VOTE, etc */
        uint32_t flags;         /* Is this needed? */
        uint32_t timestamp;

        gpp::permissions msgPermissions; 

        gxip::signset signatures;
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


