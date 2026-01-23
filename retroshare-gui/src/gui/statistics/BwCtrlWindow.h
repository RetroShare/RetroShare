/*******************************************************************************
 * gui/statistics/BwCtrlWindow.h                                               *
 *                                                                             *
 * Copyright (c) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
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

#pragma once

#include <QMainWindow>

#include <QAbstractItemDelegate>

#include <retroshare-gui/RsAutoUpdatePage.h>
#include "gui/common/RSGraphWidget.h"
#include "ui_BwCtrlWindow.h"

// Defines for download list list columns
#define COLUMN_RSNAME 0
#define COLUMN_PEERID 1
#define COLUMN_IN_RATE 2
#define COLUMN_IN_MAX 3
#define COLUMN_IN_QUEUE_ITEMS 4
#define COLUMN_OUT_RATE 5
#define COLUMN_OUT_MAX 6
#define COLUMN_OUT_ALLOWED 7
#define COLUMN_OUT_QUEUE_ITEMS 8
#define COLUMN_OUT_QUEUE_BYTES 9
#define COLUMN_DRAIN 10
#define COLUMN_SESSION_IN 11
#define COLUMN_SESSION_OUT 12
#define COLUMN_COUNT 13

class QModelIndex;
class QPainter;
class BWListDelegate ;

/**
 * Custom Item to force Totals row to stay at the top and handle sorting
 */
class BwCtrlWidgetItem : public QTreeWidgetItem {
public:
    BwCtrlWidgetItem(bool total = false) : QTreeWidgetItem(), isTotal(total) {}
    bool isTotal;

    // Fixed sorting logic to pin Totals row to the top
    bool operator<(const QTreeWidgetItem &other) const override {
        const BwCtrlWidgetItem *otherBw = static_cast<const BwCtrlWidgetItem*>(&other);
        int column = treeWidget()->sortColumn();
        
        // Keep Totals row at the very top regardless of Ascending/Descending order
        bool isAscending = (treeWidget()->header()->sortIndicatorOrder() == Qt::AscendingOrder);
        if (this->isTotal) return isAscending; 
        if (otherBw->isTotal) return !isAscending;

        // Standard data comparison for other rows
        QVariant v1 = data(column, Qt::DisplayRole);
        QVariant v2 = other.data(column, Qt::DisplayRole);

        if (v1.type() == QVariant::Double || v1.type() == (QVariant::Type)QMetaType::Float)
            return v1.toFloat() < v2.toFloat();
        if (v1.type() == QVariant::Int || v1.type() == QVariant::LongLong)
            return v1.toLongLong() < v2.toLongLong();
            
        return v1.toString().localeAwareCompare(v2.toString()) < 0;
    }
};

class BwCtrlWindow : public RsAutoUpdatePage,  public Ui::BwCtrlWindow
{
    Q_OBJECT
public:

    BwCtrlWindow(QWidget *parent = 0);
    ~BwCtrlWindow();

    void updateBandwidth();

public slots:
    virtual void updateDisplay() ;

protected:
    BWListDelegate *BWDelegate;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

};
