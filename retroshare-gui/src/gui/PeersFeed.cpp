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

#include "PeersFeed.h"
#include "feeds/PeerItem.h"

#include "rsiface/rspeers.h"

#include "GeneralMsgDialog.h"

const uint32_t PEERSFEED_MODE_FRIENDS 	= 0x0000;
const uint32_t PEERSFEED_MODE_ONLINE	= 0x0001;
const uint32_t PEERSFEED_MODE_FOF 	= 0x0002;
const uint32_t PEERSFEED_MODE_ALL 	= 0x0003;


/*****
 * #define PEERS_DEBUG  1
 ****/

/** Constructor */
PeersFeed::PeersFeed(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

  	connect( modeComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateMode() ) );

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
	layout2->setSpacing(0);
	layout2->setMargin(0);
	
     	frame->setLayout(layout2);

	mMode = PEERSFEED_MODE_FRIENDS;

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(updatePeers()));
	timer->start(1000);

}




/* FeedHolder Functions (for FeedItem functionality) */
void PeersFeed::deleteFeedItem(QWidget *item, uint32_t type)
{
#ifdef PEERS_DEBUG  
	std::cerr << "PeersFeed::deleteFeedItem()";
	std::cerr << std::endl;
#endif
}

void PeersFeed::openChat(std::string peerId)
{
#ifdef PEERS_DEBUG  
	std::cerr << "PeersFeed::openChat()";
	std::cerr << std::endl;
#endif
}

void PeersFeed::openMsg(uint32_t type, std::string grpId, std::string inReplyTo)
{
#ifdef PEERS_DEBUG  
	std::cerr << "PeersFeed::openMsg()";
	std::cerr << std::endl;
#endif

	GeneralMsgDialog *msgDialog = new GeneralMsgDialog(NULL);


	msgDialog->addDestination(type, grpId, inReplyTo);

	msgDialog->show();

}

void  PeersFeed::updateMode()
{
	/* NB #defines must match GUI design for correct selection */

	mMode = modeComboBox->currentIndex();
	updatePeers();
}

/* get the list of peers from the RsIface.  */
void  PeersFeed::updatePeers()
{
	std::list<std::string> peers, toAdd, toRemove;
	std::list<std::string>::iterator it;

	std::map<std::string, uint32_t> newpeers;
	std::map<std::string, uint32_t>::iterator nit;
	std::map<std::string, PeerItem *>::iterator pit;

	if (!rsPeers)
        {
                /* not ready yet! */
                return;
        }

	if (mMode == PEERSFEED_MODE_FRIENDS)
	{
		rsPeers->getFriendList(peers);
	}
	else if (mMode == PEERSFEED_MODE_ONLINE)
	{
		rsPeers->getOnlineList(peers);
	}
	else if (mMode == PEERSFEED_MODE_ALL)
	{
		rsPeers->getOthersList(peers);
	}
	else
	{
		rsPeers->getOthersList(peers);
		/* now remove friends */
		for(it = peers.begin(); it != peers.end(); )
		{
			if (rsPeers->isFriend(*it))
			{
				it = peers.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	for(it = peers.begin(); it != peers.end(); it++)
	{
		newpeers[*it] = 1;
	}

	nit = newpeers.begin();
	pit = mPeers.begin();

	while(nit != newpeers.end())
	{
		if (pit == mPeers.end())
		{
			toAdd.push_back(nit->first);
			nit++;
			continue;
		}

		if (nit->first == pit->first)
		{
			/* same - good! */
			nit++;
			pit++;
			continue;
		}

		if (nit->first < pit->first)
		{
			/* must add in new item */
			toAdd.push_back(nit->first);
			nit++;
		}
		else
		{
			toRemove.push_back(pit->first);
			pit++;
		}
	}

	/* remove remaining items */
	while (pit != mPeers.end())
	{
		toRemove.push_back(pit->first);
		pit++;
	}

	/* remove first */
	for(it = toRemove.begin(); it != toRemove.end(); it++)
	{
		pit = mPeers.find(*it);
		if (pit != mPeers.end())
		{
			delete (pit->second);
			mPeers.erase(pit);
		}
	}

	/* add in new ones */
	for(it = toAdd.begin(); it != toAdd.end(); it++)
	{
  		PeerItem *pi = new PeerItem(this, 0, *it, PEER_TYPE_STD, true);
		mPeers[*it] = pi;
		mLayout->addWidget(pi);
	}
}

//	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_STD, false);
//	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_HELLO, false);
//	PeerItem *pi = new PeerItem(this, NEWSFEED_PEERLIST, fi.mId1, PEER_TYPE_NEW_FOF, false);

#if 0

void PeersFeed::setChatDialog(ChatDialog *cd)
{
  chatDialog = cd;
}


/** Open a QFileDialog to browse for export a file. */
void PeersFeed::exportfriend()
{
        QTreeWidgetItem *c = getCurrentPeer();
        std::cerr << "PeersFeed::exportfriend()" << std::endl;
	if (!c)
	{
        	std::cerr << "PeersFeed::exportfriend() Noone Selected -- sorry" << std::endl;
		return;
	}

	std::string id = getPeerRsCertId(c);
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Certificate"), "",
	                                                     tr("Certificates (*.pqi)"));

	std::string file = fileName.toStdString();
	if (file != "")
	{
        	std::cerr << "PeersFeed::exportfriend() Saving to: " << file << std::endl;
        	std::cerr << std::endl;
		if (rsPeers)
		{
			rsPeers->SaveCertificateToFile(id, file);
		}
	}

}

void PeersFeed::chatfriend()
{
    QTreeWidgetItem *i = getCurrentPeer();

    if (!i)
	return;

    std::string name = (i -> text(2)).toStdString();
    std::string id = (i -> text(7)).toStdString();
    
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(id, detail))
    {
    	return;
    }

    if (detail.state & RS_PEER_STATE_CONNECTED)
    {
    	/* must reference ChatDialog */
    	if (chatDialog)
    	{
    		chatDialog->getPrivateChat(id, name, true);
    	}
    }
    else
    {
    	/* info dialog */
        QMessageBox::StandardButton sb = QMessageBox::question ( NULL, 
			"Friend Not Online", 
	"Your Friend is offline \nDo you want to send them a Message instead",
			        (QMessageBox::Yes | QMessageBox::No));
	if (sb == QMessageBox::Yes)
	{
		msgfriend();
	}
    }
    return;
}


void PeersFeed::msgfriend()
{
    std::cerr << "SharedFilesDialog::msgfriend()" << std::endl;

    QTreeWidgetItem *i = getCurrentPeer();

    if (!i)
	return;

    std::string status = (i -> text(1)).toStdString();
    std::string name = (i -> text(2)).toStdString();
    std::string id = (i -> text(7)).toStdString();

    rsicontrol -> ClearInMsg();
    rsicontrol -> SetInMsg(id, true);

    /* create a message */
    ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

    nMsgDialog->newMsg();
    nMsgDialog->show();
}


QTreeWidgetItem *PeersFeed::getCurrentPeer()
{
	/* get the current, and extract the Id */

	/* get a link to the table */
        QTreeWidget *peerWidget = ui.peertreeWidget;
        QTreeWidgetItem *item = peerWidget -> currentItem();
        if (!item)
        {
		std::cerr << "Invalid Current Item" << std::endl;
		return NULL;
	}

	/* Display the columns of this item. */
	std::ostringstream out;
        out << "CurrentPeerItem: " << std::endl;

	for(int i = 1; i < 6; i++)
	{
		QString txt = item -> text(i);
		out << "\t" << i << ":" << txt.toStdString() << std::endl;
	}
	std::cerr << out.str();
	return item;
}

/* So from the Peers Dialog we can call the following control Functions:
 * (1) Remove Current.              FriendRemove(id)
 * (2) Allow/DisAllow.              FriendStatus(id, accept)
 * (2) Connect.                     FriendConnectAttempt(id, accept)
 * (3) Set Address.                 FriendSetAddress(id, str, port)
 * (4) Set Trust.                   FriendTrustSignature(id, bool)
 * (5) Configure (GUI Only) -> 3/4
 *
 * All of these rely on the finding of the current Id.
 */
 

void PeersFeed::removefriend()
{
        QTreeWidgetItem *c = getCurrentPeer();
        std::cerr << "PeersFeed::removefriend()" << std::endl;
	if (!c)
	{
        	std::cerr << "PeersFeed::removefriend() Noone Selected -- sorry" << std::endl;
		return;
	}

	if (rsPeers)
	{
		rsPeers->removeFriend(getPeerRsCertId(c));
	}
}


void PeersFeed::allowfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
	std::cerr << "PeersFeed::allowfriend()" << std::endl;
	/*
	bool accept = true;
	rsServer->FriendStatus(getPeerRsCertId(c), accept);
	*/
}


void PeersFeed::connectfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
	std::cerr << "PeersFeed::connectfriend()" << std::endl;
	if (!c)
	{
        	std::cerr << "PeersFeed::connectfriend() Noone Selected -- sorry" << std::endl;
		return;
	}

	if (rsPeers)
	{
		rsPeers->connectAttempt(getPeerRsCertId(c));
	}
}

void PeersFeed::setaddressfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
	std::cerr << "PeersFeed::setaddressfriend()" << std::endl;

	/* need to get the input address / port */
	/*
 	std::string addr;
	unsigned short port;
	rsServer->FriendSetAddress(getPeerRsCertId(c), addr, port);
	*/
}

void PeersFeed::trustfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
	std::cerr << "PeersFeed::trustfriend()" << std::endl;
	/*
	bool trust = true;
	rsServer->FriendTrust(getPeerRsCertId(c), trust);
	*/
}



/* GUI stuff -> don't do anything directly with Control */
void PeersFeed::configurefriend()
{
	/* display Dialog */
	std::cerr << "PeersFeed::configurefriend()" << std::endl;
	QTreeWidgetItem *c = getCurrentPeer();


	static ConfCertDialog *confdialog = new ConfCertDialog();


	if (!c)
		return;

	/* set the Id */
	std::string id = getPeerRsCertId(c);

	confdialog -> loadId(id);
	confdialog -> show();
}

#endif

