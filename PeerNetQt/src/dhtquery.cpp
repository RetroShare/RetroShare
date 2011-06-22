#include "dhtquery.h"
#include "ui_dhtquery.h"
#include <QTimer>
#include <QDateTime>

#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>

DhtQuery::DhtQuery(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DhtQuery)
{
    ui->setupUi(this);

	// tick for gui update.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);

	// connect add Peer button.
    	//connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addPeer()));
    	//connect(ui->chatLineEdit, SIGNAL(returnPressed()), this, SLOT(sendChat()));
}

DhtQuery::~DhtQuery()
{
    delete ui;
}

void DhtQuery::changeEvent(QEvent *e)
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

void DhtQuery::setPeerNet(PeerNet *pnet)
{
	mPeerNet = pnet;
}

void DhtQuery::setQueryId(std::string id)
{
	mQueryId = id;
}

void DhtQuery::update()
{
	//std::cerr << "DhtQuery::update()" << std::endl;
	updateDhtQuery();


}


#define QTW_COL_BUCKET	0
#define QTW_COL_PEERID	1
#define QTW_COL_IPADDR	2
#define QTW_COL_FLAGS	3
#define QTW_COL_FOUND	4
#define QTW_COL_SEND	5
#define QTW_COL_RECV	6

void DhtQuery::updateDhtQuery()
{

	/* Hackish display of all Dht peers, should be split into buckets (as children) */
	time_t now = time(NULL);
	
	bdQuerySummary query;
	if (!mPeerNet->get_query_status(mQueryId, query))
	{
		return;
	}

	ui->queryLine->setText(QString::fromStdString(mQueryId));

	std::ostringstream statestr;
	switch(query.mState)
	{
		default:
			statestr << "Unknown";
			break;
		case BITDHT_QUERY_READY:
			statestr << "Ready";
			break;
		case BITDHT_QUERY_QUERYING:
			statestr << "Querying";
			break;
		case BITDHT_QUERY_FAILURE:
			statestr << "Failure";
			break;
		case BITDHT_QUERY_FOUND_CLOSEST:
			statestr << "Found Closest";
			break;
		case BITDHT_QUERY_PEER_UNREACHABLE:
			statestr << "Unreachable";
			break;
		case BITDHT_QUERY_SUCCESS:
			statestr << "Success";
			break;
	}
	ui->queryStatusLabel->setText(QString::fromStdString(statestr.str()));


	std::ostringstream extrastr;
	extrastr << "QueryLimit: ";
	bdStdPrintNodeId(extrastr, &(query.mLimit));
	extrastr << " QueryFlags: " << query.mQueryFlags;
	extrastr << " QueryAge: " << now - query.mQueryTS;
	extrastr << " SearchTime: " << query.mSearchTime;
	extrastr << " IdleRetryPeriod: " << query.mQueryIdlePeerRetryPeriod;

	ui->queryExtraLabel->setText(QString::fromStdString(extrastr.str()));

	std::multimap<bdMetric, bdPeer>::iterator cit;
	std::list<bdPeer>::iterator lit;

	//std::multimap<bdMetric, bdPeer>  mClosest;
	//std::multimap<bdMetric, bdPeer>  mPotentialClosest;
	//std::list<bdPeer>  mPotentialProxies;

	QTreeWidget *qcTreeWidget = ui->closestTreeWidget;
	qcTreeWidget->clear();


	for(cit = query.mClosest.begin(); cit != query.mClosest.end(); cit++)
	{
		/* find the entry */
		QTreeWidgetItem *item = NULL;
		bdPeer *bdp = &(cit->second);
		
		/* insert */
		item = new QTreeWidgetItem();

		int dist = bdStdBucketDistance(&(cit->first));

		std::ostringstream buckstr;
		buckstr << "(" << std::setw(3) << std::setfill('0') << dist << ") ";
		bdStdPrintNodeId(buckstr, &(cit->first));

		std::ostringstream ipstr;
		ipstr << inet_ntoa(bdp->mPeerId.addr.sin_addr);
		ipstr << ":" << ntohs(bdp->mPeerId.addr.sin_port);

		std::ostringstream idstr;
		bdStdPrintNodeId(idstr, &(bdp->mPeerId.id));

		std::ostringstream flagsstr;
		flagsstr << "0x" << std::hex << std::setfill('0') << bdp->mPeerFlags;

		std::ostringstream foundstr;
		foundstr << now - bdp->mFoundTime << " secs ago";

		std::ostringstream lastsendstr;
		if (bdp->mLastSendTime == 0)
		{
			lastsendstr << "never";
		}
		else
		{
			lastsendstr << now - bdp->mLastSendTime << " secs ago";
		}

		std::ostringstream lastrecvstr;
		if (bdp->mLastRecvTime == 0)
		{
			lastrecvstr << "never";
		}
		else
		{
			lastrecvstr << now - bdp->mLastRecvTime << " secs ago";
		}

		item -> setData(QTW_COL_BUCKET, Qt::DisplayRole, QString::fromStdString(buckstr.str()));
		item -> setData(QTW_COL_IPADDR, Qt::DisplayRole, QString::fromStdString(ipstr.str()));
		item -> setData(QTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(idstr.str()));
		item -> setData(QTW_COL_FLAGS, Qt::DisplayRole, QString::fromStdString(flagsstr.str()));

		item -> setData(QTW_COL_FOUND, Qt::DisplayRole, QString::fromStdString(foundstr.str()));
		item -> setData(QTW_COL_SEND, Qt::DisplayRole, QString::fromStdString(lastsendstr.str()));
		item -> setData(QTW_COL_RECV, Qt::DisplayRole, QString::fromStdString(lastrecvstr.str()));

		qcTreeWidget->addTopLevelItem(item);
	}


	QTreeWidget *qpcTreeWidget = ui->potTreeWidget;
	qpcTreeWidget->clear();

	for(cit = query.mPotentialClosest.begin(); cit != query.mPotentialClosest.end(); cit++)
	{
		/* find the entry */
		QTreeWidgetItem *item = NULL;
		bdPeer *bdp = &(cit->second);

		/* insert */
		item = new QTreeWidgetItem();

                int dist = bdStdBucketDistance(&(cit->first));

		std::ostringstream buckstr;
		buckstr << "(" << std::setw(3) << std::setfill('0') << dist << ") ";
		bdStdPrintNodeId(buckstr, &(cit->first));

		std::ostringstream ipstr;
		ipstr << inet_ntoa(bdp->mPeerId.addr.sin_addr);
		ipstr << ":" << ntohs(bdp->mPeerId.addr.sin_port);

		std::ostringstream idstr;
		bdStdPrintNodeId(idstr, &(bdp->mPeerId.id));

		std::ostringstream flagsstr;
		flagsstr << "0x" << std::hex << std::setfill('0') << bdp->mPeerFlags;

		std::ostringstream foundstr;
		foundstr << now - bdp->mFoundTime << " secs ago";

		std::ostringstream lastsendstr;
		if (bdp->mLastSendTime == 0)
		{
			lastsendstr << "never";
		}
		else
		{
			lastsendstr << now - bdp->mLastSendTime << " secs ago";
		}

		std::ostringstream lastrecvstr;
		if (bdp->mLastRecvTime == 0)
		{
			lastrecvstr << "never";
		}
		else
		{
			lastrecvstr << now - bdp->mLastRecvTime << " secs ago";
		}

		item -> setData(QTW_COL_BUCKET, Qt::DisplayRole, QString::fromStdString(buckstr.str()));
		item -> setData(QTW_COL_IPADDR, Qt::DisplayRole, QString::fromStdString(ipstr.str()));
		item -> setData(QTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(idstr.str()));
		item -> setData(QTW_COL_FLAGS, Qt::DisplayRole, QString::fromStdString(flagsstr.str()));

		item -> setData(QTW_COL_FOUND, Qt::DisplayRole, QString::fromStdString(foundstr.str()));
		item -> setData(QTW_COL_SEND, Qt::DisplayRole, QString::fromStdString(lastsendstr.str()));
		item -> setData(QTW_COL_RECV, Qt::DisplayRole, QString::fromStdString(lastrecvstr.str()));

		qpcTreeWidget->addTopLevelItem(item);
	}

	QTreeWidget *qpTreeWidget = ui->proxyTreeWidget;
	qpTreeWidget->clear();

	for(lit = query.mPotentialProxies.begin(); lit != query.mPotentialProxies.end(); lit++)
	{
		/* find the entry */
		QTreeWidgetItem *item = NULL;
		bdPeer *bdp = &(*lit);

		/* insert */
		item = new QTreeWidgetItem();

		std::ostringstream buckstr;
		buckstr << "n/a";
		//bdStdPrintNodeId(buckstr, &(lit->first));

		std::ostringstream ipstr;
		ipstr << inet_ntoa(bdp->mPeerId.addr.sin_addr);
		ipstr << ":" << ntohs(bdp->mPeerId.addr.sin_port);

		std::ostringstream idstr;
		bdStdPrintNodeId(idstr, &(bdp->mPeerId.id));

		std::ostringstream flagsstr;
		flagsstr << "0x" << std::hex << std::setfill('0') << bdp->mPeerFlags;

		std::ostringstream foundstr;
		foundstr << now - bdp->mFoundTime << " secs ago";

		std::ostringstream lastsendstr;
		if (bdp->mLastSendTime == 0)
		{
			lastsendstr << "never";
		}
		else
		{
			lastsendstr << now - bdp->mLastSendTime << " secs ago";
		}

		std::ostringstream lastrecvstr;
		if (bdp->mLastRecvTime == 0)
		{
			lastrecvstr << "never";
		}
		else
		{
			lastrecvstr << now - bdp->mLastRecvTime << " secs ago";
		}

		item -> setData(QTW_COL_BUCKET, Qt::DisplayRole, QString::fromStdString(buckstr.str()));
		item -> setData(QTW_COL_IPADDR, Qt::DisplayRole, QString::fromStdString(ipstr.str()));
		item -> setData(QTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(idstr.str()));
		item -> setData(QTW_COL_FLAGS, Qt::DisplayRole, QString::fromStdString(flagsstr.str()));

		item -> setData(QTW_COL_FOUND, Qt::DisplayRole, QString::fromStdString(foundstr.str()));
		item -> setData(QTW_COL_SEND, Qt::DisplayRole, QString::fromStdString(lastsendstr.str()));
		item -> setData(QTW_COL_RECV, Qt::DisplayRole, QString::fromStdString(lastrecvstr.str()));

		qpTreeWidget->addTopLevelItem(item);
	}
}



