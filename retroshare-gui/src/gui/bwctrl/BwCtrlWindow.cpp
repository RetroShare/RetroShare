/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "BwCtrlWindow.h"
#include "ui_BwCtrlWindow.h"
#include <QTimer>
#include <QDateTime>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <time.h>

#include "retroshare-gui/RsAutoUpdatePage.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rspeers.h"

#include <QModelIndex>
#include <QPainter>
#include <limits>

BWListDelegate::BWListDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{
	;
}

BWListDelegate::~BWListDelegate(void)
{
	;
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
	QVariant value = index.data(Qt::TextColorRole);
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
		temp.sprintf("%.3f ", index.data().toFloat());
		//temp=QString::number(index.data().toFloat());
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_IN_MAX:
		temp.sprintf("%.3f ", index.data().toFloat());
		//temp=QString::number(index.data().toFloat());
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_IN_QUEUE:
		temp=QString::number(index.data().toInt());
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_IN_ALLOC:
		flValue = index.data().toFloat();
		if (flValue < std::numeric_limits<float>::max()){
			temp.sprintf("%.3f ", flValue);
		} else {
			temp=strNA;
		}
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_IN_ALLOC_SENT:
		qi64Value = index.data().value<qint64>();
		if (qi64Value < std::numeric_limits<qint64>::max()){
			temp= QString::number(qi64Value);
		} else {
			temp = strNA;
		}
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_OUT_RATE:
		temp.sprintf("%.3f ", index.data().toFloat());
		//temp=QString::number(index.data().toFloat());
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_OUT_MAX:
		temp.sprintf("%.3f ", index.data().toFloat());
		//temp=QString::number(index.data().toFloat());
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_OUT_QUEUE:
		temp=QString::number(index.data().toInt());
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_OUT_ALLOC:
		flValue = index.data().toFloat();
		if (flValue < std::numeric_limits<float>::max()){
			temp=QString::number(flValue);
		} else {
			temp = strNA;
		}
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	case COLUMN_OUT_ALLOC_SENT:
		qi64Value = index.data().value<qint64>();
		if (qi64Value < std::numeric_limits<qint64>::max()){
			temp= QString::number(qi64Value);
		} else {
			temp = strNA;
		}
		painter->drawText(option.rect, Qt::AlignRight, temp);
		break;
	default:
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
	}

	// done
	painter->restore();
}

QSize BWListDelegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
	return QSize(50,17);
}

/**************************************************************************************************/
/********************************************** STATIC WINDOW *************************************/
BwCtrlWindow * BwCtrlWindow::mInstance = NULL;

void BwCtrlWindow::showYourself()
{
    if (mInstance == NULL) {
        mInstance = new BwCtrlWindow();
    }

    mInstance->show();
    mInstance->activateWindow();
}

BwCtrlWindow* BwCtrlWindow::getInstance()
{
    return mInstance;
}

void BwCtrlWindow::releaseInstance()
{
    if (mInstance) {
        delete mInstance;
    }
}

/********************************************** STATIC WINDOW *************************************/



BwCtrlWindow::BwCtrlWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BwCtrlWindow)
{
    ui->setupUi(this);

    setAttribute ( Qt::WA_DeleteOnClose, true );

	BWDelegate = new BWListDelegate();
	ui->bwTreeWidget->setItemDelegate(BWDelegate);


	// tick for gui update.
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);
}

BwCtrlWindow::~BwCtrlWindow()
{
    delete ui;
    mInstance = NULL;
}

void BwCtrlWindow::changeEvent(QEvent *e)
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

void BwCtrlWindow::update()
{
	if (!isVisible())
	{
#ifdef DEBUG_BWCTRLWINDOW
		//std::cerr << "BwCtrlWindow::update() !Visible" << std::endl;
#endif
		return;
	}

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

	RsAutoUpdatePage::lockAllEvents();

	//std::cerr << "BwCtrlWindow::update()" << std::endl;
	updateBandwidth();

	RsAutoUpdatePage::unlockAllEvents() ;
}

void BwCtrlWindow::updateBandwidth()
{
	QTreeWidget *peerTreeWidget = ui->bwTreeWidget;

	peerTreeWidget->clear();

	RsConfigDataRates totalRates;
	std::map<std::string, RsConfigDataRates> rateMap;
	std::map<std::string, RsConfigDataRates>::iterator it;

	rsConfig->getTotalBandwidthRates(totalRates);
	rsConfig->getAllBandwidthRates(rateMap);

			/* insert */
	QTreeWidgetItem *item = new QTreeWidgetItem();
	peerTreeWidget->addTopLevelItem(item);

	/* do Totals */
	item -> setData(COLUMN_PEERID, Qt::DisplayRole, QString("TOTALS"));
	item -> setData(COLUMN_RSNAME, Qt::DisplayRole, QString("Totals"));

	item -> setData(COLUMN_IN_RATE, Qt::DisplayRole, totalRates.mRateIn);
	item -> setData(COLUMN_IN_MAX, Qt::DisplayRole,totalRates.mRateMaxIn);
	item -> setData(COLUMN_IN_QUEUE, Qt::DisplayRole, totalRates.mQueueIn);
	item -> setData(COLUMN_IN_ALLOC, Qt::DisplayRole, std::numeric_limits<float>::max());
	item -> setData(COLUMN_IN_ALLOC_SENT, Qt::DisplayRole, std::numeric_limits<qint64>::max());

	item -> setData(COLUMN_OUT_RATE, Qt::DisplayRole, totalRates.mRateOut);
	item -> setData(COLUMN_OUT_MAX, Qt::DisplayRole, totalRates.mRateMaxOut);
	item -> setData(COLUMN_OUT_QUEUE, Qt::DisplayRole, totalRates.mQueueOut);
	item -> setData(COLUMN_OUT_ALLOC, Qt::DisplayRole, std::numeric_limits<float>::max());
	item -> setData(COLUMN_OUT_ALLOC_SENT, Qt::DisplayRole, std::numeric_limits<qint64>::max());

	time_t now = time(NULL);
	for(it = rateMap.begin(); it != rateMap.end(); it++)
	{
		/* find the entry */
		QTreeWidgetItem *peer_item = NULL;
#if 0
		QString qpeerid = QString::fromStdString(*it);
		int itemCount = peerTreeWidget->topLevelItemCount();
		for (int nIndex = 0; nIndex < itemCount; nIndex++) 
		{
			QTreeWidgetItem *tmp_item = peerTreeWidget->topLevelItem(nIndex);
			if (tmp_item->data(COLUMN_PEERID, Qt::DisplayRole).toString() == qpeerid)
			{
				peer_item = tmp_item;
				break;
			}
		}
#endif

		if (!peer_item)
		{
			/* insert */
			peer_item = new QTreeWidgetItem();
			peerTreeWidget->addTopLevelItem(peer_item);
		}

		std::string name = rsPeers->getPeerName(it->first);

		peer_item -> setData(COLUMN_PEERID, Qt::DisplayRole, QString::fromStdString(it->first));
		peer_item -> setData(COLUMN_RSNAME, Qt::DisplayRole, QString::fromStdString(name));

		peer_item -> setData(COLUMN_IN_RATE, Qt::DisplayRole, it->second.mRateIn);
		peer_item -> setData(COLUMN_IN_MAX, Qt::DisplayRole, it->second.mRateMaxIn);
		peer_item -> setData(COLUMN_IN_QUEUE, Qt::DisplayRole, it->second.mQueueIn);
		peer_item -> setData(COLUMN_IN_ALLOC, Qt::DisplayRole, it->second.mAllocIn);
		peer_item -> setData(COLUMN_IN_ALLOC_SENT, Qt::DisplayRole, qint64(now - it->second.mAllocTs));

		peer_item -> setData(COLUMN_OUT_RATE, Qt::DisplayRole, it->second.mRateOut);
		peer_item -> setData(COLUMN_OUT_MAX, Qt::DisplayRole, it->second.mRateMaxOut);
		peer_item -> setData(COLUMN_OUT_QUEUE, Qt::DisplayRole, it->second.mQueueOut);
		if (it->second.mAllowedTs != 0)
		{
			peer_item -> setData(COLUMN_OUT_ALLOC, Qt::DisplayRole, it->second.mAllowedOut);
			peer_item -> setData(COLUMN_OUT_ALLOC_SENT, Qt::DisplayRole,qint64(now - it->second.mAllowedTs));
		}
		else
		{
			peer_item -> setData(COLUMN_OUT_ALLOC, Qt::DisplayRole, std::numeric_limits<float>::max());
			peer_item -> setData(COLUMN_OUT_ALLOC_SENT, Qt::DisplayRole, std::numeric_limits<qint64>::max());
		}


		/* colour the columns */
		if (it->second.mAllowedTs != 0)
		{
			if (it->second.mAllowedOut < it->second.mRateOut)
			{	
				/* RED */
				QColor bc("#ff4444"); // red
				peer_item -> setBackground(COLUMN_OUT_RATE,QBrush(bc));
	
			}
			else if (it->second.mAllowedOut < it->second.mRateMaxOut)
			{
				/* YELLOW */
				QColor bc("#ffff66"); // yellow
				peer_item -> setBackground(COLUMN_OUT_MAX,QBrush(bc));
	
			}
			else
			{
				/* GREEN */
				QColor bc("#44ff44");//bright green
				peer_item -> setBackground(COLUMN_OUT_ALLOC,QBrush(bc));
			}
		}
		else
		{
			/* GRAY */
			QColor bc("#444444");// gray
			peer_item -> setBackground(COLUMN_OUT_ALLOC,QBrush(bc));
			peer_item -> setBackground(COLUMN_OUT_ALLOC_SENT,QBrush(bc));

		}

		/* queueOut */
#define QUEUE_RED	10000
#define QUEUE_ORANGE	2000
#define QUEUE_YELLOW	500

		if (it->second.mQueueOut > QUEUE_RED)
		{	
			/* RED */
			QColor bc("#ff4444"); // red
			peer_item -> setBackground(COLUMN_OUT_QUEUE,QBrush(bc));
	
		}
		else if (it->second.mQueueOut > QUEUE_ORANGE)
		{
			/* ORANGE */
			QColor bc("#ff9900"); //orange
			peer_item -> setBackground(COLUMN_OUT_QUEUE,QBrush(bc));

		}
		else if (it->second.mQueueOut > QUEUE_YELLOW)
		{
			/* YELLOW */
			QColor bc("#ffff66"); // yellow
			peer_item -> setBackground(COLUMN_OUT_QUEUE,QBrush(bc));

		}
		else
		{
			/* GREEN */
			QColor bc("#44ff44");//bright green
			peer_item -> setBackground(COLUMN_OUT_QUEUE,QBrush(bc));
		}
	}
}



