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

#include "pqi/pqibin.h"
#include "pqi/pqiarchive.h"
#include "pqi/p3linkmgr.h"
#include "pqi/authgpg.h"

#include "services/p3msgservice.h"
#include "pgp/pgpkeyutil.h"
#include "pqi/p3cfgmgr.h"
#include "rsserver/p3face.h"
#include "serialiser/rsconfigitems.h"
#ifdef GROUTER
#include "grouter/p3grouter.h"
#include "grouter/groutertypes.h"
#endif

#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "util/rsstring.h"
#include "util/radix64.h"
#include "util/rsrandom.h"

#include <iomanip>
#include <map>

//#define MSG_DEBUG 1
//#define DEBUG_DISTANT_MSG
//#define DISABLE_DISTANT_MESSAGES 

const int msgservicezone = 54319;

static const uint32_t RS_DISTANT_MSG_STATUS_TUNNEL_OK = 0x0001 ;
static const uint32_t RS_DISTANT_MSG_STATUS_TUNNEL_DN = 0x0000 ;
static const uint32_t DISTANT_MSG_HASH_SIZE = Sha1CheckSum::SIZE_IN_BYTES ;

static const uint8_t ENCRYPTED_MSG_PROTOCOL_VERSION_01 = 0x37 ;

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


p3MsgService::p3MsgService(p3ServiceControl *sc)
	:p3Service(), p3Config(),
	mServiceCtrl(sc), mMsgMtx("p3MsgService"), mMsgUniqueId(time(NULL))
{
	_serialiser = new RsMsgSerialiser();
	addSerialType(_serialiser);

	mDistantMessagingEnabled = true ;

	/* Initialize standard tag types */
	if(sc)
		initStandardTagTypes();

#ifdef GROUTER
	// Debug stuff. Create a random key and register it.
	const RsPeerId& own_ssl_id = rsPeers->getOwnId() ;
	const RsPgpId& gpg_id = rsPeers->getGPGOwnId() ;

	RsPeerDetails d;
    rsPeers->getGPGDetails(gpg_id,d) ;
	PGPFingerprintType fingerp( d.fpr ) ;

	// Re-hash the SSL id, to make it one way. Will be replaced by proper invitations in the future.
	//
    GRouterKeyId key ( RsDirUtil::sha1sum( own_ssl_id.toByteArray(),own_ssl_id.SIZE_IN_BYTES ))  ;

	static GRouterServiceId client_id = GROUTER_CLIENT_ID_MESSAGES;
	static std::string description = "Test string for debug purpose" ;

	mGRouter->registerKey(key,fingerp,client_id,description) ;
#endif
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
	Sha1CheckSum hash ;

	if( msg->msgFlags & RS_MSG_FLAGS_DISTANT )
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

#ifdef DEBUG_DISTANT_MSG
		std::cerr << "checkOutgoingMessages(): removing pending message flag for hash " << (*it)->PeerId() << "." << std::endl;
#endif
		if(!locked_findHashForVirtualPeerId(msg->PeerId(),hash))
		{
			std::cerr << "p3MsgService::checkSizeAndSendMessage(): Cannot find hash for virtual peer id " << msg->PeerId() << ". This is unexpected!" << std::endl;
			return ;
		}
		_messenging_contacts[hash].pending_messages = false ;
	}

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
		{


			sendPrivateMsgItem(hash,item) ;
		}
		else
			sendItem(item) ;
	}
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Chopped off msg of size " << msg->message.size() << std::endl;
#endif

	if(msg->msgFlags & RS_MSG_FLAGS_DISTANT)
		sendPrivateMsgItem(hash,msg) ;
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
			if (mit->second->msgFlags & RS_MSG_FLAGS_TRASH) {
				continue;
			}

			/* find the certificate */
            RsPeerId pid = mit->second->PeerId();
			bool tunnel_is_ok = false ;

			if(mit->second->msgFlags & RS_MSG_FLAGS_DISTANT)
			{
				// Do we have a tunnel already?
				//
#ifdef GROUTER
				tunnel_is_ok = true ;
#else
//				const RsPeerId& hash = mit->second->PeerId() ;
//				std::map<Sha1CheckSum,DistantMessengingContact>::iterator it = _messenging_contacts.find(hash) ;
//
//				if(it != _messenging_contacts.end())
//				{
//					tunnel_is_ok = (it->second.status == RS_DISTANT_MSG_STATUS_TUNNEL_OK) ;
//#ifdef DEBUG_DISTANT_MSG
//					std::cerr << "checkOutGoingMessages(): distant contact found. tunnel_is_ok = " << tunnel_is_ok << std::endl;
//#endif
//				}
//				else
//				{
//#ifdef DEBUG_DISTANT_MSG
//					std::cerr << "checkOutGoingMessages(): distant contact not found. Asking for tunnels for hash " << hash << std::endl;
//#endif
//					// no. Ask for monitoring tunnels.
//					rsTurtle->monitorTunnels(hash,this) ;					 
//					tunnel_is_ok = false ;
//
//					DistantMessengingContact& contact( _messenging_contacts[hash] ) ;
//					contact.status = RS_DISTANT_MSG_STATUS_TUNNEL_DN ;
//					contact.pending_messages = true ;
//				}
#endif
			}

			if(tunnel_is_ok || mServiceCtrl->isPeerConnected(getServiceInfo().mServiceType, pid) || pid == ownId) /* FEEDBACK Msg to Ourselves */
			{
				/* send msg */
				pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
					"p3MsgService::checkOutGoingMessages() Sending out message");
				/* remove the pending flag */
				(mit->second)->msgFlags &= ~RS_MSG_FLAGS_PENDING;

				output_queue.push_back(mit->second) ;
				toErase.push_back(mit->first);

				changed = true ;
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

//	for(std::map<Sha1CheckSum,DistantMessengingInvite>::const_iterator it(_messenging_invites.begin());it!=_messenging_invites.end();++it)
//	{
//		RsPublicMsgInviteConfigItem *item = new RsPublicMsgInviteConfigItem ;
//		item->hash = it->first ;
//		item->time_stamp = it->second.time_of_validity ;
//
//		itemList.push_back(item) ;
//	}
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
//		else if(NULL != (msv = dynamic_cast<RsPublicMsgInviteConfigItem *>(*it)))
//		{
//			_messenging_invites[msv->hash].time_of_validity = msv->time_stamp ;
//		}

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
	if(!distant_messaging_set)
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
    std::list<RsPeerId>::const_iterator pit;

	for(pit = info.msgto.begin(); pit != info.msgto.end(); pit++)
	{
		RsMsgItem *msg = initMIRsMsg(info, *pit);
		if (msg)
		{
			sendMessage(msg);
		}
	}

	for(pit = info.msgcc.begin(); pit != info.msgcc.end(); pit++)
	{
		RsMsgItem *msg = initMIRsMsg(info, *pit);
		if (msg)
		{
			sendMessage(msg);
		}
	}

	for(pit = info.msgbcc.begin(); pit != info.msgbcc.end(); pit++)
	{
		RsMsgItem *msg = initMIRsMsg(info, *pit);
		if (msg)
		{
			sendMessage(msg);
		}
	}

	/* send to ourselves as well */
	RsMsgItem *msg = initMIRsMsg(info, mServiceCtrl->getOwnId());
	if (msg)
	{
        std::list<RsPgpId>::iterator it ;

		// Update destination ids in place of distant message hash, since this Outgoing message is for display
		//
//		for(it = msg->msgbcc.ids.begin(); it != msg->msgbcc.ids.end(); it++)
//			if(info.encryption_keys.find(*it) != info.encryption_keys.end()) *it = info.encryption_keys[*it] ;
//		for(it = msg->msgcc.ids.begin(); it != msg->msgcc.ids.end(); it++)
//			if(info.encryption_keys.find(*it) != info.encryption_keys.end()) *it = info.encryption_keys[*it] ;
//		for(it = msg->msgto.ids.begin(); it != msg->msgto.ids.end(); it++)
//			if(info.encryption_keys.find(*it) != info.encryption_keys.end()) *it = info.encryption_keys[*it] ;

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

	msg->msgto.ids.push_back(ownId);

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
	if ((msg->msgFlags & RS_MSG_FLAGS_OUTGOING)
	   /*|| (msg->PeerId() == mServiceCtrl->getOwnId())*/)
	{
		mi.msgflags |= RS_MSG_OUTGOING;
	}
	/* if it has a pending flag, then its in the outbox */
	if (msg->msgFlags & RS_MSG_FLAGS_PENDING)
	{
		mi.msgflags |= RS_MSG_PENDING;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_DRAFT)
	{
		mi.msgflags |= RS_MSG_DRAFT;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_NEW)
	{
		mi.msgflags |= RS_MSG_NEW;
	}

	if (msg->msgFlags & RS_MSG_FLAGS_SIGNED)
		mi.msgflags |= RS_MSG_SIGNED ;

	if (msg->msgFlags & RS_MSG_FLAGS_SIGNATURE_CHECKS)
		mi.msgflags |= RS_MSG_SIGNATURE_CHECKS ;

	if (msg->msgFlags & RS_MSG_FLAGS_ENCRYPTED)
		mi.msgflags |= RS_MSG_ENCRYPTED ;

	if (msg->msgFlags & RS_MSG_FLAGS_DECRYPTED)
		mi.msgflags |= RS_MSG_DECRYPTED ;

	if (msg->msgFlags & RS_MSG_FLAGS_TRASH)
	{
		mi.msgflags |= RS_MSG_TRASH;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_UNREAD_BY_USER)
	{
		mi.msgflags |= RS_MSG_UNREAD_BY_USER;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_REPLIED)
	{
		mi.msgflags |= RS_MSG_REPLIED;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_FORWARDED)
	{
		mi.msgflags |= RS_MSG_FORWARDED;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_STAR)
	{
		mi.msgflags |= RS_MSG_STAR;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_USER_REQUEST)
	{
		mi.msgflags |= RS_MSG_USER_REQUEST;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_FRIEND_RECOMMENDATION)
	{
		mi.msgflags |= RS_MSG_FRIEND_RECOMMENDATION;
	}
	if (msg->msgFlags & RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES)
	{
		mi.msgflags |= RS_MSG_LOAD_EMBEDDED_IMAGES;
	}

	mi.ts = msg->sendTime;
	mi.srcId = msg->PeerId();
	{
		//msg->msgId;
		rs_sprintf(mi.msgId, "%lu", msg->msgId);
	}

    std::list<RsPeerId>::iterator pit;

	for(pit = msg->msgto.ids.begin(); 
        pit != msg->msgto.ids.end(); pit++)
	{
		mi.msgto.push_back(*pit);
	}

	for(pit = msg->msgcc.ids.begin(); 
		pit != msg->msgcc.ids.end(); pit++)
	{
		mi.msgcc.push_back(*pit);
	}

	for(pit = msg->msgbcc.ids.begin(); 
		pit != msg->msgbcc.ids.end(); pit++)
	{
		mi.msgbcc.push_back(*pit);
	}

	mi.title = msg->subject;
	mi.msg   = msg->message;

	mi.attach_title = msg->attachment.title;
	mi.attach_comment = msg->attachment.comment;

	mi.count = 0;
	mi.size = 0;

	std::list<RsTlvFileItem>::iterator it;
	for(it = msg->attachment.items.begin(); 
			it != msg->attachment.items.end(); it++)
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

RsMsgItem *p3MsgService::initMIRsMsg(MessageInfo &info, const RsPeerId &to)
{
	RsMsgItem *msg = new RsMsgItem();

	msg -> PeerId(to);

	msg -> msgFlags = 0;
	msg -> msgId = 0;
	msg -> sendTime = time(NULL);
	msg -> recvTime = 0;
	
	msg -> subject = info.title;

		msg -> message = info.msg;

    std::list<RsPeerId>::iterator pit;
	for(pit = info.msgto.begin(); pit != info.msgto.end(); pit++)
	{
		msg -> msgto.ids.push_back(*pit);
	}

	for(pit = info.msgcc.begin(); pit != info.msgcc.end(); pit++)
	{
		msg -> msgcc.ids.push_back(*pit);
	}

	/* We don't fill in bcc (unless to ourselves) */
	if (to == mServiceCtrl->getOwnId())
	{
		for(pit = info.msgbcc.begin(); pit != info.msgbcc.end(); pit++)
		{
			msg -> msgbcc.ids.push_back(*pit);
		}
	}

	msg -> attachment.title   = info.attach_title;
	msg -> attachment.comment = info.attach_comment;

	RsPeerDetails details ;
	if(!rsPeers->getPeerDetails(to,details))
		msg->msgFlags |= RS_MSG_FLAGS_DISTANT; 

	std::list<FileInfo>::iterator it;
	for(it = info.files.begin(); it != info.files.end(); it++)
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

	if (info.msgflags & RS_MSG_SIGNED)
		msg->msgFlags |= RS_MSG_FLAGS_SIGNED;

	/* load embedded images from own messages */
	msg->msgFlags |= RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES;

	// See if we need to encrypt this message.  If so, we replace the msg text
	// by the whole message serialized and binary encrypted, so as to obfuscate
	// all its content.
	//

	if(!info.encryption_keys.empty())
		std::cerr << "(WW) Cannot encrypt message. Code needs to be improved." << std::endl;
//	if(info.encryption_keys.find(to) != info.encryption_keys.end())
//		encryptMessage(info.encryption_keys[to],msg) ;

		//std::cerr << "p3MsgService::initMIRsMsg()" << std::endl;
	//msg->print(std::cerr);
	return msg;
}

bool p3MsgService::encryptMessage(const RsPgpId& pgp_id,RsMsgItem *item)
{
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "Encrypting message with public key " << pgp_id << " in place." << std::endl;
#endif

	// 0 - append own id to the data.
	//
	uint32_t rssize = _serialiser->size(item) ;
	unsigned char *data = (unsigned char *)malloc(1+rssize+PGP_KEY_ID_SIZE) ;
	
	// -1 - setup protocol version
	//
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  adding protocol version TAG = " << std::hex << (int)ENCRYPTED_MSG_PROTOCOL_VERSION_01 << std::dec << std::endl;
#endif
	data[0] = ENCRYPTED_MSG_PROTOCOL_VERSION_01 ;
	
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  adding own key ID " << AuthGPG::getAuthGPG()->getGPGOwnId() << std::endl;
#endif
	memcpy(&data[1], RsPgpId(AuthGPG::getAuthGPG()->getGPGOwnId()).toByteArray(), PGP_KEY_ID_SIZE) ;

	// 1 - serialise the whole message item into a binary chunk.
	//

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  serialising item..." << std::endl;
#endif
	if(!_serialiser->serialise(item,&data[1+PGP_KEY_ID_SIZE],&rssize))
	{
		std::cerr << "(EE) p3MsgService::encryptMessage(): Serialization error." << std::endl;
		free(data) ;
		return false;
	}

	// 2 - now sign the data, if necessary, and put the signature up front

	uint32_t signature_length = 0 ;
	unsigned char *signature_data = NULL ;

	if(item->msgFlags & RS_MSG_FLAGS_SIGNED)
	{
#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  Signing the message..." << std::endl;
#endif
		signature_length = 2000 ;
		signature_data = new unsigned char[signature_length] ;

		if(!AuthGPG::getAuthGPG()->SignDataBin(data,1+rssize+PGP_KEY_ID_SIZE,signature_data,&signature_length))
		{
			free(data) ;
			std::cerr << "Signature failed!" << std::endl;
			return false;
		}
#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  After signature: signature size = " << signature_length << std::endl;
#endif
	}
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  total decrypted size = " << PGP_KEY_ID_SIZE + 1 + rssize + signature_length << std::endl;
#endif
	// 3 - append the signature to the serialized data.

	if(signature_length > 0)
	{
#ifdef DEBUG_DISTANT_MSG
		std::cerr << "  Appending signature." << std::endl;
#endif
		data = (uint8_t*)realloc(data,1+rssize+signature_length+PGP_KEY_ID_SIZE) ;
		memcpy(&data[1+rssize+PGP_KEY_ID_SIZE],signature_data,signature_length) ;
	}

	// 2 - pgp-encrypt the whole chunk with the user-supplied public key.
	//
	uint32_t encrypted_size = 1+rssize + PGP_KEY_ID_SIZE + signature_length + 1000 ;
	unsigned char *encrypted_data = new unsigned char[encrypted_size] ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Encrypting for Key ID " << pgp_id << std::endl;
#endif
	if(!AuthGPG::getAuthGPG()->encryptDataBin(pgp_id,data,1+rssize+signature_length+PGP_KEY_ID_SIZE,encrypted_data,&encrypted_size))
	{
		free(data) ;
		delete[] encrypted_data ;
		std::cerr << "Encryption failed!" << std::endl;
		return false;
	}
	free(data) ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Decrypted size = " << 1+rssize+signature_length+PGP_KEY_ID_SIZE << std::endl;
	std::cerr << "  Encrypted size = " << encrypted_size << std::endl;
	std::cerr << "  First bytes of encrypted data: " << std::hex << (int)encrypted_data[0] << " " << (int)encrypted_data[1] << " " << (int)encrypted_data[2] << std::dec << std::endl;
	std::cerr << "  Encrypted data hash = " << RsDirUtil::sha1sum(encrypted_data,encrypted_size).toStdString() << std::endl;
#endif
	// Now turn the binary encrypted chunk into a readable radix string.
	//
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Converting to radix64" << std::endl;
#endif
	std::string armoured_data ;
	Radix64::encode((char *)encrypted_data,encrypted_size,armoured_data) ;
	delete[] encrypted_data ;

	//std::wstring encrypted_msg = armoured_data;
	//if(!librs::util::ConvertUtf8ToUtf16(armoured_data,encrypted_msg)) 
	//	return false;

	// wipe the item clean and replace the message by the encrypted data.

	item->message = armoured_data ;
	item->subject = "" ;
	item->msgcc.ids.clear() ;
	item->msgbcc.ids.clear() ;
	item->msgto.ids.clear() ;
	item->msgFlags |= RS_MSG_FLAGS_ENCRYPTED ;
	item->attachment.TlvClear() ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Done" << std::endl;
#endif
	return true ;
}

bool p3MsgService::decryptMessage(const std::string& mId)
{
	uint32_t msgId = atoi(mId.c_str());
	std::string encrypted_string ;

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
	}

	char *encrypted_data ;
	size_t encrypted_size ;

	Radix64::decode(encrypted_string,encrypted_data,encrypted_size) ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Message has been radix64 decoded." << std::endl;
#endif

	uint32_t decrypted_size = encrypted_size + 500 ;
	unsigned char *decrypted_data = new unsigned char[decrypted_size] ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Calling decryption. Encrypted data size = " << encrypted_size << ", Allocated size = " << decrypted_size << std::endl;
	std::cerr << "  First bytes of encrypted data: " << std::hex << (int)((unsigned char*)encrypted_data)[0] << " " << (int)((unsigned char*)encrypted_data)[1] << " " << (int)((unsigned char*)encrypted_data)[2] << std::dec << std::endl;
	std::cerr << "  Encrypted data hash = " << RsDirUtil::sha1sum((uint8_t*)encrypted_data,encrypted_size).toStdString() << std::endl;
#endif
	if(!AuthGPG::getAuthGPG()->decryptDataBin(encrypted_data,encrypted_size,decrypted_data,&decrypted_size))
	{
		delete[] encrypted_data ;
		delete[] decrypted_data ;
		std::cerr << "decryption failed!" << std::endl;
		return false;
	}
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Message has succesfully decrypted. Decrypted size = " << decrypted_size << std::endl;
#endif

	// 1 - get the sender's id
	
	unsigned char protocol_version = decrypted_data[0] ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Read protocol version number " << std::hex << (int)protocol_version << std::dec << std::endl;
#endif
	if(protocol_version != ENCRYPTED_MSG_PROTOCOL_VERSION_01)
	{
		std::cerr << "Bad version number " << std::hex << (int)protocol_version << std::dec << " => packet is dropped." << std::endl;
		delete[] encrypted_data ;
		delete[] decrypted_data ;
		return false;
	}

	RsPgpId senders_id(&decrypted_data[1]) ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Sender's ID: " << senders_id.toStdString() << std::endl;
#endif
	// 2 - deserialize the item

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Deserializing..." << std::endl;
#endif
	uint32_t item_size = decrypted_size ;	// just needs to be larger than the actual size.
	RsMsgItem *item = dynamic_cast<RsMsgItem*>(_serialiser->deserialise(&decrypted_data[1+PGP_KEY_ID_SIZE],&item_size)) ;

	if(item == NULL)
	{
		std::cerr << "Decrypted message could not be deserialized." << std::endl;
		return false ;
	}

	// 3 - check signature

	bool signature_present = false ;
	bool signature_ok = false ;

	if(1+item_size + PGP_KEY_ID_SIZE < decrypted_size)
	{
		std::cerr << "  Signature is present. Verifying it..." << std::endl;

		PGPFingerprintType fingerprint ;
		if(!AuthGPG::getAuthGPG()->getKeyFingerprint(senders_id,fingerprint)) 
		{
			std::cerr << "Cannot get fingerprint" << std::endl;
			return false ;
		}
		std::cerr << "  Fingerprint = " << fingerprint.toStdString() << std::endl;

		signature_present = true ;
		signature_ok = AuthGPG::getAuthGPG()->VerifySignBin(decrypted_data, 1+PGP_KEY_ID_SIZE+item_size, &decrypted_data[1+PGP_KEY_ID_SIZE+item_size], decrypted_size - PGP_KEY_ID_SIZE - item_size - 1, fingerprint) ;
	}
	else if(1 + item_size + PGP_KEY_ID_SIZE == decrypted_size)
		std::cerr << "  No signature in this packet" << std::endl;
	else
	{
		std::cerr << "Structural error in packet: sizes do not match. Dropping the message." << std::endl;
		return false ;
	}

	delete[] decrypted_data ;

	// 4 - replace the item with the decrypted data, and update flags
	
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "  Decrypted message was succesfully deserialized. New message:" << std::endl;
	item->print(std::cerr,0) ;
#endif
	RsPeerId own_id ;
	const RsPgpId& own_pgp_id = AuthGPG::getAuthGPG()->getGPGOwnId();
	getDistantMessagePeerId(own_pgp_id,own_id) ;

	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		// Keer the id.
		RsMsgItem& msgi( *imsg[msgId] ) ;

		msgi = *item ;											// copy everything
		msgi.msgId = msgId ;									// restore the correct message id, to make it consistent
		msgi.msgFlags &= ~RS_MSG_FLAGS_ENCRYPTED ;	// just in case.
		msgi.msgFlags |=  RS_MSG_FLAGS_DECRYPTED ;	// previousy encrypted msg is now decrypted
	
		DistantMsgPeerId senders_vpid ;
		getDistantMessagePeerId(senders_id,senders_vpid) ;
		msgi.PeerId(senders_vpid) ;

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
			msi->srcId  = senders_vpid ;

			mSrcIds.insert(std::pair<uint32_t, RsMsgSrcId*>(msi->msgId, msi));
		}
		else
		{
			std::cerr << "Substituting source name for message id " << msgi.msgId << ": " << it->second->srcId << " -> " << senders_id << std::endl;
			it->second->srcId = senders_vpid ;
		}
	}
	delete item ;

	IndicateConfigChanged() ;
	RsServer::notify()->notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);

	return true ;
}

#ifdef GROUTER
void p3MsgService::connectToGlobalRouter(p3GRouter *gr)
{
	mGRouter = gr ;
	gr->registerClientService(RS_SERVICE_TYPE_MSG,this) ;
}
#endif
void p3MsgService::connectToTurtleRouter(p3turtle *pt)
{
	mTurtle = pt ;
	pt->registerTunnelService(this) ;
}

bool p3MsgService::createDistantOfflineMessengingInvite(time_t time_of_validity,DistantMsgPeerId& peer_id)
{
    std::cerr << __PRETTY_FUNCTION__ << ": disabled for now" << std::endl;

//    peer_id = DistantMsgPeerId::random();
//    TurtleFileHash hash = rsdir::sha1sum(peer_id.toByteArray(),DistantMsgPeerId::SIZE_IN_BYTES) ;
//
//	DistantMessengingInvite invite ;
//    invite.time_of_validity = time_of_validity + time(NULL);
//
//
//	{
//		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
//        _messenging_invites[hash] = invite ;
//	}
//	IndicateConfigChanged() ;

	return true ;
}

void p3MsgService::enableDistantMessaging(bool b)
{
	// compute the hash
	
    Sha1CheckSum hash ;
	
    if(!getDistantMessageHash(AuthGPG::getAuthGPG()->getGPGOwnId(),hash))
		return ;

	bool cchanged = false ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << (b?"Enabling":"Disabling") << " distant messaging, with hash  = " << hash << std::endl;
#endif
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		std::map<Sha1CheckSum,DistantMessengingInvite>::iterator it = _messenging_invites.find(hash) ;

		if(b && it == _messenging_invites.end())
		{
			DistantMessengingInvite invite ;
			invite.time_of_validity = time(NULL) + 10*365*86400;	// 10 years from now
			_messenging_invites[hash] = invite ;
			mDistantMessagingEnabled = true ;
#ifdef GROUTER
			std::cerr << "Notifying the global router." << std::endl;

			// Debug stuff. Create a random key and register it.
			const RsPeerId& own_ssl_id = rsPeers->getOwnId() ;
			const RsPgpId& gpg_id = rsPeers->getGPGOwnId() ;

			RsPeerDetails d;
            rsPeers->getGPGDetails(gpg_id,d) ;
			PGPFingerprintType fingerp( d.fpr ) ;

			// Re-hash the SSL id, to make it one way. Will be replaced by proper invitations in the future.
			//
            GRouterKeyId key ( RsDirUtil::sha1sum( own_ssl_id.toByteArray(),own_ssl_id.SIZE_IN_BYTES ) ) ;

			static GRouterServiceId client_id = GROUTER_CLIENT_ID_MESSAGES;
			static std::string description = "Test string for debug purpose" ;

			mGRouter->registerKey(key,fingerp,client_id,description) ;
#endif
			cchanged = true ;
		}
		if((!b) && it != _messenging_invites.end())
		{
			_messenging_invites.erase(it) ;
			mDistantMessagingEnabled = false ;
#ifdef GROUTER
			mGRouter->unregisterKey(GRouterKeyId(hash)) ;
#endif
			cchanged = true ;
		}
	}

	if(cchanged)
		IndicateConfigChanged() ;

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "List of distant message invites: " << std::endl;
	for(std::map<Sha1CheckSum,DistantMessengingInvite>::const_iterator it(_messenging_invites.begin());it!=_messenging_invites.end();++it)
		std::cerr << "   hash = " << it->first << std::endl;
#endif
}
bool p3MsgService::distantMessagingEnabled()
{
	// compute the hash
	
	Sha1CheckSum hash ;
	
	if(!getDistantMessageHash(AuthGPG::getAuthGPG()->getGPGOwnId(),hash))
		return false;

	bool res ;
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
		res = _messenging_invites.find(hash) != _messenging_invites.end() ;
	}
	return res ;
}
bool p3MsgService::getDistantMessagePeerId(const RsPgpId& pgp_id,DistantMsgPeerId& peer_id)
{
    if(!AuthGPG::getAuthGPG()->isKeySupported(pgp_id))
        return false ;

    unsigned char tmp[RsPeerId::SIZE_IN_BYTES] ;
    memset(tmp,0,RsPeerId::SIZE_IN_BYTES) ;
    memcpy(tmp,pgp_id.toByteArray(),RsPgpId::SIZE_IN_BYTES) ;

    peer_id = RsPeerId(tmp) ;

	// Also check that we have the public key.
	
    return true ;
}
bool p3MsgService::getDistantMessageHash(const RsPgpId& pgp_id,Sha1CheckSum& hash)
{
    if(!AuthGPG::getAuthGPG()->isKeySupported(pgp_id))
        return false ;

    hash = RsDirUtil::sha1sum(pgp_id.toByteArray(),RsPgpId::SIZE_IN_BYTES) ;

    // Also check that we have the public key.

    return true ;
}
bool p3MsgService::getDistantOfflineMessengingInvites(std::vector<DistantOfflineMessengingInvite>& invites) 
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	for(std::map<Sha1CheckSum,DistantMessengingInvite>::const_iterator it(_messenging_invites.begin());it!=_messenging_invites.end();++it)
	{
		DistantOfflineMessengingInvite invite ;
		invite.hash = it->first ;
		invite.issuer_pgp_id = AuthGPG::getAuthGPG()->getGPGOwnId() ;
		invite.time_of_validity = it->second.time_of_validity ;

		invites.push_back(invite) ;

#ifdef DEBUG_DISTANT_MSG
		std::cerr << "   adding invite with hash " << invite.hash << std::endl;
#endif
	}
	return true ;
}

bool p3MsgService::handleTunnelRequest(const Sha1CheckSum &hash,const RsPeerId& /*peer_id*/)
{

	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
    std::map<Sha1CheckSum,DistantMessengingInvite>::const_iterator it = _messenging_invites.find(hash) ;

#ifdef DEBUG_DISTANT_MSG
	if(it != _messenging_invites.end())
	{
		std::cerr << "p3MsgService::handleTunnelRequest: received TR for hash " << hash << std::endl;
		std::cerr << "Responding OK!" << std::endl;
	}
#endif

	return it != _messenging_invites.end() ;
}

void p3MsgService::manageDistantPeers()
{
	// now possibly flush pending messages

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "p3MsgService::manageDistantPeers()" << std::endl;
#endif
	time_t now = time(NULL) ;
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		// clean dead invites.
		//
		for(std::map<Sha1CheckSum,DistantMessengingInvite>::iterator it(_messenging_invites.begin());it!=_messenging_invites.end();)
			if(it->second.time_of_validity < now)
			{
#ifdef DEBUG_DISTANT_MSG
				std::cerr << "    Removing outdated invite " << it->second.time_of_validity << ", hash=" << it->first << std::endl;
#endif
				std::map<Sha1CheckSum,DistantMessengingInvite>::iterator tmp(it) ;
				++tmp ;
				_messenging_invites.erase(it) ;
				it = tmp ;
			}
			else
				++it ;

//		// clean dead contacts.
//		//
//		for(std::map<Sha1CheckSum,DistantMessengingContact>::iterator it(_messenging_contacts.begin());it!=_messenging_contacts.end();)
//			if((!it->second.pending_messages) && it->second.status == RS_DISTANT_MSG_STATUS_TUNNEL_DN)
//			{
//#ifdef DEBUG_DISTANT_MSG
//				std::cerr << "    Removing dead contact with no pending msgs and dead tunnel. hash=" << it->first << std::endl;
//#endif
//				std::map<Sha1CheckSum,DistantMessengingContact>::iterator tmp(it) ;
//				++tmp ;
//				_messenging_contacts.erase(it) ;
//				it = tmp ;
//			}
//			else
//				++it ;
	}
}

// void p3MsgService::addVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& vpid,RsTurtleGenericTunnelItem::Direction /*dir*/)
// {
// 	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
// 	// A new tunnel has been created. We need to flush pending messages for the corresponding peer.
// 
// 	//std::map<Sha1CheckSum,DistantMessengingContact>::const_iterator it = _messenging_contacts.find(hash) ;
// 
// 	DistantMessengingContact& contact(_messenging_contacts[hash]) ; // possibly creates it.
// 
// 	contact.virtual_peer_id = vpid ;
// 	contact.last_hit_time = time(NULL) ;
// 	contact.status = RS_DISTANT_MSG_STATUS_TUNNEL_OK ;
// 
// #ifdef DEBUG_DISTANT_MSG
// 	std::cerr << "p3MsgService::addVirtualPeer(): adding virtual peer " << vpid << " for hash " << hash << std::endl;
// #endif
// }
// void p3MsgService::removeVirtualPeer(const TurtleFileHash& hash, const TurtleVirtualPeerId& vpid)
// {
// #ifdef DEBUG_DISTANT_MSG
// 	std::cerr << "Removing virtual peer " << vpid << " for hash " << hash << std::endl;
// #endif
// 
// 	bool remove_tunnel = false ;
// 	{
// 		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
// 
// 		DistantMessengingContact& contact(_messenging_contacts[hash]) ; // possibly creates it.
// 
// 		contact.status = RS_DISTANT_MSG_STATUS_TUNNEL_DN ;
// 		contact.virtual_peer_id.clear() ;
// 
// 		if(!contact.pending_messages)
// 			remove_tunnel = true ;
// 	}
// 
// 	if(remove_tunnel)	// We do that whenever we're client or server. But normally, we should only do it when we're client.
// 	{
// #ifdef DEBUG_DISTANT_MSG
// 		std::cerr << "Also removing tunnel, since pending messages have been sent." << std::endl;
// #endif
// 		mTurtle->stopMonitoringTunnels(hash) ;
// 	}
// }

#ifdef DEBUG_DISTANT_MSG
static void printBinaryData(void *data,uint32_t size)
{
	static const char outl[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' } ;

	for(uint32_t j = 0; j < size; j++) 
	{ 
		std::cerr << outl[ ( ((uint8_t*)data)[j]>>4) ] ; 
		std::cerr << outl[ ((uint8_t*)data)[j] & 0xf ] ; 
	}
}
#endif

void p3MsgService::sendTurtleData(const Sha1CheckSum& hash,RsMsgItem *msgitem) 
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

	RsTurtleGenericDataItem *item = new RsTurtleGenericDataItem ;

	item->data_bytes = malloc(msg_serialized_rssize) ;
	item->data_size = msg_serialized_rssize ;
	memcpy(item->data_bytes,msg_serialized_data,msg_serialized_rssize) ;

	delete[] msg_serialized_data ;

#ifdef DEBUG_DISTANT_MSG
	printBinaryData(item->data_bytes,item->data_size) ;
	std::cerr << std::endl;
#endif

	// do we have a working tunnel for that hash ?
	// If not, put on the contact's waiting list.

    RsPeerId virtual_peer_id ;
    {
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
		std::map<Sha1CheckSum,DistantMessengingContact>::const_iterator it = _messenging_contacts.find(hash) ;

		if(it == _messenging_contacts.end())
		{
			std::cerr << "(EE) p3MsgService::sendTurtleData(): Can't find hash " << hash << " in recorded contact list." << std::endl;
			delete[] msg_serialized_data ;
			return ;
		}

		if(!it->second.status == RS_DISTANT_MSG_STATUS_TUNNEL_OK)
		{
			std::cerr << "p3MsgService::sendTurtleData(): tunnel is not ok. Putting items on waiting list." << std::endl;
			return ;
		}
		virtual_peer_id = it->second.virtual_peer_id ;
	}
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "p3MsgService::sendTurtleData(): Sending through virtual peer: " << virtual_peer_id << std::endl;
	std::cerr << "   item->data_size = " << item->data_size << std::endl;
	std::cerr << "   data = " ;
#endif

	mTurtle->sendTurtleData(virtual_peer_id,item) ;
}
//void p3MsgService::receiveTurtleData(RsTurtleGenericTunnelItem *gitem,const Sha1CheckSum& hash,
//												const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction /*direction*/)
//{
//	RsTurtleGenericDataItem *item = dynamic_cast<RsTurtleGenericDataItem*>(gitem) ;
//
//	if(item == NULL)
//	{
//		std::cerr << "(EE) p3MsgService::receiveTurtleData(): item is not a data item. That is an error." << std::endl;
//		return ;
//	}
//#ifdef DISABLE_DISTANT_MESSAGES
//	std::cerr << "Received distant message item. Protocol is not yet finalized. Droping the item." << std::endl;
//	delete item ;
//	return ;
//#endif
//
//#ifdef DEBUG_DISTANT_MSG
//	std::cerr << "p3MsgService::sendTurtleData(): Receiving through virtual peer: " << virtual_peer_id << std::endl;
//	std::cerr << "   gitem->data_size = " << item->data_size << std::endl;
//	std::cerr << "   data = " ;
//	printBinaryData(item->data_bytes,item->data_size) ;
//	std::cerr << std::endl;
//#else
//	(void) virtual_peer_id;
//#endif
//
//	{
//		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
//		std::map<Sha1CheckSum,DistantMessengingContact>::iterator it = _messenging_contacts.find(hash) ;
//
//		if(it == _messenging_contacts.end())
//		{
//			std::cerr << "(EE) p3MsgService::sendTurtleData(): Can't find hash " << hash << " in recorded contact list." << std::endl;
//			return ;
//		}
//
//		it->second.status = RS_DISTANT_MSG_STATUS_TUNNEL_OK ;
//		it->second.last_hit_time = time(NULL) ;
//	}
//		
//	RsItem *itm = _serialiser->deserialise(item->data_bytes,&item->data_size) ;
//	RsMsgItem *mitm = dynamic_cast<RsMsgItem*>(itm) ;
//
//	if(mitm != NULL)
//	{
//		mitm->PeerId(hash) ;
//		mitm->msgto.ids.push_back(rsPeers->getGPGOwnId()) ;
//		handleIncomingItem(mitm) ;
//	}
//	else
//	{
//		std::cerr << "(EE) p3MsgService::receiveTurtleData(): received item is not a RsMsgItem!!" << std::endl;
//		delete itm ;
//	}
//}
#ifdef GROUTER
void p3MsgService::sendGRouterData(const Sha1CheckSum& hash,RsMsgItem *msgitem) 
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

	mGRouter->sendData(GRouterKeyId(hash),item) ;
}
void p3MsgService::receiveGRouterData(RsGRouterGenericDataItem *gitem,const GRouterKeyId& key)
{
	std::cerr << "(WW) p3msgservice::receiveGRouterData(): received message. Not handled yet. Needs implementing." << std::endl; 
}
#endif

bool p3MsgService::locked_findHashForVirtualPeerId(const RsPeerId& vpid,Sha1CheckSum& hash)
{
	for(std::map<Sha1CheckSum,DistantMessengingContact>::const_iterator it = _messenging_contacts.begin(); it!=_messenging_contacts.end();++it)
		if(it->second.virtual_peer_id == vpid)
		{
			hash = it->first ;
			return true ;
		}
	return false ;
}

void p3MsgService::sendPrivateMsgItem(const Sha1CheckSum& hash,RsMsgItem *msgitem) 
{
#ifdef DEBUG_DISTANT_MSG
	std::cerr << "p3MsgService::sendDistanteMsgItem(): sending distant msg item to peer " << msgitem->PeerId() << std::endl;
#endif
	{
		RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

		// allocate a new contact. If it does not exist, set its tunnel state to DN
		//
		std::map<Sha1CheckSum,DistantMessengingContact>::iterator it = _messenging_contacts.find(hash) ;
		
		if(it == _messenging_contacts.end())
		{
			std::cerr << "(EE) p3MsgService::sendPrivateMsgItem(): ERROR: no tunnel for message to send. This should not happen. " << std::endl;
			return ;
		}

		if(it->second.status != RS_DISTANT_MSG_STATUS_TUNNEL_OK)
		{
			std::cerr << "(WW) p3MsgService::sendPrivateMsgItem(): WARNING: no tunnel for message to send. This should not happen. " << std::endl;
			return ;
		}

		if(!it->second.pending_messages)
			std::cerr << "(WW) p3MsgService::sendPrivateMsgItem(): WARNING: no pending message flag. This should not happen. " << std::endl;
	}

#ifdef DEBUG_DISTANT_MSG
	std::cerr << "    Flushing msg " << msgitem->msgId << " for peer id " << msgitem->PeerId() << std::endl;
#endif
#ifdef GROUTER
	sendGRouterData(hash,msgitem) ;
#else
	sendTurtleData(hash,msgitem) ;
#endif
}




