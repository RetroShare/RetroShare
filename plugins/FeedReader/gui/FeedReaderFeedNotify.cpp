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

void FeedReaderFeedNotify::msgChanged(const QString &feedId, const QString &msgId, int type)
{
	if (feedId.isEmpty() || msgId.isEmpty()) {
		return;
	}

	if (type != NOTIFY_TYPE_ADD) {
		return;
	}

	if (!notifyEnabled()) {
		return;
	}

	mMutex->lock();

	FeedItem feedItem;
	feedItem.mFeedId = feedId;
	feedItem.mMsgId = msgId;

	mPendingNewsFeed.push_back(feedItem);

	mMutex->unlock();
}

QWidget *FeedReaderFeedNotify::feedItem(FeedHolder *parent)
{
	bool msgPending = false;
	FeedInfo feedInfo;
	FeedMsgInfo msgInfo;

	mMutex->lock();
	while (!mPendingNewsFeed.empty()) {
		FeedItem feedItem = mPendingNewsFeed.front();
		mPendingNewsFeed.pop_front();

		if (mFeedReader->getFeedInfo(feedItem.mFeedId.toStdString(), feedInfo) &&
			mFeedReader->getMsgInfo(feedItem.mFeedId.toStdString(), feedItem.mMsgId.toStdString(), msgInfo)) {
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

	return new FeedReaderFeedItem(mFeedReader, mNotify, parent, feedInfo, msgInfo);
}

QWidget *FeedReaderFeedNotify::testFeedItem(FeedHolder *parent)
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

	return new FeedReaderFeedItem(mFeedReader, mNotify, parent, feedInfo, msgInfo);
}
