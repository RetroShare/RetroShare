#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>

#include <sstream>
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


void MainWindow::update()
{
	//std::cerr << "MainWindow::update()" << std::endl;
	updateNetStatus();
	updateDhtPeers();
	updateNetPeers();
	updateChat();
}


void MainWindow::updateNetStatus()
{



}

#define DTW_COL_BUCKET	0
#define DTW_COL_IPADDR	1
#define DTW_COL_PEERID	2
#define DTW_COL_FLAGS	3
#define DTW_COL_FOUND	4
#define DTW_COL_SEND	5
#define DTW_COL_RECV	6

void MainWindow::updateDhtPeers()
{

	/* Hackish display of all Dht peers, should be split into buckets (as children) */

	bdNodeId ownId;
	mPeerNet->getOwnId(&ownId);

        QString status = QString::fromStdString(mPeerNet->getDhtStatusString());
	ui->dhtLabel->setText(status);
	
	std::list<bdPeer> allpeers;
	std::list<bdPeer>::iterator it;
	int i;
	for(i = 0; i < 160; i++)
	{
		bdBucket peers;
        	mPeerNet->get_dht_peers(i, peers);

		for(it = peers.entries.begin(); it != peers.entries.end(); it++)
		{
			allpeers.push_back(*it);
		}
	}
	QTreeWidget *dhtTreeWidget = ui->dhtTreeWidget;

	dhtTreeWidget->clear();

#if 0
	/* clear old entries */
	int itemCount = dhtTreeWidget->topLevelItemCount();
	for (int nIndex = 0; nIndex < itemCount;) 
	{
		QTreeWidgetItem *tmp_item = dhtTreeWidget->topLevelItem(nIndex);
		std::string tmpid = tmp_item->data(DTW_COL_PEERID, Qt::DisplayRole).toString().toStdString();
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

	time_t now = time(NULL);
	for(it = allpeers.begin(); it != allpeers.end(); it++)
	{
		/* find the entry */
		QTreeWidgetItem *dht_item = NULL;

		/* insert */
		dht_item = new QTreeWidgetItem();

		int dist = bdStdBucketDistance(&ownId, &(it->mPeerId.id));
		std::ostringstream buckstr;
		buckstr << dist;

		std::ostringstream ipstr;
		ipstr << inet_ntoa(it->mPeerId.addr.sin_addr);
		ipstr << ":" << ntohs(it->mPeerId.addr.sin_port);

		std::ostringstream idstr;
		bdStdPrintNodeId(idstr, &(it->mPeerId.id));

		std::ostringstream flagsstr;
		flagsstr << "0x" << std::hex << std::setfill('0') << it->mPeerFlags;

		std::ostringstream foundstr;
		foundstr << now - it->mFoundTime << " secs ago";

		std::ostringstream lastsendstr;
		lastsendstr << now - it->mLastSendTime << " secs ago";

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


void MainWindow::updateNetPeers()
{
        QString status = QString::fromStdString(mPeerNet->getPeerStatusString());
	QString oldstatus = ui->peerLine->text();
	if (oldstatus != status)
	{
		ui->peerLine->setText(status);
	}


	QTreeWidget *peerTreeWidget = ui->peerTreeWidget;

	std::list<std::string> peerIds;
	std::list<std::string>::iterator it;
	mPeerNet->get_net_peers(peerIds);

#define PTW_COL_PEERID		0
#define PTW_COL_STATUS		1
#define PTW_COL_UPDATETS	2

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

		std::ostringstream updatestr;
		time_t now = time(NULL);
		updatestr << now - status.mUpdateTS << " secs ago";

		peer_item -> setData(PTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(*it));
		peer_item -> setData(PTW_COL_STATUS, Qt::DisplayRole, QString::fromStdString(status.mStatusMsg));
		peer_item -> setData(PTW_COL_UPDATETS, Qt::DisplayRole, QString::fromStdString(updatestr.str()));
	}
}


void MainWindow::addPeer()
{
	std::string id = ui->peerLineEdit->text().toStdString();
	mPeerNet->add_peer(id);
}


void MainWindow::sendChat()
{


}


void MainWindow::updateChat()
{



}

