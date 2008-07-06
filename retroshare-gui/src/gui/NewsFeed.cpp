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
#include <QtGui>

#include "NewsFeed.h"

#include <iostream>

#include "rsiface/rsnotify.h"

#include "feeds/ChanNewItem.h"
#include "feeds/ChanMsgItem.h"
#include "feeds/ForumNewItem.h"
#include "feeds/ForumMsgItem.h"
#include "feeds/PeerItem.h"
#include "feeds/BlogMsgItem.h"
#include "feeds/MsgItem.h"


#include "GeneralMsgDialog.h"


const uint32_t NEWSFEED_PEERLIST = 	0x0001;
const uint32_t NEWSFEED_FORUMNEWLIST = 	0x0002;
const uint32_t NEWSFEED_FORUMMSGLIST = 	0x0003;
const uint32_t NEWSFEED_CHANNEWLIST = 	0x0004;
const uint32_t NEWSFEED_CHANMSGLIST = 	0x0005;
const uint32_t NEWSFEED_BLOGMSGLIST = 	0x0006;
const uint32_t NEWSFEED_MESSAGELIST = 	0x0007;

/** Constructor */
NewsFeed::NewsFeed(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

	/* mLayout -> to add widgets to */
	mLayout = new QVBoxLayout;

	QWidget *middleWidget = new QWidget();
	//middleWidget->setSizePolicy( QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);
	middleWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	middleWidget->setLayout(mLayout);


     	QScrollArea *scrollArea = new QScrollArea;
        //scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(middleWidget);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QVBoxLayout *layout2 = new QVBoxLayout;
	layout2->addWidget(scrollArea);
	
     	frame->setLayout(layout2);

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateFeed()));
	timer->start(1000);

}




void NewsFeed::updateFeed()
{
	if (!rsNotify)
		return;

	/* check for new messages */
	RsFeedItem fi;
	if (rsNotify->GetFeedItem(fi))
	{
		switch(fi.mType)
		{
			case RS_FEED_ITEM_PEER_CONNECT:
				addFeedItemPeerConnect(fi);
				break;
			case RS_FEED_ITEM_PEER_DISCONNECT:
				addFeedItemPeerDisconnect(fi);
				break;
			case RS_FEED_ITEM_PEER_NEW:
				addFeedItemPeerNew(fi);
				break;
			case RS_FEED_ITEM_PEER_HELLO:
				addFeedItemPeerHello(fi);
				break;

			case RS_FEED_ITEM_CHAN_NEW:
				addFeedItemChanNew(fi);
				break;
			case RS_FEED_ITEM_CHAN_UPDATE:
				addFeedItemChanUpdate(fi);
				break;
			case RS_FEED_ITEM_CHAN_MSG:
				addFeedItemChanMsg(fi);
				break;

			case RS_FEED_ITEM_FORUM_NEW:
				addFeedItemForumNew(fi);
				break;
			case RS_FEED_ITEM_FORUM_UPDATE:
				addFeedItemForumUpdate(fi);
				break;
			case RS_FEED_ITEM_FORUM_MSG:
				addFeedItemForumMsg(fi);
				break;

			case RS_FEED_ITEM_BLOG_MSG:
				addFeedItemBlogMsg(fi);
				break;
			case RS_FEED_ITEM_CHAT_NEW:
				addFeedItemChatNew(fi);
				break;
			case RS_FEED_ITEM_MESSAGE:
				addFeedItemMessage(fi);
				break;
			case RS_FEED_ITEM_FILES_NEW:
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
	mLayout->addWidget(pi);

	std::cerr << "NewsFeed::addFeedItemPeerConnect()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemPeerDisconnect(RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_STD, false);

	/* store */

	/* add to layout */
	mLayout->addWidget(pi);



	std::cerr << "NewsFeed::addFeedItemPeerDisconnect()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemPeerHello(RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_HELLO, false);

	/* store */

	/* add to layout */
	mLayout->addWidget(pi);


	std::cerr << "NewsFeed::addFeedItemPeerHello()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemPeerNew(RsFeedItem &fi)
{
	/* make new widget */
	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_NEW_FOF, false);

	/* store */

	/* add to layout */
	mLayout->addWidget(pi);

	std::cerr << "NewsFeed::addFeedItemPeerNew()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemChanNew(RsFeedItem &fi)
{
	/* make new widget */
	ChanNewItem *cni = new ChanNewItem(this, NEWSFEED_CHANNEWLIST, fi.mId1, false, true);

	/* store in list */

	/* add to layout */
	mLayout->addWidget(cni);

	std::cerr << "NewsFeed::addFeedItemChanNew()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemChanUpdate(RsFeedItem &fi)
{
	/* make new widget */
	ChanNewItem *cni = new ChanNewItem(this, NEWSFEED_CHANNEWLIST, fi.mId1, false, false);

	/* store in list */

	/* add to layout */
	mLayout->addWidget(cni);

	std::cerr << "NewsFeed::addFeedItemChanUpdate()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemChanMsg(RsFeedItem &fi)
{
	/* make new widget */
	ChanMsgItem *cm = new ChanMsgItem(this, NEWSFEED_CHANMSGLIST, fi.mId1, fi.mId2, false);

	/* store in forum list */

	/* add to layout */
	mLayout->addWidget(cm);

	std::cerr << "NewsFeed::addFeedItemChanMsg()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemForumNew(RsFeedItem &fi)
{
	/* make new widget */
	ForumNewItem *fni = new ForumNewItem(this, NEWSFEED_FORUMNEWLIST, fi.mId1, false, true);

	/* store in forum list */
	mForumNewItems.push_back(fni);

	/* add to layout */
	mLayout->addWidget(fni);


	std::cerr << "NewsFeed::addFeedItemForumNew()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemForumUpdate(RsFeedItem &fi)
{
	/* make new widget */
	ForumNewItem *fni = new ForumNewItem(this, NEWSFEED_FORUMNEWLIST, fi.mId1, false, false);

	/* store in forum list */
	mForumNewItems.push_back(fni);

	/* add to layout */
	mLayout->addWidget(fni);

	std::cerr << "NewsFeed::addFeedItemForumUpdate()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemForumMsg(RsFeedItem &fi)
{
	/* make new widget */
	ForumMsgItem *fm = new ForumMsgItem(this, NEWSFEED_FORUMMSGLIST, fi.mId1, fi.mId2, false);

	/* store in forum list */

	/* add to layout */
	mLayout->addWidget(fm);

	std::cerr << "NewsFeed::addFeedItemForumMsg()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemBlogMsg(RsFeedItem &fi)
{
	/* make new widget */
	BlogMsgItem *bm = new BlogMsgItem(this, NEWSFEED_BLOGMSGLIST, fi.mId1, fi.mId2, false);

	/* store in forum list */

	/* add to layout */
	mLayout->addWidget(bm);

	std::cerr << "NewsFeed::addFeedItemBlogMsg()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemChatNew(RsFeedItem &fi)
{
	std::cerr << "NewsFeed::addFeedItemChatNew()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemMessage(RsFeedItem &fi)
{
	/* make new widget */
	MsgItem *mi = new MsgItem(this, NEWSFEED_MESSAGELIST, fi.mId1, false);

	/* store in list */

	/* add to layout */
	mLayout->addWidget(mi);

	std::cerr << "NewsFeed::addFeedItemMessage()";
	std::cerr << std::endl;
}


void	NewsFeed::addFeedItemFilesNew(RsFeedItem &fi)
{
	std::cerr << "NewsFeed::addFeedItemFilesNew()";
	std::cerr << std::endl;
}




/* FeedHolder Functions (for FeedItem functionality) */
void NewsFeed::deleteFeedItem(QWidget *item, uint32_t type)
{
	std::cerr << "NewsFeed::deleteFeedItem()";
	std::cerr << std::endl;
}

void NewsFeed::openChat(std::string peerId)
{
	std::cerr << "NewsFeed::openChat()";
	std::cerr << std::endl;
}

void NewsFeed::openMsg(uint32_t type, std::string grpId, std::string inReplyTo)
{
	std::cerr << "NewsFeed::openMsg()";
	std::cerr << std::endl;
	GeneralMsgDialog *msgDialog = new GeneralMsgDialog(NULL);


	msgDialog->addDestination(type, grpId, inReplyTo);

	msgDialog->show();

}


