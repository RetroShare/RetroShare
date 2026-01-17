/*******************************************************************************
 * gui/statistics/BwCtrlWindow.cpp                                             *
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

#include "BwCtrlWindow.h"
#include "gui/common/RSGraphWidget.h"
#include "ui_BwCtrlWindow.h"
#include "util/RsQtVersion.h"
#include <QTimer>
#include <QDateTime>
#include <QSettings>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <time.h>

#include "retroshare-gui/RsAutoUpdatePage.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rspeers.h"

#include <QModelIndex>
#include <QHeaderView>
#include <QPainter>
#include <limits>

class BWListDelegate: public QAbstractItemDelegate
{
public:
    BWListDelegate(QObject *parent=0);
    virtual ~BWListDelegate();
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
};

BWListDelegate::BWListDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{
}

BWListDelegate::~BWListDelegate(void)
{
}

void BWListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QString strNA = tr("N/A");
	QStyleOptionViewItem opt = option;

	QString temp ;
	float flValue;
	qint64 qi64Value;

	// prepare
	painter->save();
	painter->setClipRect(opt.rect);

	//set text color
	QVariant value = index.data(Qt::ForegroundRole);
	if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
		opt.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));
	}
	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if(option.state & QStyle::State_Selected){
		painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
	} else {
		painter->setPen(opt.palette.color(cg, QPalette::Text));
	}

	// draw the background color
	if(option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
		if(cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
			cg = QPalette::Inactive;
		}
		painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
	} else {
		value = index.data(Qt::BackgroundRole);
		if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
			painter->fillRect(option.rect, qvariant_cast<QColor>(value));
		}
	}

	switch(index.column()) {
	case COLUMN_IN_RATE:
        case COLUMN_IN_MAX:
        case COLUMN_IN_ALLOWED:
        case COLUMN_OUT_RATE:
        case COLUMN_OUT_MAX:
        case COLUMN_OUT_ALLOWED:
		{
		QVariant d = index.data();
		if (!d.isValid() || d.isNull()) {
			temp = ""; // Display empty instead of 0.0
		} else {
			temp = QString::asprintf("%.1f ", d.toFloat());
		}
		painter->drawText(option.rect, Qt::AlignRight | Qt::AlignVCenter, temp);
		}
		break;

	case COLUMN_IN_QUEUE_ITEMS:
	case COLUMN_OUT_QUEUE_ITEMS:
	        temp = QString::number(index.data().toInt());
        	painter->drawText(option.rect, Qt::AlignRight, temp);
	        break;
	case COLUMN_OUT_QUEUE_BYTES:
	        qi64Value = index.data().value<qint64>();
        	if (qi64Value >= 1024 * 1024) 
	            temp = QString::asprintf("%.2f MB ", qi64Value / (1024.0 * 1024.0));
        	else if (qi64Value >= 1024)
	            temp = QString::asprintf("%.1f KB ", qi64Value / 1024.0);
	        else
	            temp = QString::number(qi64Value) + " B ";
	        painter->drawText(option.rect, Qt::AlignRight, temp);
	        break;

	case COLUMN_DRAIN:
		// Drain time: Integer only, no decimals, no "s"
		temp = QString::number((int)index.data().toFloat()) + " ";
		painter->drawText(option.rect, Qt::AlignRight | Qt::AlignVCenter, temp);
		break;

	default:
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
	}

	// done
	painter->restore();
}

QSize BWListDelegate::sizeHint(const QStyleOptionViewItem & option/*option*/, const QModelIndex & index) const
{
    float FS = QFontMetricsF(option.font).height();
    //float fact = FS/14.0 ;

    float w = QFontMetrics_horizontalAdvance(QFontMetricsF(option.font), index.data(Qt::DisplayRole).toString());

    return QSize(w,FS*1.2);
    //return QSize(50*fact,17*fact);
}

BwCtrlWindow::BwCtrlWindow(QWidget *parent) 
: RsAutoUpdatePage(1000,parent)
{
    setupUi(this);
    BWDelegate = new BWListDelegate(this);
    bwTreeWidget->setItemDelegate(BWDelegate);
    
    QHeaderView *header = bwTreeWidget->header();

    // Set Header alignments
    for (int i = 0; i < COLUMN_COUNT; ++i) {
        if (i == COLUMN_RSNAME || i == COLUMN_PEERID)
            bwTreeWidget->headerItem()->setTextAlignment(i, Qt::AlignLeft | Qt::AlignVCenter);
        else
            bwTreeWidget->headerItem()->setTextAlignment(i, Qt::AlignRight | Qt::AlignVCenter);
    }

    // Ensure the Totals row doesn't get messed up by initial sorting
    bwTreeWidget->setSortingEnabled(true);
    header->setSortIndicator(COLUMN_RSNAME, Qt::AscendingOrder);
}

BwCtrlWindow::~BwCtrlWindow()
{
}

void BwCtrlWindow::updateDisplay()
{
	/* do nothing if locked, or not visible */
	if (RsAutoUpdatePage::eventsLocked() == true) 
	{
#ifdef DEBUG_BWCTRLWINDOW
		std::cerr << "BwCtrlWindow::update() events Are Locked" << std::endl;
#endif
		return;
    	}

	if (!rsConfig)
	{
#ifdef DEBUG_BWCTRLWINDOW
		std::cerr << "BwCtrlWindow::update rsConfig NOT Set" << std::endl;
#endif
		return;
	}

    updateBandwidth();
}

void BwCtrlWindow::updateBandwidth()
{
	QTreeWidget *peerTreeWidget = bwTreeWidget;

	// Disable sorting while clearing and refilling to avoid UI glitches
	peerTreeWidget->setSortingEnabled(false);
	peerTreeWidget->clear();

	RsConfigDataRates totalRates;
	std::map<RsPeerId, RsConfigDataRates> rateMap;
	std::map<RsPeerId, RsConfigDataRates>::iterator it;

	rsConfig->getTotalBandwidthRates(totalRates);
	rsConfig->getAllBandwidthRates(rateMap);

	// some calculation
	float totalEffectiveSpeed = std::max(totalRates.mRateOut, 1.0f);
	float totalDrain = (float)totalRates.mQueueOutBytes / (totalEffectiveSpeed * 1024.0f);

	/* insert */
	BwCtrlWidgetItem *item = new BwCtrlWidgetItem(true); // true = isTotal
	peerTreeWidget->addTopLevelItem(item);
	peerTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	
	/* do Totals */
	item -> setData(COLUMN_PEERID, Qt::DisplayRole, tr(""));
	item -> setData(COLUMN_RSNAME, Qt::DisplayRole, tr("Totals"));

	item -> setData(COLUMN_IN_RATE, Qt::DisplayRole, totalRates.mRateIn);
	item -> setData(COLUMN_IN_MAX, Qt::DisplayRole,totalRates.mRateMaxIn);
	item -> setData(COLUMN_IN_ALLOWED, Qt::DisplayRole, QVariant());

	item -> setData(COLUMN_IN_QUEUE_ITEMS, Qt::DisplayRole, totalRates.mQueueIn);

	item -> setData(COLUMN_OUT_RATE, Qt::DisplayRole, totalRates.mRateOut);
	item -> setData(COLUMN_OUT_MAX, Qt::DisplayRole, totalRates.mRateMaxOut);
	item -> setData(COLUMN_OUT_ALLOWED, Qt::DisplayRole, QVariant());

	item -> setData(COLUMN_OUT_QUEUE_ITEMS, Qt::DisplayRole, totalRates.mQueueOut);
        item -> setData(COLUMN_OUT_QUEUE_BYTES, Qt::DisplayRole, qint64(totalRates.mQueueOutBytes));
        item -> setData(COLUMN_DRAIN, Qt::DisplayRole, totalDrain);


	time_t now = time(NULL);
	for(it = rateMap.begin(); it != rateMap.end(); ++it)
	{
		/* find the entry */
		BwCtrlWidgetItem *peer_item = new BwCtrlWidgetItem(false); // false = not total
	        peerTreeWidget->addTopLevelItem(peer_item);
#if 0
		QString qpeerid = QString::fromStdString(*it);
		int itemCount = peerTreeWidget->topLevelItemCount();
		for (int nIndex = 0; nIndex < itemCount; ++nIndex)
		{
			QTreeWidgetItem *tmp_item = peerTreeWidget->topLevelItem(nIndex);
			if (tmp_item->data(COLUMN_PEERID, Qt::DisplayRole).toString() == qpeerid)
			{
				peer_item = tmp_item;
				break;
			}
		}
#endif

		std::string name = rsPeers->getPeerName(it->first);

		peer_item -> setData(COLUMN_PEERID, Qt::DisplayRole, QString::fromStdString(it->first.toStdString()));
		peer_item -> setData(COLUMN_RSNAME, Qt::DisplayRole, QString::fromUtf8(name.c_str()));

		peer_item -> setData(COLUMN_IN_RATE, Qt::DisplayRole, it->second.mRateIn);
		peer_item -> setData(COLUMN_IN_MAX, Qt::DisplayRole, it->second.mRateMaxIn);
		peer_item -> setData(COLUMN_IN_ALLOWED, Qt::DisplayRole, it->second.mAllocIn);

		peer_item -> setData(COLUMN_IN_QUEUE_ITEMS, Qt::DisplayRole, it->second.mQueueIn);

		peer_item -> setData(COLUMN_OUT_RATE, Qt::DisplayRole, it->second.mRateOut);
		peer_item -> setData(COLUMN_OUT_MAX, Qt::DisplayRole, it->second.mRateMaxOut);
		peer_item -> setData(COLUMN_OUT_ALLOWED, Qt::DisplayRole, it->second.mAllowedOut);

                peer_item -> setData(COLUMN_OUT_QUEUE_ITEMS, Qt::DisplayRole, it->second.mQueueOut);
                peer_item -> setData(COLUMN_OUT_QUEUE_BYTES, Qt::DisplayRole, qint64(it->second.mQueueOutBytes));

		float effectiveSpeed = std::max(it->second.mRateOut, 1.0f);
	        float drainTime = (float)it->second.mQueueOutBytes / (effectiveSpeed * 1024.0f);
	        peer_item -> setData(COLUMN_DRAIN, Qt::DisplayRole, drainTime);
		// Orange if >= 30, Red if >= 60
		if (drainTime >= 60.0f) {
        		// Red background with white text for critical congestion
		        peer_item->setBackground(COLUMN_DRAIN, QBrush(QColor("#FF4444"))); 
		        peer_item->setForeground(COLUMN_DRAIN, QBrush(Qt::white));
		} 
		else if (drainTime >= 30.0f) {
		        // Orange background for warning levels
		        peer_item->setBackground(COLUMN_DRAIN, QBrush(QColor("#FFA500"))); 
		        peer_item->setForeground(COLUMN_DRAIN, QBrush(Qt::black));
		}	
	}
	// Re-enable sorting - the custom operator< will now keep Totals at index 0
	peerTreeWidget->setSortingEnabled(true);
}

// Persist column sizes
void BwCtrlWindow::showEvent(QShowEvent *) {
    QSettings settings;
    bwTreeWidget->header()->restoreState(settings.value("BwCtrlColumns").toByteArray());
}

void BwCtrlWindow::hideEvent(QHideEvent *) {
    QSettings settings;
    settings.setValue("BwCtrlColumns", bwTreeWidget->header()->saveState());
}

