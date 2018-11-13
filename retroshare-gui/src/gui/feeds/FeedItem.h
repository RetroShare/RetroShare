/*******************************************************************************
 * gui/feeds/FeedItem.h                                                        *
 *                                                                             *
 * Copyright (c) 2014, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef _FEED_ITEM_H
#define _FEED_ITEM_H

#include <QWidget>

class FeedItem : public QWidget
{
	Q_OBJECT

public:
	/** Default Constructor */
	FeedItem(QWidget *parent = 0);
	/** Default Destructor */
	virtual ~FeedItem();

	bool wasExpanded() { return mWasExpanded; }
	void expand(bool open);

protected:
	virtual void doExpand(bool open) = 0;
	virtual void expandFill(bool /*first*/) {}

signals:
	void sizeChanged(FeedItem *feedItem);
	void feedItemDestroyed(FeedItem *feedItem);

private:
	bool mWasExpanded;
};

#endif
