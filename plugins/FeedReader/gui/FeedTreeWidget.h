/*******************************************************************************
 * plugins/FeedReader/gui/FeedTreeWidget.h                                     *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
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

#ifndef _FEEDTREEWIDGET_H
#define _FEEDTREEWIDGET_H

#include "gui/common/RSTreeWidget.h"

/* Subclassing RSTreeWidget */
class FeedTreeWidget : public RSTreeWidget
{
	Q_OBJECT

public:
	FeedTreeWidget(QWidget *parent = 0);

Q_SIGNALS:
	void feedReparent(QTreeWidgetItem *item, QTreeWidgetItem *newParent);

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragLeaveEvent(QDragLeaveEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

private:
	bool canDrop(QDropEvent *event, QTreeWidgetItem **dropItem = NULL);

private:
	QTreeWidgetItem *mDraggedItem;
};

#endif
