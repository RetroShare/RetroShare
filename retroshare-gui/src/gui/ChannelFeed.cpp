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

#include <iostream>

#include "rsiface/rschannels.h"

#include "ChannelFeed.h"
#include "gui/feeds/ChanGroupItem.h"
#include "gui/feeds/ChanMenuItem.h"
#include "gui/feeds/ChanMsgItem.h"

#include "gui/forums/CreateForum.h"

#include "GeneralMsgDialog.h"

/****
 * #define CHAN_DEBUG
 ***/

/** Constructor */
ChannelFeed::ChannelFeed(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

  	connect(chanButton, SIGNAL(clicked()), this, SLOT(createChannel()));
  	connect(postButton, SIGNAL(clicked()), this, SLOT(sendMsg()));
  	connect(subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeChannel ( void ) ) );
  	connect(unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeChannel ( void ) ) );

	/*************** Setup Left Hand Side (List of Channels) ****************/

	mGroupLayout = new QVBoxLayout;

	mGroupOwn = new ChanGroupItem("Own Channels");
	mGroupSub = new ChanGroupItem("Subscribed Channels");
	mGroupPop = new ChanGroupItem("Popular Channels");
	mGroupOther = new ChanGroupItem("Other Channels");

	mGroupLayout->addWidget(mGroupOwn);
	mGroupLayout->addWidget(mGroupSub);
	mGroupLayout->addWidget(mGroupPop);
	mGroupLayout->addWidget(mGroupOther);


	QWidget *middleWidget = new QWidget();
	//middleWidget->setSizePolicy( QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);
	middleWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	middleWidget->setLayout(mGroupLayout);

     	QScrollArea *scrollArea = new QScrollArea;
        //scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(middleWidget);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QVBoxLayout *layout2 = new QVBoxLayout;
	layout2->addWidget(scrollArea);
	
     	chanFrame->setLayout(layout2);

	/*************** Setup Left Hand Side (List of Channels) ****************/

	mMsgLayout = new QVBoxLayout;

	QWidget *middleWidget2 = new QWidget();
	middleWidget2->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	middleWidget2->setLayout(mMsgLayout);

     	QScrollArea *scrollArea2 = new QScrollArea;
        //scrollArea2->setBackgroundRole(QPalette::Dark);
	scrollArea2->setWidget(middleWidget2);
	scrollArea2->setWidgetResizable(true);
	scrollArea2->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QVBoxLayout *layout3 = new QVBoxLayout;
	layout3->addWidget(scrollArea2);

     	msgFrame->setLayout(layout3);

	mChannelId = "OWNID"; 

	updateChannelList();

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

}


void ChannelFeed::createChannel()
{
	CreateForum *cf = new CreateForum(NULL, false);
	cf->show();
}

void ChannelFeed::channelSelection()
{
	/* which item was selected? */


	/* update mChannelId */

	updateChannelMsgs();
}

void ChannelFeed::sendMsg()
{
#ifdef CHAN_DEBUG
	std::cerr << "ChannelFeed::sendMsg()";
	std::cerr << std::endl;
#endif

	if (mChannelId != "")
	{
		openMsg(FEEDHOLDER_MSG_CHANNEL, mChannelId, "");
	}
	else
	{
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::sendMsg() no Channel Selected!";
		std::cerr << std::endl;
#endif
	}

}


/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

void ChannelFeed::deleteFeedItem(QWidget *item, uint32_t type)
{
	return;
}

void ChannelFeed::openChat(std::string peerId)
{
	return;
}


void ChannelFeed::openMsg(uint32_t type, std::string grpId, std::string inReplyTo)
{
#ifdef CHAN_DEBUG
	std::cerr << "ChannelFeed::openMsg()";
	std::cerr << std::endl;
#endif
	GeneralMsgDialog *msgDialog = new GeneralMsgDialog(NULL);


	msgDialog->addDestination(type, grpId, inReplyTo);

	msgDialog->show();
	return;
}

void ChannelFeed::selectChannel( std::string cId)
{
	mChannelId = cId;

	updateChannelMsgs();
}

void ChannelFeed::checkUpdate()
{
	std::list<std::string> chanIds;
	std::list<std::string>::iterator it;
	if (!rsChannels)
		return;

	if (rsChannels->channelsChanged(chanIds))
	{
		/* update Forums List */
		updateChannelList();

		it = std::find(chanIds.begin(), chanIds.end(), mChannelId);
		if (it != chanIds.end())
		{
			updateChannelMsgs();
		}
	}
}




void ChannelFeed::updateChannelList()
{

	std::list<ChannelInfo> channelList;
	std::list<ChannelInfo>::iterator it;
	if (!rsChannels)
	{
		return;
	}

	rsChannels->getChannelList(channelList);

	/* get the ids for our lists */
	std::list<std::string> adminIds;
	std::list<std::string> subIds;
	std::list<std::string> popIds;
	std::list<std::string> otherIds;
	std::multimap<uint32_t, std::string> popMap;

	for(it = channelList.begin(); it != channelList.end(); it++)
	{
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->channelFlags;

		if (flags & RS_DISTRIB_ADMIN)
		{
			adminIds.push_back(it->channelId);
		}
		else if (flags & RS_DISTRIB_SUBSCRIBED)
		{
			subIds.push_back(it->channelId);
		}
		else
		{
			/* rate the others by popularity */
			popMap.insert(std::make_pair(it->pop, it->channelId));
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10)
	{
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	uint32_t popLimit = 0;
	std::multimap<uint32_t, std::string>::reverse_iterator rit;
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); rit++, i++)
	{
		popIds.push_back(rit->second);
	}

	if (rit != popMap.rend())
	{
		popLimit = rit->first;
	}

	for(it = channelList.begin(); it != channelList.end(); it++)
	{
		/* ignore the ones we've done already */
		uint32_t flags = it->channelFlags;

		if (flags & RS_DISTRIB_ADMIN)
		{
			continue;
		}
		else if (flags & RS_DISTRIB_SUBSCRIBED)
		{
			continue;
		}
		else
		{
			if (it->pop < popLimit)
			{
				otherIds.push_back(it->channelId);
			}
		}
	}

	/* now we have our lists ---> update entries */

	updateChannelListOwn(adminIds);
	updateChannelListSub(subIds);
	updateChannelListPop(popIds);
	updateChannelListOther(otherIds);
}

void ChannelFeed::updateChannelListOwn(std::list<std::string> &ids)
{
	std::list<ChanMenuItem *>::iterator it;
	std::list<std::string>::iterator iit;

	/* TEMP just replace all of them */
	for(it = mChannelListOwn.begin(); it != mChannelListOwn.end(); it++)
	{
		delete (*it);
	}
	mChannelListOwn.clear();

	int topIndex = mGroupLayout->indexOf(mGroupOwn);
	int index = topIndex + 1;
	for (iit = ids.begin(); iit != ids.end(); iit++, index++)
	{
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::updateChannelListOwn(): " << *iit << " at: " << index;
		std::cerr << std::endl;
#endif

		ChanMenuItem *cmi = new ChanMenuItem(*iit);
		mChannelListOwn.push_back(cmi);
		mGroupLayout->insertWidget(index, cmi);
  	
		connect(cmi, SIGNAL( selectMe( std::string )), this, SLOT( selectChannel( std::string )));
	}
}

void ChannelFeed::updateChannelListSub(std::list<std::string> &ids)
{
	std::list<ChanMenuItem *>::iterator it;
	std::list<std::string>::iterator iit;

	/* TEMP just replace all of them */
	for(it = mChannelListSub.begin(); it != mChannelListSub.end(); it++)
	{
		delete (*it);
	}
	mChannelListSub.clear();

	int topIndex = mGroupLayout->indexOf(mGroupSub);
	int index = topIndex + 1;
	for (iit = ids.begin(); iit != ids.end(); iit++, index++)
	{
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::updateChannelListSub(): " << *iit << " at: " << index;
		std::cerr << std::endl;
#endif

		ChanMenuItem *cmi = new ChanMenuItem(*iit);
		mChannelListSub.push_back(cmi);
		mGroupLayout->insertWidget(index, cmi);
		connect(cmi, SIGNAL( selectMe( std::string )), this, SLOT( selectChannel( std::string )));
	}

}

void ChannelFeed::updateChannelListPop(std::list<std::string> &ids)
{
	std::list<ChanMenuItem *>::iterator it;
	std::list<std::string>::iterator iit;

	/* TEMP just replace all of them */
	for(it = mChannelListPop.begin(); it != mChannelListPop.end(); it++)
	{
		delete (*it);
	}
	mChannelListPop.clear();

	int topIndex = mGroupLayout->indexOf(mGroupPop);
	int index = topIndex + 1;
	for (iit = ids.begin(); iit != ids.end(); iit++, index++)
	{
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::updateChannelListPop(): " << *iit << " at: " << index;
		std::cerr << std::endl;
#endif

		ChanMenuItem *cmi = new ChanMenuItem(*iit);
		mChannelListPop.push_back(cmi);
		mGroupLayout->insertWidget(index, cmi);
		connect(cmi, SIGNAL( selectMe( std::string )), this, SLOT( selectChannel( std::string )));
	}


}

void ChannelFeed::updateChannelListOther(std::list<std::string> &ids)
{
	std::list<ChanMenuItem *>::iterator it;
	std::list<std::string>::iterator iit;

	/* TEMP just replace all of them */
	for(it = mChannelListOther.begin(); it != mChannelListOther.end(); it++)
	{
		delete (*it);
	}
	mChannelListOther.clear();

	int topIndex = mGroupLayout->indexOf(mGroupOther);
	int index = topIndex + 1;
	for (iit = ids.begin(); iit != ids.end(); iit++, index++)
	{
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::updateChannelListOther(): " << *iit << " at: " << index;
		std::cerr << std::endl;
#endif

		ChanMenuItem *cmi = new ChanMenuItem(*iit);
		mChannelListOther.push_back(cmi);
		mGroupLayout->insertWidget(index, cmi);
		connect(cmi, SIGNAL( selectMe( std::string )), this, SLOT( selectChannel( std::string )));
	}
}

void ChannelFeed::updateChannelMsgs()
{
	if (!rsChannels)
		return;

	ChannelInfo ci;
	if (!rsChannels->getChannelInfo(mChannelId, ci))
	{
		postButton->setEnabled(false);
		subscribeButton->setEnabled(false);
		unsubscribeButton->setEnabled(false);
		nameLabel->setText("No Channel Selected");
		return;
	}
	/* set channel name */
	nameLabel->setText(QString::fromStdWString(ci.channelName));

	/* do buttons */
	if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED)
	{
		subscribeButton->setEnabled(false);
		unsubscribeButton->setEnabled(true);
	}
	else
	{
		subscribeButton->setEnabled(true);
		unsubscribeButton->setEnabled(false);
	}

	if (ci.channelFlags & RS_DISTRIB_PUBLISH)
	{
		postButton->setEnabled(true);
	}
	else
	{
		postButton->setEnabled(false);
	}

	/* replace all the messages with new ones */
	std::list<ChanMsgItem *>::iterator mit;
	for(mit = mChanMsgItems.begin(); mit != mChanMsgItems.end(); mit++)
	{
		delete (*mit);
	}
	mChanMsgItems.clear();

	std::list<ChannelMsgSummary> msgs;
	std::list<ChannelMsgSummary>::iterator it;

	rsChannels->getChannelMsgList(mChannelId, msgs);

	for(it = msgs.begin(); it != msgs.end(); it++)
	{
		ChanMsgItem *cmi = new ChanMsgItem(this, 0, mChannelId, it->msgId, true);

		mChanMsgItems.push_back(cmi);
		mMsgLayout->addWidget(cmi);
	}
}





void ChannelFeed::unsubscribeChannel()
{
#ifdef CHAN_DEBUG
	std::cerr << "ChannelFeed::unsubscribeChannel()";
	std::cerr << std::endl;
#endif
	if (rsChannels)
	{
		rsChannels->channelSubscribe(mChannelId, false);
	}
	updateChannelMsgs();
}


void ChannelFeed::subscribeChannel()
{
#ifdef CHAN_DEBUG
	std::cerr << "ChannelFeed::subscribeChannel()";
	std::cerr << std::endl;
#endif
	if (rsChannels)
	{
		rsChannels->channelSubscribe(mChannelId, true);
	}
	updateChannelMsgs();
}




