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

#include "feeds/ChanNewItem.h"
#include "feeds/ChanMsgItem.h"
#include "feeds/ForumNewItem.h"
#include "feeds/ForumMsgItem.h"

#ifdef BLOGS
#include "feeds/BlogNewItem.h"
#include "feeds/BlogMsgItem.h"
#endif

#include "feeds/MsgItem.h"
#include "feeds/PeerItem.h"
#include "feeds/ChatMsgItem.h"

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

/*****
 * #define NEWS_DEBUG  1
 ****/

/** Constructor */
NewsFeed::NewsFeed(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);


	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateFeed()));
	timer->start(1000);

}



void NewsFeed::updateFeed()
{
	if (!rsNotify)
		return;

	uint flags = Settings->getNewsFeedFlags();

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

void	NewsFeed::addFeedItemPeerConnect(RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_CONNECT, false);

	/* store */

	/* add to layout */
	verticalLayout->insertWidget(0, pi); 

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
	verticalLayout->insertWidget(0, pi); 



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
	verticalLayout->insertWidget(0, pi); 


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
	verticalLayout->insertWidget(0, pi); 

#ifdef NEWS_DEBUG
	std::cerr << "NewsFeed::addFeedItemPeerNew()";
	std::cerr << std::endl;
#endif
}


void	NewsFeed::addFeedItemChanNew(RsFeedItem &fi)
{
	/* make new widget */
	ChanNewItem *cni = new ChanNewItem(this, NEWSFEED_CHANNEWLIST, fi.mId1, false, true);

	/* store in list */

	/* add to layout */
	verticalLayout->insertWidget(0, cni);

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
	verticalLayout->insertWidget(0, cni); 

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
	verticalLayout->insertWidget(0, cm);

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
	verticalLayout->insertWidget(0, fni);


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
	verticalLayout->insertWidget(0, fni);

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
	verticalLayout->insertWidget(0, fm);

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
	verticalLayout->insertWidget(0, bni);
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
	verticalLayout->insertWidget(0, bm);
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

	/* make new widget */
	ChatMsgItem *cm = new ChatMsgItem(this, NEWSFEED_CHATMSGLIST, fi.mId1, fi.mId2, true);

	/* store in forum list */

	/* add to layout */
	verticalLayout->insertWidget(0, cm);
}

void	NewsFeed::addFeedItemMessage(RsFeedItem &fi)
{
	/* make new widget */
	MsgItem *mi = new MsgItem(this, NEWSFEED_MESSAGELIST, fi.mId1, false);

	/* store in list */

	/* add to layout */
	verticalLayout->insertWidget(0, mi);

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
