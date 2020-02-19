/*******************************************************************************
 * gui/statistics/DhtWindow.cpp                                                *
 *                                                                             *
 * Copyright (c) 2011 Robert Fernie   <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "DhtWindow.h"
#include "ui_DhtWindow.h"
#include "util/QtVersion.h"

#include <QClipboard>
#include <QTimer>
#include <QDateTime>
#include <QMenu>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <time.h>

#include "retroshare-gui/RsAutoUpdatePage.h"
#include "retroshare/rsdht.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rspeers.h"

#define DTW_COL_BUCKET	0
#define DTW_COL_IPADDR	1
#define DTW_COL_PEERID	2
#define DTW_COL_FLAGS	3
#define DTW_COL_FOUND	4
#define DTW_COL_SEND	5
#define DTW_COL_RECV	6

DhtWindow::DhtWindow(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    ui.setupUi(this);
    
    connect( ui.filterLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterItems(QString)));
    connect( ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));
    connect( ui.dhtTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(DHTCustomPopupMenu(QPoint)));
    
        /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("IP"), DTW_COL_IPADDR, tr("Search IP"));
    ui.filterLineEdit->setCurrentFilter(DTW_COL_IPADDR);
}

DhtWindow::~DhtWindow()
{

}

void DhtWindow::DHTCustomPopupMenu( QPoint )
{
	QMenu contextMnu( this );
	
	QTreeWidgetItem *item = ui.dhtTreeWidget->currentItem();
	if (item) {
	
	QString Ip = item->text(DTW_COL_IPADDR);

	contextMnu.addAction(QIcon(), tr("Copy %1 to clipboard").arg(Ip), this, SLOT(copyIP()));

  }

	contextMnu.exec(QCursor::pos());
}

void DhtWindow::updateDisplay()
{
	/* do nothing if locked, or not visible */
	if (RsAutoUpdatePage::eventsLocked() == true) 
	{
#ifdef DEBUG_DHTWINDOW
		std::cerr << "DhtWindow::update() events Are Locked" << std::endl;
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

	RsAutoUpdatePage::lockAllEvents();

	//std::cerr << "DhtWindow::update()" << std::endl;
	updateNetStatus();
	updateNetPeers();
	updateDhtPeers();
	updateRelays();

	RsAutoUpdatePage::unlockAllEvents() ;

	QHeaderView_setSectionResizeMode(ui.peerTreeWidget->header(), QHeaderView::ResizeToContents);
	QHeaderView_setSectionResizeMode(ui.dhtTreeWidget->header(), QHeaderView::ResizeToContents);
	QHeaderView_setSectionResizeMode(ui.relayTreeWidget->header(), QHeaderView::ResizeToContents);
}


void DhtWindow::updateNetStatus()
{

		QString status;
	QString oldstatus;

#if 0
		status = QString::fromStdString(mPeerNet->getPeerStatusString());
	oldstatus = ui.peerLine->text();
	if (oldstatus != status)
	{
		ui.peerLine->setText(status);
	}
#endif

		status = QString::fromStdString(rsDht->getUdpAddressString());
	oldstatus = ui.peerAddressLabel->text();
	if (oldstatus != status)
	{
		ui.peerAddressLabel->setText(status);
	}

	RsNetworkMode netMode = rsConfig->getNetworkMode();

	QLabel *label = ui.networkLabel;
	switch(netMode)
	{
	    case RsNetworkMode::UNKNOWN:
			label->setText(tr("Unknown NetState"));
			break;
	    case RsNetworkMode::RESTARTING:
		    label->setText(tr("Restarting"));
		    break;
	    case RsNetworkMode::OFFLINE:
			label->setText(tr("Offline"));
			break;
	    case RsNetworkMode::LOCALNET:
			label->setText(tr("Local Net"));
			break;
	    case RsNetworkMode::BEHINDNAT:
			label->setText(tr("Behind NAT"));
			break;
	    case RsNetworkMode::EXTERNALIP:
			label->setText(tr("External IP"));
			break;
	}

	label = ui.natTypeLabel;

	RsNatTypeMode natType = rsConfig->getNatTypeMode();
	switch(natType)
	{
	    case RsNatTypeMode::UNKNOWN:
			label->setText(tr("UNKNOWN NAT STATE"));
			break;
	    case RsNatTypeMode::SYMMETRIC:
			label->setText(tr("SYMMETRIC NAT"));
			break;
	    case RsNatTypeMode::DETERM_SYM:
			label->setText(tr("DETERMINISTIC SYM NAT"));
			break;
	    case RsNatTypeMode::RESTRICTED_CONE:
			label->setText(tr("RESTRICTED CONE NAT"));
			break;
	    case RsNatTypeMode::FULL_CONE:
			label->setText(tr("FULL CONE NAT"));
			break;
	    case RsNatTypeMode::OTHER:
			label->setText(tr("OTHER NAT"));
			break;
	    case RsNatTypeMode::NONE:
			label->setText(tr("NO NAT"));
			break;
	}



	label = ui.natHoleLabel;
	RsNatHoleMode natHole = rsConfig->getNatHoleMode();

	switch(natHole)
	{
	    case RsNatHoleMode::UNKNOWN:
			label->setText(tr("UNKNOWN NAT HOLE STATUS"));
			break;
	    case RsNatHoleMode::NONE:
			label->setText(tr("NO NAT HOLE"));
			break;
	    case RsNatHoleMode::UPNP:
			label->setText(tr("UPNP FORWARD"));
			break;
	    case RsNatHoleMode::NATPMP:
			label->setText(tr("NATPMP FORWARD"));
			break;
	    case RsNatHoleMode::FORWARDED:
			label->setText(tr("MANUAL FORWARD"));
			break;
	}

	RsConnectModes connect = rsConfig->getConnectModes();

	label = ui.connectLabel;
	QString connOut;
	if (!!(connect & RsConnectModes::OUTGOING_TCP))
	{
		connOut += "TCP_OUT ";
	}
	if (!!(connect & RsConnectModes::ACCEPT_TCP))
	{
		connOut += "TCP_IN ";
	}
	if (!!(connect & RsConnectModes::DIRECT_UDP))
	{
		connOut += "DIRECT_UDP ";
	}
	if (!!(connect & RsConnectModes::PROXY_UDP))
	{
		connOut += "PROXY_UDP ";
	}
	if (!!(connect & RsConnectModes::RELAY_UDP))
	{
		connOut += "RELAY_UDP ";
	}

	label->setText(connOut);

	RsNetState netState = rsConfig->getNetState();

	label = ui.netStatusLabel;
	switch(netState)
	{
	    case RsNetState::BAD_UNKNOWN:
			label->setText(tr("NET BAD: Unknown State"));
			break;
	    case RsNetState::BAD_OFFLINE:
			label->setText(tr("NET BAD: Offline"));
			break;
	    case RsNetState::BAD_NATSYM:
			label->setText(tr("NET BAD: Behind Symmetric NAT"));
			break;
	    case RsNetState::BAD_NODHT_NAT:
			label->setText(tr("NET BAD: Behind NAT & No DHT"));
			break;
	    case RsNetState::WARNING_RESTART:
			label->setText(tr("NET WARNING: NET Restart"));
			break;
	    case RsNetState::WARNING_NATTED:
			label->setText(tr("NET WARNING: Behind NAT"));
			break;
	    case RsNetState::WARNING_NODHT:
			label->setText(tr("NET WARNING: No DHT"));
			break;
	    case RsNetState::GOOD:
			label->setText(tr("NET STATE GOOD!"));
			break;
	    case RsNetState::ADV_FORWARD:
			label->setText(tr("CAUTION: UNVERIFIABLE FORWARD!"));
			break;
	    case RsNetState::ADV_DARK_FORWARD:
			label->setText(tr("CAUTION: UNVERIFIABLE FORWARD & NO DHT"));
			break;
	}
}

void DhtWindow::updateNetPeers()
{
#ifdef RS_USE_BITDHT
	//QTreeWidget *peerTreeWidget = ui.peerTreeWidget;

	std::list<RsPeerId> peerIds;
	std::list<RsPeerId>::iterator it;

	rsDht->getNetPeerList(peerIds);

	/* collate peer stats */
	size_t nPeers = peerIds.size();

	// from DHT peers
	int nOnlinePeers = 0;
	int nUnreachablePeers = 0;
	int nOfflinePeers = 0;

	// Connect States.
	int nDisconnPeers = 0;
	int nDirectPeers = 0;
	int nProxyPeers = 0;
	int nRelayPeers = 0;


#define PTW_COL_RSNAME			0
#define PTW_COL_PEERID			1
#define PTW_COL_DHT_STATUS		2
	
#define PTW_COL_PEER_CONNECTLOGIC	3

#define PTW_COL_PEER_CONNECT_STATUS	4
#define PTW_COL_PEER_CONNECT_MODE	5
#define PTW_COL_PEER_REQ_STATUS		6
	
#define PTW_COL_PEER_CB_MSG		7
#define PTW_COL_RSID			8

#if 0
	/* clear old entries */
	int itemCount = peerTreeWidget->topLevelItemCount();
	for (int nIndex = 0; nIndex < itemCount;) 
	{
		QTreeWidgetItem *tmp_item = ui.peerTreeWidget->topLevelItem(nIndex);
		std::string tmpid = tmp_item->data(PTW_COL_PEERID, Qt::DisplayRole).toString().toStdString();
		if (peerIds.end() == std::find(peerIds.begin(), peerIds.end(), tmpid))
		{
			ui.peerTreeWidget->removeItemWidget(tmp_item, 0);
			/* remove it! */
			itemCount--;
		}
		else
		{
			++nIndex;
		}
	}
#endif
	ui.peerTreeWidget->clear();

	for(it = peerIds.begin(); it != peerIds.end(); ++it)
	{
		/* find the entry */
		QTreeWidgetItem *peer_item = nullptr;
#if 0
		QString qpeerid = QString::fromStdString(*it);
		int itemCount = ui.peerTreeWidget->topLevelItemCount();
		for (int nIndex = 0; nIndex < itemCount; ++nIndex)
		{
			QTreeWidgetItem *tmp_item = ui.peerTreeWidget->topLevelItem(nIndex);
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
			ui.peerTreeWidget->addTopLevelItem(peer_item);
		}
		
		/* Set header resize modes and initial section sizes Peer TreeView*/
    QHeaderView * _header = ui.peerTreeWidget->header () ;
    _header->resizeSection ( PTW_COL_RSNAME, 170 );
    
		/* update the data */
		RsDhtNetPeer status;
		rsDht->getNetPeerStatus(*it, status);

		std::string name = rsPeers->getPeerName(*it);

		peer_item -> setData(PTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(status.mDhtId));
		peer_item -> setData(PTW_COL_RSNAME, Qt::DisplayRole, QString::fromUtf8(name.c_str()));
		peer_item -> setData(PTW_COL_RSID, Qt::DisplayRole, QString::fromStdString(status.mRsId.toStdString()));

		QString dhtstate;
		switch(status.mDhtState)
		{
			default:
		    case RsDhtPeerDht::NOT_ACTIVE:
				dhtstate = tr("Not Active (Maybe Connected!)");
				break;
		    case RsDhtPeerDht::SEARCHING:
				dhtstate = tr("Searching");
				break;
		    case RsDhtPeerDht::FAILURE:
				dhtstate = tr("Failed");
				break;
		    case RsDhtPeerDht::OFFLINE:
				dhtstate = tr("offline");
				++nOfflinePeers;
				break;
		    case RsDhtPeerDht::UNREACHABLE:
				dhtstate = tr("Unreachable");
				++nUnreachablePeers;
				break;
		    case RsDhtPeerDht::ONLINE:
				dhtstate = tr("ONLINE");
				++nOnlinePeers;
				break;
		}
			
		peer_item -> setData(PTW_COL_DHT_STATUS, Qt::DisplayRole, dhtstate);

		// NOW CONNECT STATE
		QString cpmstr;
		switch(status.mPeerConnectMode)
		{
			case RSDHT_TOU_MODE_DIRECT:
				cpmstr = tr("Direct");
				break;
			case RSDHT_TOU_MODE_PROXY:
				cpmstr = tr("Proxy VIA")+" " + QString::fromStdString(status.mPeerConnectProxyId);
				break;
			case RSDHT_TOU_MODE_RELAY:
				cpmstr = tr("Relay VIA")+" " + QString::fromStdString(status.mPeerConnectProxyId);
				break;
			default:
			case RSDHT_TOU_MODE_NONE:
				cpmstr = tr("None");
				break;
		}


		QString cpsstr;
		switch(status.mPeerConnectState)
		{
			default:
			case RSDHT_PEERCONN_DISCONNECTED:
				cpsstr = tr("Disconnected");
				++nDisconnPeers;
				break;
			case RSDHT_PEERCONN_UDP_STARTED:
				cpsstr = tr("Udp Started");
				break;
			case RSDHT_PEERCONN_CONNECTED:
			{
				cpsstr = tr("Connected");

				switch(status.mPeerConnectMode)
				{
					default:
					case RSDHT_TOU_MODE_DIRECT:
						++nDirectPeers;
						break;
					case RSDHT_TOU_MODE_PROXY:
						++nProxyPeers;
						break;
					case RSDHT_TOU_MODE_RELAY:
						++nRelayPeers;
						break;
				}
			}
				break;
		}

		peer_item -> setData(PTW_COL_PEER_CONNECT_STATUS, Qt::DisplayRole, cpsstr);
		
		if (status.mPeerConnectState == RSDHT_PEERCONN_DISCONNECTED)
		{
			peer_item -> setData(PTW_COL_PEER_CONNECT_MODE, Qt::DisplayRole, "");
		}
		else 
		{
			peer_item -> setData(PTW_COL_PEER_CONNECT_MODE, Qt::DisplayRole, cpmstr);
		}
	
		// NOW REQ STATE.
		QString reqstr;
		if (status.mExclusiveProxyLock)
		{
				reqstr = "(E) ";
		}
		switch(status.mPeerReqState)
		{
			case RSDHT_PEERREQ_RUNNING:
				reqstr += tr("Request Active");
				break;
			case RSDHT_PEERREQ_STOPPED:
				reqstr += tr("No Request");
				break;
			default:
				reqstr += tr("Unknown");
				break;
		}
		peer_item -> setData(PTW_COL_PEER_REQ_STATUS, Qt::DisplayRole, reqstr);

		peer_item -> setData(PTW_COL_PEER_CB_MSG, Qt::DisplayRole, QString::fromStdString(status.mCbPeerMsg));
		peer_item -> setData(PTW_COL_PEER_CONNECTLOGIC, Qt::DisplayRole, 
						QString::fromStdString(status.mConnectState));
	}


	/*QString connstr;
	connstr =  tr("#Peers: ") + QString::number(nPeers);
	connstr += tr(" DHT: (#off:") + QString::number(nOfflinePeers);
	connstr += tr(",unreach:") + QString::number(nUnreachablePeers);
	connstr += tr(",online:") + QString::number(nOnlinePeers);
	connstr += tr(") Connections: (#dis:") + QString::number(nDisconnPeers);
	connstr += tr(",#dir:") + QString::number(nDirectPeers);
	connstr += tr(",#proxy:") + QString::number(nProxyPeers);
	connstr += tr(",#relay:") + QString::number(nRelayPeers);
	connstr += ")";*/
	
	ui.label_peers->setText(QString::number(nPeers));
	ui.label_offline->setText(QString::number(nOfflinePeers)); 
	ui.label_unreachable->setText(QString::number(nUnreachablePeers)); 
	ui.label_online->setText(QString::number(nOnlinePeers)); 
	
	ui.label_disconnected->setText(QString::number(nDisconnPeers));
	ui.label_direct->setText(QString::number(nDirectPeers)); 
	ui.label_proxy->setText(QString::number(nProxyPeers)); 
	ui.label_relay->setText(QString::number(nRelayPeers)); 
	
	ui.tabWidget_2->setTabText(1, tr("Peers") + " (" + QString::number(ui.peerTreeWidget->topLevelItemCount()) + ")" );


	//peerSummaryLabel->setText(connstr);
#endif
}



void DhtWindow::updateRelays()
{

	QTreeWidget *relayTreeWidget = ui.relayTreeWidget;

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

	for(reit = relayEnds.begin(); reit != relayEnds.end(); ++reit)
	{
		/* find the entry */
		QTreeWidgetItem *item = new QTreeWidgetItem();
		ui.relayTreeWidget->addTopLevelItem(item);

		QString typestr = tr("RELAY END");
		QString srcstr = tr("Yourself");
		QString proxystr = QString::fromStdString(reit->mProxyAddr);
		QString deststr = QString::fromStdString(reit->mRemoteAddr);
		QString agestr = tr("unknown");
		QString lastsendstr = tr("unknown");
		QString bandwidthstr = tr("unlimited");
		QString classstr = tr("Own Relay");

		//std::ostringstream dhtupdatestr;
		//dhtupdatestr << now - status.mDhtUpdateTS << " secs ago";

		item -> setData(RTW_COL_TYPE, Qt::DisplayRole, typestr);
		item -> setData(RTW_COL_SRC, Qt::DisplayRole, srcstr);
		item -> setData(RTW_COL_PROXY, Qt::DisplayRole, proxystr);
		item -> setData(RTW_COL_DEST, Qt::DisplayRole, deststr);
		item -> setData(RTW_COL_CLASS, Qt::DisplayRole, classstr);
		item -> setData(RTW_COL_AGE, Qt::DisplayRole, agestr);
		item -> setData(RTW_COL_LASTSEND, Qt::DisplayRole, lastsendstr);
		item -> setData(RTW_COL_BANDWIDTH, Qt::DisplayRole, bandwidthstr);

	}


	for(rpit = relayProxies.begin(); rpit != relayProxies.end(); ++rpit)
	{
		/* find the entry */
		QTreeWidgetItem *item = new QTreeWidgetItem();
		ui.relayTreeWidget->addTopLevelItem(item);

		QString typestr = tr("RELAY PROXY");
		QString srcstr = QString::fromStdString(rpit->mSrcAddr);
		QString proxystr = tr("Yourself");
		QString deststr = QString::fromStdString(rpit->mDestAddr);
		QString agestr = QString(tr("%1 secs ago")).arg(now - rpit->mCreateTS);
		QString lastsendstr = QString(tr("%1 secs ago")).arg(now - rpit->mLastTS);
		QString bandwidthstr = QString(tr("%1B/s")).arg(QString::number(rpit->mBandwidth));
		QString classstr = QString::number(rpit->mRelayClass);

		item -> setData(RTW_COL_TYPE, Qt::DisplayRole, typestr);
		item -> setData(RTW_COL_SRC, Qt::DisplayRole, srcstr);
		item -> setData(RTW_COL_PROXY, Qt::DisplayRole, proxystr);
		item -> setData(RTW_COL_DEST, Qt::DisplayRole, deststr);
		item -> setData(RTW_COL_CLASS, Qt::DisplayRole, classstr);
		item -> setData(RTW_COL_AGE, Qt::DisplayRole, agestr);
		item -> setData(RTW_COL_LASTSEND, Qt::DisplayRole, lastsendstr);
		item -> setData(RTW_COL_BANDWIDTH, Qt::DisplayRole, bandwidthstr);

	}
	
		ui.tabWidget_2->setTabText(2, tr("Relays") + " (" + QString::number(ui.relayTreeWidget->topLevelItemCount()) + ")" );

}



/****************************/


class DhtTreeWidgetItem : public QTreeWidgetItem
    {
public:
    virtual bool operator<(const QTreeWidgetItem &other) const
	{
	  int column = treeWidget()->sortColumn();
	  if (column == DTW_COL_RECV || column == DTW_COL_SEND
	      || column == DTW_COL_BUCKET)
	      {
	      QString t1 = text(column);
	      QString t2 = other.text(column);
	      t1 = t1.left(t1.indexOf(' '));
	      t2 = t2.left(t2.indexOf(' '));
	      return t1.toLong() < t2.toLong();
	      }
	  return text(column) < other.text(column);
	}
    };

void DhtWindow::updateDhtPeers()
{

	/* Hackish display of all Dht peers, should be split into buckets (as children) */
  //QString status = QString::fromStdString(mPeerNet->getDhtStatusString());
	//dhtLabel->setText(status);
	
	std::list<RsDhtPeer> allpeers;
	std::list<RsDhtPeer>::iterator it;
	int i;
	for(i = 0; i < 160; ++i)
	{
		std::list<RsDhtPeer> peers;
        	rsDht->getDhtPeers(i, peers);

		for(it = peers.begin(); it != peers.end(); ++it)
		{
			allpeers.push_back(*it);
		}
	}

	//QTreeWidget *dhtTreeWidget = ui.dhtTreeWidget;

	ui.dhtTreeWidget->clear();

	time_t now = time(NULL);
	for(it = allpeers.begin(); it != allpeers.end(); ++it)
	{
		/* find the entry */
		QTreeWidgetItem *dht_item = NULL;

		/* insert */
		dht_item = new DhtTreeWidgetItem();

		QString buckstr = QString::number(it->mBucket);
		QString ipstr = QString::fromStdString(it->mAddr);
		QString idstr = QString::fromStdString(it->mDhtId);
		QString flagsstr = QString(tr("0x%1 EX:0x%2")).arg(it->mPeerFlags, 0, 16, QChar('0')).arg(it->mExtraFlags, 0, 16, QChar('0'));
		QString foundstr = QString(tr("%1 secs ago")).arg(now - it->mFoundTime);

		QString lastsendstr;
		if (it->mLastSendTime == 0)
		{
			lastsendstr = tr("never");
		}
		else
		{
			lastsendstr = QString (tr("%1 secs ago")).arg(now - it->mLastSendTime);
		}

		QString lastrecvstr = QString (tr("%1 secs ago")).arg(now - it->mLastRecvTime);

		dht_item -> setData(DTW_COL_BUCKET, Qt::DisplayRole, buckstr);
		dht_item -> setData(DTW_COL_IPADDR, Qt::DisplayRole, ipstr);
		dht_item -> setData(DTW_COL_PEERID, Qt::DisplayRole, idstr);
		dht_item -> setData(DTW_COL_FLAGS, Qt::DisplayRole, flagsstr);

		dht_item -> setData(DTW_COL_FOUND, Qt::DisplayRole, foundstr);
		dht_item -> setData(DTW_COL_SEND, Qt::DisplayRole, lastsendstr);
		dht_item -> setData(DTW_COL_RECV, Qt::DisplayRole, lastrecvstr);

		ui.dhtTreeWidget->addTopLevelItem(dht_item);
		
		if (ui.filterLineEdit->text().isEmpty() == false) {
		filterItems(ui.filterLineEdit->text());
    }
    
    ui.tabWidget_2->setTabText(0, tr("DHT") + " (" + QString::number(ui.dhtTreeWidget->topLevelItemCount()) + ")" );
	}
	


}

void DhtWindow::getDHTStatus()
{

// 	RsConfigNetStatus config;
// 	rsConfig->getConfigNetStatus(config);
//
// 	if (!(config.DHTActive))
// 	{
// 		// GRAY.
// 	}
// 	else
// 	{
// 		if (config.netDhtOk)
// 		{
// #define MIN_RS_NET_SIZE		10
// 			// YELLOW or GREEN.
// 			if (config.netDhtRsNetSize < MIN_RS_NET_SIZE)
// 			{
//         updateGraph(config.netDhtRsNetSize,config.netDhtNetSize);
// 			}
// 			else
// 			{
//         updateGraph(config.netDhtRsNetSize,config.netDhtNetSize);
// 			}
// 		}
// 		else
// 		{
// 			// RED - some issue.
//
// 		}
// 	}
}

void DhtWindow::filterColumnChanged(int)
{
    filterItems(ui.filterLineEdit->text());
}

void DhtWindow::filterItems(const QString &text)
{
    int filterColumn = ui.filterLineEdit->currentFilter();

    int count = ui.dhtTreeWidget->topLevelItemCount ();
    for (int index = 0; index < count; ++index) {
        filterItem(ui.dhtTreeWidget->topLevelItem(index), text, filterColumn);
    }
}

bool DhtWindow::filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn)
{
    bool visible = true;

    if (text.isEmpty() == false) {
        if (item->text(filterColumn).contains(text, Qt::CaseInsensitive) == false) {
            visible = false;
        }
    }

    int visibleChildCount = 0;
    int count = item->childCount();
    for (int index = 0; index < count; ++index) {
        if (filterItem(item->child(index), text, filterColumn)) {
            ++visibleChildCount;
        }
    }

    if (visible || visibleChildCount) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    return (visible || visibleChildCount);
}

void DhtWindow::copyIP()
{
	QTreeWidgetItem *item = ui.dhtTreeWidget->currentItem();
	if (!item)
	{
		return;
	}

	QString Ip = item->text(DTW_COL_IPADDR);

	QApplication::clipboard()->setText(Ip, QClipboard::Clipboard);

}



