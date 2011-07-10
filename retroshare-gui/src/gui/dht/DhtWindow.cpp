/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 Robert Fernie
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

#include "DhtWindow.h"
#include "ui_DhtWindow.h"
#include <QTimer>
#include <QDateTime>

#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <time.h>

#include "gui/RsAutoUpdatePage.h"
#include "retroshare/rsdht.h"


/********************************************** STATIC WINDOW *************************************/
DhtWindow * DhtWindow::mInstance = NULL;

void DhtWindow::showYourself()
{
    if (mInstance == NULL) {
        mInstance = new DhtWindow();
    }

    mInstance->show();
    mInstance->activateWindow();
}

DhtWindow* DhtWindow::getInstance()
{
    return mInstance;
}

void DhtWindow::releaseInstance()
{
    if (mInstance) {
        delete mInstance;
    }
}

/********************************************** STATIC WINDOW *************************************/



DhtWindow::DhtWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DhtWindow)
{
    ui->setupUi(this);


    setAttribute ( Qt::WA_DeleteOnClose, true );
#ifdef MINIMAL_RSGUI
    setAttribute (Qt::WA_QuitOnClose, true);
#endif // MINIMAL_RSGUI


	// tick for gui update.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);



}

DhtWindow::~DhtWindow()
{
    delete ui;
}

void DhtWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DhtWindow::update()
{
	/* do nothing if locked, or not visible */
	if (RsAutoUpdatePage::eventsLocked() == true) 
	{
#ifdef DEBUG_DHTWINDOW
		std::cerr << "DhtWindow::update() events Are Locked" << std::endl;
#endif
		return;
    	}

	if (!isVisible())
	{
#ifdef DEBUG_DHTWINDOW
		//std::cerr << "DhtWindow::update() !Visible" << std::endl;
#endif
		return;
	}

	if (!rsDht)
	{
#ifdef DEBUG_DHTWINDOW
		std::cerr << "DhtWindow::update rsDht NOT Set" << std::endl;
#endif
		return;
	}

	//std::cerr << "DhtWindow::update()" << std::endl;
	updateNetStatus();
	updateNetPeers();
	updateDhtPeers();
	updateRelays();
}


void DhtWindow::updateNetStatus()
{

#if 0
        QString status = QString::fromStdString(mPeerNet->getPeerStatusString());
	QString oldstatus = ui->peerLine->text();
	if (oldstatus != status)
	{
		ui->peerLine->setText(status);
	}

        status = QString::fromStdString(mPeerNet->getPeerAddressString());
	oldstatus = ui->peerAddressLabel->text();
	if (oldstatus != status)
	{
		ui->peerAddressLabel->setText(status);
	}

	uint32_t netMode = mPeerNet->getNetStateNetworkMode();

	QLabel *label = ui->networkLabel;
	switch(netMode)
	{
		case PNSB_NETWORK_UNKNOWN:
			label->setText("Unknown NetState");
			break;
		case PNSB_NETWORK_OFFLINE:
			label->setText("Offline");
			break;
		case PNSB_NETWORK_LOCALNET:
			label->setText("Local Net");
			break;
		case PNSB_NETWORK_BEHINDNAT:
			label->setText("Behind NAT");
			break;
		case PNSB_NETWORK_EXTERNALIP:
			label->setText("External IP");
			break;
	}

	label = ui->natTypeLabel;
	switch(mPeerNet->getNetStateNatTypeMode())
	{
		case PNSB_NATTYPE_UNKNOWN:
			label->setText("UNKNOWN NAT STATE");
			break;
		case PNSB_NATTYPE_SYMMETRIC:
			label->setText("SYMMETRIC NAT");
			break;
		case PNSB_NATTYPE_RESTRICTED_CONE:
			label->setText("RESTRICTED CONE NAT");
			break;
		case PNSB_NATTYPE_FULL_CONE:
			label->setText("FULL CONE NAT");
			break;
		case PNSB_NATTYPE_OTHER:
			label->setText("OTHER NAT");
			break;
		case PNSB_NATTYPE_NONE:
			label->setText("NO NAT");
			break;
	}


	label = ui->natHoleLabel;
	switch(mPeerNet->getNetStateNatHoleMode())
	{
		case PNSB_NATHOLE_UNKNOWN:
			label->setText("UNKNOWN NAT HOLE STATUS");
			break;
		case PNSB_NATHOLE_NONE:
			label->setText("NO NAT HOLE");
			break;
		case PNSB_NATHOLE_UPNP:
			label->setText("UPNP FORWARD");
			break;
		case PNSB_NATHOLE_NATPMP:
			label->setText("NATPMP FORWARD");
			break;
		case PNSB_NATHOLE_FORWARDED:
			label->setText("MANUAL FORWARD");
			break;
	}

	label = ui->connectLabel;
	std::ostringstream connOut;
	uint32_t connect = mPeerNet->getNetStateConnectModes();
	if (connect & PNSB_CONNECT_OUTGOING_TCP)
	{
		connOut << "TCP_OUT ";
	}
	if (connect & PNSB_CONNECT_ACCEPT_TCP)
	{
		connOut << "TCP_IN ";
	}
	if (connect & PNSB_CONNECT_DIRECT_UDP)
	{
		connOut << "DIRECT_UDP ";
	}
	if (connect & PNSB_CONNECT_PROXY_UDP)
	{
		connOut << "PROXY_UDP ";
	}
	if (connect & PNSB_CONNECT_RELAY_UDP)
	{
		connOut << "RELAY_UDP ";
	}

	label->setText(QString::fromStdString(connOut.str()));

	label = ui->netStatusLabel;
	switch(mPeerNet->getNetStateNetStateMode())
	{
		case PNSB_NETSTATE_BAD_UNKNOWN:
			label->setText("NET BAD: Unknown State");
			break;
		case PNSB_NETSTATE_BAD_OFFLINE:
			label->setText("NET BAD: Offline");
			break;
		case PNSB_NETSTATE_BAD_NATSYM:
			label->setText("NET BAD: Behind Symmetric NAT");
			break;
		case PNSB_NETSTATE_BAD_NODHT_NAT:
			label->setText("NET BAD: Behind NAT & No DHT");
			break;
		case PNSB_NETSTATE_WARNING_RESTART:
			label->setText("NET WARNING: NET Restart");
			break;
		case PNSB_NETSTATE_WARNING_NATTED:
			label->setText("NET WARNING: Behind NAT");
			break;
		case PNSB_NETSTATE_WARNING_NODHT:
			label->setText("NET WARNING: No DHT");
			break;
		case PNSB_NETSTATE_GOOD:
			label->setText("NET STATE GOOD!");
			break;
		case PNSB_NETSTATE_ADV_FORWARD:
			label->setText("CAUTION: UNVERIFABLE FORWARD!");
			break;
		case PNSB_NETSTATE_ADV_DARK_FORWARD:
			label->setText("CAUTION: UNVERIFABLE FORWARD & NO DHT");
			break;
	}

#endif

}

void DhtWindow::updateNetPeers()
{


	QTreeWidget *peerTreeWidget = ui->peerTreeWidget;

	std::list<std::string> peerIds;
	std::list<std::string>::iterator it;

	rsDht->getNetPeerList(peerIds);

	/* collate peer stats */
	int nPeers = peerIds.size();

	// from DHT peers
	int nOnlinePeers = 0;
	int nUnreachablePeers = 0;
	int nOfflinePeers = 0;

	// Connect States.
	int nDisconnPeers = 0;
	int nDirectPeers = 0;
	int nProxyPeers = 0;
	int nRelayPeers = 0;


#define PTW_COL_PEERID			0
#define PTW_COL_DHT_STATUS		1
	
#define PTW_COL_PEER_CONNECTLOGIC	2

#define PTW_COL_PEER_CONNECT_STATUS	3
#define PTW_COL_PEER_CONNECT_MODE	4
#define PTW_COL_PEER_REQ_STATUS		5
	
#define PTW_COL_PEER_CB_MSG		6
#define PTW_COL_RSID			7

#if 0
	/* clear old entries */
	int itemCount = peerTreeWidget->topLevelItemCount();
	for (int nIndex = 0; nIndex < itemCount;) 
	{
		QTreeWidgetItem *tmp_item = peerTreeWidget->topLevelItem(nIndex);
		std::string tmpid = tmp_item->data(PTW_COL_PEERID, Qt::DisplayRole).toString().toStdString();
		if (peerIds.end() == std::find(peerIds.begin(), peerIds.end(), tmpid))
		{
			peerTreeWidget->removeItemWidget(tmp_item, 0);
			/* remove it! */
			itemCount--;
		}
		else
		{
			nIndex++;
		}
	}
#endif
	peerTreeWidget->clear();

	time_t now = time(NULL);
	for(it = peerIds.begin(); it != peerIds.end(); it++)
	{
		/* find the entry */
		QTreeWidgetItem *peer_item = NULL;
#if 0
		QString qpeerid = QString::fromStdString(*it);
		int itemCount = peerTreeWidget->topLevelItemCount();
		for (int nIndex = 0; nIndex < itemCount; nIndex++) 
		{
			QTreeWidgetItem *tmp_item = peerTreeWidget->topLevelItem(nIndex);
			if (tmp_item->data(PTW_COL_PEERID, Qt::DisplayRole).toString() == qpeerid) 
			{
				peer_item = tmp_item;
				break;
			}
		}
#endif

		if (!peer_item)
		{
			/* insert */
			peer_item = new QTreeWidgetItem();
			peerTreeWidget->addTopLevelItem(peer_item);
		}

		/* update the data */
		RsDhtNetPeer status;
		rsDht->getNetPeerStatus(*it, status);

		peer_item -> setData(PTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(status.mDhtId));
		peer_item -> setData(PTW_COL_RSID, Qt::DisplayRole, QString::fromStdString(status.mRsId));

		std::ostringstream dhtstate;
		switch(status.mDhtState)
		{
			default:
			case RSDHT_PEERDHT_NOT_ACTIVE:
				dhtstate << "Unknown";
				break;
			case RSDHT_PEERDHT_SEARCHING:
				dhtstate << "Searching";
				break;
			case RSDHT_PEERDHT_FAILURE:
				dhtstate << "Failed";
				break;
			case RSDHT_PEERDHT_OFFLINE:
				dhtstate << "offline";
				nOfflinePeers++;
				break;
			case RSDHT_PEERDHT_UNREACHABLE:
				dhtstate << "Unreachable";
				nUnreachablePeers++;
				break;
			case RSDHT_PEERDHT_ONLINE:
				dhtstate << "ONLINE";
				nOnlinePeers++;
				break;
		}
			
		peer_item -> setData(PTW_COL_DHT_STATUS, Qt::DisplayRole, QString::fromStdString(dhtstate.str()));

		// NOW CONNECT STATE
		std::ostringstream cpmstr;
		switch(status.mPeerConnectMode)
		{
			case RSDHT_TOU_MODE_DIRECT:
				cpmstr << "Direct";
				break;
			case RSDHT_TOU_MODE_PROXY:
				cpmstr << "Proxy VIA ";
				cpmstr << status.mPeerConnectProxyId;
				break;
			case RSDHT_TOU_MODE_RELAY:
				cpmstr << "Relay VIA ";
				cpmstr << status.mPeerConnectProxyId;
				break;
			default:
				cpmstr << "None";
				break;
		}


		std::ostringstream cpsstr;
		switch(status.mPeerConnectState)
		{
			default:
			case RSDHT_PEERCONN_DISCONNECTED:
				cpsstr << "Disconnected";
				nDisconnPeers++;
				break;
			case RSDHT_PEERCONN_UDP_STARTED:
				cpsstr << "Udp Started";
				break;
			case RSDHT_PEERCONN_CONNECTED:
			{
				cpsstr << "Connected";
				break;
				switch(status.mPeerConnectMode)
				{
					default:
					case RSDHT_TOU_MODE_DIRECT:
						nDirectPeers++;
						break;
					case RSDHT_TOU_MODE_PROXY:
						nProxyPeers++;
						break;
					case RSDHT_TOU_MODE_RELAY:
						nRelayPeers++;
						break;
				}
			}
				break;
		}

		peer_item -> setData(PTW_COL_PEER_CONNECT_STATUS, Qt::DisplayRole, QString::fromStdString(cpsstr.str()));
		
		if (status.mPeerConnectState == RSDHT_PEERCONN_DISCONNECTED)
		{
			peer_item -> setData(PTW_COL_PEER_CONNECT_MODE, Qt::DisplayRole, "");
		}
		else 
		{
			peer_item -> setData(PTW_COL_PEER_CONNECT_MODE, Qt::DisplayRole, QString::fromStdString(cpmstr.str()));
		}
	
		// NOW REQ STATE.
		std::ostringstream reqstr;
		switch(status.mPeerReqState)
		{
			case RSDHT_PEERREQ_RUNNING:
				reqstr << "Request Active";
				break;
			case RSDHT_PEERREQ_STOPPED:
				reqstr << "No Request";
				break;
			default:
				reqstr << "Unknown";
				break;
		}
		peer_item -> setData(PTW_COL_PEER_REQ_STATUS, Qt::DisplayRole, QString::fromStdString(reqstr.str()));

		peer_item -> setData(PTW_COL_PEER_CB_MSG, Qt::DisplayRole, QString::fromStdString(status.mCbPeerMsg));
		peer_item -> setData(PTW_COL_PEER_CONNECTLOGIC, Qt::DisplayRole, 
						QString::fromStdString(status.mConnectState));
	}


	std::ostringstream connstr;
	connstr << "#Peers: " << nPeers;
	connstr << " DHT: (#off:" << nOfflinePeers;
	connstr << ",unreach:" << nUnreachablePeers;
	connstr << ",online:" << nOnlinePeers;
	connstr << ") Connections: (#dis:" << nDisconnPeers;
	connstr << ",#dir:" << nDirectPeers;
	connstr << ",#proxy:" << nProxyPeers;
	connstr << ",#relay:" << nRelayPeers;
	connstr << ")";

	QLabel *label = ui->peerSummaryLabel;
	label->setText(QString::fromStdString(connstr.str()));

}



void DhtWindow::updateRelays()
{

	QTreeWidget *relayTreeWidget = ui->relayTreeWidget;

	std::list<RsDhtRelayEnd> relayEnds;
	std::list<RsDhtRelayProxy> relayProxies;

	std::list<RsDhtRelayEnd>::iterator reit;
	std::list<RsDhtRelayProxy>::iterator rpit;

	rsDht->getRelayEnds(relayEnds);
	rsDht->getRelayProxies(relayProxies);


#define RTW_COL_TYPE		0
#define RTW_COL_SRC		1
#define RTW_COL_PROXY		2
#define RTW_COL_DEST		3
#define RTW_COL_CLASS		4
#define RTW_COL_AGE		5
#define RTW_COL_LASTSEND	6
#define RTW_COL_BANDWIDTH	7

	relayTreeWidget->clear();
	time_t now = time(NULL);

	for(reit = relayEnds.begin(); reit != relayEnds.end(); reit++)
	{
		/* find the entry */
		QTreeWidgetItem *item = new QTreeWidgetItem();
		relayTreeWidget->addTopLevelItem(item);

		std::ostringstream typestr;
		typestr << "RELAY END";

		std::ostringstream srcstr;
		srcstr << "Yourself";

		std::ostringstream proxystr;
		proxystr << reit->mProxyAddr;

		std::ostringstream deststr;
		deststr << reit->mRemoteAddr;
		
		std::ostringstream agestr;
		agestr << "unknown";

		std::ostringstream lastsendstr;
		lastsendstr << "unknown";

		std::ostringstream bandwidthstr;
		bandwidthstr << "unlimited";

		std::ostringstream classstr;
		classstr << "Own Relay";

		//std::ostringstream dhtupdatestr;
		//dhtupdatestr << now - status.mDhtUpdateTS << " secs ago";


		item -> setData(RTW_COL_TYPE, Qt::DisplayRole, QString::fromStdString(typestr.str()));
		item -> setData(RTW_COL_SRC, Qt::DisplayRole, QString::fromStdString(srcstr.str()));
		item -> setData(RTW_COL_PROXY, Qt::DisplayRole, QString::fromStdString(proxystr.str()));
		item -> setData(RTW_COL_DEST, Qt::DisplayRole, QString::fromStdString(deststr.str()));
		item -> setData(RTW_COL_CLASS, Qt::DisplayRole, QString::fromStdString(classstr.str()));
		item -> setData(RTW_COL_AGE, Qt::DisplayRole, QString::fromStdString(agestr.str()));
		item -> setData(RTW_COL_LASTSEND, Qt::DisplayRole, QString::fromStdString(lastsendstr.str()));
		item -> setData(RTW_COL_BANDWIDTH, Qt::DisplayRole, QString::fromStdString(bandwidthstr.str()));

	}


	for(rpit = relayProxies.begin(); rpit != relayProxies.end(); rpit++)
	{
		/* find the entry */
		QTreeWidgetItem *item = new QTreeWidgetItem();
		relayTreeWidget->addTopLevelItem(item);

		std::ostringstream typestr;
		typestr << "RELAY PROXY";

		std::ostringstream srcstr;
		srcstr << rpit->mSrcAddr;
		
		std::ostringstream proxystr;
		proxystr << "Yourself";

		std::ostringstream deststr;
		deststr << rpit->mDestAddr;
		
		std::ostringstream agestr;
		agestr << now - rpit->mCreateTS << " secs ago";

		std::ostringstream lastsendstr;
		lastsendstr << now - rpit->mLastTS << " secs ago";

		std::ostringstream bandwidthstr;
		bandwidthstr << rpit->mBandwidth << "B/s";

		std::ostringstream classstr;
		classstr << rpit->mRelayClass;

		item -> setData(RTW_COL_TYPE, Qt::DisplayRole, QString::fromStdString(typestr.str()));
		item -> setData(RTW_COL_SRC, Qt::DisplayRole, QString::fromStdString(srcstr.str()));
		item -> setData(RTW_COL_PROXY, Qt::DisplayRole, QString::fromStdString(proxystr.str()));
		item -> setData(RTW_COL_DEST, Qt::DisplayRole, QString::fromStdString(deststr.str()));
		item -> setData(RTW_COL_CLASS, Qt::DisplayRole, QString::fromStdString(classstr.str()));
		item -> setData(RTW_COL_AGE, Qt::DisplayRole, QString::fromStdString(agestr.str()));
		item -> setData(RTW_COL_LASTSEND, Qt::DisplayRole, QString::fromStdString(lastsendstr.str()));
		item -> setData(RTW_COL_BANDWIDTH, Qt::DisplayRole, QString::fromStdString(bandwidthstr.str()));

	}
}





/****************************/


#define DTW_COL_BUCKET	0
#define DTW_COL_IPADDR	1
#define DTW_COL_PEERID	2
#define DTW_COL_FLAGS	3
#define DTW_COL_FOUND	4
#define DTW_COL_SEND	5
#define DTW_COL_RECV	6

void DhtWindow::updateDhtPeers()
{

	/* Hackish display of all Dht peers, should be split into buckets (as children) */
        //QString status = QString::fromStdString(mPeerNet->getDhtStatusString());
	//ui->dhtLabel->setText(status);
	
	std::list<RsDhtPeer> allpeers;
	std::list<RsDhtPeer>::iterator it;
	int i;
	for(i = 0; i < 160; i++)
	{
		std::list<RsDhtPeer> peers;
        	rsDht->getDhtPeers(i, peers);

		for(it = peers.begin(); it != peers.end(); it++)
		{
			allpeers.push_back(*it);
		}
	}

	QTreeWidget *dhtTreeWidget = ui->dhtTreeWidget;

	dhtTreeWidget->clear();

	time_t now = time(NULL);
	for(it = allpeers.begin(); it != allpeers.end(); it++)
	{
		/* find the entry */
		QTreeWidgetItem *dht_item = NULL;

		/* insert */
		dht_item = new QTreeWidgetItem();

		int dist = it->mBucket;
		std::ostringstream buckstr;
		buckstr << dist;

		std::ostringstream ipstr;
		ipstr << it->mAddr;

		std::ostringstream idstr;
		idstr << it->mDhtId;

		std::ostringstream flagsstr;
		flagsstr << "0x" << std::hex << std::setfill('0') << it->mPeerFlags;
		flagsstr << " EX:0x" << std::hex << std::setfill('0') << it->mExtraFlags;
		std::ostringstream foundstr;
		foundstr << now - it->mFoundTime << " secs ago";

		std::ostringstream lastsendstr;
		if (it->mLastSendTime == 0)
		{
			lastsendstr << "never";
		}
		else
		{
			lastsendstr << now - it->mLastSendTime << " secs ago";
		}

		std::ostringstream lastrecvstr;
		lastrecvstr << now - it->mLastRecvTime << " secs ago";

		dht_item -> setData(DTW_COL_BUCKET, Qt::DisplayRole, QString::fromStdString(buckstr.str()));
		dht_item -> setData(DTW_COL_IPADDR, Qt::DisplayRole, QString::fromStdString(ipstr.str()));
		dht_item -> setData(DTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(idstr.str()));
		dht_item -> setData(DTW_COL_FLAGS, Qt::DisplayRole, QString::fromStdString(flagsstr.str()));

		dht_item -> setData(DTW_COL_FOUND, Qt::DisplayRole, QString::fromStdString(foundstr.str()));
		dht_item -> setData(DTW_COL_SEND, Qt::DisplayRole, QString::fromStdString(lastsendstr.str()));
		dht_item -> setData(DTW_COL_RECV, Qt::DisplayRole, QString::fromStdString(lastrecvstr.str()));

		dhtTreeWidget->addTopLevelItem(dht_item);
	}

}


