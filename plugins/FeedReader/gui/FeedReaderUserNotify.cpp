/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderUserNotify.cpp                             *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
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

bool FeedReaderUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("FeedReader Message");
	if (group) *group = "FeedReader";

	return true;
}

QIcon FeedReaderUserNotify::getIcon()
{
	return QIcon(":/images/Feed.png");
}

QIcon FeedReaderUserNotify::getMainIcon(bool hasNew)
{
	return hasNew ? QIcon(":/images/feedreader-notify.png") : QIcon(":/images/FeedReader.png");
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
