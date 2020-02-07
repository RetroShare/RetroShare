/*******************************************************************************
 * gui/feeds/FeedHolder.h                                                      *
 *                                                                             *
 * Copyright (c) 2008, Robert Fernie   <retroshare.project@gmail.com>          *
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

#ifndef _FEED_HOLDER_H
#define _FEED_HOLDER_H

#include <string>
#include <stdint.h>

#include <retroshare/rsgxschannels.h> // WRONG ONE - BUT IT'LL DO FOR NOW.

class QScrollArea;
class FeedItem;

class FeedHolder
{
public:
	FeedHolder();

	virtual QScrollArea *getScrollArea() = 0;
	virtual void deleteFeedItem(FeedItem *item, uint32_t type) = 0;
    virtual	void openChat(const RsPeerId& peerId) = 0;
	virtual void openComments(uint32_t type, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId> &msg_versions, const RsGxsMessageId &msgId, const QString &title)=0;

	// Workaround for QTBUG-3372
	void lockLayout(QWidget *feedItem, bool lock);

protected:
	int mLockCount;
};

#endif

