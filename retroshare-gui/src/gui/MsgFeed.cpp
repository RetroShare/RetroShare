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

#include "MsgFeed.h"
#include "feeds/MsgItem.h"
#include "GeneralMsgDialog.h"

#include "rsiface/rsmsgs.h"
#include "rsiface/rspeers.h"

/*****
 * #define MSG_DEBUG  1
 ****/

/** Constructor */
MsgFeed::MsgFeed(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

  	connect(boxComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateBox() ) );
  	connect(peerComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateMode() ) );
  	connect(sortComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateSort() ) );
  	connect(msgButton, SIGNAL( clicked( ) ), this, SLOT( newMsg() ) );

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

	/* default Inbox + All */
	mMsgbox = RS_MSG_INBOX;
	mPeerId = "";


	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(updatePeerIds()));
	timer->start(10000);

}




/* FeedHolder Functions (for FeedItem functionality) */
void MsgFeed::deleteFeedItem(QWidget *item, uint32_t type)
{
#ifdef MSG_DEBUG
	std::cerr << "MsgFeed::deleteFeedItem()";
	std::cerr << std::endl;
#endif
}

void MsgFeed::openChat(std::string peerId)
{
#ifdef MSG_DEBUG
	std::cerr << "MsgFeed::openChat()";
	std::cerr << std::endl;
#endif
}

void MsgFeed::openMsg(uint32_t type, std::string grpId, std::string inReplyTo)
{
#ifdef MSG_DEBUG
	std::cerr << "MsgFeed::openMsg()";
	std::cerr << std::endl;
#endif

	GeneralMsgDialog *msgDialog = new GeneralMsgDialog(NULL);


	msgDialog->addDestination(type, grpId, inReplyTo);

	msgDialog->show();

}

void MsgFeed::newMsg()
{
#ifdef MSG_DEBUG
	std::cerr << "MsgFeed::newMsg()";
	std::cerr << std::endl;
#endif

	GeneralMsgDialog *msgDialog = new GeneralMsgDialog(NULL);

	msgDialog->setMsgType(FEEDHOLDER_MSG_MESSAGE);

	msgDialog->show();
}



void  MsgFeed::updateBox()
{
	int idx = boxComboBox->currentIndex();
	switch(idx)
	{
		case 3:
			mMsgbox = RS_MSG_SENTBOX;
			break;
		case 2:
			mMsgbox = RS_MSG_DRAFTBOX;
			break;
		case 1:
			mMsgbox = RS_MSG_OUTBOX;
			break;
		case 0:
		default:
			mMsgbox = RS_MSG_INBOX;
			break;
        }
	updateMsgs();
}
	
void  MsgFeed::updateMode()
{
	/* NB #defines must match GUI design for correct selection */

        if (peerComboBox->currentIndex() >= 0)
	{
		QVariant qv = peerComboBox->itemData(peerComboBox->currentIndex());
		mPeerId = qv.toString().toStdString();
        }
	updateMsgs();
}

void  MsgFeed::updateSort()
{


}
	
void 	MsgFeed::updatePeerIds()
{
#ifdef MSG_DEBUG
	std::cerr << "MsgFeed::updatePeerIds()";
	std::cerr << std::endl;
#endif
	
	int selectIdx = 0; /* ALL */
	std::string peerId;

        if (peerComboBox->currentIndex() >= 0)
	{
		QVariant qv = peerComboBox->itemData(peerComboBox->currentIndex());
		peerId = qv.toString().toStdString();
	}
	
	peerComboBox->clear();
	peerComboBox->addItem("All Msgs", "");

	/* add a list of friends */
	if (rsPeers)
	{
		std::list<std::string> friends;
		std::list<std::string>::iterator it;
	
		rsPeers->getFriendList(friends);
		for(it = friends.begin(); it != friends.end(); it++)
		{
			QString id = QString::fromStdString(*it);
			QString name = QString::fromStdString(rsPeers->getPeerName(*it));
			peerComboBox->addItem(name, id);

			if (peerId == *it)
			{
				selectIdx = peerComboBox->count() - 1;
			}
		}
	}
	peerComboBox->setCurrentIndex(selectIdx);

	/* this will trigger updateMsgs?? */
}

	

void  MsgFeed::updateMsgs()
{
	std::list<MsgInfoSummary> msgs;
	std::list<MsgInfoSummary>::iterator mit;

	std::list<std::string> toAdd, toRemove;
	std::list<std::string>::iterator it;

	std::map<std::string, uint32_t> newmsgs;
	std::map<std::string, uint32_t>::iterator nit;
	std::map<std::string, MsgItem *>::iterator iit;

	if (!rsMsgs)
        {
                /* not ready yet! */
                return;
        }

	rsMsgs->getMessageSummaries(msgs);

	for(mit = msgs.begin(); mit != msgs.end(); )
	{
		/* filter on box type */
		if (mMsgbox != (mit->msgflags & RS_MSG_BOXMASK))
		{
			mit = msgs.erase(mit);
		}
		else
		{
			mit++;
		}
	}

	if (mPeerId != "")
	{
		for(mit = msgs.begin(); mit != msgs.end(); )
		{
			/* filter on source */
			if (mPeerId != mit->srcId)
			{
				mit = msgs.erase(mit);
			}
			else
			{
				mit++;
			}
		}
	}

	for(mit = msgs.begin(); mit != msgs.end(); mit++)
	{
		newmsgs[mit->msgId] = 1;
	}

	nit = newmsgs.begin();
	iit = mMsgs.begin();

	while(nit != newmsgs.end())
	{
		if (iit == mMsgs.end())
		{
			toAdd.push_back(nit->first);
			nit++;
			continue;
		}

		if (nit->first == iit->first)
		{
			/* same - good! */
			nit++;
			iit++;
			continue;
		}

		if (nit->first < iit->first)
		{
			/* must add in new item */
			toAdd.push_back(nit->first);
			nit++;
		}
		else
		{
			toRemove.push_back(iit->first);
			iit++;
		}
	}

	/* remove remaining items */
	while (iit != mMsgs.end())
	{
		toRemove.push_back(iit->first);
		iit++;
	}

	/* remove first */
	for(it = toRemove.begin(); it != toRemove.end(); it++)
	{
		iit = mMsgs.find(*it);
		if (iit != mMsgs.end())
		{
			delete (iit->second);
			mMsgs.erase(iit);
		}
	}

	/* add in new ones */
	for(it = toAdd.begin(); it != toAdd.end(); it++)
	{
  		MsgItem *mi = new MsgItem(this, 0, *it, true);
		mMsgs[*it] = mi;
		mLayout->addWidget(mi);
	}
}


