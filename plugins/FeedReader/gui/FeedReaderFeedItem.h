/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013 by Thunder
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

#ifndef _FEEDREADERFEEDITEM_H
#define _FEEDREADERFEEDITEM_H

#include <QWidget>

namespace Ui {
class FeedReaderFeedItem;
}

class RsFeedReader;
class FeedReaderNotify;
class FeedHolder;
class FeedInfo;
class FeedMsgInfo;

class FeedReaderFeedItem : public QWidget
{
	Q_OBJECT

public:
	FeedReaderFeedItem(RsFeedReader *feedReader, FeedReaderNotify *notify, FeedHolder *parent, const FeedInfo &feedInfo, const FeedMsgInfo &msgInfo);
	~FeedReaderFeedItem();

private slots:
	/* default stuff */
	void removeItem();
	void toggle();

	void readAndClearItem();
	void copyLink();
	void openLink();

	void msgChanged(const QString &feedId, const QString &msgId, int type);

private:
	void setMsgRead();

	RsFeedReader *mFeedReader;
	FeedReaderNotify *mNotify;
	FeedHolder *mParent;

	std::string mFeedId;
	std::string mMsgId;
	QString mLink;

	Ui::FeedReaderFeedItem *ui;
};

#endif // _FEEDREADERFEEDITEM_H
