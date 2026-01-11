/*******************************************************************************
 * gui/statistics/RttStatistics.cpp                                            *
 *                                                                             *
 * Copyright (c) 2011 Retroshare Team <retroshare.project@gmail.com>           *
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

/*******************************************************************************
 * gui/statistics/RttStatistics.cpp                                            *
 *******************************************************************************/

#include <iostream>
#include <limits>
#include <QTimer>
#include <QObject>
#include <QPainter>
#include <QStylePainter>
#include <QHostAddress>
#include <QHash>

#include <retroshare/rsrtt.h>
#include <retroshare/rspeers.h>
#include "RttStatistics.h"
#include "time.h"

#include "gui/settings/rsharesettings.h"

// --- Custom Item for Sorting (Table) ---
class RttTreeItem : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;

    bool operator<(const QTreeWidgetItem &other) const {
        int sortCol = treeWidget() ? treeWidget()->sortColumn() : 0;

        if (sortCol == RttStatistics::COL_RTT) {
            QString txt1 = text(RttStatistics::COL_RTT);
            QString txt2 = other.text(RttStatistics::COL_RTT);
            long val1 = (txt1 == "?") ? std::numeric_limits<long>::max() : txt1.toLong();
            long val2 = (txt2 == "?") ? std::numeric_limits<long>::max() : txt2.toLong();
            return val1 < val2;
        }

        if (sortCol == RttStatistics::COL_IP_ADDRESS) {
            QString s1 = text(RttStatistics::COL_IP_ADDRESS);
            QString s2 = other.text(RttStatistics::COL_IP_ADDRESS);
            QHostAddress ip1(s1); QHostAddress ip2(s2);
            
            bool isTorI2p1 = s1.contains(".onion") || s1.contains(".i2p");
            bool isTorI2p2 = s2.contains(".onion") || s2.contains(".i2p");

            if (!ip1.isNull() && !ip2.isNull() && !isTorI2p1 && !isTorI2p2) {
                if (ip1.protocol() != ip2.protocol()) return ip1.protocol() < ip2.protocol();
                if (ip1.protocol() == QAbstractSocket::IPv4Protocol) return ip1.toIPv4Address() < ip2.toIPv4Address();
            }
            return s1 < s2;
        }
        return QTreeWidgetItem::operator<(other);
    }
};

// --- Main Constructor ---

RttStatistics::RttStatistics(QWidget * /*parent*/)
{
	setupUi(this) ;

	m_bProcessSettings = false;

    // Setup Graph in Tab 1
    _tunnel_statistics_F->setWidget( _tst_CW = new RttStatisticsGraph(this) ) ;
	_tunnel_statistics_F->setWidgetResizable(true);
	_tunnel_statistics_F->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_tunnel_statistics_F->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	_tunnel_statistics_F->viewport()->setBackgroundRole(QPalette::NoRole);
	_tunnel_statistics_F->setFrameStyle(QFrame::NoFrame);
	_tunnel_statistics_F->setFocusPolicy(Qt::NoFocus);

    // Setup Table in Tab 2
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setSortingEnabled(true);
    treeWidget->sortByColumn(1, Qt::AscendingOrder);

    // Timer for Table Update
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateRttValues()));
    m_timer->start(1000);

	// load settings
    processSettings(true);
}

RttStatistics::~RttStatistics()
{
    if(m_timer) {
        m_timer->stop();
        delete m_timer; // Explicitly delete the timer as requested
        m_timer = nullptr;
    }
    // save settings
    processSettings(false);
}

void RttStatistics::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("RttStatistics"));

    if (bLoad) {
        // load settings
        QByteArray state = Settings->value("TreeState").toByteArray();
        if (!state.isEmpty()) treeWidget->header()->restoreState(state);

        tabWidget->setCurrentIndex(Settings->value("ActiveTab", 0).toInt());
    } else {
        // save settings
        Settings->setValue("TreeState", treeWidget->header()->saveState());
        Settings->setValue("ActiveTab", tabWidget->currentIndex());
    }

    Settings->endGroup();

    m_bProcessSettings = false;
}

// --- Table Update Logic (O(N) Optimized) ---
void RttStatistics::updateRttValues()
{
    // Only update if the RTT tab is visible and the table view is selected
    if (!isVisible() || tabWidget->currentIndex() != 1) return;

    std::list<RsPeerId> idList;
    if (!rsPeers) return;
    rsPeers->getOnlineList(idList);

    // 1. Collect all current items into a hash map to track them.
    // We use the Peer ID (stored in UserRole) as the key.
    QHash<QString, QTreeWidgetItem*> existingItems;
    for(int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = treeWidget->topLevelItem(i);
        existingItems.insert(item->data(0, Qt::UserRole).toString(), item);
    }

    for(std::list<RsPeerId>::const_iterator it = idList.begin(); it != idList.end(); ++it)
    {
        std::string peerIdStr = (*it).toStdString();
        QString qPeerId = QString::fromStdString(peerIdStr);

        // Fetch RTT and peer details
        std::list<RsRttPongResult> results;
        rsRtt->getPongResults(*it, 1, results);
        int rttInMs = -1;
        if (!results.empty()) {
            float rttSec = results.back().mRTT;
            if (rttSec > 0.000001f) rttInMs = (int)(rttSec * 1000.0f);
        }

        RsPeerDetails details;
        rsPeers->getPeerDetails(*it, details);
        QString peerName = QString::fromUtf8(details.name.c_str());
        QString ipAddress = QString::fromStdString(details.extAddr);

        // 2. Take the item from the map. 
        // This removes it from the 'existingItems' hash so it won't be deleted later.
        QTreeWidgetItem* item = existingItems.take(qPeerId);

        if (!item) {
            // If the peer is new, create a new item
            item = new RttTreeItem(treeWidget);
            item->setData(0, Qt::UserRole, qPeerId);
        }

        // Update the row values using the enums defined in the header
        item->setText(COL_PEER_NAME, peerName);
        item->setText(COL_RTT, (rttInMs != -1) ? QString::number(rttInMs) : "?");
        item->setText(COL_NODE_ID, qPeerId);
        item->setText(COL_IP_ADDRESS, ipAddress);
    }

    // 3. Delete any items remaining in the hash map.
    // These correspond to peers that are no longer online or have been removed.
    qDeleteAll(existingItems);
}

// --- Graph Source Implementation (Unchanged logic) ---

QString RttGraphSource::unitName() const
{
    return QObject::tr("secs") ;
}

QString RttGraphSource::displayName(int i) const
{
    int n=0 ;
    for(std::map<std::string, std::list<std::pair<qint64,float> > >::const_iterator it=_points.begin();it!=_points.end();++it,++n)
        if(n==i)
            return QString::fromUtf8(rsPeers->getPeerName(RsPeerId(it->first)).c_str()) ;

    return QString() ;
}

void RttGraphSource::getValues(std::map<std::string,float>& vals) const
{
    std::list<RsPeerId> idList;
    rsPeers->getOnlineList(idList);

    vals.clear() ;
    std::list<RsRttPongResult> results ;

    for(std::list<RsPeerId>::const_iterator it(idList.begin());it!=idList.end();++it)
    {
        rsRtt->getPongResults(*it, 1, results);
        if(!results.empty()) {
            vals[(*it).toStdString()] = results.back().mRTT ;
        }
    }
}

RttStatisticsGraph::RttStatisticsGraph(QWidget *parent)
        : RSGraphWidget(parent)
{
    RttGraphSource *src = new RttGraphSource() ;

    src->setCollectionTimeLimit(10*60*1000) ; // 10 mins
    src->setCollectionTimePeriod(1000) ;     // collect every second
    src->setDigits(3) ;
    src->start() ;

    setSource(src) ;

    setTimeScale(2.0f) ; // 1 pixels per second of time.

    resetFlags(RSGRAPH_FLAGS_LOG_SCALE_Y) ;
    resetFlags(RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;
    setFlags(RSGRAPH_FLAGS_SHOW_LEGEND) ;

	int graphColor = Settings->valueFromGroup("BandwidthStatsWidget", "cmbGraphColor", 0).toInt();

	if(graphColor==0)
		resetFlags(RSGraphWidget::RSGraphWidget::RSGRAPH_FLAGS_DARK_STYLE);
	else
		setFlags(RSGraphWidget::RSGraphWidget::RSGRAPH_FLAGS_DARK_STYLE);
}
