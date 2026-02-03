/*******************************************************************************
 * gui/statistics/CumulativeStatsWidget.h                                     *
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

#ifndef CUMULATIVE_STATS_WIDGET_H
#define CUMULATIVE_STATS_WIDGET_H

#include <retroshare-gui/RsAutoUpdatePage.h>
#include <QChartView>
#include <QChart>
#include <map>
#include <string>

#include <QTreeWidgetItem>

class QTreeWidget;
class QTabWidget;
class QPushButton;
class QCheckBox;

QT_CHARTS_USE_NAMESPACE

class StatsTreeWidgetItem : public QTreeWidgetItem
{
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem &other) const override;
};

class CumulativeStatsWidget : public RsAutoUpdatePage
{
    Q_OBJECT
public:
    CumulativeStatsWidget(QWidget *parent = nullptr);
    virtual ~CumulativeStatsWidget();

public slots:
    virtual void updateDisplay();

private slots:
    void clearStatistics();
    void handleItemChanged(QTreeWidgetItem *item, int column);
    void toggleAutoPeers(bool checked);
    void toggleAutoServices(bool checked);

private:
    void updatePeerStats();
    void updateServiceStats();
    
    // Helper to format bytes
    QString formatSize(uint64_t bytes);

    QTabWidget *tabWidget;
    
    // Peer tab
    QTreeWidget *peerTree;
    QChartView *peerBarChartView;
    QChartView *peerPieChartView;
    
    // Service tab
    QTreeWidget *serviceTree;
    QChartView *serviceBarChartView;
    QChartView *servicePieChartView;
    
    // Clear button
    QPushButton *clearButton;

    bool autoPeers;
    bool autoServices;
    QCheckBox *autoPeersCb;
    QCheckBox *autoServicesCb;
};

#endif // CUMULATIVE_STATS_WIDGET_H
