/*******************************************************************************
 * gui/statistics/CumulativeStatsWidget.cpp                                    *
 *                                                                             *
 * Copyright (C) 2026                                                          *
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

#include "CumulativeStatsWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QSplitter>

// Qt Charts includes
#include <QBarSeries>
#include <QBarSet>
#include <QPieSeries>
#include <QPieSlice>
#include <QBarCategoryAxis>
#include <QValueAxis>

#include "retroshare/rsconfig.h"
#include "retroshare/rspeers.h"
#include "retroshare/rstypes.h"

CumulativeStatsWidget::CumulativeStatsWidget(QWidget *parent)
    : RsAutoUpdatePage(5000, parent)  // 5 seconds to reduce CPU usage
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    tabWidget = new QTabWidget(this);
    
    // === FRIENDS TAB ===
    QWidget *friendsTab = new QWidget();
    QVBoxLayout *friendsLayout = new QVBoxLayout(friendsTab);
    
    // Charts container
    QSplitter *friendChartSplitter = new QSplitter(Qt::Horizontal);
    
    // Bar Chart for friends
    peerBarChartView = new QChartView(new QChart());
    peerBarChartView->setRenderHint(QPainter::Antialiasing);
    peerBarChartView->chart()->setTitle("Data Transfer per Friend");
    peerBarChartView->chart()->setAnimationOptions(QChart::NoAnimation);
    peerBarChartView->chart()->legend()->setVisible(true);
    peerBarChartView->chart()->legend()->setAlignment(Qt::AlignBottom);
    
    // Pie Chart for friends
    peerPieChartView = new QChartView(new QChart());
    peerPieChartView->setRenderHint(QPainter::Antialiasing);
    peerPieChartView->chart()->setTitle("Traffic Distribution");
    peerPieChartView->chart()->legend()->setVisible(true);
    peerPieChartView->chart()->legend()->setAlignment(Qt::AlignRight);
    
    friendChartSplitter->addWidget(peerBarChartView);
    friendChartSplitter->addWidget(peerPieChartView);
    friendChartSplitter->setStretchFactor(0, 3);
    friendChartSplitter->setStretchFactor(1, 2);
    
    // Table for friends
    peerTree = new QTreeWidget();
    peerTree->setHeaderLabels(QStringList() << tr("Friend") << tr("Bytes In") << tr("Bytes Out") << tr("Total"));
    peerTree->setSortingEnabled(true);
    peerTree->header()->setStretchLastSection(false);
    peerTree->header()->resizeSection(0, 200);
    
    friendsLayout->addWidget(friendChartSplitter, 3);
    friendsLayout->addWidget(peerTree, 1);
    
    tabWidget->addTab(friendsTab, tr("Friends"));
    
    // === SERVICES TAB ===
    QWidget *servicesTab = new QWidget();
    QVBoxLayout *servicesLayout = new QVBoxLayout(servicesTab);
    
    // Charts container
    QSplitter *serviceChartSplitter = new QSplitter(Qt::Horizontal);
    
    // Bar Chart for services
    serviceBarChartView = new QChartView(new QChart());
    serviceBarChartView->setRenderHint(QPainter::Antialiasing);
    serviceBarChartView->chart()->setTitle("Data Transfer per Service");
    serviceBarChartView->chart()->setAnimationOptions(QChart::NoAnimation);
    serviceBarChartView->chart()->legend()->setVisible(true);
    serviceBarChartView->chart()->legend()->setAlignment(Qt::AlignBottom);
    
    // Pie Chart for services
    servicePieChartView = new QChartView(new QChart());
    servicePieChartView->setRenderHint(QPainter::Antialiasing);
    servicePieChartView->chart()->setTitle("Service Distribution");
    servicePieChartView->chart()->legend()->setVisible(true);
    servicePieChartView->chart()->legend()->setAlignment(Qt::AlignRight);
    
    serviceChartSplitter->addWidget(serviceBarChartView);
    serviceChartSplitter->addWidget(servicePieChartView);
    serviceChartSplitter->setStretchFactor(0, 3);
    serviceChartSplitter->setStretchFactor(1, 2);
    
    // Table for services
    serviceTree = new QTreeWidget();
    serviceTree->setHeaderLabels(QStringList() << tr("Service") << tr("Bytes In") << tr("Bytes Out") << tr("Total"));
    serviceTree->setSortingEnabled(true);
    serviceTree->header()->setStretchLastSection(false);
    serviceTree->header()->resizeSection(0, 200);
    
    servicesLayout->addWidget(serviceChartSplitter, 3);
    servicesLayout->addWidget(serviceTree, 1);
    
    tabWidget->addTab(servicesTab, tr("Services"));
    
    // Clear button
    clearButton = new QPushButton(tr("Clear All Statistics"));
    connect(clearButton, &QPushButton::clicked, this, &CumulativeStatsWidget::clearStatistics);
    
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(clearButton);
    
    setLayout(mainLayout);
}

CumulativeStatsWidget::~CumulativeStatsWidget()
{
}

void CumulativeStatsWidget::updateDisplay()
{
    updatePeerStats();
    updateServiceStats();
}

void CumulativeStatsWidget::updatePeerStats()
{
    if (!rsConfig) return;
    
    std::map<RsPeerId, RsCumulativeTrafficStats> stats;
    if (!rsConfig->getCumulativeTrafficByPeer(stats)) return;
    
    // Clear existing charts - remove series AND axes to prevent accumulation
    peerBarChartView->chart()->removeAllSeries();
    peerPieChartView->chart()->removeAllSeries();
    
    // Remove old axes (critical fix for accumulation bug)
    for (QAbstractAxis *axis : peerBarChartView->chart()->axes()) {
        peerBarChartView->chart()->removeAxis(axis);
        delete axis;
    }
    
    // Prepare data
    QBarSet *bytesInSet = new QBarSet(tr("Bytes In"));
    QBarSet *bytesOutSet = new QBarSet(tr("Bytes Out"));
    QStringList categories;
    QPieSeries *pieSeries = new QPieSeries();
    
    peerTree->clear();
    
    int count = 0;
    const int maxItems = 10; // Limit to top 10 for readability
    
    for (const auto& kv : stats) {
        if (count >= maxItems) break;
        
        RsPeerDetails details;
        rsPeers->getPeerDetails(kv.first, details);
        QString name = QString::fromUtf8(details.name.c_str());
        if (name.length() > 20) name = name.left(17) + "...";
        
        categories << name;
        
        // Bar chart data (convert to MB for better scale)
        double mbIn = kv.second.bytesIn / (1024.0 * 1024.0);
        double mbOut = kv.second.bytesOut / (1024.0 * 1024.0);
        *bytesInSet << mbIn;
        *bytesOutSet << mbOut;
        
        // Pie chart data (total)
        uint64_t total = kv.second.bytesIn + kv.second.bytesOut;
        if (total > 0) {
            QPieSlice *slice = pieSeries->append(name, (double)total);
            slice->setLabelVisible(true);
            slice->setLabel(QString("%1: %2").arg(name).arg(formatSize(total)));
        }
        
        // Tree widget
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, QString::fromUtf8(details.name.c_str()));
        item->setText(1, formatSize(kv.second.bytesIn));
        item->setText(2, formatSize(kv.second.bytesOut));
        item->setText(3, formatSize(total));
        peerTree->addTopLevelItem(item);
        
        count++;
    }
    
    if (!categories.isEmpty()) {
        // Configure Bar Chart
        QBarSeries *barSeries = new QBarSeries();
        barSeries->append(bytesInSet);
        barSeries->append(bytesOutSet);
        
        peerBarChartView->chart()->addSeries(barSeries);
        
        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        peerBarChartView->chart()->addAxis(axisX, Qt::AlignBottom);
        barSeries->attachAxis(axisX);
        
        QValueAxis *axisY = new QValueAxis();
        axisY->setTitleText("MB");
        axisY->setLabelFormat("%.1f");
        peerBarChartView->chart()->addAxis(axisY, Qt::AlignLeft);
        barSeries->attachAxis(axisY);
        
        // Configure Pie Chart
        peerPieChartView->chart()->addSeries(pieSeries);
    }
}

void CumulativeStatsWidget::updateServiceStats()
{
    if (!rsConfig) return;
    
    std::map<uint16_t, RsCumulativeTrafficStats> stats;
    if (!rsConfig->getCumulativeTrafficByService(stats)) return;
    
    // Clear existing charts - remove series AND axes to prevent accumulation
    serviceBarChartView->chart()->removeAllSeries();
    servicePieChartView->chart()->removeAllSeries();
    
    // Remove old axes (critical fix for accumulation bug)
    for (QAbstractAxis *axis : serviceBarChartView->chart()->axes()) {
        serviceBarChartView->chart()->removeAxis(axis);
        delete axis;
    }
    
    // Prepare data
    QBarSet *bytesInSet = new QBarSet(tr("Bytes In"));
    QBarSet *bytesOutSet = new QBarSet(tr("Bytes Out"));
    QStringList categories;
    QPieSeries *pieSeries = new QPieSeries();
    
    serviceTree->clear();
   
    // Service name mapping
    QMap<uint16_t, QString> serviceNames;
    QMap<uint16_t, QString> serviceNames;
    serviceNames[0x0011] = "Discovery";
    serviceNames[0x0012] = "Chat";
    serviceNames[0x0013] = "Messages";
    serviceNames[0x0014] = "Turtle Router";
    serviceNames[0x0016] = "Heartbeat";
    serviceNames[0x0017] = "File Transfer";
    serviceNames[0x0018] = "Global Router";
    serviceNames[0x0019] = "File Database";
    serviceNames[0x0020] = "Service Info";
    serviceNames[0x0021] = "Bandwidth";
    serviceNames[0x0101] = "Banlist";
    serviceNames[0x0102] = "Status";
    serviceNames[0x0028] = "GXS Tunnels";
    serviceNames[0x0211] = "GXS Identity";
    serviceNames[0x0213] = "GXS Wiki";
    serviceNames[0x0214] = "GXS Wire";
    serviceNames[0x0215] = "GXS Forums";
    serviceNames[0x0216] = "GXS Boards";
    serviceNames[0x0217] = "GXS Channels";
    serviceNames[0x0218] = "GXS Circles";
    serviceNames[0x0219] = "GXS Reputation";
    serviceNames[0x0230] = "GXS Mails";
    serviceNames[0x1011] = "RTT";
    serviceNames[0x2003] = "FeedReader";
    
    for (const auto& kv : stats) {
        QString name = serviceNames.value(kv.first, QString("Service 0x%1").arg(kv.first, 4, 16, QChar('0')));
        
        categories << name;
        
        // Bar chart data (MB)
        double mbIn = kv.second.bytesIn / (1024.0 * 1024.0);
        double mbOut = kv.second.bytesOut / (1024.0 * 1024.0);
        *bytesInSet << mbIn;
        *bytesOutSet << mbOut;
        
        // Pie chart data
        uint64_t total = kv.second.bytesIn + kv.second.bytesOut;
        if (total > 0) {
            QPieSlice *slice = pieSeries->append(name, (double)total);
            slice->setLabelVisible(true);
            slice->setLabel(QString("%1: %2").arg(name).arg(formatSize(total)));
        }
        
        // Tree widget
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, name);
        item->setText(1, formatSize(kv.second.bytesIn));
        item->setText(2, formatSize(kv.second.bytesOut));
        item->setText(3, formatSize(total));
        serviceTree->addTopLevelItem(item);
    }
    
    if (!categories.isEmpty()) {
        // Configure Bar Chart
        QBarSeries *barSeries = new QBarSeries();
        barSeries->append(bytesInSet);
        barSeries->append(bytesOutSet);
        
        serviceBarChartView->chart()->addSeries(barSeries);
        
        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        serviceBarChartView->chart()->addAxis(axisX, Qt::AlignBottom);
        barSeries->attachAxis(axisX);
        
        QValueAxis *axisY = new QValueAxis();
        axisY->setTitleText("MB");
        axisY->setLabelFormat("%.1f");
        serviceBarChartView->chart()->addAxis(axisY, Qt::AlignLeft);
        barSeries->attachAxis(axisY);
        
        // Configure Pie Chart
        servicePieChartView->chart()->addSeries(pieSeries);
    }
}

QString CumulativeStatsWidget::formatSize(uint64_t bytes)
{
    const char *sizes[] = { "B", "KB", "MB", "GB", "TB" };
    int i = 0;
    double dblByte = bytes;
    while (dblByte >= 1024 && i < 4) {
        dblByte /= 1024;
        i++;
    }
    return QString::number(dblByte, 'f', 2) + " " + sizes[i];
}

void CumulativeStatsWidget::clearStatistics()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Clear Statistics"),
        tr("Are you sure you want to clear all cumulative traffic statistics? This cannot be undone."),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes && rsConfig) {
        rsConfig->clearCumulativeTraffic(true, true);
        updateDisplay();
    }
}
