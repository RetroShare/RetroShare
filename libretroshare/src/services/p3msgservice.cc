/*
 * libretroshare/src/services msgservice.cc
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


#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsidentity.h"

#include "pqi/pqibin.h"
#include "pqi/pqiarchive.h"
#include "pqi/p3linkmgr.h"
#include "pqi/authgpg.h"
#include "pqi/p3cfgmgr.h"

#include "gxs/gxssecurity.h"

#include "services/p3idservice.h"
#include "services/p3msgservice.h"

#include "pgp/pgpkeyutil.h"
#include "rsserver/p3face.h"
#include "serialiser/rsconfigitems.h"

#include "grouter/p3grouter.h"
#include "grouter/groutertypes.h"

#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "util/rsstring.h"
#include "util/radix64.h"
#include "util/rsrandom.h"
#include "util/rsprint.h"

#include <unistd.h>
#include <iomanip>
#include <map>
#include <sstream>

//#define MSG_DEBUG 1
//#define DEBUG_DISTANT_MSG
//#define DISABLE_DISTANT_MESSAGES 
//#define DEBUG_DISTANT_MSG

const int msgservicezone = 54319;

static const uint8_t DISTANT_MSG_TAG_IDENTITY        = 0x10 ;
static const uint8_t DISTANT_MSG_TAG_CLEAR_MSG       = 0x20 ;
static const uint8_t DISTANT_MSG_TAG_ENCRYPTED_MSG   = 0x21 ;
static const uint8_t DISTANT_MSG_TAG_SIGNATURE       = 0x30 ;

static const uint8_t DISTANT_MSG_PROTOCOL_VERSION_01 = 0x47 ;
static const uint8_t DISTANT_MSG_PROTOCOL_VERSION_02 = 0x48 ;

/* Another little hack ..... unique message Ids
 * will be handled in this class.....
 * These are unique within this run of the server, 
 * and are not stored long term....
 *
 * Only 3 entry points:
 * (1) from network....
 * (2) from local send
 * (3) from storage...
 */


p3MsgService::p3MsgService(p3ServiceControl *sc, p3IdService *id_serv)
	:p3Service(), p3Config(), mIdService(id_serv), mServiceCtrl(sc), mMsgMtx("p3MsgService"), mMsgUniqueId(time(NULL))
{
	_serialiser = new RsMsgSerialiser();
	addSerialType(_serialiser);

	mDistantMessagingEnabled = true ;

	/* Initialize standard tag types */
	if(sc)
		initStandardTagTypes();

}

const std::string MSG_APP_NAME = "msg";
const uint16_t MSG_APP_MAJOR_VERSION	= 	1;
const uint16_t MSG_APP_MINOR_VERSION  = 	0;
const uint16_t MSG_MIN_MAJOR_VERSION  = 	1;
const uint16_t MSG_MIN_MINOR_VERSION	=	0;

RsServiceInfo p3MsgService::getServiceInfo()
{
	return RsServiceInfo(RS_SERVICE_TYPE_MSG, 
		MSG_APP_NAME,
		MSG_APP_MAJOR_VERSION, 
		MSG_APP_MINOR_VERSION, 
		MSG_MIN_MAJOR_VERSION, 
		MSG_MIN_MINOR_VERSION);
}


uint32_t p3MsgService::getNewUniqueMsgId()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
	return mMsgUniqueId++;
}

int	p3MsgService::tick()
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::tick()");

	/* don't worry about increasing tick rate! 
	 * (handled by p3service)
	 */

	incomingMsgs(); 

	static time_t last_management_time = 0 ;
	time_t now = time(NULL) ;

	if(now > last_management_time + 5)
	{
		manageDistantPeers() ;
		checkOutgoingMessages(); 

		last_management_time = now ;
	}

	return 0;
}


int	p3MsgService::status()
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::status()");

	return 1;
}

void p3MsgService::processMsg(RsMsgItem *mi, bool incoming)
{
	mi -> recvTime = time(NULL);
	mi -> msgId = getNewUniqueMsgId();

	{
		RsStackMutex stack(mMsgMtx); /*** STACK LOCKED MTX ***/

		if (incoming)
		{
			/* from a peer */

			mi->msgFlags &= (RS_MSG_FLAGS_ENCRYPTED | RS_MSG_FLAGS_SYSTEM); // remove flags except those
			mi->msgFlags |= RS_MSG_FLAGS_NEW;

			p3Notify *notify = RsServer::notify();
			if (notify)
			{
				if(mi->msgFlags & RS_MSG_FLAGS_ENCRYPTED)
                    notify->AddPopupMessage(RS_POPUP_ENCRYPTED_MSG, mi->PeerId().toStdString(), mi->subject, mi->message);
				else
                    notify->AddPopupMessage(RS_POPUP_MSG, mi->PeerId().toStdString(), mi->subject, mi->message);

				std::string out;
				rs_sprintf(out, "%lu", mi->msgId);
				notify->AddFeedItem(RS_FEED_ITEM_MESSAGE, out, "", "");
			}
		}
		else
		{
			mi->msgFlags |= RS_MSG_OUTGOING;
		}

		imsg[mi->msgId] = mi;
		RsMsgSrcId* msi = new RsMsgSrcId();
		msi->msgId = mi->msgId;
		msi->srcId = mi->PeerId();
		mSrcIds.insert(std::pair<uint32_t, RsMsgSrcId*>(msi->msgId, msi));
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}

	RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_ADD);

	/**** STACK UNLOCKED ***/
}
bool p3MsgService::checkAndRebuildPartialMessage(RsMsgItem *ci)
{
	// Check is the item is ending an incomplete item.
	//
    std::map<RsPeerId,RsMsgItem*>::iterator it = _pendingPartialMessages.find(ci->PeerId()) ;

	bool ci_is_partial = ci->msgFlags & RS_MSG_FLAGS_PARTIAL ;

	if(it != _pendingPartialMessages.end())
	{
#ifdef MSG_DEBUG
		std::cerr << "Pending message found. Appending it." << std::endl;
#endif
		// Yes, there is. Append the item to ci.

		ci->message = it->second->message + ci->message ;
		ci->msgFlags |= it->second->msgFlags ;

		delete it->second ;

		if(!ci_is_partial)
			_pendingPartialMessages.erase(it) ;
	}

	if(ci_is_partial)
	{
#ifdef MSG_DEBUG
		std::cerr << "Message is partial, storing for later." << std::endl;
#endif
		// The item is a partial message. Push it, and wait for the rest.
		//
		_pendingPartialMessages[ci->PeerId()] = ci ;
		return false ;
	}
	else
	{
#ifdef MSG_DEBUG
		std::cerr << "Message is complete, using it now." << std::endl;
#endif
		return true ;
	}
}

int p3MsgService::incomingMsgs()
{
	RsMsgItem *mi;
	int i = 0;

	while((mi = (RsMsgItem *) recvItem()) != NULL)
	{
		handleIncomingItem(mi) ;
		++i ;
	}

	return i;
}

void p3MsgService::handleIncomingItem(RsMsgItem *mi)
{
	bool changed = false ;

	if(checkAndRebuildPartialMessage(mi))	// only returns true when a msg is complete.
	{
		processMsg(mi, true);
		changed = true ;
	}
	if(changed)
		RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
}

void    p3MsgService::statusChange(const std::list<pqiServicePeer> &plist)
{
	/* should do it properly! */
	/* only do this when a new peer is connected */
	bool newPeers = false;
	std::list<pqiServicePeer>::const_iterator it;
	for(it = plist.begin(); it != plist.end(); it++)
	{
		if (it->actions & RS_SERVICE_PEER_CONNECTED)
		{
			newPeers = true;
		}
	}

	if (newPeers)
	{
		checkOutgoingMessages();
	}
}

void p3MsgService::checkSizeAndSendMessage(RsMsgItem *msg)
{
	// We check the message item, and possibly split it into multiple messages, if the message is too big.

	static const uint32_t MAX_STRING_SIZE = 15000 ;

	std::cerr << "Msg is size " << msg->message.size() << std::endl;

	while(msg->message.size() > MAX_STRING_SIZE)
	{
		// chop off the first 15000 wchars

		RsMsgItem *item = new RsMsgItem(*msg) ;

		item->message = item->message.substr(0,MAX_STRING_SIZE) ;
		msg->message = msg->message.substr(MAX_STRING_SIZE,msg->message.size()-MAX_STRING_SIZE) ;

#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  Chopped off msg of size " << item->message.size() << std::endl;
#endif

		// Indicate that the message is to be continued.
		//
		item->msgFlags |= RS_MSG_FLAGS_PARTIAL ;

		if(msg->msgFlags & RS_MSG_FLAGS_DISTANT)
            sendPrivateMsgItem(item) ;
		else
			sendItem(item) ;
	}
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Chopped off msg of size " << msg->message.size() << std::endl;
#endif

	if(msg->msgFlags & RS_MSG_FLAGS_DISTANT)
        sendPrivateMsgItem(msg) ;
	else
		sendItem(msg) ;
}

int     p3MsgService::checkOutgoingMessages()
{
	/* iterate through the outgoing queue 
	 *
	 * if online, send
	 */

	bool changed = false ;
	std::list<RsMsgItem*> output_queue ;

	{
        const RsPeerId& ownId = mServiceCtrl->getOwnId();

		std::list<uint32_t>::iterator it;
		std::list<uint32_t> toErase;
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		std::map<uint32_t, RsMsgItem *>::iterator mit;
		for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
		{
			if (mit->second->msgFlags & RS_MSG_FLAGS_TRASH) 
				continue;

			/* find the certificate */
			RsPeerId pid = mit->second->PeerId();

			if( pid == ownId 
					|| ( (mit->second->msgFlags & RS_MSG_FLAGS_DISTANT) && (!(mit->second->msgFlags & RS_MSG_FLAGS_ROUTED)))
					|| mServiceCtrl->isPeerConnected(getServiceInfo().mServiceType, pid) ) /* FEEDBACK Msg to Ourselves */
			{
				/* send msg */
				pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
					"p3MsgService::checkOutGoingMessages() Sending out message");
				/* remove the pending flag */

				output_queue.push_back(mit->second) ;

				// When the message is a distant msg, dont remove it yet from the list. Only mark it as being sent, so that we don't send it again.
				//
				if(!(mit->second->msgFlags & RS_MSG_FLAGS_DISTANT))
				{
					(mit->second)->msgFlags &= ~RS_MSG_FLAGS_PENDING;
					toErase.push_back(mit->first);
					changed = true ;
				}
				else
				{
#ifdef DEBUG_DISTANT_MSG
					std::cerr << "Message id " << mit->first << " is distant: kept in outgoing, and marked as ROUTED" << std::endl;
#endif
					mit->second->msgFlags |= RS_MSG_FLAGS_ROUTED ;
				}
			}
			else
			{
				pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
					"p3MsgService::checkOutGoingMessages() Delaying until available...");
			}
		}

		/* clean up */
		for(it = toErase.begin(); it != toErase.end(); it++)
		{
			mit = msgOutgoing.find(*it);
			if (mit != msgOutgoing.end())
			{
				msgOutgoing.erase(mit);
			}

			std::map<uint32_t, RsMsgSrcId*>::iterator srcIt = mSrcIds.find(*it);
			if (srcIt != mSrcIds.end()) {
				delete (srcIt->second);
				mSrcIds.erase(srcIt);
			}
		}

		if (toErase.size() > 0)
		{
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
	}

	for(std::list<RsMsgItem*>::const_iterator it(output_queue.begin());it!=output_queue.end();++it)
		checkSizeAndSendMessage(*it) ;

	if(changed)
		RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);

	return 0;
}

bool    p3MsgService::saveList(bool& cleanup, std::list<RsItem*>& itemList)
{

	std::map<uint32_t, RsMsgItem *>::iterator mit;
	std::map<uint32_t, RsMsgTagType* >::iterator mit2;
	std::map<uint32_t, RsMsgTags* >::iterator mit3;
	std::map<uint32_t, RsMsgSrcId* >::iterator lit;
	std::map<uint32_t, RsMsgParentId* >::iterator mit4;

    MsgTagType stdTags;

	cleanup = false;

	mMsgMtx.lock();

	for(mit = imsg.begin(); mit != imsg.end(); mit++)
		itemList.push_back(mit->second);

	for(lit = mSrcIds.begin(); lit != mSrcIds.end(); lit++)
		itemList.push_back(lit->second);


	for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
		itemList.push_back(mit->second) ;

	for(mit2 = mTags.begin();  mit2 != mTags.end(); mit2++)
		itemList.push_back(mit2->second);

	for(mit3 = mMsgTags.begin();  mit3 != mMsgTags.end(); mit3++)
		itemList.push_back(mit3->second);

	for(mit4 = mParentId.begin();  mit4 != mParentId.end(); mit4++)
		itemList.push_back(mit4->second);

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;
	kv.key = "DISTANT_MESSAGES_ENABLED" ;
	kv.value = mDistantMessagingEnabled?"YES":"NO" ;
	vitem->tlvkvs.pairs.push_back(kv) ;

	itemList.push_back(vitem) ;

	return true;
}

void p3MsgService::saveDone()
{
	// unlocks mutex which has been locked by savelist
	mMsgMtx.unlock();
}

RsSerialiser* p3MsgService::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;

	rss->addSerialType(new RsMsgSerialiser(true));
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss;
}

// build list of standard tag types
static void getStandardTagTypes(MsgTagType &tags)
{
	/* create standard tag types, the text must be translated in the GUI */
	tags.types [RS_MSGTAGTYPE_IMPORTANT] = std::pair<std::string, uint32_t> ("Important", 0xFF0000);
	tags.types [RS_MSGTAGTYPE_WORK]      = std::pair<std::string, uint32_t> ("Work",      0xFF9900);
	tags.types [RS_MSGTAGTYPE_PERSONAL]  = std::pair<std::string, uint32_t> ("Personal",  0x009900);
	tags.types [RS_MSGTAGTYPE_TODO]      = std::pair<std::string, uint32_t> ("Todo",      0x3333FF);
	tags.types [RS_MSGTAGTYPE_LATER]     = std::pair<std::string, uint32_t> ("Later",     0x993399);
}

// Initialize the standard tag types after load
void p3MsgService::initStandardTagTypes()
{
	bool bChanged = false;
    const RsPeerId& ownId = mServiceCtrl->getOwnId();

	MsgTagType tags;
	getStandardTagTypes(tags);

	std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator tit;
	for (tit = tags.types.begin(); tit != tags.types.end(); tit++) {
		std::map<uint32_t, RsMsgTagType*>::iterator mit = mTags.find(tit->first);
		if (mit == mTags.end()) {
			RsMsgTagType* tagType = new RsMsgTagType();
            tagType->PeerId (ownId);
			tagType->tagId = tit->first;
			tagType->text = tit->second.first;
			tagType->rgb_color = tit->second.second;

			mTags.insert(std::pair<uint32_t, RsMsgTagType*>(tit->first, tagType));

			bChanged = true;
		}
	}

	if (bChanged) {
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}
}

bool    p3MsgService::loadList(std::list<RsItem*>& load)
{
    RsMsgItem *mitem;
    RsMsgTagType* mtt;
    RsMsgTags* mti;
    RsMsgSrcId* msi;
    RsMsgParentId* msp;
//    RsPublicMsgInviteConfigItem* msv;

    std::list<RsMsgItem*> items;
    std::list<RsItem*>::iterator it;
    std::map<uint32_t, RsMsgTagType*>::iterator tagIt;
    std::map<uint32_t, RsPeerId> srcIdMsgMap;
    std::map<uint32_t, RsPeerId>::iterator srcIt;

	bool distant_messaging_set = false ;

	// load items and calculate next unique msgId
	for(it = load.begin(); it != load.end(); it++)
	{

		if (NULL != (mitem = dynamic_cast<RsMsgItem *>(*it)))
		{
			/* STORE MsgID */
			if (mitem->msgId >= mMsgUniqueId) {
				mMsgUniqueId = mitem->msgId + 1;
			}
			items.push_back(mitem);
		}
		else if(NULL != (mtt = dynamic_cast<RsMsgTagType *>(*it)))
		{
			// delete standard tags as they are now save in config
			if(mTags.end() == (tagIt = mTags.find(mtt->tagId)))
			{
				mTags.insert(std::pair<uint32_t, RsMsgTagType* >(mtt->tagId, mtt));
			}
			else
			{
				delete mTags[mtt->tagId];
				mTags.erase(tagIt);
				mTags.insert(std::pair<uint32_t, RsMsgTagType* >(mtt->tagId, mtt));
			}

		}
		else if(NULL != (mti = dynamic_cast<RsMsgTags *>(*it)))
		{
			mMsgTags.insert(std::pair<uint32_t, RsMsgTags* >(mti->msgId, mti));
		}
		else if(NULL != (msi = dynamic_cast<RsMsgSrcId *>(*it)))
		{
            srcIdMsgMap.insert(std::pair<uint32_t, RsPeerId>(msi->msgId, msi->srcId));
			mSrcIds.insert(std::pair<uint32_t, RsMsgSrcId*>(msi->msgId, msi)); // does not need to be kept
		}
		else if(NULL != (msp = dynamic_cast<RsMsgParentId *>(*it)))
		{
			mParentId.insert(std::pair<uint32_t, RsMsgParentId*>(msp->msgId, msp));
		}

		RsConfigKeyValueSet *vitem = NULL ;

		if(NULL != (vitem = dynamic_cast<RsConfigKeyValueSet*>(*it)))
			for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) 
				if(kit->key == "DISTANT_MESSAGES_ENABLED")
				{
#ifdef MSG_DEBUG
					std::cerr << "Loaded config default nick name for distant chat: " << kit->value << std::endl ;
#endif
					enableDistantMessaging(kit->value == "YES") ;
					distant_messaging_set = true ;
				}

	}
	if(mDistantMessagingEnabled || !distant_messaging_set)
	{
#ifdef DEBUG_DISTANT_MSG
		std::cerr << "No config value for distant messaging. Setting it to true." << std::endl;
#endif
		enableDistantMessaging(true) ;
	}

        // sort items into lists
	std::list<RsMsgItem*>::iterator msgIt;
	for (msgIt = items.begin(); msgIt != items.end(); msgIt++)
	{
		mitem = *msgIt;

		/* STORE MsgID */
		if (mitem->msgId == 0) {
		    mitem->msgId = getNewUniqueMsgId();
		}

		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		srcIt = srcIdMsgMap.find(mitem->msgId);
		if(srcIt != srcIdMsgMap.end()) {
			mitem->PeerId(srcIt->second);
			srcIdMsgMap.erase(srcIt);
		}

		/* switch depending on the PENDING
		 * flags
		 */
		if (mitem -> msgFlags & RS_MSG_FLAGS_PENDING)
		{

			//std::cerr << "MSG_PENDING";
			//std::cerr << std::endl;
			//mitem->print(std::cerr);

			msgOutgoing[mitem->msgId] = mitem;
		}
		else
		{
			imsg[mitem->msgId] = mitem;
		}
	}

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	/* remove missing msgId in mSrcIds */
	for (srcIt = srcIdMsgMap.begin(); srcIt != srcIdMsgMap.end(); srcIt++) {
		std::map<uint32_t, RsMsgSrcId*>::iterator it = mSrcIds.find(srcIt->first);
		if (it != mSrcIds.end()) {
			delete(it->second);
			mSrcIds.erase(it);
		}
	}

	/* remove missing msgId in mParentId */
	std::map<uint32_t, RsMsgParentId *>::iterator mit = mParentId.begin();
	while (mit != mParentId.end()) {
		if (imsg.find(mit->first) == imsg.end()) {
			if (msgOutgoing.find(mit->first) == msgOutgoing.end()) {
				/* not found */
				mParentId.erase(mit++);
				continue;
			}
		}
	
		mit++;
	}
	
	return true;
}

void p3MsgService::loadWelcomeMsg()
{
	/* Load Welcome Message */
	RsMsgItem *msg = new RsMsgItem();

	//msg -> PeerId(mServiceCtrl->getOwnId());

	msg -> sendTime = time(NULL);
	msg -> recvTime = time(NULL);
	msg -> msgFlags = RS_MSG_FLAGS_NEW;

	msg -> subject = "Welcome to Retroshare";

	msg -> message  = "Send and receive messages with your friends...\n";
	msg -> message += "These can hold recommendations from your local shared files.\n\n";
	msg -> message += "Add recommendations through the Local Files Dialog.\n\n";
	msg -> message += "Enjoy.";

	msg -> msgId = getNewUniqueMsgId();

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	imsg[msg->msgId] = msg;

	IndicateConfigChanged();
}


/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/


/****************************************/
/****************************************/

bool p3MsgService::getMessageSummaries(std::list<MsgInfoSummary> &msgList)
{
	/* do stuff */
	msgList.clear();

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	std::map<uint32_t, RsMsgItem *>::iterator mit;
	for(mit = imsg.begin(); mit != imsg.end(); mit++)
	{
		MsgInfoSummary mis;
		initRsMIS(mit->second, mis);
		msgList.push_back(mis);
	}

	for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
	{
		MsgInfoSummary mis;
		initRsMIS(mit->second, mis);
		msgList.push_back(mis);
	}
	return true;
}

bool p3MsgService::getMessage(const std::string &mId, MessageInfo &msg)
{
  	std::map<uint32_t, RsMsgItem *>::iterator mit;
	uint32_t msgId = atoi(mId.c_str());

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	mit = imsg.find(msgId);
	if (mit == imsg.end())
	{
		mit = msgOutgoing.find(msgId);
		if (mit == msgOutgoing.end())
		{
			return false;
		}
	}

	/* mit valid */
	initRsMI(mit->second, msg);

	std::map<uint32_t, RsMsgSrcId*>::const_iterator it = mSrcIds.find(msgId) ;
	if(it != mSrcIds.end())
		msg.rsgxsid_srcId = RsGxsId(it->second->srcId) ;

	return true;
}

void p3MsgService::getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox)
{
    RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

    if (pnInbox) *pnInbox = 0;
    if (pnInboxNew) *pnInboxNew = 0;
    if (pnOutbox) *pnOutbox = 0;
    if (pnDraftbox) *pnDraftbox = 0;
    if (pnSentbox) *pnSentbox = 0;
    if (pnTrashbox) *pnTrashbox = 0;

    std::map<uint32_t, RsMsgItem *>::iterator mit;
    std::map<uint32_t, RsMsgItem *> *apMsg [2] = { &imsg, &msgOutgoing };

    for (int i = 0; i < 2; i++) {
        for (mit = apMsg [i]->begin(); mit != apMsg [i]->end(); mit++) {
            MsgInfoSummary mis;
            initRsMIS(mit->second, mis);

            if (mis.msgflags & RS_MSG_TRASH) {
                if (pnTrashbox) (*pnTrashbox)++;
                continue;
            }
            switch (mis.msgflags & RS_MSG_BOXMASK) {
            case RS_MSG_INBOX:
                    if (pnInbox) (*pnInbox)++;
                    if ((mis.msgflags & RS_MSG_NEW) == RS_MSG_NEW) {
                        if (pnInboxNew) (*pnInboxNew)++;
                    }
                    break;
            case RS_MSG_OUTBOX:
                    if (pnOutbox) (*pnOutbox)++;
                    break;
            case RS_MSG_DRAFTBOX:
                    if (pnDraftbox) (*pnDraftbox)++;
                    break;
            case RS_MSG_SENTBOX:
                    if (pnSentbox) (*pnSentbox)++;
                    break;
            }
        }
    }
}

/* remove based on the unique mid (stored in sid) */
bool    p3MsgService::removeMsgId(const std::string &mid)
{
  	std::map<uint32_t, RsMsgItem *>::iterator mit;
	uint32_t msgId = atoi(mid.c_str());
	if (msgId == 0) {
		std::cerr << "p3MsgService::removeMsgId: Unknown msgId " << msgId << std::endl;
		return false;
	}

	bool changed = false;

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		mit = imsg.find(msgId);
		if (mit != imsg.end())
		{
			changed = true;
			RsMsgItem *mi = mit->second;
			imsg.erase(mit);
			delete mi;
		}

		mit = msgOutgoing.find(msgId);
		if (mit != msgOutgoing.end())
		{
			changed = true ;
			RsMsgItem *mi = mit->second;
			msgOutgoing.erase(mit);
			delete mi;
		}

		std::map<uint32_t, RsMsgSrcId*>::iterator srcIt = mSrcIds.find(msgId);
		if (srcIt != mSrcIds.end()) {
			changed = true;
			delete (srcIt->second);
			mSrcIds.erase(srcIt);
		}
	}

	if(changed) {
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

		setMessageTag(mid, 0, false);
		setMsgParentId(msgId, 0);

		RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
	}

	return changed;
}

bool    p3MsgService::markMsgIdRead(const std::string &mid, bool unreadByUser)
{
	std::map<uint32_t, RsMsgItem *>::iterator mit;
	uint32_t msgId = atoi(mid.c_str());
	bool changed = false;

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		mit = imsg.find(msgId);
		if (mit != imsg.end())
		{
			RsMsgItem *mi = mit->second;

			uint32_t msgFlags = mi->msgFlags;

			/* remove new state */
			mi->msgFlags &= ~(RS_MSG_FLAGS_NEW);

			/* set state from user */
			if (unreadByUser) {
				mi->msgFlags |= RS_MSG_FLAGS_UNREAD_BY_USER;
			} else {
				mi->msgFlags &= ~RS_MSG_FLAGS_UNREAD_BY_USER;
			}

			if (mi->msgFlags != msgFlags)
			{
				changed = true;
				IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
			}
		} else {
			return false;
		}
	} /* UNLOCKED */

	if (changed) {
		RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
	}

	return true;
}

bool    p3MsgService::setMsgFlag(const std::string &mid, uint32_t flag, uint32_t mask)
{
  	std::map<uint32_t, RsMsgItem *>::iterator mit;
	uint32_t msgId = atoi(mid.c_str());

	bool changed = false;

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		mit = imsg.find(msgId);
		if (mit == imsg.end())
		{
			mit = msgOutgoing.find(msgId);
			if (mit == msgOutgoing.end())
			{
				return false;
			}
		}

		uint32_t oldFlag = mit->second->msgFlags;

		mit->second->msgFlags &= ~mask;
		mit->second->msgFlags |= flag;

		if (mit->second->msgFlags != oldFlag) {
			changed = true;
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
	} /* UNLOCKED */

	if (changed) {
		RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
	}

	return true;
}

bool    p3MsgService::getMsgParentId(const std::string &msgId, std::string &msgParentId)
{
	msgParentId.clear();

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	std::map<uint32_t, RsMsgParentId *>::iterator mit = mParentId.find(atoi(msgId.c_str()));
	if (mit == mParentId.end()) {
		return false;
	}

	rs_sprintf(msgParentId, "%lu", mit->second->msgParentId);
	
	return true;
}

bool    p3MsgService::setMsgParentId(uint32_t msgId, uint32_t msgParentId)
{
	std::map<uint32_t, RsMsgParentId *>::iterator mit;

	bool changed = false;

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		mit = mParentId.find(msgId);
		if (mit == mParentId.end())
		{
			if (msgParentId) {
				RsMsgParentId* msp = new RsMsgParentId();
				msp->PeerId (mServiceCtrl->getOwnId());
				msp->msgId = msgId;
				msp->msgParentId = msgParentId;
				mParentId.insert(std::pair<uint32_t, RsMsgParentId*>(msgId, msp));

				changed = true;
			}
		} else {
			if (msgParentId) {
				if (mit->second->msgParentId != msgParentId) {
					mit->second->msgParentId = msgParentId;
					changed = true;
				}
			} else {
				delete mit->second;
				mParentId.erase(mit);
				changed = true;
			}
		}
	} /* UNLOCKED */

	if (changed) {
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}

	return true;
}

/****************************************/
/****************************************/
	/* Message Items */
int     p3MsgService::sendMessage(RsMsgItem *item)
{
    if(!item)
        return 0 ;

	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::sendMessage()");

	item -> msgId = getNewUniqueMsgId(); /* grabs Mtx as well */

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		/* add pending flag */
		item->msgFlags |= (RS_MSG_FLAGS_OUTGOING | RS_MSG_FLAGS_PENDING);
		/* STORE MsgID */
		msgOutgoing[item->msgId] = item;

		if (item->PeerId() != mServiceCtrl->getOwnId()) {
			/* not to the loopback device */
			RsMsgSrcId* msi = new RsMsgSrcId();
			msi->msgId = item->msgId;
			msi->srcId = item->PeerId();
			mSrcIds.insert(std::pair<uint32_t, RsMsgSrcId*>(msi->msgId, msi));
		}
	}

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST, NOTIFY_TYPE_ADD);

	checkOutgoingMessages();

	return 1;
}

bool 	p3MsgService::MessageSend(MessageInfo &info)
{
	for(std::list<RsPeerId>::const_iterator pit = info.rspeerid_msgto.begin();  pit != info.rspeerid_msgto.end();  pit++) sendMessage(initMIRsMsg(info, *pit));
	for(std::list<RsPeerId>::const_iterator pit = info.rspeerid_msgcc.begin();  pit != info.rspeerid_msgcc.end();  pit++) sendMessage(initMIRsMsg(info, *pit));
	for(std::list<RsPeerId>::const_iterator pit = info.rspeerid_msgbcc.begin(); pit != info.rspeerid_msgbcc.end(); pit++) sendMessage(initMIRsMsg(info, *pit));

	for(std::list<RsGxsId>::const_iterator pit = info.rsgxsid_msgto.begin();  pit != info.rsgxsid_msgto.end();  pit++) sendMessage(initMIRsMsg(info, *pit));
	for(std::list<RsGxsId>::const_iterator pit = info.rsgxsid_msgcc.begin();  pit != info.rsgxsid_msgcc.end();  pit++) sendMessage(initMIRsMsg(info, *pit));
	for(std::list<RsGxsId>::const_iterator pit = info.rsgxsid_msgbcc.begin(); pit != info.rsgxsid_msgbcc.end(); pit++) sendMessage(initMIRsMsg(info, *pit));

	/* send to ourselves as well */
	RsMsgItem *msg = initMIRsMsg(info, mServiceCtrl->getOwnId());

	if (msg)
	{
		std::list<RsPgpId>::iterator it ;

		if (msg->msgFlags & RS_MSG_FLAGS_SIGNED)
			msg->msgFlags |= RS_MSG_FLAGS_SIGNATURE_CHECKS;	// this is always true, since we are sending the message

		/* use processMsg to get the new msgId */
		processMsg(msg, false);

		// return new message id
		rs_sprintf(info.msgId, "%lu", msg->msgId);
	}

	return true;
}

bool p3MsgService::SystemMessage(const std::string &title, const std::string &message, uint32_t systemFlag)
{
	if ((systemFlag & RS_MSG_SYSTEM) == 0) {
		/* no flag specified */
		return false;
	}

    const RsPeerId& ownId = mServiceCtrl->getOwnId();

	RsMsgItem *msg = new RsMsgItem();

	msg->PeerId(ownId);

	msg->msgFlags = 0;

	if (systemFlag & RS_MSG_USER_REQUEST) {
		msg->msgFlags |= RS_MSG_FLAGS_USER_REQUEST;
	}
	if (systemFlag & RS_MSG_FRIEND_RECOMMENDATION) {
		msg->msgFlags |= RS_MSG_FLAGS_FRIEND_RECOMMENDATION;
	}

	msg->msgId = 0;
	msg->sendTime = time(NULL);
	msg->recvTime = 0;

	msg->subject = title;
	msg->message = message;

	msg->rspeerid_msgto.ids.push_back(ownId);

	processMsg(msg, true);

	return true;
}

bool p3MsgService::MessageToDraft(MessageInfo &info, const std::string &msgParentId)
{
    RsMsgItem *msg = initMIRsMsg(info, mServiceCtrl->getOwnId());
    if (msg)
    {
        uint32_t msgId = 0;
        if (info.msgId.empty() == false) {
            msgId = atoi(info.msgId.c_str());
        }

        if (msgId) {
            msg->msgId = msgId;
        } else {
            msg->msgId = getNewUniqueMsgId(); /* grabs Mtx as well */
        }

        {
            RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

            /* add pending flag */
            msg->msgFlags |= (RS_MSG_OUTGOING | RS_MSG_FLAGS_DRAFT);

            if (msgId) {
                // remove existing message
                std::map<uint32_t, RsMsgItem *>::iterator mit;
                mit = imsg.find(msgId);
                if (mit != imsg.end()) {
                    RsMsgItem *mi = mit->second;
                    imsg.erase(mit);
                    delete mi;
                }
            }
            /* STORE MsgID */
            imsg[msg->msgId] = msg;

            // return new message id
            rs_sprintf(info.msgId, "%lu", msg->msgId);
        }

        setMsgParentId(msg->msgId, atoi(msgParentId.c_str()));

        IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

		  RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);

        return true;
    }

    return false;
}

bool 	p3MsgService::getMessageTagTypes(MsgTagType& tags)
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	std::map<uint32_t, RsMsgTagType*>::iterator mit;

	for(mit = mTags.begin(); mit != mTags.end(); mit++) {
		std::pair<std::string, uint32_t> p(mit->second->text, mit->second->rgb_color);
		tags.types.insert(std::pair<uint32_t, std::pair<std::string, uint32_t> >(mit->first, p));
	}

	return true;
}

bool  	p3MsgService::setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color)
{
	int nNotifyType = 0;

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		std::map<uint32_t, RsMsgTagType*>::iterator mit;
		mit = mTags.find(tagId);

		if (mit == mTags.end()) {
			if (tagId < RS_MSGTAGTYPE_USER) {
				std::cerr << "p3MsgService::MessageSetTagType: Standard tag type " <<  tagId << " cannot be inserted" << std::endl;
				return false;
			}

			/* new tag */
			RsMsgTagType* tagType = new RsMsgTagType();
			tagType->PeerId (mServiceCtrl->getOwnId());
			tagType->rgb_color = rgb_color;
			tagType->tagId = tagId;
			tagType->text = text;

			mTags.insert(std::pair<uint32_t, RsMsgTagType*>(tagId, tagType));

			nNotifyType = NOTIFY_TYPE_ADD;
		} else {
			if (mit->second->text != text || mit->second->rgb_color != rgb_color) {
				/* modify existing tag */
				if (tagId >= RS_MSGTAGTYPE_USER) {
					mit->second->text = text;
				} else {
					/* don't change text for standard tag types */
					if (mit->second->text != text) {
						std::cerr << "p3MsgService::MessageSetTagType: Text " << text << " for standard tag type " <<  tagId << " cannot be changed" << std::endl;
					}
				}
				mit->second->rgb_color = rgb_color;

				nNotifyType = NOTIFY_TYPE_MOD;
			}
		}

	} /* UNLOCKED */

	if (nNotifyType) {
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

		RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGE_TAGS, nNotifyType);
		
		return true;
	}

	return false;
}

bool    p3MsgService::removeMessageTagType(uint32_t tagId)
{
	if (tagId < RS_MSGTAGTYPE_USER) {
		std::cerr << "p3MsgService::MessageRemoveTagType: Can't delete standard tag type " << tagId << std::endl;
		return false;
	}

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		std::map<uint32_t, RsMsgTagType*>::iterator mit;
		mit = mTags.find(tagId);

		if (mit == mTags.end()) {
			/* tag id not found */
			std::cerr << "p3MsgService::MessageRemoveTagType: Tag Id not found " << tagId << std::endl;
			return false;
		}

		/* search for messages with this tag type */
		std::map<uint32_t, RsMsgTags*>::iterator mit1;
                for (mit1 = mMsgTags.begin(); mit1 != mMsgTags.end(); ) {
			RsMsgTags* tag = mit1->second;

			std::list<uint32_t>::iterator lit;
			lit = std::find(tag->tagIds.begin(), tag->tagIds.end(), tagId);
			if (lit != tag->tagIds.end()) {
				tag->tagIds.erase(lit);

				if (tag->tagIds.size() == 0) {
					/* remove empty tag */
					delete(tag);

					mMsgTags.erase(mit1++);
					continue;
				}
			}
			mit1++;
		}

		/* remove tag type */
		delete(mit->second);
		mTags.erase(mit);

	} /* UNLOCKED */

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGE_TAGS, NOTIFY_TYPE_DEL);

	return true;
}

bool 	p3MsgService::getMessageTag(const std::string &msgId, MsgTagInfo& info)
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	uint32_t mid = atoi(msgId.c_str());
	if (mid == 0) {
		std::cerr << "p3MsgService::MessageGetMsgTag: Unknown msgId " << msgId << std::endl;
		return false;
	}

	std::map<uint32_t, RsMsgTags*>::iterator mit;

	if(mMsgTags.end() != (mit = mMsgTags.find(mid))) {
		rs_sprintf(info.msgId, "%lu", mit->second->msgId);
		info.tagIds = mit->second->tagIds;

		return true;
	}

	return false;
}

/* set == false && tagId == 0 --> remove all */
bool 	p3MsgService::setMessageTag(const std::string &msgId, uint32_t tagId, bool set)
{
	uint32_t mid = atoi(msgId.c_str());
	if (mid == 0) {
		std::cerr << "p3MsgService::MessageSetMsgTag: Unknown msgId " << msgId << std::endl;
		return false;
	}

	if (tagId == 0) {
		if (set == true) {
			std::cerr << "p3MsgService::MessageSetMsgTag: No valid tagId given " << tagId << std::endl;
			return false;
		}
	}
	
	int nNotifyType = 0;

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		std::map<uint32_t, RsMsgTags*>::iterator mit;
		mit = mMsgTags.find(mid);

		if (mit == mMsgTags.end()) {
			if (set) {
				/* new msg */
				RsMsgTags* tag = new RsMsgTags();
				tag->PeerId (mServiceCtrl->getOwnId());

				tag->msgId = mid;
				tag->tagIds.push_back(tagId);

				mMsgTags.insert(std::pair<uint32_t, RsMsgTags*>(tag->msgId, tag));

				nNotifyType = NOTIFY_TYPE_ADD;
			}
		} else {
			RsMsgTags* tag = mit->second;

			/* search existing tagId */
			std::list<uint32_t>::iterator lit;
			if (tagId) {
				lit = std::find(tag->tagIds.begin(), tag->tagIds.end(), tagId);
			} else {
				lit = tag->tagIds.end();
			}

			if (set) {
				if (lit == tag->tagIds.end()) {
					tag->tagIds.push_back(tagId);
					/* keep the list sorted */
					tag->tagIds.sort();
					nNotifyType = NOTIFY_TYPE_ADD;
				}
			} else {
				if (tagId == 0) {
					/* remove all */
					delete(tag);
					mMsgTags.erase(mit);
					nNotifyType = NOTIFY_TYPE_DEL;
				} else {
					if (lit != tag->tagIds.end()) {
						tag->tagIds.erase(lit);
						nNotifyType = NOTIFY_TYPE_DEL;

						if (tag->tagIds.size() == 0) {
							/* remove empty tag */
							delete(tag);
							mMsgTags.erase(mit);
						}
					}
				}
			}
		}

	} /* UNLOCKED */

	if (nNotifyType) {
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

		RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGE_TAGS, nNotifyType);

		return true;
	}

	return false;
}

bool    p3MsgService::resetMessageStandardTagTypes(MsgTagType& tags)
{
	MsgTagType standardTags;
        getStandardTagTypes(standardTags);

	std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator mit;
	for (mit = standardTags.types.begin(); mit != standardTags.types.end(); mit++) {
		tags.types[mit->first] = mit->second;
	}

	return true;
}

/* move message to trash based on the unique mid */
bool p3MsgService::MessageToTrash(const std::string &mid, bool bTrash)
{
    std::map<uint32_t, RsMsgItem *>::iterator mit;
    uint32_t msgId = atoi(mid.c_str());

    bool bChanged = false;
    bool bFound = false;

    {
        RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

        RsMsgItem *mi = NULL;

        mit = imsg.find(msgId);
        if (mit != imsg.end()) {
            mi = mit->second;
        } else {
            mit = msgOutgoing.find(msgId);
            if (mit != msgOutgoing.end()) {
                mi = mit->second;
            }
        }

        if (mi) {
            bFound = true;

            if (bTrash) {
                if ((mi->msgFlags & RS_MSG_FLAGS_TRASH) == 0) {
                    mi->msgFlags |= RS_MSG_FLAGS_TRASH;
                    bChanged = true;
                }
            } else {
                if (mi->msgFlags & RS_MSG_FLAGS_TRASH) {
                    mi->msgFlags &= ~RS_MSG_FLAGS_TRASH;
                    bChanged = true;
                }
            }
        }
    }

    if (bChanged) {
        IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

        checkOutgoingMessages();

		  RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
    }

    return bFound;
}

/****************************************/
/****************************************/


/****************************************/

/**** HELPER FNS For Chat/Msg/Channel Lists ************
 * These aren't required to be locked, unless
 * the data used is from internal stores -> then they should be.
 */

void p3MsgService::initRsMI(RsMsgItem *msg, MessageInfo &mi)
{

	mi.msgflags = 0;

	/* translate flags, if we sent it... outgoing */

	if (msg->msgFlags & RS_MSG_FLAGS_OUTGOING)        mi.msgflags |= RS_MSG_OUTGOING;
	if (msg->msgFlags & RS_MSG_FLAGS_PENDING)         mi.msgflags |= RS_MSG_PENDING;    /* if it has a pending flag, then its in the outbox */
	if (msg->msgFlags & RS_MSG_FLAGS_DRAFT)           mi.msgflags |= RS_MSG_DRAFT;
	if (msg->msgFlags & RS_MSG_FLAGS_NEW)             mi.msgflags |= RS_MSG_NEW;

	if (msg->msgFlags & RS_MSG_FLAGS_SIGNED)                  mi.msgflags |= RS_MSG_SIGNED ;
	if (msg->msgFlags & RS_MSG_FLAGS_SIGNATURE_CHECKS)        mi.msgflags |= RS_MSG_SIGNATURE_CHECKS ;
	if (msg->msgFlags & RS_MSG_FLAGS_ENCRYPTED)               mi.msgflags |= RS_MSG_ENCRYPTED ;
	if (msg->msgFlags & RS_MSG_FLAGS_DECRYPTED)               mi.msgflags |= RS_MSG_DECRYPTED ;
	if (msg->msgFlags & RS_MSG_FLAGS_TRASH)                   mi.msgflags |= RS_MSG_TRASH;
	if (msg->msgFlags & RS_MSG_FLAGS_UNREAD_BY_USER)          mi.msgflags |= RS_MSG_UNREAD_BY_USER;
	if (msg->msgFlags & RS_MSG_FLAGS_REPLIED)                 mi.msgflags |= RS_MSG_REPLIED;
	if (msg->msgFlags & RS_MSG_FLAGS_FORWARDED)               mi.msgflags |= RS_MSG_FORWARDED;
	if (msg->msgFlags & RS_MSG_FLAGS_STAR)                    mi.msgflags |= RS_MSG_STAR;
	if (msg->msgFlags & RS_MSG_FLAGS_USER_REQUEST)            mi.msgflags |= RS_MSG_USER_REQUEST;
	if (msg->msgFlags & RS_MSG_FLAGS_FRIEND_RECOMMENDATION)   mi.msgflags |= RS_MSG_FRIEND_RECOMMENDATION;
	if (msg->msgFlags & RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES)    mi.msgflags |= RS_MSG_LOAD_EMBEDDED_IMAGES;

	mi.ts = msg->sendTime;
	mi.rspeerid_srcId = msg->PeerId();

	mi.rspeerid_msgto  = msg->rspeerid_msgto.ids ;
	mi.rspeerid_msgcc  = msg->rspeerid_msgcc.ids ;
	mi.rspeerid_msgbcc = msg->rspeerid_msgbcc.ids ;

	mi.rsgxsid_msgto  = msg->rsgxsid_msgto.ids ;
	mi.rsgxsid_msgcc  = msg->rsgxsid_msgcc.ids ;
	mi.rsgxsid_msgbcc = msg->rsgxsid_msgbcc.ids ;

	mi.title = msg->subject;
	mi.msg   = msg->message;

	mi.attach_title = msg->attachment.title;
	mi.attach_comment = msg->attachment.comment;

	mi.count = 0;
	mi.size = 0;

	for(std::list<RsTlvFileItem>::iterator it = msg->attachment.items.begin(); it != msg->attachment.items.end(); it++)
	{
		FileInfo fi;
		fi.fname = RsDirUtil::getTopDir(it->name);
		fi.size  = it->filesize;
		fi.hash  = it->hash;
		fi.path  = it->path;
		mi.files.push_back(fi);
		mi.count++;
		mi.size += fi.size;
	}
}

void p3MsgService::initRsMIS(RsMsgItem *msg, MsgInfoSummary &mis)
{
	mis.msgflags = 0;

	if (msg->msgFlags & RS_MSG_FLAGS_ENCRYPTED)
		mis.msgflags |= RS_MSG_ENCRYPTED ;

	if (msg->msgFlags & RS_MSG_FLAGS_DECRYPTED)
		mis.msgflags |= RS_MSG_DECRYPTED ;

	if (msg->msgFlags & RS_MSG_FLAGS_SIGNED)
		mis.msgflags |= RS_MSG_SIGNED ;

	if (msg->msgFlags & RS_MSG_FLAGS_SIGNATURE_CHECKS)
		mis.msgflags |= RS_MSG_SIGNATURE_CHECKS ;

	/* translate flags, if we sent it... outgoing */
	if ((msg->msgFlags & RS_MSG_FLAGS_OUTGOING)
	   /*|| (msg->PeerId() == mServiceCtrl->getOwnId())*/)
	{
		mis.msgflags |= RS_MSG_OUTGOING;
	}
	/* if it has a pending flag, then its in the outbox */
	if (msg->msgFlags & RS_MSG_FLAGS_PENDING)
	{
		mis.msgflags |= RS_MSG_PENDING;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_DRAFT)
	{
		mis.msgflags |= RS_MSG_DRAFT;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_NEW)
	{
		mis.msgflags |= RS_MSG_NEW;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_TRASH)
	{
		mis.msgflags |= RS_MSG_TRASH;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_UNREAD_BY_USER)
	{
		mis.msgflags |= RS_MSG_UNREAD_BY_USER;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_REPLIED)
	{
		mis.msgflags |= RS_MSG_REPLIED;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_FORWARDED)
	{
		mis.msgflags |= RS_MSG_FORWARDED;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_STAR)
	{
		mis.msgflags |= RS_MSG_STAR;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_USER_REQUEST)
	{
		mis.msgflags |= RS_MSG_USER_REQUEST;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_FRIEND_RECOMMENDATION)
	{
		mis.msgflags |= RS_MSG_FRIEND_RECOMMENDATION;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES)
	{
		mis.msgflags |= RS_MSG_LOAD_EMBEDDED_IMAGES;
	}

	mis.srcId = msg->PeerId();
	{
		//msg->msgId;
		rs_sprintf(mis.msgId, "%lu", msg->msgId);
	}

	mis.title = msg->subject;
	mis.count = msg->attachment.items.size();
	mis.ts = msg->sendTime;
}

void p3MsgService::initMIRsMsg(RsMsgItem *msg,const MessageInfo& info)
{
	msg -> msgFlags = 0;
	msg -> msgId = 0;
	msg -> sendTime = time(NULL);
	msg -> recvTime = 0;
	msg -> subject = info.title;
	msg -> message = info.msg;

	msg->rspeerid_msgto.ids  = info.rspeerid_msgto ;
	msg->rspeerid_msgcc.ids  = info.rspeerid_msgcc ;

	msg->rsgxsid_msgto.ids  = info.rsgxsid_msgto ;
	msg->rsgxsid_msgcc.ids  = info.rsgxsid_msgcc ;

	/* We don't fill in bcc (unless to ourselves) */

	if (msg->PeerId() == mServiceCtrl->getOwnId())
	{
		msg->rsgxsid_msgbcc.ids = info.rsgxsid_msgbcc ;
		msg->rspeerid_msgbcc.ids = info.rspeerid_msgbcc ;
	}

	msg -> attachment.title   = info.attach_title;
	msg -> attachment.comment = info.attach_comment;

	for(std::list<FileInfo>::const_iterator it = info.files.begin(); it != info.files.end(); it++)
	{
		RsTlvFileItem mfi;
		mfi.hash = it -> hash;
		mfi.name = it -> fname;
		mfi.filesize = it -> size;
		msg -> attachment.items.push_back(mfi);
	}
	/* translate flags from outside */
	if (info.msgflags & RS_MSG_USER_REQUEST)
		msg->msgFlags |= RS_MSG_FLAGS_USER_REQUEST;

	if (info.msgflags & RS_MSG_FRIEND_RECOMMENDATION)
		msg->msgFlags |= RS_MSG_FLAGS_FRIEND_RECOMMENDATION;
}
RsMsgItem *p3MsgService::initMIRsMsg(const MessageInfo& info, const RsGxsId& to)
{
	RsMsgItem *msg = new RsMsgItem();

	initMIRsMsg(msg,info) ;

    msg->PeerId(RsPeerId());
	msg->msgFlags |= RS_MSG_FLAGS_DISTANT; 

	if (info.msgflags & RS_MSG_SIGNED)
		msg->msgFlags |= RS_MSG_FLAGS_SIGNED;

	// We replace the msg text by the whole message serialized possibly signed,
	// and binary encrypted, so as to obfuscate all its content.
	//
	if(!createDistantMessage(to,info.rsgxsid_srcId,msg))
	{
		std::cerr << "Cannot encrypt distant message. Something went wrong." << std::endl;
		delete msg ;
		return NULL ;
	}

	return msg ;
}

RsMsgItem *p3MsgService::initMIRsMsg(const MessageInfo &info, const RsPeerId& to)
{
	RsMsgItem *msg = new RsMsgItem();

	initMIRsMsg(msg,info) ;

	msg->PeerId(to) ;

	/* load embedded images from own messages */
	msg->msgFlags |= RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES;

	return msg;
}

bool p3MsgService::createDistantMessage(const RsGxsId& destination_gxs_id,const RsGxsId& source_gxs_id,RsMsgItem *item)
{
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "Creating distant message for recipient " << destination_gxs_id << std::endl;
#endif
	unsigned char *data = NULL ;
	uint8_t *encrypted_data = NULL ;

	try
	{
		// 0 - append own id to the data.
		//
		uint32_t conservative_max_signature_size = 1000 ;
		uint32_t message_size = _serialiser->size(item) ;
		uint32_t total_data_size = 1+5+source_gxs_id.SIZE_IN_BYTES+5+message_size+conservative_max_signature_size ;

		data = (unsigned char *)malloc(total_data_size) ;

		// -1 - setup protocol version
		//
#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  adding protocol version TAG = " << std::hex << (int)DISTANT_MSG_PROTOCOL_VERSION_02 << std::dec << std::endl;
#endif
		uint32_t offset = 0 ;
		data[offset++] = DISTANT_MSG_PROTOCOL_VERSION_02 ;

#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  adding own source key ID " << source_gxs_id << std::endl;
#endif
		data[offset++] = DISTANT_MSG_TAG_IDENTITY ;
		offset += PGPKeyParser::write_125Size(&data[offset],source_gxs_id.SIZE_IN_BYTES) ;
		memcpy(&data[offset], source_gxs_id.toByteArray(), source_gxs_id.SIZE_IN_BYTES) ;
		offset += source_gxs_id.SIZE_IN_BYTES ;

		// 1 - serialise the whole message item into a binary chunk.
		//

#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  serialising item..." << std::endl;
#endif
		data[offset++] = DISTANT_MSG_TAG_CLEAR_MSG ;
		offset += PGPKeyParser::write_125Size(&data[offset],message_size) ;

		uint32_t remaining_size = total_data_size - offset ;

		if(!_serialiser->serialise(item,&data[offset],&remaining_size))
			throw std::runtime_error("Serialization error.") ;
		
		offset += message_size ;

		// 2 - now sign the data, if necessary, and put the signature up front

		if(item->msgFlags & RS_MSG_FLAGS_SIGNED)
		{
#ifdef DEBUG_DISTANT_MSG
			std::cerr << "  Signing the message with key id " << source_gxs_id << std::endl;
#endif
			RsTlvKeySignature signature ;
			RsTlvSecurityKey signature_key ;

#ifdef DEBUG_DISTANT_MSG
			std::cerr << "     Getting key material..." << std::endl;
#endif
			if(!mIdService->getPrivateKey(source_gxs_id,signature_key))
				throw std::runtime_error("Cannot get signature key for id " + source_gxs_id.toStdString()) ;

#ifdef DEBUG_DISTANT_MSG
			std::cerr << "     Signing..." << std::endl;
#endif
			if(!GxsSecurity::getSignature((char *)data,offset,signature_key,signature))
				throw std::runtime_error("Cannot sign for id " + source_gxs_id.toStdString() + ". Signature call failed.") ;

			// 3 - append the signature to the serialized data.

#ifdef DEBUG_DISTANT_MSG
			std::cerr << "  Appending signature." << std::endl;
			std::cerr << "    data length: " << offset     << std::endl;
			std::cerr << "    data hash  : " << RsDirUtil::sha1sum(data,offset) << std::endl;
			std::cerr << "    sign size  : " << signature.signData.bin_len << std::endl;
			std::cerr << "    sign hex   : " << RsUtil::BinToHex((const char*)signature.signData.bin_data,std::min(50u,signature.signData.bin_len)) << "..." << std::endl;
			std::cerr << "    sign hash  : " << RsDirUtil::sha1sum((const uint8_t*)signature.signData.bin_data,signature.signData.bin_len) << std::endl;
#endif
			if(offset + signature.signData.bin_len + 5 + 1 >= total_data_size)
				throw std::runtime_error("Conservative size is not enough! Can't serialise encrypted message.") ;

			data[offset++] = DISTANT_MSG_TAG_SIGNATURE ;
			offset += PGPKeyParser::write_125Size(&data[offset],signature.signData.bin_len) ;
			memcpy(&data[offset],signature.signData.bin_data,signature.signData.bin_len) ;
			offset += signature.signData.bin_len ;
		}
#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  total decrypted size = " << offset << std::endl;
#endif
		// 2 - pgp-encrypt the whole chunk with the user-supplied public key.
		//

#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  Encrypting for Key ID " << destination_gxs_id << std::endl;
#endif
		// Do GXS encryption here

		RsTlvSecurityKey encryption_key ;

		if(!mIdService->getKey(destination_gxs_id,encryption_key))
			throw std::runtime_error("Cannot get encryption key for id " + destination_gxs_id.toStdString()) ;

		int encrypted_size = 0 ;

		if(!GxsSecurity::encrypt(encrypted_data,encrypted_size,data,offset,encryption_key))
			throw std::runtime_error("Encryption failed!") ;

		free(data) ;
		data = NULL ;

#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  Decrypted size = " << offset << std::endl;
		std::cerr << "  Encrypted size = " << encrypted_size << std::endl;
		std::cerr << "  First bytes of encrypted data: " << RsUtil::BinToHex((const char *)encrypted_data,std::min(encrypted_size,50)) << "..."<< std::endl;
		std::cerr << "  Encrypted data hash = " << RsDirUtil::sha1sum((const uint8_t *)encrypted_data,encrypted_size) << std::endl;
#endif
		// Now turn the binary encrypted chunk into a readable radix string.
		//
#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  Converting to radix64" << std::endl;
#endif
		std::string armoured_data ;
		Radix64::encode((char *)encrypted_data,encrypted_size,armoured_data) ;

		delete[] encrypted_data ;
		encrypted_data = NULL ;

		// wipe the item clean and replace the message by the encrypted data.

		item->message = armoured_data ;
		item->subject = "" ;
		item->rspeerid_msgcc.ids.clear() ;
		item->rspeerid_msgbcc.ids.clear() ;
		item->rspeerid_msgto.ids.clear() ;
		item->rsgxsid_msgcc.ids.clear() ;
		item->rsgxsid_msgbcc.ids.clear() ;
		item->rsgxsid_msgto.ids.clear() ;
		item->msgFlags |= RS_MSG_FLAGS_ENCRYPTED ;
		item->attachment.TlvClear() ;
		item->PeerId(RsPeerId(destination_gxs_id)) ;	// This is a trick, based on the fact that both IDs have the same size.

#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  Done" << std::endl;
#endif
		return true ;
	}
	catch(std::exception& e)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": Error. reason: " << e.what() << std::endl;

		if(encrypted_data) free(encrypted_data) ;
		if(data) free(data) ;
		return false ;
	}
}

std::string printNumber(uint32_t n,bool hex)
{
    std::ostringstream os ;
    if(hex) os << std::hex ;
    os << n ;
    return os.str() ;
}
bool p3MsgService::decryptMessage(const std::string& mId)
{
    uint8_t *decrypted_data = NULL;
    char *encrypted_data = NULL;

    try
    {
        uint32_t msgId = atoi(mId.c_str());
        std::string encrypted_string ;
		  RsGxsId destination_gxs_id ;

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "Decrypting message with Id " << mId << std::endl;
#endif
        {
            RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

            std::map<uint32_t, RsMsgItem *>::iterator mit = imsg.find(msgId);

            if(mit == imsg.end())
            {
                std::cerr << "Can't find this message in msg list. Id=" << mId << std::endl;
                return false;
            }

            encrypted_string = mit->second->message ;
				destination_gxs_id = RsGxsId(mit->second->PeerId()) ;
        }

        size_t encrypted_size ;

        Radix64::decode(encrypted_string,encrypted_data,encrypted_size) ;

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "  Message has been radix64 decoded." << std::endl;
#endif

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "  Calling decryption. Encrypted data size = " << encrypted_size << std::endl;
		  std::cerr << "    Destination GxsId : " << destination_gxs_id << std::endl;
        std::cerr << "    First bytes of encrypted data: " << RsUtil::BinToHex((char*)encrypted_data,std::min(encrypted_size,(size_t)50)) << std::endl;
        std::cerr << "    Encrypted data hash = " << RsDirUtil::sha1sum((uint8_t*)encrypted_data,encrypted_size) << std::endl;
#endif
        // Use GXS decryption here.

        int decrypted_size = 0 ;
        decrypted_data = NULL ;

		RsTlvSecurityKey encryption_key ;

		if(!mIdService->getPrivateKey(destination_gxs_id,encryption_key))
			throw std::runtime_error("Cannot get private encryption key for id " + destination_gxs_id.toStdString()) ;

		if(!GxsSecurity::decrypt(decrypted_data,decrypted_size,(uint8_t*)encrypted_data,encrypted_size,encryption_key))
			throw std::runtime_error("Decryption failed!") ;

		std::cerr << "  First bytes of decrypted data: " << RsUtil::BinToHex((const char *)decrypted_data,std::min(decrypted_size,50)) << "..."<< std::endl;

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "  Message has succesfully decrypted. Decrypted size = " << decrypted_size << std::endl;
#endif
        // 1 - get the sender's id

        uint32_t offset = 0 ;
        unsigned char protocol_version = decrypted_data[offset++] ;

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "  Read protocol version number " << std::hex << (int)protocol_version << std::dec << std::endl;
#endif
        if(protocol_version != DISTANT_MSG_PROTOCOL_VERSION_02)
        throw std::runtime_error("Bad protocol version number " + printNumber(protocol_version,true) + " => packet is dropped.") ;

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "  Reading identity section " << std::endl;
#endif
        uint8_t ptag = decrypted_data[offset++] ;

        if(ptag != DISTANT_MSG_TAG_IDENTITY)
        throw std::runtime_error("Bad ptag in encrypted msg packet "+printNumber(ptag,true)+" => packet is dropped.") ;

        unsigned char *tmp_data = &decrypted_data[offset] ;
		  unsigned char *old_data = tmp_data ;
        uint32_t identity_size = PGPKeyParser::read_125Size(tmp_data) ;
        offset += tmp_data - old_data ;

        if(identity_size != RsGxsId::SIZE_IN_BYTES)
        throw std::runtime_error("Bad size in Identity section " + printNumber(identity_size,false) + " => packet is dropped.") ;

        RsGxsId senders_id(&decrypted_data[offset]) ;
        offset += identity_size ;

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "  Sender's ID: " << senders_id.toStdString() << std::endl;
#endif
        // 2 - deserialize the item

        ptag = decrypted_data[offset++] ;

        if(ptag != DISTANT_MSG_TAG_CLEAR_MSG)
            throw std::runtime_error("Bad ptag in encrypted msg packet " + printNumber(ptag,true) + " => packet is dropped.") ;

        tmp_data = &decrypted_data[offset] ;
		  old_data = tmp_data ;
        uint32_t item_size = PGPKeyParser::read_125Size(tmp_data) ;
        offset += tmp_data - old_data ;

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "  Deserializing..." << std::endl;
#endif
        RsMsgItem *item = dynamic_cast<RsMsgItem*>(_serialiser->deserialise(&decrypted_data[offset],&item_size)) ;
		  offset += item_size ;

        if(item == NULL)
        throw std::runtime_error("Decrypted message could not be deserialized.") ;

        // 3 - check signature

        bool signature_present = false ;
        bool signature_ok = false ;
    uint32_t size_of_signed_data = offset ;

        if(offset < decrypted_size)
		  {
			  uint8_t ptag = decrypted_data[offset++] ;

			  if(ptag != DISTANT_MSG_TAG_SIGNATURE)
				  throw std::runtime_error("Bad ptag in signature packet " + printNumber(ptag,true) + " => packet is dropped.") ;

			  tmp_data = &decrypted_data[offset] ;
			  old_data = tmp_data ;
			  uint32_t signature_size = PGPKeyParser::read_125Size(tmp_data) ;
			  offset += tmp_data - old_data ;

			  RsTlvKeySignature signature ;
			  signature.keyId = senders_id.toStdString() ;
			  signature.signData.bin_len = signature_size ;
			  signature.signData.bin_data = malloc(signature_size) ;
			  memcpy(signature.signData.bin_data,&decrypted_data[offset],signature_size) ;

#ifdef DEBUG_DISTANT_MSG
			  std::cerr << "  Signature is present. Verifying it..." << std::endl;
			  std::cerr << "    data length: " << size_of_signed_data     << std::endl;
			  std::cerr << "    data hash  : " << RsDirUtil::sha1sum(decrypted_data,size_of_signed_data) << std::endl;
			  std::cerr << "    Sign length: " << signature.signData.bin_len << std::endl;
			  std::cerr << "    Sign hash  : " << RsDirUtil::sha1sum((const uint8_t*)signature.signData.bin_data,signature.signData.bin_len) << std::endl;
			  std::cerr << "    Sign key id: " << signature.keyId << std::endl;
#endif
			  signature_present = true ;

			  RsTlvSecurityKey signature_key ;

			  // We need to get the key of the sender, but if the key is not cached, we need to get it first. So we let
			  // the system work for 2-3 seconds before giving up. Normally this would only cause a delay for uncached 
			  // keys, which is rare. To force the system to cache the key, we first call for getIdDetails().
			  //
			  RsIdentityDetails details  ;
			  mIdService->getIdDetails(senders_id,details);

			  for(int i=0;i<6;++i)
				  if(!mIdService->getKey(senders_id,signature_key) || signature_key.keyData.bin_data == NULL)
				  {
					  std::cerr << "  Cannot get key. Waiting for caching. try " << i << "/6" << std::endl;
					  usleep(500000) ;	// sleep for 500 msec.
				  }
				  else
					  break ;

			  if(signature_key.keyData.bin_data == NULL)
				  std::cerr << "(EE) No key for checking signature from " << senders_id << ", can't verify signature." << std::endl;
			  else if(!GxsSecurity::validateSignature((char*)decrypted_data,size_of_signed_data,signature_key,signature))
				  std::cerr << "(EE) Signature was verified and it doesn't check! This is a security issue!" << std::endl;
			  else
				  signature_ok = true ;

			  offset += signature_size ;
		  }
        else if(offset == decrypted_size)
            std::cerr << "  No signature in this packet" << std::endl;
        else
            throw std::runtime_error("Structural error in packet: sizes do not match. Dropping the message.") ;

        delete[] decrypted_data ;
		  decrypted_data = NULL ;

        // 4 - replace the item with the decrypted data, and update flags

#ifdef DEBUG_DISTANT_MSG
        std::cerr << "  Decrypted message was succesfully deserialized. New message:" << std::endl;
        item->print(std::cerr,0) ;
#endif
        //RsGxsId own_gxs_id = destination_gxs_id;		// we should use the correct own GXS ID that was used to send that message!

        {
            RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

            // Keep the id.
            RsMsgItem& msgi( *imsg[msgId] ) ;

            msgi = *item ;				// copy everything
            msgi.msgId = msgId ;		// restore the correct message id, to make it consistent
            msgi.msgFlags &= ~RS_MSG_FLAGS_ENCRYPTED ;	// just in case.
            msgi.msgFlags |=  RS_MSG_FLAGS_DECRYPTED ;	// previousy encrypted msg is now decrypted

            //for(std::list<RsPeerId>::iterator it(msgi.msgto.ids.begin());it!=msgi.msgto.ids.end();++it) if(*it == own_id) *it = own_pgp_id ;
            //for(std::list<RsPeerId>::iterator it(msgi.msgcc.ids.begin());it!=msgi.msgcc.ids.end();++it) if(*it == own_id) *it = own_pgp_id ;
            //for(std::list<RsPeerId>::iterator it(msgi.msgbcc.ids.begin());it!=msgi.msgbcc.ids.end();++it) if(*it == own_id) *it = own_pgp_id ;

            if(signature_present)
            {
                msgi.msgFlags |= RS_MSG_FLAGS_SIGNED ;

                if(signature_ok)
                    msgi.msgFlags |= RS_MSG_FLAGS_SIGNATURE_CHECKS ;	// just in case.
                else
                    msgi.msgFlags &= ~RS_MSG_FLAGS_SIGNATURE_CHECKS ;	// just in case.
            }

            std::map<uint32_t, RsMsgSrcId*>::iterator it = mSrcIds.find(msgi.msgId) ;

            if(it == mSrcIds.end())
            {
                std::cerr << "Warning: could not find message source for id " << msgi.msgId << ". Weird." << std::endl;

                RsMsgSrcId* msi = new RsMsgSrcId();
                msi->msgId = msgi.msgId;
                msi->srcId = RsPeerId(senders_id) ;

                mSrcIds.insert(std::pair<uint32_t, RsMsgSrcId*>(msi->msgId, msi));
            }
            else
            {
                std::cerr << "Substituting source name for message id " << msgi.msgId << ": " << it->second->srcId << " -> " << senders_id << std::endl;
                it->second->srcId = RsPeerId(senders_id) ;
            }
        }
        delete item ;

        IndicateConfigChanged() ;
        RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);

        return true ;
    }
    catch(std::exception& e)
    {
        std::cerr << "Decryption failed: " << e.what() << std::endl;
        if(encrypted_data != NULL) delete[] encrypted_data ;
        if(decrypted_data != NULL) delete[] decrypted_data ;

        return false ;
    }
}

void p3MsgService::connectToGlobalRouter(p3GRouter *gr)
{
	mGRouter = gr ;
	gr->registerClientService(GROUTER_CLIENT_ID_MESSAGES,this) ;
}

void p3MsgService::enableDistantMessaging(bool b)
{
	// Normally this method should work on the basis of each GXS id. For now, we just blindly enable
	// messaging for all GXS ids for which we have the private key.
	
	bool cchanged ;
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		cchanged = (mDistantMessagingEnabled != b) ;
		mDistantMessagingEnabled = b ;
	}

	std::list<RsGxsId> own_id_list ;
	mIdService->getOwnIds(own_id_list) ;

#ifdef DEBUG_DISTANT_MSG
	for(std::list<RsGxsId>::const_iterator it(own_id_list.begin());it!=own_id_list.end();++it)
        std::cerr << (b?"Enabling":"Disabling") << " distant messaging, with peer id = " << *it << std::endl;
#endif

	for(std::list<RsGxsId>::const_iterator it(own_id_list.begin());it!=own_id_list.end();++it)
		if(b)
			mGRouter->registerKey(GRouterKeyId(*it),GROUTER_CLIENT_ID_MESSAGES,"Messaging contact") ;
		else
			mGRouter->unregisterKey(GRouterKeyId(*it)) ;

	if(cchanged)
		IndicateConfigChanged() ;
}
bool p3MsgService::distantMessagingEnabled()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
	return mDistantMessagingEnabled ;
}

void p3MsgService::manageDistantPeers()
{
	// now possibly flush pending messages

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "p3MsgService::manageDistantPeers()" << std::endl;
#endif
	enableDistantMessaging(mDistantMessagingEnabled) ;
}

void p3MsgService::sendGRouterData(const GRouterKeyId& key_id,RsMsgItem *msgitem)
{
	// The item is serialized and turned into a generic turtle item.
	
	uint32_t msg_serialized_rssize = _serialiser->size(msgitem) ;
	unsigned char *msg_serialized_data = new unsigned char[msg_serialized_rssize] ;

	if(!_serialiser->serialise(msgitem,msg_serialized_data,&msg_serialized_rssize))
	{
		std::cerr << "(EE) p3MsgService::sendTurtleData(): Serialization error." << std::endl;
		delete[] msg_serialized_data ;
		return ;
	}

	RsGRouterGenericDataItem *item = new RsGRouterGenericDataItem ;

	item->data_bytes = (uint8_t*)malloc(msg_serialized_rssize) ;
	item->data_size = msg_serialized_rssize ;
	memcpy(item->data_bytes,msg_serialized_data,msg_serialized_rssize) ;

	delete[] msg_serialized_data ;

	GRouterMsgPropagationId grouter_message_id ;

    mGRouter->sendData(key_id,GROUTER_CLIENT_ID_MESSAGES,item,grouter_message_id) ;

	 // now store the grouter id along with the message id, so that we can keep track of received messages

	 _ongoing_messages[grouter_message_id] = msgitem->msgId ;
}
void p3MsgService::acknowledgeDataReceived(const GRouterMsgPropagationId& id)
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "p3MsgService::acknowledgeDataReceived(): acknowledging data received for msg propagation id  " << id << std::endl;
#endif
	std::map<GRouterMsgPropagationId,uint32_t>::iterator it = _ongoing_messages.find(id) ;

	if(it == _ongoing_messages.end())
	{
		std::cerr << "  (EE) cannot find pending message to acknowledge. Weird. grouter id = " << id << std::endl;
		return ;
	}

	uint32_t msg_id = it->second ;

	// we should now remove the item from the msgOutgoing list.
	
	std::map<uint32_t,RsMsgItem*>::iterator it2 = msgOutgoing.find(msg_id) ;

	if(it2 == msgOutgoing.end())
	{
		std::cerr << "(EE) message has been ACKed, but is not in outgoing list. Something's wrong!!" << std::endl;
		return ;
	}

	delete it2->second ;
	msgOutgoing.erase(it2) ;

	RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_ADD);
	IndicateConfigChanged() ;
}
void p3MsgService::receiveGRouterData(const GRouterKeyId& key, const RsGRouterGenericDataItem *gitem)
{
	std::cerr << "p3MsgService::receiveGRouterData(): received message item of size " << gitem->data_size << ", for key " << key << std::endl;

	uint32_t size = gitem->data_size ;
		RsItem *item = _serialiser->deserialise(gitem->data_bytes,&size) ;

	RsMsgItem *encrypted_msg_item = dynamic_cast<RsMsgItem*>(item) ;

	if(encrypted_msg_item != NULL)
	{
		std::cerr << "  Encrypted item correctly deserialised. Passing on to incoming list." << std::endl;

		encrypted_msg_item->PeerId(RsPeerId(key)) ;	// hack to pass on GXS id.
		handleIncomingItem(encrypted_msg_item) ;
	}
	else
		std::cerr << "  Item could not be deserialised. Format error??" << std::endl;
}

void p3MsgService::sendPrivateMsgItem(RsMsgItem *msgitem)
{
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "p3MsgService::sendDistanteMsgItem(): sending distant msg item to peer " << msgitem->PeerId() << std::endl;
#endif
	GRouterKeyId key_id(msgitem->PeerId()) ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "    Flushing msg " << msgitem->msgId << " for peer id " << msgitem->PeerId() << std::endl;
#endif

	sendGRouterData(key_id,msgitem) ;
}




