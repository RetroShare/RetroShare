/*******************************************************************************
 * gui/statistics/CumulativeStatsWidget.h                                      *
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

#ifndef CUMULATIVE_STATS_WIDGET_H
#define CUMULATIVE_STATS_WIDGET_H

#include <retroshare-gui/RsAutoUpdatePage.h>
#include <QChartView>
#include <QChart>
#include <map>
#include <string>

class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class QPushButton;

// Check if we are on Qt 6 or higher
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #include <QtCharts/QChartView>
    #include <QtCharts/QChart>
    
    // Qt 6 uses the QtCharts namespace explicitly
    namespace QtCharts {
        class QChartView;
        class QChart;
    }
    // Create aliases
    using QChartView = QtCharts::QChartView;
    using QChart = QtCharts::QChart;
#else
    // Qt 5 branch
    #include <QChartView>
    #include <QChart>
    
    // Use the macro that Qt 5 provides for Charts
    QT_CHARTS_USE_NAMESPACE
#endif

class CumulativeStatsWidget : public RsAutoUpdatePage
{
    Q_OBJECT
public:
    CumulativeStatsWidget(QWidget *parent = nullptr);
    virtual ~CumulativeStatsWidget();
    virtual void updateDisplay();

private slots:
    void clearStatistics();

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
};

#endif // CUMULATIVE_STATS_WIDGET_H
