/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2015, RetroShare Team
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

#include "GxsFeedWidget.h"
#include "gui/gxs/GxsFeedItem.h"

#define PAIR(groupId,messageId) QPair<RsGxsGroupId, RsGxsMessageId>(groupId, messageId)

GxsFeedWidget::GxsFeedWidget(QWidget *parent)
    : RSFeedWidget(parent)
{
}

GxsFeedWidget::~GxsFeedWidget()
{
}

void GxsFeedWidget::feedAdded(FeedItem *feedItem, QTreeWidgetItem *treeItem)
{
	RSFeedWidget::feedAdded(feedItem, treeItem);

	GxsFeedItem *gxsFeedItem = dynamic_cast<GxsFeedItem*>(feedItem);
	if (!gxsFeedItem) {
		return;
	}

	mGxsItems.insert(PAIR(gxsFeedItem->groupId(), gxsFeedItem->messageId()), feedItem);
}

void GxsFeedWidget::feedRemoved(FeedItem *feedItem)
{
	RSFeedWidget::feedRemoved(feedItem);

	GxsFeedItem *gxsFeedItem = dynamic_cast<GxsFeedItem*>(feedItem);
	if (!gxsFeedItem) {
		return;
	}

	mGxsItems.remove(PAIR(gxsFeedItem->groupId(), gxsFeedItem->messageId()));
}

void GxsFeedWidget::feedsCleared()
{
	RSFeedWidget::feedsCleared();

	mGxsItems.clear();
}

GxsFeedItem *GxsFeedWidget::findGxsFeedItem(const RsGxsGroupId &groupId, const RsGxsMessageId &messageId)
{
	QMap<QPair<RsGxsGroupId, RsGxsMessageId>, FeedItem*>::iterator it = mGxsItems.find(PAIR(groupId, messageId));
	if (it == mGxsItems.end()) {
		return NULL;
	}

	return dynamic_cast<GxsFeedItem*>(it.value());
}
