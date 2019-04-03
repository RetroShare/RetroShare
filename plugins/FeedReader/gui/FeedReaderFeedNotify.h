/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderFeedNotify.h                               *
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

#ifndef FEEDREADERFEEDNOTIFY_H
#define FEEDREADERFEEDNOTIFY_H

#include "gui/common/FeedNotify.h"

class RsFeedReader;
class FeedReaderNotify;
class QMutex;
class FeedItem;

class FeedReaderFeedNotify : public FeedNotify
{
	Q_OBJECT

protected:
	class FeedItemData
	{
	public:
		FeedItemData() {}

	public:
		QString mFeedId;
		QString mMsgId;
	};

public:
	FeedReaderFeedNotify(RsFeedReader *feedReader, FeedReaderNotify *notify, QObject *parent = 0);
	~FeedReaderFeedNotify();

	virtual bool hasSetting(QString &name);
	virtual bool notifyEnabled();
	virtual void setNotifyEnabled(bool enabled);
	virtual FeedItem *feedItem(FeedHolder *parent);
	virtual FeedItem *testFeedItem(FeedHolder *parent);

private slots:
	void msgChanged(const QString &feedId, const QString &msgId, int type);

private:
	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;

	QMutex *mMutex;
	QList<FeedItemData> mPendingNewsFeed;
};

#endif // FEEDREADERFEEDNOTIFY_H
