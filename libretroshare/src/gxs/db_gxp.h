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

#ifndef RS_GXP_H
#define RS_GXP_H



/************************************************************************
 * This File describes the following components for a generalised exchange protocol (GXP).
 *
 * GPP: General Permissions Protocol: Who is allowed access to what?
 * GDP: General Data Protocol: High Level Interface for exchanging data.
 * GSP: General Storage Protocol: Class implementing GDP for disk access.
 * GNP: General Network Protocol: Class implementing GDP for network exchange.
 *
 * This will be the mechanism to enforce groups.
 * No idea what the data types will look like.
 *
 *****/

/************************************************************************
 * GPP: General Permissions Protocol
 *
 * This will be the mechanism to enforce groups.
 * No idea what the data types will look like.
 * The real challenge here is to ensure that the permissions are universal, 
 * so they can be baked into the messages.
 *
 * PUBLIC:
 *
 * RESTRICTED:
 *
 * PUBLISHER_ONLY_SHARE:
 *
 * GROUP: list<gpgid>
 *
 * GROUP: signed by list<keyid>
 *
 * SAME AS GROUP_DESCRIPTION: (for messages)
 *
 * These permissions will need to be coupled to local permissions...
 * eg. I don't want Party Photos going to Work Collegues.
 *
 * The combination of both permissions will determine who things are shared with.
 *
 *****/

class gpp::group
{


}

class gpp::Permissions
{



};


/************************************************************************
 * GDP: General Data Protocol
 *
 * Generic Data Interface used for both GNP and GSP
 *
 * Permissions are handled at this level... totally generic to all messages.
 *
 * gxmp::signedmsg represents the "original data" container format that will
 * be tranported and stored by GSP / GNP cmodules.
 *****/

class gxp::id
{
	std::string id;
};

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
 * 1) It is possible for the packet to be encrypted, rather than just signed.
 *    so the uncrypted data mustn't give away too much about whats in the msg.
 * 2) This is based on the DistribSignedMsg format.
 * 3) Combined signatures instead of explict publish/personal signs - 
 *    to make it more general. Expect first signature == msgId.
 * 4) Should the permissions be only a group level, or for individual msgs as well?
 *
 */

class gxp::signedmsg
{
	gxp::id groupId;  /* high level groupings, e.g. specific forum, channel, or twitter user, etc. */
	gxp::id msgId;    /* unique msgId within that group, normally signature of the message */

        uint32_t timestamp;
        gpp::permissions grpPermissions; 

        gxp::data packet; /* compressed data */

        gxip::signset signatures;
};


class gdp::interface
{
	/* query for available groups & messages */
	int listgroups(std::list<gdb::id> &grpIds);
	/* response from listmsgs: -1 = invalid parameters, 0 = incomplete list, 1 = all known msgs */
	int listmsgs(const gdp::id grpId, std::list<gdb::id> &msgIds, const time_t from, const time_t to, const int maxmsgs);

	/* response from requestMsg: YES (available immediately), RETRIEVING (known to exist), 
	 * IN_REQUEST (might exist), NOT_AVAILABLE (still might exist)
	 * NB: NOT_AVAILABLE could mean that it will be retrievable later when other peers come online, or possibilly we are not allowed it. 
	 */

	int requestMsg(gdp::id groupId, gdp::id msgId, double &delay); 
	int getMsg(gdp::id groupId, gdp::id msgId, gdp::message &msg);   
	int storeMsg(const gdp::message &msg);

	/* search interface, is it here? or next level up */

};


/************************************************************************
 * GSP: General Storage Protocol
 *
 * This will be the mechanism used to store both GIXP and GMXP data.
 *
 * This will have a decent index system - which is loaded immediately.
 * meaning we know if a message exists or not.
 *
 * We will also implement a cache system, meaning recently accessed data
 * is available immediately.
 *
 *
 * 
 *****/

class gsp::storage: public gdp::interface
{
	/*** gdp::iterface *****/
	int requestMsg(gdp::id groupId, gdp::id msgId, double &delay); 

	int getMsg(gdp::id groupId, gdp::id msgId, gdp::message &msg);   
	int storeMsg(const gdp::message &msg);

	int flagMsg(gxp::id grpId, gxp::id msgId, bool pleaseCache);


	/*** IMPLEMENTATION DETAILS ****/

	loadIndex();
	loadCache();

	bool isInCache(gdp::id grpId, gdp::id msgId);
	bool isInIndex(gdp::id grpId, gdp::id msgId);	
	void fetchToCache(gdp::id grpId, gdp::id msgId);

};



/************************************************************************
 * GNP: General Network Protocol
 *
 * This will be the mechanism share data with peers.
 * It will be designed and closely couple to GSP for effective data managment.
 *
 * This part will effectively interface with librs Network Service.
 * This will also push message ids around - to indicate new messages.
 *
 * This would sit above the GSP, and push things into storage 
 * as they come over the network
 *
 * 
 *****/


class gnp::exchange: public gdp::interface
{
	/*** gdp::iterface *****/
	int requestMsg(gdp::id groupId, gdp::id msgId, double &delay); 

	int getMsg(gdp::id groupId, gdp::id msgId, gdp::message &msg);   
	int storeMsg(const gdp::message &msg);

	/*** IMPLEMENTATION DETAILS ****/

	/* Get/Send Messages */
	getAvailableMsgs(gdp::id grpId, time_t from, time_t to); /* request over the network */
	sendAvailableMsgs(std::string peerId, gdp::id grpId, time_t from, time_t to); /* send to peers */

	requestMessages(std::string peerId, gdp::id grpId, std::list<gdp::id> msgIds); 	
	sendMessages(std::string peerId, gdp::id grpId, std::list<gdp::id> msgIds);	  /* send to peer, obviously permissions have been checked first */

	/* Search the network */

};


/************************************************************************
 * GIXP: General Identity Exchange Protocol.
 *
 * As we're always running into troubles with GPG signatures... we are going to 
 * create a layer of RSA Keys for the following properties:
 *
 * 1) RSA Keys can be Anonymous, Self-Signed with Pseudonym, Signed by GPG Key.
 *	- Anonymous & Pseudonym Keys will be shared network-wide (Hop by Hop).
	- GPG signed Keys will only be shared if we can validate the signature 
		(providing similar behaviour to existing GPG Keys).
	- GPG signed Keys can optionally be marked for Network-wide sharing.
 * 2) These keys can be used anywhere, specifically in the protocols described below.
 * 3) These keys can be used to sign, encrypt, verify & decrypt
 * 4) Keys will never need to be directly accessed - stored in this class.
 * 5) They will be cached locally and exchanged p2p, by pull request.
 * 6) This class will use the generalised packet storage for efficient caching & loading.
 * 7) Data will be stored encrypted.
 *****/

class gixp::key
{
	gxip::keyref mKeyId;

	PubKey *pubKey;
	PrivateKey *privKey; /* NULL if non-existant */
};

class gixp::profile
{
	public:

	gxip::keyref mKeyId;

	std::string mPseudonym;
	time_t      mTimestamp;   /* superseded by newer timestamps */
	uint32_t    mProfileType; /* ANONYMOUS (no name, self-signed), PSEUDONYM (self-signed), GPG (name=gpgname, gpgsigned), REVOCATION?? */ 
	gpp::permissions mPermissions;

	gxip::signature mSignature;
};

class  gxip::keyref
{
	std::string keyId;
}

class gxip::keyrefset
{
	std::list<gxip::keyref> keyIds;
}

class gxip::signature
{
	gxip::keyref signer;
	std::string signature;
}

class gxip::signset
{
	std::list<gxip::signature> signs;
};

/*******
 * NOTES:
 * 1) much of this is already implemented in libretroshare/src/distrib/distribsecurity.cc
 * 2) Data types will need to be shoehorned into gxmp::signedmsg format for transport.
 * 3) Likewise this class will sit above a gdp/gnp/gsp data handling.
 */

class p3gixp
{
	bool createKey(gixp::profile); /* fills in mKeyId, and signature */

	bool haveKey(keyId);
	bool havePrivateKey(keyId);
	bool requestKey(keyId);

	gixp::profile getProfile(keyId);

	/*** process data ***/
	bool sign(KeyId, Data, signature); 	
	bool verify(KeyId, Data, signature);
	bool decrypt(KeyId, Data, decryptedData);
	bool encrypt(KeyId, Data, decryptedData);

};


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

class gxmp::msg
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


#endif /* RS_GXP_H */


