/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsFeedItem.h                                    *
 *                                                                             *
 * Copyright 2015 Retroshare Team         <retroshare.project@gmail.com>       *
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
