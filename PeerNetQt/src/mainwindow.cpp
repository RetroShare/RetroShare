#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QDateTime>

#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	// tick for gui update.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);

	// connect add Peer button.
    	connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addPeer()));
    	connect(ui->dhtButton, SIGNAL(clicked()), this, SLOT(showDhtWindow()));
    	connect(ui->chatLineEdit, SIGNAL(returnPressed()), this, SLOT(sendChat()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
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

void MainWindow::setPeerNet(PeerNet *pnet)
{
	mPeerNet = pnet;
}


void MainWindow::setDhtWindow(DhtWindow *dw)
{
	mDhtWindow = dw;
}

void MainWindow::showDhtWindow()
{
	mDhtWindow->show();
}

void MainWindow::update()
{
	//std::cerr << "MainWindow::update()" << std::endl;
	updateNetStatus();
	updateNetPeers();
	updateRelays();
	updateChat();

	// Shouldn't do it here! but for now.
	mPeerNet->tick();
}


void MainWindow::updateNetStatus()
{
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

}

void MainWindow::updateNetPeers()
{

	QTreeWidget *peerTreeWidget = ui->peerTreeWidget;

	std::list<std::string> peerIds;
	std::list<std::string> failedPeerIds;
	std::list<std::string>::iterator it;
	mPeerNet->get_net_peers(peerIds);
	mPeerNet->get_net_failedpeers(failedPeerIds);

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


#define PTW_COL_PEERID		0
#define PTW_COL_DHT_STATUS	1
#define PTW_COL_DHT_ADDRESS	2
#define PTW_COL_DHT_UPDATETS	3

#define PTW_COL_PEER_STATUS	4
#define PTW_COL_PEER_ADDRESS	5
#define PTW_COL_PEER_UPDATETS	6

	/* clear old entries */
	int itemCount = peerTreeWidget->topLevelItemCount();
	for (int nIndex = 0; nIndex < itemCount;) 
	{
		QTreeWidgetItem *tmp_item = peerTreeWidget->topLevelItem(nIndex);
		std::string tmpid = tmp_item->data(PTW_COL_PEERID, Qt::DisplayRole).toString().toStdString();
		if (peerIds.end() == std::find(peerIds.begin(), peerIds.end(), tmpid))
		{
			if (failedPeerIds.end() == std::find(failedPeerIds.begin(), failedPeerIds.end(), tmpid))
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
		else
		{
			nIndex++;
		}
	}

	for(it = peerIds.begin(); it != peerIds.end(); it++)
	{
		/* find the entry */
		QTreeWidgetItem *peer_item = NULL;
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

		if (!peer_item)
		{
			/* insert */
			peer_item = new QTreeWidgetItem();
			peerTreeWidget->addTopLevelItem(peer_item);
		}

		/* update the data */
		PeerStatus status;
		mPeerNet->get_peer_status(*it, status);
		time_t now = time(NULL);

		std::ostringstream dhtipstr;
		if ((status.mDhtState == PN_DHT_STATE_ONLINE) || (status.mDhtState == PN_DHT_STATE_UNREACHABLE)) 
		{
			dhtipstr << inet_ntoa(status.mDhtAddr.sin_addr);
			dhtipstr << ":" << ntohs(status.mDhtAddr.sin_port);
		}

		std::ostringstream dhtupdatestr;
		dhtupdatestr << now - status.mDhtUpdateTS << " secs ago";

		std::ostringstream peeripstr;
		//if (status.mPeerState == PN_PEER_STATE_ONLINE)
		{
			peeripstr << inet_ntoa(status.mPeerAddr.sin_addr);
			peeripstr << ":" << ntohs(status.mPeerAddr.sin_port);
		}

		std::ostringstream peerupdatestr;
		peerupdatestr << now - status.mPeerUpdateTS << " secs ago";


		peer_item -> setData(PTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(*it));
		peer_item -> setData(PTW_COL_DHT_STATUS, Qt::DisplayRole, QString::fromStdString(status.mDhtStatusMsg));
		peer_item -> setData(PTW_COL_DHT_ADDRESS, Qt::DisplayRole, QString::fromStdString(dhtipstr.str()));
		peer_item -> setData(PTW_COL_DHT_UPDATETS, Qt::DisplayRole, QString::fromStdString(dhtupdatestr.str()));

		peer_item -> setData(PTW_COL_PEER_STATUS, Qt::DisplayRole, QString::fromStdString(status.mPeerStatusMsg));
		peer_item -> setData(PTW_COL_PEER_ADDRESS, Qt::DisplayRole, QString::fromStdString(peeripstr.str()));
		peer_item -> setData(PTW_COL_PEER_UPDATETS, Qt::DisplayRole, QString::fromStdString(peerupdatestr.str()));


		switch(status.mDhtState)
		{
			default:
			case PN_DHT_STATE_UNKNOWN:
			case PN_DHT_STATE_SEARCHING:
			case PN_DHT_STATE_FAILURE:
			case PN_DHT_STATE_OFFLINE:
				nOfflinePeers++;
				break;
			case PN_DHT_STATE_UNREACHABLE:
				nUnreachablePeers++;
				break;
			case PN_DHT_STATE_ONLINE:
				nOnlinePeers++;
				break;
		}
			

		switch(status.mPeerState)
		{
			default:
			case PN_PEER_STATE_DISCONNECTED:
				nDisconnPeers++;
				break;
			case PN_PEER_STATE_CONNECTED:
			{
				switch(status.mPeerConnectMode)
				{
					default:
					case PN_CONNECT_UDP_DIRECT:
						nDirectPeers++;
						break;
					case PN_CONNECT_UDP_PROXY:
						nProxyPeers++;
						break;
					case PN_CONNECT_UDP_RELAY:
						nRelayPeers++;
						break;
				}
			}
				break;
		}
			
	}


	for(it = failedPeerIds.begin(); it != failedPeerIds.end(); it++)
	{
		/* find the entry */
		QTreeWidgetItem *peer_item = NULL;
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

		if (!peer_item)
		{
			/* insert */
			peer_item = new QTreeWidgetItem();
			peerTreeWidget->addTopLevelItem(peer_item);
		}

		/* update the data */
		PeerStatus status;
		mPeerNet->get_failedpeer_status(*it, status);
		time_t now = time(NULL);

		std::ostringstream peeripstr;
		//if (status.mPeerState == PN_PEER_STATE_ONLINE)
		{
			peeripstr << inet_ntoa(status.mPeerAddr.sin_addr);
			peeripstr << ":" << ntohs(status.mPeerAddr.sin_port);
		}

		std::ostringstream peerupdatestr;
		peerupdatestr << now - status.mPeerUpdateTS << " secs ago";


		peer_item -> setData(PTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(*it));
		peer_item -> setData(PTW_COL_DHT_STATUS, Qt::DisplayRole, "Unknown Peer");
		peer_item -> setData(PTW_COL_DHT_ADDRESS, Qt::DisplayRole, "");
		peer_item -> setData(PTW_COL_DHT_UPDATETS, Qt::DisplayRole, "");

		peer_item -> setData(PTW_COL_PEER_STATUS, Qt::DisplayRole, QString::fromStdString(status.mPeerStatusMsg));
		peer_item -> setData(PTW_COL_PEER_ADDRESS, Qt::DisplayRole, QString::fromStdString(peeripstr.str()));
		peer_item -> setData(PTW_COL_PEER_UPDATETS, Qt::DisplayRole, QString::fromStdString(peerupdatestr.str()));
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



void MainWindow::updateRelays()
{

	QTreeWidget *relayTreeWidget = ui->relayTreeWidget;

	std::list<UdpRelayEnd> relayEnds;
	std::list<UdpRelayProxy> relayProxies;

	std::list<UdpRelayEnd>::iterator reit;
	std::list<UdpRelayProxy>::iterator rpit;

	std::list<std::string> failedPeerIds;
	std::list<std::string>::iterator it;
	mPeerNet->get_relayends(relayEnds);
	mPeerNet->get_relayproxies(relayProxies);


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
		proxystr << rs_inet_ntoa(reit->mProxyAddr.sin_addr) << ":"
				 << ntohs(reit->mProxyAddr.sin_port);

		std::ostringstream deststr;
		deststr << rs_inet_ntoa(reit->mRemoteAddr.sin_addr) << ":"
				 << ntohs(reit->mRemoteAddr.sin_port);

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
		srcstr << rs_inet_ntoa(rpit->mAddrs.mSrcAddr.sin_addr) << ":"
				 << ntohs(rpit->mAddrs.mSrcAddr.sin_port);

		std::ostringstream proxystr;
		proxystr << "Yourself";

		std::ostringstream deststr;
		deststr << rs_inet_ntoa(rpit->mAddrs.mDestAddr.sin_addr) << ":"
				 << ntohs(rpit->mAddrs.mDestAddr.sin_port);

		std::ostringstream agestr;
		agestr << "unknown";
		//agestr << now - rpit->mLastTS << " secs ago";

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



void MainWindow::addPeer()
{
	std::string id = ui->peerLineEdit->text().toStdString();
	mPeerNet->add_peer(id);
}


void MainWindow::sendChat()
{
	std::string msg = ui->chatLineEdit->text().toStdString();
	ui->chatLineEdit->clear();

	if (msg.size() > 0)
	{
		mPeerNet->sendMessage(msg);
	}
	addChatMsg("yourself", msg);
}


void MainWindow::updateChat()
{
	std::list<std::string> peerIds;
	std::list<std::string>::iterator it;
	mPeerNet->get_net_peers(peerIds);
	for(it = peerIds.begin(); it != peerIds.end(); it++)
	{
		std::string msg;
		if (mPeerNet->getMessage(*it, msg))
		{
			addChatMsg(*it, msg);
		}
	}
}

void MainWindow::addChatMsg(std::string id, std::string msg)
{
	QDateTime now = QDateTime::currentDateTime();
	QString nowstr = now.toString("hh:mm:ss");
	QString chat = ui->chatBrowser->toPlainText();
	QString newmsg = "<";
	newmsg += QString::fromStdString(id.substr(0,5));
	newmsg += "...@";
	newmsg += nowstr;
	newmsg += "> ";
	newmsg += QString::fromStdString(msg);
	newmsg += "\n";

	chat += newmsg;
	ui->chatBrowser->setPlainText(chat);
}

