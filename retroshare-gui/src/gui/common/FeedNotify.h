/*******************************************************************************
 * gui/common/FeedNotify.h                                                     *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef FEEDNOTIFY_H
#define FEEDNOTIFY_H

#include <QObject>

class FeedHolder;
class FeedItem;

class FeedNotify : public QObject
{
	Q_OBJECT

public:
	FeedNotify(QObject *parent = 0);
	~FeedNotify();

	virtual bool hasSetting(QString &/*name*/);
	virtual bool notifyEnabled();
	virtual void setNotifyEnabled(bool /*enabled*/);
	virtual FeedItem *feedItem(FeedHolder */*parent*/);
	virtual FeedItem *testFeedItem(FeedHolder */*parent*/);
};

#endif // FEEDNOTIFY_H
