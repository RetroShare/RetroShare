/*******************************************************************************
 * gui/statistics/CumulativeStatsWidget.cpp                                   *
 *                                                                             *
 * Copyright (C) 2024                                                          *
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
#include "retroshare/rsservicecontrol.h"

#include <QCheckBox>

bool StatsTreeWidgetItem::operator<(const QTreeWidgetItem &other) const
{
    if (!treeWidget()) {
        return QTreeWidgetItem::operator<(other);
    }

    int col = treeWidget()->sortColumn();
    if (col >= 1 && col <= 3) {
        qulonglong v1 = data(col, Qt::UserRole).toULongLong();
        qulonglong v2 = other.data(col, Qt::UserRole).toULongLong();
        return v1 < v2;
    }
    return QTreeWidgetItem::operator<(other);
}

CumulativeStatsWidget::CumulativeStatsWidget(QWidget *parent)
    : RsAutoUpdatePage(5000, parent)
    , autoPeers(true)
    , autoServices(true)
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
    
    peerTree = new QTreeWidget();
    peerTree->setHeaderLabels(QStringList() << tr("Friend") << tr("Bytes In") << tr("Bytes Out") << tr("Total"));
    peerTree->setSortingEnabled(true);
    peerTree->header()->setStretchLastSection(true);
    peerTree->header()->resizeSection(0, 250);
    
    connect(peerTree, &QTreeWidget::itemChanged, this, &CumulativeStatsWidget::handleItemChanged);

    QSplitter *friendMainSplitter = new QSplitter(Qt::Vertical);
    friendMainSplitter->addWidget(friendChartSplitter);
    friendMainSplitter->addWidget(peerTree);
    friendMainSplitter->setStretchFactor(0, 2);
    friendMainSplitter->setStretchFactor(1, 1);

    // Auto mode toggle
    QHBoxLayout *friendControls = new QHBoxLayout();
    autoPeersCb = new QCheckBox(tr("Auto Selection (Top 5)"));
    autoPeersCb->setChecked(autoPeers);
    connect(autoPeersCb, &QCheckBox::toggled, this, &CumulativeStatsWidget::toggleAutoPeers);
    friendControls->addWidget(autoPeersCb);
    friendControls->addStretch();
    
    friendsLayout->addLayout(friendControls);
    friendsLayout->addWidget(friendMainSplitter);
    
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
    
    serviceTree = new QTreeWidget();
    serviceTree->setHeaderLabels(QStringList() << tr("Service") << tr("Bytes In") << tr("Bytes Out") << tr("Total"));
    serviceTree->setSortingEnabled(true);
    serviceTree->header()->setStretchLastSection(true);
    serviceTree->header()->resizeSection(0, 250);
    
    connect(serviceTree, &QTreeWidget::itemChanged, this, &CumulativeStatsWidget::handleItemChanged);

    QSplitter *serviceMainSplitter = new QSplitter(Qt::Vertical);
    serviceMainSplitter->addWidget(serviceChartSplitter);
    serviceMainSplitter->addWidget(serviceTree);
    serviceMainSplitter->setStretchFactor(0, 2);
    serviceMainSplitter->setStretchFactor(1, 1);

    // Auto mode toggle
    QHBoxLayout *serviceControls = new QHBoxLayout();
    autoServicesCb = new QCheckBox(tr("Auto Selection (Top 5)"));
    autoServicesCb->setChecked(autoServices);
    connect(autoServicesCb, &QCheckBox::toggled, this, &CumulativeStatsWidget::toggleAutoServices);
    serviceControls->addWidget(autoServicesCb);
    serviceControls->addStretch();

    servicesLayout->addLayout(serviceControls);
    servicesLayout->addWidget(serviceMainSplitter);
    
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

void CumulativeStatsWidget::toggleAutoPeers(bool checked)
{
    autoPeers = checked;
    updateDisplay();
}

void CumulativeStatsWidget::toggleAutoServices(bool checked)
{
    autoServices = checked;
    updateDisplay();
}

void CumulativeStatsWidget::handleItemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(item);
    if (column == 0 && !autoPeers && !autoServices) { // Only handle manual changes
        // Use QueuedConnection to avoid deleting the item while the view is still processing the click event
        QMetaObject::invokeMethod(this, "updateDisplay", Qt::QueuedConnection);
    }
}

void CumulativeStatsWidget::updatePeerStats()
{
    if (!rsConfig) return;
    
    std::map<RsPeerId, RsCumulativeTrafficStats> stats;
    if (!rsConfig->getCumulativeTrafficByPeer(stats)) return;
    
    // Save current checkbox states to avoid losing them
    QMap<QString, Qt::CheckState> previousStates;
    for (int i = 0; i < peerTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = peerTree->topLevelItem(i);
        QString peerIdStr = it->data(0, Qt::UserRole).toString();
        previousStates[peerIdStr] = it->checkState(0);
    }

    // Clear existing charts
    peerBarChartView->chart()->removeAllSeries();
    peerPieChartView->chart()->removeAllSeries();
    
    // Remove old axes
    for (QAbstractAxis *axis : peerBarChartView->chart()->axes()) {
        peerBarChartView->chart()->removeAxis(axis);
        delete axis;
    }
    
    peerTree->setSortingEnabled(false);
    peerTree->blockSignals(true);
    peerTree->clear();
    
    // Convert to vector for sorting (initial display order)
    std::vector<std::pair<RsPeerId, RsCumulativeTrafficStats>> sortedStats(stats.begin(), stats.end());
    std::sort(sortedStats.begin(), sortedStats.end(), [](const auto& a, const auto& b) {
        return (a.second.bytesIn + a.second.bytesOut) > (b.second.bytesIn + b.second.bytesOut);
    });

    QBarSet *bytesInSet = new QBarSet(tr("Bytes In"));
    QBarSet *bytesOutSet = new QBarSet(tr("Bytes Out"));
    QStringList categories;
    QPieSeries *pieSeries = new QPieSeries();
    pieSeries->setLabelsPosition(QPieSlice::LabelOutside);
    pieSeries->setPieSize(0.45); // Give more room for labels to breathe
    
    uint64_t totalAll = 0;
    for (const auto& s : stats) totalAll += (s.second.bytesIn + s.second.bytesOut);
    if (totalAll == 0) totalAll = 1;

    int count = 0;
    int displayedCount = 0;
    double otherIn = 0, otherOut = 0, otherTotal = 0;

    for (const auto& kv : sortedStats) {
        RsPeerDetails details;
        rsPeers->getPeerDetails(kv.first, details);
        QString name = QString::fromUtf8(details.name.c_str());
        uint64_t total = kv.second.bytesIn + kv.second.bytesOut;

        StatsTreeWidgetItem *item = new StatsTreeWidgetItem();
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setText(0, name);
        item->setText(1, formatSize(kv.second.bytesIn));
        item->setText(2, formatSize(kv.second.bytesOut));
        item->setText(3, formatSize(total));
        
        QString peerIdStr = QString::fromStdString(kv.first.toStdString());
        item->setData(0, Qt::UserRole, peerIdStr);
        item->setData(1, Qt::UserRole, (qulonglong)kv.second.bytesIn);
        item->setData(2, Qt::UserRole, (qulonglong)kv.second.bytesOut);
        item->setData(3, Qt::UserRole, (qulonglong)total);
        
        Qt::CheckState state;
        if (autoPeers) {
            state = (count < 5 && total > 0) ? Qt::Checked : Qt::Unchecked;
            item->setCheckState(0, state);
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled); // Disable interaction in auto mode
        } else {
            if (previousStates.contains(peerIdStr)) {
                state = previousStates[peerIdStr];
            } else {
                state = (count < 5 && total > 0) ? Qt::Checked : Qt::Unchecked;
            }
            item->setCheckState(0, state);
        }
        
        peerTree->addTopLevelItem(item);

        double ratio = (double)total / (double)totalAll;
        if (item->checkState(0) == Qt::Checked && total > 0 && ratio >= 0.02 && displayedCount < 5) {
            QString chartName = name;
            if (chartName.length() > 20) chartName = chartName.left(17) + "...";
            categories << chartName;
            *bytesInSet << (kv.second.bytesIn / (1024.0 * 1024.0));
            *bytesOutSet << (kv.second.bytesOut / (1024.0 * 1024.0));
            
            QPieSlice *slice = pieSeries->append(chartName, (double)total);
            slice->setLabelVisible(true);
            slice->setLabelArmLengthFactor((displayedCount % 2 == 0) ? 0.1 : 0.2);
            slice->setLabel(QString("%1<br/>%2 (%3%)").arg(chartName).arg(formatSize(total)).arg(ratio * 100.0, 0, 'f', 1));
            displayedCount++;
        } else if (total > 0) {
            otherIn += (double)kv.second.bytesIn;
            otherOut += (double)kv.second.bytesOut;
            otherTotal += (double)total;
        }
        count++;
    }

    // Always add Others, even if 0
    categories << tr("Others");
    *bytesInSet << (otherIn / (1024.0 * 1024.0));
    *bytesOutSet << (otherOut / (1024.0 * 1024.0));

    QPieSlice *otherSlice = pieSeries->append(tr("Others"), otherTotal + 0.0001); // Minor offset to ensure it's "present"
    double otherRatio = (otherTotal / (double)totalAll) * 100.0;
    otherSlice->setLabelVisible(true);
    otherSlice->setLabelArmLengthFactor(0.15);
    otherSlice->setLabel(QString("%1<br/>%2 (%3%)").arg(tr("Others")).arg(formatSize((uint64_t)otherTotal)).arg(otherRatio, 0, 'f', 1));
    
    peerTree->blockSignals(false);
    peerTree->setSortingEnabled(true);

    if (!categories.isEmpty()) {
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
        axisY->applyNiceNumbers();
        peerBarChartView->chart()->addAxis(axisY, Qt::AlignLeft);
        barSeries->attachAxis(axisY);
        
        peerPieChartView->chart()->addSeries(pieSeries);
    }
}

void CumulativeStatsWidget::updateDisplay()
{
    updatePeerStats();
    updateServiceStats();
}

void CumulativeStatsWidget::updateServiceStats()
{
    if (!rsConfig) return;
    
    std::map<uint16_t, RsCumulativeTrafficStats> stats;
    if (!rsConfig->getCumulativeTrafficByService(stats)) return;
    
    // Save current checkbox states
    QMap<uint16_t, Qt::CheckState> previousStates;
    for (int i = 0; i < serviceTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = serviceTree->topLevelItem(i);
        uint16_t serviceId = it->data(0, Qt::UserRole).toUInt();
        previousStates[serviceId] = it->checkState(0);
    }

    // Clear existing charts
    serviceBarChartView->chart()->removeAllSeries();
    servicePieChartView->chart()->removeAllSeries();
    
    for (QAbstractAxis *axis : serviceBarChartView->chart()->axes()) {
        serviceBarChartView->chart()->removeAxis(axis);
        delete axis;
    }
    
    serviceTree->setSortingEnabled(false);
    serviceTree->blockSignals(true);
    serviceTree->clear();
    
    // Service name mapping with fallbacks
    QMap<uint16_t, QString> fallbackNames;
    fallbackNames[0x0011] = tr("Discovery");
    fallbackNames[0x0012] = tr("Chat");
    fallbackNames[0x0013] = tr("Messages");
    fallbackNames[0x0014] = tr("Turtle Router");
    fallbackNames[0x0016] = tr("Heartbeat");
    fallbackNames[0x0017] = tr("File Transfer");
    fallbackNames[0x0019] = tr("File Database");
    fallbackNames[0x0021] = tr("Bandwidth Control");
    fallbackNames[0x0102] = tr("State");
    fallbackNames[0x0211] = tr("GXS Identities");
    fallbackNames[0x0215] = tr("GXS Forums");
    fallbackNames[0x0216] = tr("GXS Posted");
    fallbackNames[0x0217] = tr("GXS Channels");
    fallbackNames[0x0218] = tr("GXS Circles");
    fallbackNames[0x1011] = tr("Round Trip Time");

    std::vector<std::pair<uint16_t, RsCumulativeTrafficStats>> sortedStats(stats.begin(), stats.end());
    std::sort(sortedStats.begin(), sortedStats.end(), [](const auto& a, const auto& b) {
        return (a.second.bytesIn + a.second.bytesOut) > (b.second.bytesIn + b.second.bytesOut);
    });

    QBarSet *bytesInSet = new QBarSet(tr("Bytes In"));
    QBarSet *bytesOutSet = new QBarSet(tr("Bytes Out"));
    QStringList categories;
    QPieSeries *pieSeries = new QPieSeries();
    pieSeries->setLabelsPosition(QPieSlice::LabelOutside);
    pieSeries->setPieSize(0.45);

    uint64_t totalAll = 0;
    for (const auto& s : stats) totalAll += (s.second.bytesIn + s.second.bytesOut);
    if (totalAll == 0) totalAll = 1;

    int count = 0;
    int displayedCount = 0;
    double otherIn = 0, otherOut = 0, otherTotal = 0;

    for (const auto& kv : sortedStats) {
        // Try to get name from service control first
        QString name = QString::fromStdString(rsServiceControl->getServiceName(RsServiceInfo::RsServiceInfoUIn16ToFullServiceId(kv.first)));
        
        // Fallback to manual map if empty
        if (name.isEmpty()) {
            name = fallbackNames.value(kv.first, QString("Service 0x%1").arg(kv.first, 4, 16, QChar('0')));
        }
        uint64_t total = kv.second.bytesIn + kv.second.bytesOut;

        StatsTreeWidgetItem *item = new StatsTreeWidgetItem();
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setText(0, name);
        item->setText(1, formatSize(kv.second.bytesIn));
        item->setText(2, formatSize(kv.second.bytesOut));
        item->setText(3, formatSize(total));
        
        uint16_t serviceId = kv.first;
        item->setData(0, Qt::UserRole, (uint)serviceId);
        item->setData(1, Qt::UserRole, (qulonglong)kv.second.bytesIn);
        item->setData(2, Qt::UserRole, (qulonglong)kv.second.bytesOut);
        item->setData(3, Qt::UserRole, (qulonglong)total);
        
        Qt::CheckState state;
        if (autoServices) {
            state = (count < 5 && total > 0) ? Qt::Checked : Qt::Unchecked;
            item->setCheckState(0, state);
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        } else {
            if (previousStates.contains(serviceId)) {
                state = previousStates[serviceId];
            } else {
                state = (count < 5 && total > 0) ? Qt::Checked : Qt::Unchecked;
            }
            item->setCheckState(0, state);
        }
        
        serviceTree->addTopLevelItem(item);

        double ratio = (double)total / (double)totalAll;
        if (item->checkState(0) == Qt::Checked && total > 0 && ratio >= 0.02 && displayedCount < 5) {
            categories << name;
            *bytesInSet << (kv.second.bytesIn / (1024.0 * 1024.0));
            *bytesOutSet << (kv.second.bytesOut / (1024.0 * 1024.0));
            
            QPieSlice *slice = pieSeries->append(name, (double)total);
            slice->setLabelVisible(true);
            slice->setLabelArmLengthFactor((displayedCount % 2 == 0) ? 0.1 : 0.2);
            slice->setLabel(QString("%1<br/>%2 (%3%)").arg(name).arg(formatSize(total)).arg(ratio * 100.0, 0, 'f', 1));
            displayedCount++;
        } else if (total > 0) {
            otherIn += (double)kv.second.bytesIn;
            otherOut += (double)kv.second.bytesOut;
            otherTotal += (double)total;
        }
        count++;
    }

    // Always add Others
    categories << tr("Others");
    *bytesInSet << (otherIn / (1024.0 * 1024.0));
    *bytesOutSet << (otherOut / (1024.0 * 1024.0));

    QPieSlice *otherSlice = pieSeries->append(tr("Others"), otherTotal + 0.0001);
    double otherRatio = (otherTotal / (double)totalAll) * 100.0;
    otherSlice->setLabelVisible(true);
    otherSlice->setLabelArmLengthFactor(0.15);
    otherSlice->setLabel(QString("%1<br/>%2 (%3%)").arg(tr("Others")).arg(formatSize((uint64_t)otherTotal)).arg(otherRatio, 0, 'f', 1));

    serviceTree->blockSignals(false);
    serviceTree->setSortingEnabled(true);

    if (!categories.isEmpty()) {
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
        axisY->applyNiceNumbers();
        serviceBarChartView->chart()->addAxis(axisY, Qt::AlignLeft);
        barSeries->attachAxis(axisY);
        
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
