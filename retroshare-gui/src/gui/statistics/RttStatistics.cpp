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

#include "RttStatistics.h"
#include <iostream>
#include <limits>
#include <QHeaderView>
#include <QHostAddress>
#include <retroshare/rsrtt.h>
#include <retroshare/rspeers.h>
#include "gui/settings/rsharesettings.h"

// --- CUSTOM SORTING CLASS ---
// Handles numeric sorting for RTT and IP addresses instead of default alphabetical sort
class RttTreeItem : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;

    bool operator<(const QTreeWidgetItem &other) const override {
        int sortCol = treeWidget() ? treeWidget()->sortColumn() : 0;

        // 1. RTT SORTING (Column 1)
        if (sortCol == 1) {
            QString txt1 = text(1);
            QString txt2 = other.text(1);

            // Treat "?" as infinite so it appears at the bottom when sorting ascending
            long val1 = (txt1 == "?") ? std::numeric_limits<long>::max() : txt1.toLong();
            long val2 = (txt2 == "?") ? std::numeric_limits<long>::max() : txt2.toLong();

            return val1 < val2;
        }

        // 2. SMART IP SORTING (Column 3)
        if (sortCol == 3) {
            QString s1 = text(3);
            QString s2 = other.text(3);

            QHostAddress ip1(s1);
            QHostAddress ip2(s2);

            // Check if addresses are Tor/I2P (non-numeric IPs)
            bool isTorI2p1 = s1.contains(".onion") || s1.contains(".i2p");
            bool isTorI2p2 = s2.contains(".onion") || s2.contains(".i2p");

            // If both are valid standard IPs (IPv4/IPv6) and NOT Tor/I2P
            if (!ip1.isNull() && !ip2.isNull() && !isTorI2p1 && !isTorI2p2) {
                // Sort by protocol first (IPv4 vs IPv6)
                if (ip1.protocol() != ip2.protocol())
                    return ip1.protocol() < ip2.protocol();

                // Sort IPv4 numerically
                if (ip1.protocol() == QAbstractSocket::IPv4Protocol)
                    return ip1.toIPv4Address() < ip2.toIPv4Address();
            }

            // Fallback to standard string sort (handles Tor/I2P/Mixed)
            return s1 < s2;
        }

        // For Name (0) and Node ID (2), standard alphabetical sort is fine
        return QTreeWidgetItem::operator<(other);
    }
};

// --- IMPLEMENTATION ---

RttStatistics::RttStatistics(QWidget *parent)
    : MainPage(parent)
{
    setupUi(this);

    m_bProcessSettings = false;

    // UI Configuration
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setSortingEnabled(true);

    // Default sort: RTT Ascending (Lowest ping first)
    treeWidget->sortByColumn(1, Qt::AscendingOrder);

    // Timer setup (1000ms = 1 second)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &RttStatistics::updateRttValues);
    m_timer->start(1000);

    // Initial immediate update
    updateRttValues();

    // Restore column layout (Must be done AFTER initial update to apply correctly)
    processSettings(true);
}

RttStatistics::~RttStatistics()
{
    if(m_timer) m_timer->stop();

    // Force save settings on destruction
    processSettings(false);
}

void RttStatistics::processSettings(bool bLoad)
{
    m_bProcessSettings = true;
    Settings->beginGroup(QString("RttStatistics"));

    if (bLoad) {
        // Load column configuration (widths, sort order, hidden columns)
        QByteArray state = Settings->value("TreeState").toByteArray();
        if (!state.isEmpty()) {
            treeWidget->header()->restoreState(state);
        }
    } else {
        // Save column configuration
        Settings->setValue("TreeState", treeWidget->header()->saveState());
    }

    Settings->endGroup();
    m_bProcessSettings = false;
}

void RttStatistics::updateRttValues()
{
    std::list<RsPeerId> idList;
    if (!rsPeers) return;

    rsPeers->getOnlineList(idList);
    QList<QString> currentPeerIds;

    for(const RsPeerId& pid : idList)
    {
        std::string peerIdStr = pid.toStdString();
        QString qPeerId = QString::fromStdString(peerIdStr);
        currentPeerIds.append(qPeerId);

        // 1. Get RTT Data
        std::list<RsRttPongResult> results;
        rsRtt->getPongResults(pid, 1, results);
        int rttInMs = -1;

        if (!results.empty()) {
            float rttSec = results.back().mRTT;
            // Only consider RTT valid if slightly positive
            if (rttSec > 0.000001f) rttInMs = (int)(rttSec * 1000.0f);
        }

        // 2. Get Peer Details
        RsPeerDetails details;
        rsPeers->getPeerDetails(pid, details);

        QString peerName = QString::fromUtf8(details.name.c_str());
        QString ipAddress = QString::fromStdString(details.extAddr);

        // 3. Update Table Item
        QTreeWidgetItem* item = nullptr;

        // Find existing item via hidden ID (UserRole)
        for(int i=0; i < treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem* temp = treeWidget->topLevelItem(i);
            if(temp->data(0, Qt::UserRole).toString() == qPeerId) {
                item = temp;
                break;
            }
        }

        if (!item) {
            // Create new item with custom sorting logic
            item = new RttTreeItem(treeWidget);
            // Store Peer ID in hidden data to identify the row later
            item->setData(0, Qt::UserRole, qPeerId);
        }

        // Col 0: Peer Name
        item->setText(0, peerName);

        // Col 1: RTT (Display number or "?")
        if (rttInMs != -1) item->setText(1, QString::number(rttInMs));
        else item->setText(1, "?");

        // Col 2: Node ID (Full Peer ID)
        item->setText(2, qPeerId);

        // Col 3: IP Address
        item->setText(3, ipAddress);
    }

    // 4. Cleanup disconnected peers
    for(int i = treeWidget->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem* item = treeWidget->topLevelItem(i);
        // If the item in table is not in the current online list, delete it
        if(!currentPeerIds.contains(item->data(0, Qt::UserRole).toString())) {
            delete item;
        }
    }
}
