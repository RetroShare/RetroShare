/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderFeedItem.h                                 *
 *                                                                             *
 * Copyright (C) 2012 by Thunder                                               *
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

#ifndef _FEEDREADERFEEDITEM_H
#define _FEEDREADERFEEDITEM_H

#include "gui/feeds/FeedItem.h"

namespace Ui {
class FeedReaderFeedItem;
}

class RsFeedReader;
class FeedReaderNotify;
class FeedHolder;
class FeedInfo;
class FeedMsgInfo;

class FeedReaderFeedItem : public FeedItem
{
	Q_OBJECT

public:
	FeedReaderFeedItem(RsFeedReader *feedReader, FeedReaderNotify *notify, FeedHolder *parent, const FeedInfo &feedInfo, const FeedMsgInfo &msgInfo);
	~FeedReaderFeedItem();

protected:
	/* FeedItem */
	virtual void doExpand(bool open);

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
