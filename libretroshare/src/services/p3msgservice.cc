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

#include "pqi/pqibin.h"
#include "pqi/pqiarchive.h"
#include "pqi/p3linkmgr.h"

#include "services/p3msgservice.h"
#include "pqi/pqinotify.h"

#include "util/rsdebug.h"
#include "util/rsdir.h"

#include <sstream>
#include <iomanip>
#include <map>

const int msgservicezone = 54319;

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


p3MsgService::p3MsgService(p3LinkMgr *lm)
	:p3Service(RS_SERVICE_TYPE_MSG), p3Config(CONFIG_TYPE_MSGS),
	mLinkMgr(lm), mMsgMtx("p3MsgService"), msgChanged(1), mMsgUniqueId(1)
{
	addSerialType(new RsMsgSerialiser());

        /* Initialize standard tag types */
        if(lm)
            initStandardTagTypes();
}

uint32_t p3MsgService::getNewUniqueMsgId()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
	return mMsgUniqueId++;
}

/****** Mods/Notifications ****/

bool	p3MsgService::MsgsChanged()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	bool m1 = msgChanged.Changed();

	return (m1);
}

bool	p3MsgService::MsgNotifications()
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/
	return (msgNotifications.size() > 0);
}

bool    p3MsgService::getMessageNotifications(std::list<MsgInfoSummary> &noteList)
{
	RsStackMutex stack(mMsgMtx); /********** STACK LOCKED MTX ******/

	noteList = msgNotifications;
	msgNotifications.clear();
	
	return (noteList.size() > 0);
}



int	p3MsgService::tick()
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::tick()");

	/* don't worry about increasing tick rate! 
	 * (handled by p3service)
	 */

	incomingMsgs(); 
	//checkOutgoingMessages(); 

	return 0;
}


int	p3MsgService::status()
{
	pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
		"p3MsgService::status()");

	return 1;
}

void p3MsgService::processMsg(RsMsgItem *mi)
{
	mi -> recvTime = time(NULL);
	mi -> msgId = getNewUniqueMsgId();

	std::string mesg;

	RsStackMutex stack(mMsgMtx); /*** STACK LOCKED MTX ***/

	if (mi -> PeerId() == mLinkMgr->getOwnId())
	{
		/* from the loopback device */
		mi -> msgFlags |= RS_MSG_FLAGS_OUTGOING;
	}
	else
	{
		mi -> msgFlags = RS_MSG_FLAGS_NEW;

		/* from a peer */
		MsgInfoSummary mis;
		initRsMIS(mi, mis);

		// msgNotifications.push_back(mis);
		pqiNotify *notify = getPqiNotify();
		if (notify)
		{
			std::string message , title;
			notify->AddPopupMessage(RS_POPUP_MSG, mi->PeerId(),
									title.assign(mi->subject.begin(), mi->subject.end()),
									message.assign(mi->message.begin(),mi->message.end()));

			std::ostringstream out;
			out << mi->msgId;
			notify->AddFeedItem(RS_FEED_ITEM_MESSAGE, out.str(), "", "");
		}
	}

	imsg[mi->msgId] = mi;
	RsMsgSrcId* msi = new RsMsgSrcId();
	msi->msgId = mi->msgId;
	msi->srcId = mi->PeerId();
	mSrcIds.insert(std::pair<uint32_t, RsMsgSrcId*>(msi->msgId, msi));
	msgChanged.IndicateChanged();
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	/**** STACK UNLOCKED ***/
}

int p3MsgService::incomingMsgs()
{
	RsMsgItem *mi;
	int i = 0;
	bool changed = false ;

	while((mi = (RsMsgItem *) recvItem()) != NULL)
	{
		changed = true ;
		++i;

		processMsg(mi);
	}
	if(changed)
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);

	return 1;
}

void    p3MsgService::statusChange(const std::list<pqipeer> &/*plist*/)
{
	/* should do it properly! */
	checkOutgoingMessages();
}

int     p3MsgService::checkOutgoingMessages()
{
	/* iterate through the outgoing queue 
	 *
	 * if online, send
	 */

	bool changed = false ;

	{
		const std::string ownId = mLinkMgr->getOwnId();

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
			std::string pid = mit->second->PeerId();
			bool toSend = false;

			if (mLinkMgr->isOnline(pid))
			{
				toSend = true;
			}
			else if (pid == ownId) /* FEEDBACK Msg to Ourselves */
			{
				toSend = true;
			}

			if (toSend)
			{
				/* send msg */
				pqioutput(PQL_DEBUG_BASIC, msgservicezone, 
					"p3MsgService::checkOutGoingMessages() Sending out message");
				/* remove the pending flag */
				(mit->second)->msgFlags &= ~RS_MSG_FLAGS_PENDING;

				sendItem(mit->second);
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

	if(changed)
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);

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
	std::string ownId = mLinkMgr->getOwnId();

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

    std::list<RsMsgItem*> items;
    std::list<RsItem*>::iterator it;
    std::map<uint32_t, RsMsgTagType*>::iterator tagIt;
	std::map<uint32_t, std::string> srcIdMsgMap;
	std::map<uint32_t, std::string>::iterator srcIt;


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
			srcIdMsgMap.insert(std::pair<uint32_t, std::string>(msi->msgId, msi->srcId));
			mSrcIds.insert(std::pair<uint32_t, RsMsgSrcId*>(msi->msgId, msi)); // does not need to be kept
		}
		else if(NULL != (msp = dynamic_cast<RsMsgParentId *>(*it)))
		{
			mParentId.insert(std::pair<uint32_t, RsMsgParentId*>(msp->msgId, msp));
		}
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

	//msg -> PeerId(mLinkMgr->getOwnId());

	msg -> sendTime = time(NULL);
	msg -> recvTime = time(NULL);
	msg -> msgFlags = RS_MSG_FLAGS_NEW;

	msg -> subject = L"Welcome to Retroshare";

	msg -> message  = L"Send and receive messages with your friends...\n";
	msg -> message += L"These can hold recommendations from your local shared files.\n\n";
	msg -> message += L"Add recommendations through the Local Files Dialog.\n\n";
	msg -> message += L"Enjoy.";

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

		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
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
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
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
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
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

	std::ostringstream out;
	out << mit->second->msgParentId;
	msgParentId = out.str();
	
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
				msp->PeerId (mLinkMgr->getOwnId());
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

		if (item->PeerId() != mLinkMgr->getOwnId()) {
			/* not to the loopback device */
			RsMsgSrcId* msi = new RsMsgSrcId();
			msi->msgId = item->msgId;
			msi->srcId = item->PeerId();
			mSrcIds.insert(std::pair<uint32_t, RsMsgSrcId*>(msi->msgId, msi));
		}
	}

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGELIST, NOTIFY_TYPE_ADD);

	checkOutgoingMessages();

	return 1;
}

bool 	p3MsgService::MessageSend(MessageInfo &info)
{
	std::list<std::string>::const_iterator pit;
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
	RsMsgItem *msg = initMIRsMsg(info, mLinkMgr->getOwnId());
	if (msg)
	{
		/* use processMsg to get the new msgId */
//		sendMessage(msg);
		processMsg(msg);

		// return new message id
		std::ostringstream out;
		out << msg->msgId;
		info.msgId = out.str();
	}

	return true;
}

bool p3MsgService::MessageToDraft(MessageInfo &info, const std::string &msgParentId)
{
    RsMsgItem *msg = initMIRsMsg(info, mLinkMgr->getOwnId());
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
            std::ostringstream out;
            out << msg->msgId;
            info.msgId = out.str();
        }

        setMsgParentId(msg->msgId, atoi(msgParentId.c_str()));

        IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);

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
			tagType->PeerId (mLinkMgr->getOwnId());
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

		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGE_TAGS, nNotifyType);
		
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

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGE_TAGS, NOTIFY_TYPE_DEL);

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
		std::ostringstream out;
		out << mit->second->msgId;

		info.msgId = out.str();
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
				tag->PeerId (mLinkMgr->getOwnId());

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

		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGE_TAGS, nNotifyType);

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

        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_MESSAGELIST,NOTIFY_TYPE_MOD);
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
	   || (msg->PeerId() == mLinkMgr->getOwnId()))
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

	mi.ts = msg->sendTime;
	mi.srcId = msg->PeerId();
	{
		//msg->msgId;
		std::ostringstream out;
		out << msg->msgId;
		mi.msgId = out.str();
	}

	std::list<std::string>::iterator pit;

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

	/* translate flags, if we sent it... outgoing */
	if ((msg->msgFlags & RS_MSG_FLAGS_OUTGOING)
	   || (msg->PeerId() == mLinkMgr->getOwnId()))
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

	mis.srcId = msg->PeerId();
	{
		//msg->msgId;
		std::ostringstream out;
		out << msg->msgId;
		mis.msgId = out.str();
	}

	mis.title = msg->subject;
	mis.count = msg->attachment.items.size();
	mis.ts = msg->sendTime;
}

RsMsgItem *p3MsgService::initMIRsMsg(MessageInfo &info, std::string to)
{
	RsMsgItem *msg = new RsMsgItem();

	msg -> PeerId(to);

	msg -> msgFlags = 0;
	msg -> msgId = 0;
	msg -> sendTime = time(NULL);
	msg -> recvTime = 0;
	
	msg -> subject = info.title;
	msg -> message = info.msg;

	std::list<std::string>::iterator pit;
	for(pit = info.msgto.begin(); pit != info.msgto.end(); pit++)
	{
		msg -> msgto.ids.push_back(*pit);
	}

	for(pit = info.msgcc.begin(); pit != info.msgcc.end(); pit++)
	{
		msg -> msgcc.ids.push_back(*pit);
	}

	/* We don't fill in bcc (unless to ourselves) */
	if (to == mLinkMgr->getOwnId())
	{
		for(pit = info.msgbcc.begin(); pit != info.msgbcc.end(); pit++)
		{
			msg -> msgbcc.ids.push_back(*pit);
		}
	}

	msg -> attachment.title   = info.attach_title;
	msg -> attachment.comment = info.attach_comment;

	std::list<FileInfo>::iterator it;
	for(it = info.files.begin(); it != info.files.end(); it++)
	{
		RsTlvFileItem mfi;
		mfi.hash = it -> hash;
		mfi.name = it -> fname;
		mfi.filesize = it -> size;
		msg -> attachment.items.push_back(mfi);
	}

	//std::cerr << "p3MsgService::initMIRsMsg()" << std::endl;
	//msg->print(std::cerr);
	return msg;
}
