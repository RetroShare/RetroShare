#include "dhtwindow.h"
#include "dhtquery.h"
#include "ui_dhtwindow.h"
#include <QTimer>
#include <QDateTime>

#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>


#define QTW_COL_PEERID	0
#define QTW_COL_STATUS	1
#define QTW_COL_FLAGS	2
#define QTW_COL_RESULTS	3


DhtWindow::DhtWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DhtWindow)
{
    ui->setupUi(this);

	// tick for gui update.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);

	// connect add Peer button.
    	connect(ui->queryTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(setQueryId()));
    	connect(ui->queryButton, SIGNAL(clicked()), this, SLOT(showQuery()));
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

void DhtWindow::setPeerNet(PeerNet *pnet)
{
	mPeerNet = pnet;
}


void DhtWindow::setDhtQuery(DhtQuery *qw)
{
	mQueryWindow = qw;
}


void DhtWindow::showQuery()
{
	mQueryWindow->show();
}


void DhtWindow::setQueryId()
{
	std::cerr << "DhtWindow::setQueryId()";
	std::cerr << std::endl;

	/* get the item that is selected in the queryWindow */
	QTreeWidget *queryTreeWidget = ui->queryTreeWidget;
	QTreeWidgetItem *item = queryTreeWidget->currentItem();
	if (item)
	{
                std::string id = item->data(QTW_COL_PEERID, Qt::DisplayRole).toString().toStdString();
		mQueryWindow->setQueryId(id);
		std::cerr << "Setting Query Id to: " << id;
		std::cerr << std::endl;
	}
}


void DhtWindow::update()
{
	//std::cerr << "DhtWindow::update()" << std::endl;
	updateDhtPeers();
	updateDhtQueries();


}


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



void DhtWindow::updateDhtQueries()
{
	std::list<bdId>::iterator lit;
        std::map<bdNodeId, bdQueryStatus> queries;
        std::map<bdNodeId, bdQueryStatus>::iterator it;
        mPeerNet->get_dht_queries(queries);

	QTreeWidget *queryTreeWidget = ui->queryTreeWidget;

	queryTreeWidget->clear();

	time_t now = time(NULL);
	for(it = queries.begin(); it != queries.end(); it++)
	{
		/* find the entry */
		QTreeWidgetItem *item = NULL;

		/* insert */
		item = new QTreeWidgetItem();

		std::ostringstream statusstr;
		statusstr << (it->second).mStatus;

		std::ostringstream resultsstr;
		for(lit = (it->second).mResults.begin(); lit != (it->second).mResults.end(); lit++)
		{
			resultsstr << "[" << inet_ntoa(lit->addr.sin_addr) << ":" << htons(lit->addr.sin_port) << "] ";
		}

		std::ostringstream idstr;
		bdStdPrintNodeId(idstr, &(it->first));

		std::ostringstream flagsstr;
		flagsstr << "0x" << std::hex << std::setfill('0') << it->second.mQFlags;

		item -> setData(QTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(idstr.str()));
		item -> setData(QTW_COL_STATUS, Qt::DisplayRole, QString::fromStdString(statusstr.str()));
		item -> setData(QTW_COL_FLAGS, Qt::DisplayRole, QString::fromStdString(flagsstr.str()));
		item -> setData(QTW_COL_RESULTS, Qt::DisplayRole, QString::fromStdString(resultsstr.str()));

		queryTreeWidget->addTopLevelItem(item);
	}

}

