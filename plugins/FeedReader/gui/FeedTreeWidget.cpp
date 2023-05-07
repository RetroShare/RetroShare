/*******************************************************************************
 * plugins/FeedReader/gui/FeedTreeWidget.cpp                                   *
 *                                                                             *
 * Copyright (C) 2012 RetroShare Team <retroshare.project@gmail.com>           *
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

#include <QDropEvent>
#include "FeedTreeWidget.h"

FeedTreeWidget::FeedTreeWidget(QWidget *parent) : RSTreeWidget(parent)
{
	mDraggedItem = NULL;
}

void FeedTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
	mDraggedItem = currentItem();
	RSTreeWidget::dragEnterEvent(event);
}

void FeedTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
	RSTreeWidget::dragLeaveEvent(event);
	mDraggedItem = NULL;
}

bool FeedTreeWidget::canDrop(QDropEvent *event, QTreeWidgetItem **dropItem)
{
	if (dropItem) {
		*dropItem = NULL;
	}

	if (!mDraggedItem) {
		/* no drag item */
		return false;
	}

	QModelIndex droppedIndex = indexAt(event->pos());
	if (!droppedIndex.isValid()) {
		/* no drop target */
		return false;
	}

	QTreeWidgetItem *dropItemIntern = itemFromIndex(droppedIndex);
	if (!dropItemIntern) {
		/* no drop item */
		return false;
	}

	if ((dropItemIntern->flags() & Qt::ItemIsDropEnabled) == 0) {
		/* drop is disabled */
		return false;
	}

	if (dropItemIntern == mDraggedItem->parent()) {
		/* drag item parent */
		return false;
	}

	if (dropItem) {
		*dropItem = dropItemIntern;
	}

	return true;
}

void FeedTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
	if (!canDrop(event)) {
		event->ignore();
		return;
	}

	RSTreeWidget::dragMoveEvent(event);
}

void FeedTreeWidget::dropEvent(QDropEvent *event)
{
	QTreeWidgetItem *dropItem;
	if (!canDrop(event, &dropItem)) {
		event->ignore();
		return;
	}

	if (!mDraggedItem) {
		/* no drag item */
		event->ignore();
		return;
	}

	QTreeWidgetItem *draggedParent = mDraggedItem->parent();
	if (!draggedParent) {
		/* no drag item parent */
		event->ignore();
		return;
	}

	if (!dropItem) {
		/* no drop item */
		event->ignore();
		return;
	}

	emit feedReparent(mDraggedItem, dropItem);
}
