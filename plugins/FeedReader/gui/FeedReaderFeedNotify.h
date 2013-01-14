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

#ifndef FEEDREADERFEEDNOTIFY_H
#define FEEDREADERFEEDNOTIFY_H

#include "gui/common/FeedNotify.h"

class RsFeedReader;
class FeedReaderNotify;
class QMutex;

class FeedReaderFeedNotify : public FeedNotify
{
	Q_OBJECT

protected:
	class FeedItem
	{
	public:
		FeedItem() {}

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
	virtual QWidget *feedItem(FeedHolder *parent);
	virtual QWidget *testFeedItem(FeedHolder *parent);

private slots:
	void msgChanged(const QString &feedId, const QString &msgId, int type);

private:
	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;

	QMutex *mMutex;
	QList<FeedItem> mPendingNewsFeed;
};

#endif // FEEDREADERFEEDNOTIFY_H
