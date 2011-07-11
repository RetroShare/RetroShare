/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QTimer>

#include "NewsFeed.h"

#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>

#include "feeds/ChanNewItem.h"
#include "feeds/ChanMsgItem.h"
#include "feeds/ForumNewItem.h"
#include "feeds/ForumMsgItem.h"
#include "settings/rsettingswin.h"

#ifdef BLOGS
#include "feeds/BlogNewItem.h"
#include "feeds/BlogMsgItem.h"
#endif

#include "feeds/MsgItem.h"
#include "feeds/PeerItem.h"
#include "feeds/ChatMsgItem.h"

#include "feeds/SecurityItem.h"

#include "settings/rsharesettings.h"
#include "chat/PopupChatDialog.h"

const uint32_t NEWSFEED_PEERLIST = 	0x0001;
const uint32_t NEWSFEED_FORUMNEWLIST = 	0x0002;
const uint32_t NEWSFEED_FORUMMSGLIST = 	0x0003;
const uint32_t NEWSFEED_CHANNEWLIST = 	0x0004;
const uint32_t NEWSFEED_CHANMSGLIST = 	0x0005;
const uint32_t NEWSFEED_BLOGNEWLIST = 	0x0006;
const uint32_t NEWSFEED_BLOGMSGLIST = 	0x0007;
const uint32_t NEWSFEED_MESSAGELIST = 	0x0008;
const uint32_t NEWSFEED_CHATMSGLIST = 	0x0009;
const uint32_t NEWSFEED_SECLIST = 	0x000a;

/*****
 * #define NEWS_DEBUG  1
 ****/

/** Constructor */
NewsFeed::NewsFeed(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

	connect(removeAllButton, SIGNAL(clicked()), this, SLOT(removeAll()));
	connect(feedOptionsButton, SIGNAL(clicked()), this, SLOT(feedoptions()));
	

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateFeed()));
	timer->start(1000);
}



void NewsFeed::updateFeed()
{
	if (!rsNotify)
		return;

	uint flags = Settings->getNewsFeedFlags();

	/* HACK until SECURITY is in feeds */
	flags |= RS_FEED_TYPE_SECURITY;

	/* check for new messages */
	RsFeedItem fi;
	if (rsNotify->GetFeedItem(fi))
	{
		switch(fi.mType)
		{
			case RS_FEED_ITEM_PEER_CONNECT:
				if (flags & RS_FEED_TYPE_PEER)
					addFeedItemPeerConnect(fi);
				break;
			case RS_FEED_ITEM_PEER_DISCONNECT:
				if (flags & RS_FEED_TYPE_PEER)
					addFeedItemPeerDisconnect(fi);
				break;
			case RS_FEED_ITEM_PEER_NEW:
				if (flags & RS_FEED_TYPE_PEER)
					addFeedItemPeerNew(fi);
				break;
			case RS_FEED_ITEM_PEER_HELLO:
				if (flags & RS_FEED_TYPE_PEER)
					addFeedItemPeerHello(fi);
				break;

			case RS_FEED_ITEM_SEC_CONNECT_ATTEMPT:
				if (flags & RS_FEED_TYPE_SECURITY)
					addFeedItemSecurityConnectAttempt(fi);
				break;
			case RS_FEED_ITEM_SEC_AUTH_DENIED:
				if (flags & RS_FEED_TYPE_SECURITY)
					addFeedItemSecurityAuthDenied(fi);
				break;
			case RS_FEED_ITEM_SEC_UNKNOWN_IN:
				if (flags & RS_FEED_TYPE_SECURITY)
					addFeedItemSecurityUnknownIn(fi);
				break;
			case RS_FEED_ITEM_SEC_UNKNOWN_OUT:
				if (flags & RS_FEED_TYPE_SECURITY)
					addFeedItemSecurityUnknownOut(fi);
				break;

			case RS_FEED_ITEM_CHAN_NEW:
				if (flags & RS_FEED_TYPE_CHAN)
					addFeedItemChanNew(fi);
				break;
			case RS_FEED_ITEM_CHAN_UPDATE:
				if (flags & RS_FEED_TYPE_CHAN)
					addFeedItemChanUpdate(fi);
				break;
			case RS_FEED_ITEM_CHAN_MSG:
				if (flags & RS_FEED_TYPE_CHAN)
					addFeedItemChanMsg(fi);
				break;

			case RS_FEED_ITEM_FORUM_NEW:
				if (flags & RS_FEED_TYPE_FORUM)
					addFeedItemForumNew(fi);
				break;
			case RS_FEED_ITEM_FORUM_UPDATE:
				if (flags & RS_FEED_TYPE_FORUM)
					addFeedItemForumUpdate(fi);
				break;
			case RS_FEED_ITEM_FORUM_MSG:
				if (flags & RS_FEED_TYPE_FORUM)
					addFeedItemForumMsg(fi);
				break;

			case RS_FEED_ITEM_BLOG_NEW:
				if (flags & RS_FEED_TYPE_BLOG)
					addFeedItemBlogNew(fi);
				break;
			case RS_FEED_ITEM_BLOG_MSG:
				if (flags & RS_FEED_TYPE_BLOG)
					addFeedItemBlogMsg(fi);
				break;
			case RS_FEED_ITEM_CHAT_NEW:
				if (flags & RS_FEED_TYPE_CHAT)
					addFeedItemChatNew(fi);
				break;
			case RS_FEED_ITEM_MESSAGE:
				if (flags & RS_FEED_TYPE_MSG)
					addFeedItemMessage(fi);
				break;
			case RS_FEED_ITEM_FILES_NEW:
				if (flags & RS_FEED_TYPE_FILES)
					addFeedItemFilesNew(fi);
				break;
			default:
				break;
		}
	}
}

void NewsFeed::addFeedItem(QWidget *item)
{
	item->setAttribute(Qt::WA_DeleteOnClose, true);

	connect(item, SIGNAL(destroyed(QObject*)), this, SLOT(itemDestroyed(QObject*)));
	widgetList.push_back(item);

	sendNewsFeedChanged();

	if (Settings->getAddFeedsAtEnd()) {
		verticalLayout->addWidget(item);
	} else {
		verticalLayout->insertWidget(0, item);
	}
}

void NewsFeed::addFeedItemIfUnique(QWidget *item, int itemType, std::string sslId, bool replace)
{
	QObjectList::iterator it;
	for (it = widgetList.begin(); it != widgetList.end(); it++) 
	{
		SecurityItem *secitem = dynamic_cast<SecurityItem*>(*it);
		if ((secitem) && (secitem->isSame(sslId, itemType)))
		{
			if (!replace)
			{	
				delete item;
				return;
			}
			else
			{
				secitem->close();
				break;
			}
		}
	}
		
	addFeedItem(item);
}

void	NewsFeed::addFeedItemPeerConnect(RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_CONNECT, false);

	/* store */

	/* add to layout */
	addFeedItem(pi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerConnect()";
	std::cerr << std::endl;
#endif

}


void	NewsFeed::addFeedItemPeerDisconnect(RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_STD, false);

	/* store */

	/* add to layout */
	addFeedItem(pi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerDisconnect()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemPeerHello(RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_HELLO, false);

	/* store */

	/* add to layout */
	addFeedItem(pi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerHello()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemPeerNew(RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_NEW_FOF, false);

	/* store */

	/* add to layout */
	addFeedItem(pi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerNew()";
	std::cerr << std::endl;
#endif
}



void	NewsFeed::addFeedItemSecurityConnectAttempt(RsFeedItem &fi)
{
	/* make new widget */
	SecurityItem *pi = new SecurityItem(this, NEWSFEED_SECLIST, fi.mId1, fi.mId2, SEC_TYPE_CONNECT_ATTEMPT, false);

	/* store */

	/* add to layout */
	addFeedItemIfUnique(pi, SEC_TYPE_CONNECT_ATTEMPT, fi.mId2, false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityConnectAttempt()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemSecurityAuthDenied(RsFeedItem &fi)
{
	/* make new widget */
	SecurityItem *pi = new SecurityItem(this, NEWSFEED_SECLIST, fi.mId1, fi.mId2, SEC_TYPE_AUTH_DENIED, false);

	/* store */

	/* add to layout */
	addFeedItemIfUnique(pi, SEC_TYPE_AUTH_DENIED, fi.mId2, false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityAuthDenied()";
	std::cerr << std::endl;
#endif
}

void	NewsFeed::addFeedItemSecurityUnknownIn(RsFeedItem &fi)
{
	/* make new widget */
	SecurityItem *pi = new SecurityItem(this, NEWSFEED_SECLIST, fi.mId1, fi.mId2, SEC_TYPE_UNKNOWN_IN, false);

	/* store */

	/* add to layout */
	addFeedItemIfUnique(pi, SEC_TYPE_UNKNOWN_IN, fi.mId2, false);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityUnknownIn()";
	std::cerr << std::endl;
#endif
}

void	NewsFeed::addFeedItemSecurityUnknownOut(RsFeedItem &fi)
{
	/* make new widget */
	SecurityItem *pi = new SecurityItem(this, NEWSFEED_SECLIST, fi.mId1, fi.mId2, SEC_TYPE_UNKNOWN_OUT, false);
	
	/* store */
	
	/* add to layout */
	addFeedItemIfUnique(pi, SEC_TYPE_UNKNOWN_OUT, fi.mId2, false);
	
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemSecurityUnknownOut()";
	std::cerr << std::endl;
#endif
}



void	NewsFeed::addFeedItemChanNew(RsFeedItem &fi)
{
	/* make new widget */
	ChanNewItem *cni = new ChanNewItem(this, NEWSFEED_CHANNEWLIST, fi.mId1, false, true);

	/* store in list */

	/* add to layout */
	addFeedItem(cni);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemChanNew()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemChanUpdate(RsFeedItem &fi)
{
	/* make new widget */
	ChanNewItem *cni = new ChanNewItem(this, NEWSFEED_CHANNEWLIST, fi.mId1, false, false);

	/* store in list */

	/* add to layout */
	addFeedItem(cni);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemChanUpdate()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemChanMsg(RsFeedItem &fi)
{
	/* make new widget */
	ChanMsgItem *cm = new ChanMsgItem(this, NEWSFEED_CHANMSGLIST, fi.mId1, fi.mId2, false);

	/* store in forum list */

	/* add to layout */
	addFeedItem(cm);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemChanMsg()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemForumNew(RsFeedItem &fi)
{
	/* make new widget */
	ForumNewItem *fni = new ForumNewItem(this, NEWSFEED_FORUMNEWLIST, fi.mId1, false, true);

	/* store in forum list */
	mForumNewItems.push_back(fni);

	/* add to layout */
	addFeedItem(fni);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemForumNew()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemForumUpdate(RsFeedItem &fi)
{
	/* make new widget */
	ForumNewItem *fni = new ForumNewItem(this, NEWSFEED_FORUMNEWLIST, fi.mId1, false, false);

	/* store in forum list */
	mForumNewItems.push_back(fni);

	/* add to layout */
	addFeedItem(fni);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemForumUpdate()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemForumMsg(RsFeedItem &fi)
{
	/* make new widget */
	ForumMsgItem *fm = new ForumMsgItem(this, NEWSFEED_FORUMMSGLIST, fi.mId1, fi.mId2, false);

	/* store in forum list */

	/* add to layout */
	addFeedItem(fm);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemForumMsg()";
	std::cerr << std::endl;
#endif
}

void	NewsFeed::addFeedItemBlogNew(RsFeedItem &fi)
{
#ifdef BLOGS
	/* make new widget */
	BlogNewItem *bni = new BlogNewItem(this, NEWSFEED_BLOGNEWLIST, fi.mId1, false, true);

	/* store in list */

	/* add to layout */
	addFeedItem(bni);
#endif

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemBlogNew()";
	std::cerr << std::endl;
#endif
}

void	NewsFeed::addFeedItemBlogMsg(RsFeedItem &fi)
{
#ifdef BLOGS
	/* make new widget */
	BlogMsgItem *bm = new BlogMsgItem(this, NEWSFEED_BLOGMSGLIST, fi.mId1, fi.mId2, false);

	/* store in forum list */

	/* add to layout */
	addFeedItem(bm);
#endif

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemBlogMsg()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemChatNew(RsFeedItem &fi)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemChatNew()";
	std::cerr << std::endl;
#endif

	if (fi.mId1 == rsPeers->getOwnId()) {
		/* chat message from myself */
		return;
	}

	/* make new widget */
	ChatMsgItem *cm = new ChatMsgItem(this, NEWSFEED_CHATMSGLIST, fi.mId1, fi.mId2, true);

	/* store in forum list */

	/* add to layout */
	addFeedItem(cm);
}

void	NewsFeed::addFeedItemMessage(RsFeedItem &fi)
{
	/* make new widget */
	MsgItem *mi = new MsgItem(this, NEWSFEED_MESSAGELIST, fi.mId1, false);

	/* store in list */

	/* add to layout */
	addFeedItem(mi);

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemMessage()";
	std::cerr << std::endl;
#endif
}

void	NewsFeed::addFeedItemFilesNew(RsFeedItem &fi)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemFilesNew()";
	std::cerr << std::endl;
#endif
}

/* FeedHolder Functions (for FeedItem functionality) */
void NewsFeed::deleteFeedItem(QWidget *item, uint32_t type)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::deleteFeedItem()";
	std::cerr << std::endl;
#endif

        if (item) {
            item->close ();
        }
}

void NewsFeed::openChat(std::string peerId)
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::openChat()";
	std::cerr << std::endl;
#endif

	PopupChatDialog::chatFriend(peerId);
}

void NewsFeed::itemDestroyed(QObject *item)
{
	int index = widgetList.indexOf(item);
	if (index >= 0) {
		widgetList.removeAt(index);
	}

	sendNewsFeedChanged();
}

void NewsFeed::removeAll()
{
#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::removeAll()" << std::endl;
#endif
	while (widgetList.count()) {
		QObject *item = widgetList.first();
		widgetList.pop_front();

		if (item) {
			item->deleteLater();
		}
	}
}

void NewsFeed::sendNewsFeedChanged()
{
	int count = 0;

	QObjectList::iterator it;
	for (it = widgetList.begin(); it != widgetList.end(); it++) {
		if (dynamic_cast<PeerItem*>(*it) == NULL) {
			/* don't count PeerItem's */
			count++;
		}
	}

	emit newsFeedChanged(count);
}

void NewsFeed::feedoptions()
{
    RSettingsWin::showYourself(this, RSettingsWin::Notify);
}
