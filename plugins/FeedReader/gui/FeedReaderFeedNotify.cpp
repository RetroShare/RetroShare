/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderFeedNotify.cpp                             *
 *                                                                             *
 * Copyright (C) 2012 by RetroShare Team <retroshare.project@gmail.com>        *
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

#include <QMutex>
#include <QDateTime>
#include <QBuffer>

#include "FeedReaderFeedNotify.h"
#include "FeedReaderNotify.h"
#include "FeedReaderFeedItem.h"
#include "gui/settings/rsharesettings.h"
#include "retroshare/rsiface.h"

FeedReaderFeedNotify::FeedReaderFeedNotify(RsFeedReader *feedReader, FeedReaderNotify *notify, QObject *parent) :
	FeedNotify(parent), mFeedReader(feedReader), mNotify(notify)
{
	mMutex = new QMutex();

	connect(mNotify, SIGNAL(msgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)), Qt::QueuedConnection);
}

FeedReaderFeedNotify::~FeedReaderFeedNotify()
{
	delete(mMutex);
}

bool FeedReaderFeedNotify::hasSetting(QString &name)
{
	name = tr("Feed Reader");

	return true;
}

bool FeedReaderFeedNotify::notifyEnabled()
{
	return Settings->valueFromGroup("FeedReader", "FeedNotifyEnable", false).toBool();
}

void FeedReaderFeedNotify::setNotifyEnabled(bool enabled)
{
	Settings->setValueToGroup("FeedReader", "FeedNotifyEnable", enabled);

	if (!enabled) {
		/* remove pending feed items */
		mMutex->lock();
		mPendingNewsFeed.clear();
		mMutex->unlock();
	}
}

void FeedReaderFeedNotify::msgChanged(uint32_t feedId, const QString &msgId, int type)
{
	if (feedId == 0 || msgId.isEmpty()) {
		return;
	}

	if (type != NOTIFY_TYPE_ADD) {
		return;
	}

	if (!notifyEnabled()) {
		return;
	}

	mMutex->lock();

	FeedItemData feedItemData;
	feedItemData.mFeedId = feedId;
	feedItemData.mMsgId = msgId;

	mPendingNewsFeed.push_back(feedItemData);

	mMutex->unlock();
}

FeedItem *FeedReaderFeedNotify::feedItem(FeedHolder */*parent*/)
{
	bool msgPending = false;
	FeedInfo feedInfo;
	FeedMsgInfo msgInfo;

	mMutex->lock();
	while (!mPendingNewsFeed.empty()) {
		FeedItemData feedItemData = mPendingNewsFeed.front();
		mPendingNewsFeed.pop_front();

		if (mFeedReader->getFeedInfo(feedItemData.mFeedId, feedInfo) &&
			mFeedReader->getMsgInfo(feedItemData.mFeedId, feedItemData.mMsgId.toStdString(), msgInfo)) {
			if (msgInfo.flag.isnew) {
				msgPending = true;
				break;
			}
		}
	}
	mMutex->unlock();

	if (!msgPending) {
		return NULL;
	}

	//TODO: parent?
	return new FeedReaderFeedItem(mFeedReader, mNotify, feedInfo, msgInfo);
}

FeedItem *FeedReaderFeedNotify::testFeedItem(FeedHolder */*parent*/)
{
	FeedInfo feedInfo;
	feedInfo.name = tr("Test").toUtf8().constData();

	QByteArray faviconData;
	QBuffer buffer(&faviconData);
	buffer.open(QIODevice::WriteOnly);
	if (QPixmap(":/images/Feed.png").scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).save(&buffer, "ICO")) {
		feedInfo.icon = faviconData.toBase64().constData();
	}
	buffer.close();

	FeedMsgInfo msgInfo;
	msgInfo.title = tr("Test message").toUtf8().constData();
	msgInfo.description = tr("This is a test message.").toUtf8().constData();
	msgInfo.pubDate = QDateTime::currentDateTime().toTime_t();

	//TODO: parent?
	return new FeedReaderFeedItem(mFeedReader, mNotify, feedInfo, msgInfo);
}
