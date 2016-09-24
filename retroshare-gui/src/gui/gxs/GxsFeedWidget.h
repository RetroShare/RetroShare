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

#ifndef _GXSFEEDTREEWIDGET_H
#define _GXSFEEDTREEWIDGET_H

#include "gui/common/RSFeedWidget.h"

#include "retroshare/rsgxsifacetypes.h"

class GxsFeedItem;

class GxsFeedWidget : public RSFeedWidget
{
	Q_OBJECT

public:
	GxsFeedWidget(QWidget *parent = 0);
	virtual ~GxsFeedWidget();

	GxsFeedItem *findGxsFeedItem(const RsGxsGroupId &groupId, const RsGxsMessageId &messageId);

protected:
	/* RSFeedWidget */
	virtual void feedAdded(FeedItem *feedItem, QTreeWidgetItem *treeItem);
	virtual void feedRemoved(FeedItem *feedItem);
	virtual void feedsCleared();

private:
	/* Items */
	QMap<QPair<RsGxsGroupId, RsGxsMessageId>, FeedItem*> mGxsItems;
};

#endif
