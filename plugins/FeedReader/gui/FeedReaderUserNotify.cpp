/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 RetroShare Team
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

#include "FeedReaderUserNotify.h"
#include "FeedReaderNotify.h"
#include "FeedReaderDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/MainWindow.h"

#include "interface/rsFeedReader.h"
#include "retroshare/rsiface.h"

FeedReaderUserNotify::FeedReaderUserNotify(FeedReaderDialog *feedReaderDialog, RsFeedReader *feedReader, FeedReaderNotify *notify, QObject *parent) :
	UserNotify(parent), mFeedReaderDialog(feedReaderDialog), mFeedReader(feedReader), mNotify(notify)
{
	connect(mNotify, SIGNAL(feedChanged(QString,int)), this, SLOT(feedChanged(QString,int)), Qt::QueuedConnection);
	connect(mNotify, SIGNAL(msgChanged(QString,QString,int)), this, SLOT(updateIcon()), Qt::QueuedConnection);
}

bool FeedReaderUserNotify::hasSetting(QString &name)
{
	name = tr("FeedReader Message");

	return true;
}

bool FeedReaderUserNotify::notifyEnabled()
{
	return Settings->valueFromGroup("FeedReader", "TrayNotifyEnable", true).toBool();
}

bool FeedReaderUserNotify::notifyCombined()
{
	return Settings->valueFromGroup("FeedReader", "TrayNotifyCombined", false).toBool();
}

bool FeedReaderUserNotify::notifyBlink()
{
	return Settings->valueFromGroup("FeedReader", "TrayNotifyBlink", false).toBool();
}

void FeedReaderUserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	Settings->beginGroup("FeedReader");
	Settings->setValue("TrayNotifyEnable", enabled);
	Settings->setValue("TrayNotifyCombined", combined);
	Settings->setValue("TrayNotifyBlink", blink);
	Settings->endGroup();
}

QIcon FeedReaderUserNotify::getIcon()
{
	return QIcon(":/images/Feed.png");
}

QIcon FeedReaderUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/FeedReader.png") : QIcon(":/images/FeedReader.png");
}

unsigned int FeedReaderUserNotify::getNewCount()
{
	uint32_t newMessageCount = 0;
	mFeedReader->getMessageCount("", NULL, &newMessageCount, NULL);

	return newMessageCount;
}

void FeedReaderUserNotify::iconClicked()
{
	MainWindow::showWindow(mFeedReaderDialog);
}

void FeedReaderUserNotify::feedChanged(const QString &/*feedId*/, int type)
{
	if (type == NOTIFY_TYPE_DEL) {
		updateIcon();
	}
}
