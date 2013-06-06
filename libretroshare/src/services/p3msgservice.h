/*
 * libretroshare/src/services msgservice.h
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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


#ifndef MESSAGE_SERVICE_HEADER
#define MESSAGE_SERVICE_HEADER

#include <list>
#include <map>
#include <iostream>

#include "retroshare/rsmsgs.h"

#include "pqi/pqi.h"
#include "pqi/pqiindic.h"

#include "pqi/pqimonitor.h"
#include "pqi/p3cfgmgr.h"

#include "services/p3service.h"
#include "serialiser/rsmsgitems.h"
#include "util/rsthreads.h"
#include "turtle/p3turtle.h"
#include "turtle/turtleclientservice.h"

class p3LinkMgr;

class p3MsgService: public p3Service, public p3Config, public pqiMonitor, public RsTurtleClientService
{
	public:
	p3MsgService(p3LinkMgr *lm);

	/* External Interface */
bool 	getMessageSummaries(std::list<MsgInfoSummary> &msgList);
bool 	getMessage(const std::string &mid, MessageInfo &msg);
void    getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox);

bool decryptMessage(const std::string& mid) ;
bool    removeMsgId(const std::string &mid); 
bool    markMsgIdRead(const std::string &mid, bool bUnreadByUser);
bool    setMsgFlag(const std::string &mid, uint32_t flag, uint32_t mask);
bool    getMsgParentId(const std::string &msgId, std::string &msgParentId);
// msgParentId == 0 --> remove
bool    setMsgParentId(uint32_t msgId, uint32_t msgParentId);

bool    MessageSend(MessageInfo &info);
bool    SystemMessage(const std::wstring &title, const std::wstring &message, uint32_t systemFlag);
bool    MessageToDraft(MessageInfo &info, const std::string &msgParentId);
bool    MessageToTrash(const std::string &mid, bool bTrash);

bool 	getMessageTagTypes(MsgTagType& tags);
bool  	setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color);
bool    removeMessageTagType(uint32_t tagId);

bool 	getMessageTag(const std::string &msgId, MsgTagInfo& info);
/* set == false && tagId == 0 --> remove all */
bool 	setMessageTag(const std::string &msgId, uint32_t tagId, bool set);

bool    resetMessageStandardTagTypes(MsgTagType& tags);

void    loadWelcomeMsg(); /* startup message */

//std::list<RsMsgItem *> &getMsgList();
//std::list<RsMsgItem *> &getMsgOutList();

int	tick();
int	status();

	/*** Overloaded from p3Config ****/
virtual RsSerialiser *setupSerialiser();
virtual bool saveList(bool& cleanup, std::list<RsItem*>&);
virtual bool loadList(std::list<RsItem*>& load);
virtual void saveDone();
	/*** Overloaded from p3Config ****/

	/*** Overloaded from pqiMonitor ***/
virtual void    statusChange(const std::list<pqipeer> &plist);
int     checkOutgoingMessages();
	/*** Overloaded from pqiMonitor ***/

	/*** overloaded from p3turtle   ***/

		void connectToTurtleRouter(p3turtle *) ;

		struct DistantMessengingInvite
		{
			time_t time_of_validity ;
		};
		struct DistantMessengingContact
		{
			time_t last_hit_time ;
			std::string virtual_peer_id ;
			uint32_t status ;
			std::vector<RsMsgItem*> pending_messages ;
		};

		bool createDistantOfflineMessengingInvite(time_t time_of_validity,TurtleFileHash& hash) ;
		bool getDistantOfflineMessengingInvites(std::vector<DistantOfflineMessengingInvite>& invites) ;
		void sendPrivateMsgItem(RsMsgItem *) ;

	private:
		// This maps contains the current invitations to respond to.
		//
		std::map<std::string,DistantMessengingInvite> _messenging_invites ;

		// This contains the ongoing tunnel handling contacts.
		//
		std::map<std::string,DistantMessengingContact> _messenging_contacts ;

		// Overloaded from RsTurtleClientService

		virtual bool handleTunnelRequest(const std::string& hash,const std::string& peer_id) ;
		virtual void receiveTurtleData(RsTurtleGenericTunnelItem *item,const std::string& hash,const std::string& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction) ;
		void addVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&,RsTurtleGenericTunnelItem::Direction dir) ;
		void removeVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&) ;

		// Utility functions

		bool encryptMessage(const std::string& pgp_id,RsMsgItem *msg) ;

		void manageDistantPeers() ;
		void sendTurtleData(const std::string& hash,RsMsgItem *) ;
		void handleIncomingItem(RsMsgItem *) ;

		p3turtle *mTurtle ;

uint32_t getNewUniqueMsgId();
int     sendMessage(RsMsgItem *item);
void    checkSizeAndSendMessage(RsMsgItem *msg);

int 	incomingMsgs();
void    processMsg(RsMsgItem *mi, bool incoming);
bool checkAndRebuildPartialMessage(RsMsgItem*) ;

void 	initRsMI(RsMsgItem *msg, MessageInfo &mi);
void 	initRsMIS(RsMsgItem *msg, MsgInfoSummary &mis);
RsMsgItem *initMIRsMsg(MessageInfo &info, const std::string &to);

void    initStandardTagTypes();

	p3LinkMgr *mLinkMgr;

	/* Mutex Required for stuff below */

	RsMutex mMsgMtx;
	RsMsgSerialiser *_serialiser ;

		/* stored list of messages */
	std::map<uint32_t, RsMsgItem *> imsg;
		/* ones that haven't made it out yet! */
	std::map<uint32_t, RsMsgItem *> msgOutgoing; 

	std::map<std::string, RsMsgItem *> _pendingPartialMessages ;

	/* maps for tags types and msg tags */

	std::map<uint32_t, RsMsgTagType*> mTags;
	std::map<uint32_t, RsMsgTags*> mMsgTags;

	uint32_t mMsgUniqueId;

	// used delete msgSrcIds after config save
	std::map<uint32_t, RsMsgSrcId*> mSrcIds;

	// save the parent of the messages in draft for replied and forwarded
	std::map<uint32_t, RsMsgParentId*> mParentId;

	std::string config_dir;
};

#endif // MESSAGE_SERVICE_HEADER
