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

#define PTW_COL_RSNAME			0
#define PTW_COL_PEERID			1

#define PTW_COL_IN_RATE			2
#define PTW_COL_IN_MAX			3
#define PTW_COL_IN_ALLOC		4
#define PTW_COL_IN_ALLOC_SENT		5

#define PTW_COL_OUT_RATE		6
#define PTW_COL_OUT_MAX			7
#define PTW_COL_OUT_ALLOC		8
#define PTW_COL_OUT_ALLOC_SENT		9

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
	item -> setData(PTW_COL_PEERID, Qt::DisplayRole, QString("TOTALS"));
	item -> setData(PTW_COL_RSNAME, Qt::DisplayRole, QString("Totals"));

	item -> setData(PTW_COL_IN_RATE, Qt::DisplayRole, QString::number(totalRates.mRateIn));
	item -> setData(PTW_COL_IN_MAX, Qt::DisplayRole, QString::number(totalRates.mRateMaxIn));
	item -> setData(PTW_COL_IN_ALLOC, Qt::DisplayRole, QString("N/A"));
	item -> setData(PTW_COL_IN_ALLOC_SENT, Qt::DisplayRole, QString("N/A"));

	item -> setData(PTW_COL_OUT_RATE, Qt::DisplayRole, QString::number(totalRates.mRateOut));
	item -> setData(PTW_COL_OUT_MAX, Qt::DisplayRole, QString::number(totalRates.mRateMaxOut));
	item -> setData(PTW_COL_OUT_ALLOC, Qt::DisplayRole, QString("N/A"));
	item -> setData(PTW_COL_OUT_ALLOC_SENT, Qt::DisplayRole, QString("N/A"));

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
			if (tmp_item->data(PTW_COL_PEERID, Qt::DisplayRole).toString() == qpeerid) 
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

		peer_item -> setData(PTW_COL_PEERID, Qt::DisplayRole, QString::fromStdString(it->first));
		peer_item -> setData(PTW_COL_RSNAME, Qt::DisplayRole, QString::fromStdString(name));

		peer_item -> setData(PTW_COL_IN_RATE, Qt::DisplayRole, QString::number(it->second.mRateIn));
		peer_item -> setData(PTW_COL_IN_MAX, Qt::DisplayRole, QString::number(it->second.mRateMaxIn));
		peer_item -> setData(PTW_COL_IN_ALLOC, Qt::DisplayRole, QString::number(it->second.mAllocIn));
		peer_item -> setData(PTW_COL_IN_ALLOC_SENT, Qt::DisplayRole, QString::number(now - it->second.mAllocTs));

		peer_item -> setData(PTW_COL_OUT_RATE, Qt::DisplayRole, QString::number(it->second.mRateOut));
		peer_item -> setData(PTW_COL_OUT_MAX, Qt::DisplayRole, QString::number(it->second.mRateMaxOut));
		peer_item -> setData(PTW_COL_OUT_ALLOC, Qt::DisplayRole, QString::number(it->second.mAllowedOut));
		peer_item -> setData(PTW_COL_OUT_ALLOC_SENT, Qt::DisplayRole, QString::number(now - it->second.mAllowedTs));

	}
}



