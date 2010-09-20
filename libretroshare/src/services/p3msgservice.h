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

class p3ConnectMgr;

class p3MsgService: public p3Service, public p3Config, public pqiMonitor
{
	public:
	p3MsgService(p3ConnectMgr *cm);

	/* External Interface */
bool    MsgsChanged();		/* should update display */
bool    MsgNotifications();	/* popup - messages */
bool 	getMessageNotifications(std::list<MsgInfoSummary> &noteList);

bool 	getMessageSummaries(std::list<MsgInfoSummary> &msgList);
bool 	getMessage(std::string &mid, MessageInfo &msg);
void    getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox);

bool    removeMsgId(std::string &mid); 
bool    markMsgIdRead(std::string &mid, bool bUnreadByUser);

bool    MessageSend(MessageInfo &info);
bool    MessageToDraft(MessageInfo &info);
bool    MessageToTrash(std::string mid, bool bTrash);

bool 	getMessageTagTypes(MsgTagType& tags);
bool  	setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color);
bool    removeMessageTagType(uint32_t tagId);

bool 	getMessageTag(std::string &msgId, MsgTagInfo& info);
/* set == false && tagId == 0 --> remove all */
bool 	setMessageTag(std::string &msgId, uint32_t tagId, bool set);

bool    resetMessageStandardTagTypes(MsgTagType& tags);

void    loadWelcomeMsg(); /* startup message */

//std::list<RsMsgItem *> &getMsgList();
//std::list<RsMsgItem *> &getMsgOutList();

int	tick();
int	status();

	/*** Overloaded from p3Config ****/
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem*> saveList(bool& cleanup);
virtual bool loadList(std::list<RsItem*> load);
virtual void saveDone();
	/*** Overloaded from p3Config ****/

	/*** Overloaded from pqiMonitor ***/
virtual void    statusChange(const std::list<pqipeer> &plist);
int     checkOutgoingMessages();
	/*** Overloaded from pqiMonitor ***/

	private:

uint32_t getNewUniqueMsgId();
int     sendMessage(RsMsgItem *item);
int 	incomingMsgs();

void 	initRsMI(RsMsgItem *msg, MessageInfo &mi);
void 	initRsMIS(RsMsgItem *msg, MsgInfoSummary &mis);
RsMsgItem *initMIRsMsg(MessageInfo &info, std::string to);

void    initStandardTagTypes();

	p3ConnectMgr *mConnMgr;

	/* Mutex Required for stuff below */

	RsMutex mMsgMtx;

		/* stored list of messages */
	std::map<uint32_t, RsMsgItem *> imsg;
		/* ones that haven't made it out yet! */
	std::map<uint32_t, RsMsgItem *> msgOutgoing; 

		/* List of notifications to post via Toaster */
	std::list<MsgInfoSummary> msgNotifications;

	/* maps for tags types and msg tags */

	std::map<uint32_t, RsMsgTagType*> mTags;
	std::map<uint32_t, RsMsgTags*> mMsgTags;


	Indicator msgChanged;
	uint32_t mMsgUniqueId;

	std::string config_dir;
};

#endif // MESSAGE_SERVICE_HEADER
