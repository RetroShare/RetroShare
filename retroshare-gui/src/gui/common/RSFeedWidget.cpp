/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2014, RetroShare Team
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

#include <QKeyEvent>

#include "RSFeedWidget.h"
#include "ui_RSFeedWidget.h"
#include "RSTreeWidgetItem.h"
#include "gui/feeds/FeedItem.h"
#include "gui/gxs/GxsFeedItem.h"

#define COLUMN_FEED 0

RSFeedWidget::RSFeedWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::RSFeedWidget)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	/* Sort */
	mFeedCompareRole = new RSTreeWidgetItemCompareRole;

	/* Filter */
	mFilterCallback = NULL;
	mFilterType = 0;

	/* Remove */
	mEnableRemove = false;

	ui->treeWidget->installEventFilter(this);
}

RSFeedWidget::~RSFeedWidget()
{
	delete(mFeedCompareRole);
	delete(ui);
}

bool RSFeedWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == ui->treeWidget) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent) {
				if (keyEvent->key() == Qt::Key_Plus || keyEvent->key() == Qt::Key_Minus) {
					bool open = (keyEvent->key() == Qt::Key_Plus);

					QList<FeedItem*> feedItems;
					selectedFeedItems(feedItems);

					foreach (FeedItem *feedItem, feedItems) {
						feedItem->expand(open);
					}

					return true; // eat event
				}

				if (mEnableRemove && keyEvent->key() == Qt::Key_Delete) {
					QList<QTreeWidgetItem*> selectedItems = ui->treeWidget->selectedItems();

					foreach (QTreeWidgetItem *treeItem, selectedItems) {
						FeedItem *feedItem = feedItemFromTreeItem(treeItem);
						if (feedItem) {
							disconnectSignals(feedItem);
							delete(feedItem);
						}
						delete(treeItem);
					}

					return true; // eat event
				}
			}
		}
	}

	/* Pass the event on to the parent class */
	return QWidget::eventFilter(object, event);
}

void RSFeedWidget::connectSignals(FeedItem *feedItem)
{
	connect(feedItem, SIGNAL(feedItemDestroyed(FeedItem*)), this, SLOT(feedItemDestroyed(FeedItem*)));
	connect(feedItem, SIGNAL(sizeChanged(FeedItem*)), this, SLOT(feedItemSizeChanged(FeedItem*)));
}

void RSFeedWidget::disconnectSignals(FeedItem *feedItem)
{
	disconnect(feedItem, SIGNAL(feedItemDestroyed(FeedItem*)), this, SLOT(feedItemDestroyed(FeedItem*)));
	disconnect(feedItem, SIGNAL(sizeChanged(FeedItem*)), this, SLOT(feedItemSizeChanged(FeedItem*)));
}

FeedItem *RSFeedWidget::feedItemFromTreeItem(QTreeWidgetItem *treeItem)
{
	return dynamic_cast<FeedItem*>(ui->treeWidget->itemWidget(treeItem, COLUMN_FEED));
}

void RSFeedWidget::addFeedItem(FeedItem *feedItem, Qt::ItemDataRole sortRole, const QVariant &value)
{
	if (!feedItem) {
		return;
	}

	QTreeWidgetItem *treeItem = new RSTreeWidgetItem(mFeedCompareRole);

	treeItem->setData(COLUMN_FEED, sortRole, value);

	ui->treeWidget->addTopLevelItem(treeItem);
	ui->treeWidget->setItemWidget(treeItem, 0, feedItem);

	connectSignals(feedItem);

	filterItem(treeItem, feedItem);
}

void RSFeedWidget::addFeedItem(FeedItem *feedItem, const QMap<Qt::ItemDataRole, QVariant> &sort)
{
	if (!feedItem) {
		return;
	}

	QTreeWidgetItem *treeItem = new RSTreeWidgetItem(mFeedCompareRole);

	QMap<Qt::ItemDataRole, QVariant>::const_iterator it;
	for (it = sort.begin(); it != sort.end(); ++it) {
		treeItem->setData(COLUMN_FEED, it.key(), it.value());
	}

	ui->treeWidget->addTopLevelItem(treeItem);
	ui->treeWidget->setItemWidget(treeItem, 0, feedItem);

	connectSignals(feedItem);

	filterItem(treeItem, feedItem);
}

void RSFeedWidget::setSort(FeedItem *feedItem, Qt::ItemDataRole sortRole, const QVariant &value)
{
	if (!feedItem) {
		return;
	}

	QTreeWidgetItem *treeItem = findTreeWidgetItem(feedItem);
	if (!treeItem) {
		return;
	}

	treeItem->setData(COLUMN_FEED, sortRole, value);
}

void RSFeedWidget::setSort(FeedItem *feedItem, const QMap<Qt::ItemDataRole, QVariant> &sort)
{
	if (!feedItem) {
		return;
	}

	QTreeWidgetItem *treeItem = findTreeWidgetItem(feedItem);
	if (!treeItem) {
		return;
	}

	QMap<Qt::ItemDataRole, QVariant>::const_iterator it;
	for (it = sort.begin(); it != sort.end(); ++it) {
		treeItem->setData(COLUMN_FEED, it.key(), it.value());
	}
}

void RSFeedWidget::clear()
{
	ui->treeWidget->clear();
}

void RSFeedWidget::setSortRole(Qt::ItemDataRole role, Qt::SortOrder order)
{
	setSortingEnabled(true);
	mFeedCompareRole->setRole(COLUMN_FEED, role);
	ui->treeWidget->sortItems(COLUMN_FEED, order);
}

void RSFeedWidget::setSortingEnabled(bool enable)
{
	ui->treeWidget->setSortingEnabled(enable);
}

void RSFeedWidget::setFilterCallback(RSFeedWidgetFilterCallbackFunction callback)
{
	mFilterCallback = callback;
	filterItems();
}

void RSFeedWidget::setFilter(const QString &text, int type)
{
	if (mFilterText == text && mFilterType == type) {
		return;
	}

	mFilterText = text;
	mFilterType = type;

	filterItems();
}

void RSFeedWidget::setFilterText(const QString &text)
{
	setFilter(text, mFilterType);
}

void RSFeedWidget::setFilterType(int type)
{
	setFilter(mFilterText, type);
}

void RSFeedWidget::filterItems()
{
	if (!mFilterCallback) {
		return;
	}

	QTreeWidgetItemIterator it(ui->treeWidget);
	QTreeWidgetItem *item;
	while ((item = *it) != NULL) {
		++it;
		FeedItem *feedItem = feedItemFromTreeItem(item);
		if (!feedItem) {
			continue;
		}

		filterItem(item, feedItem);
	}
}

void RSFeedWidget::filterItem(QTreeWidgetItem *treeItem, FeedItem *feedItem)
{
	if (!mFilterCallback) {
		return;
	}

	treeItem->setHidden(!mFilterCallback(feedItem, mFilterText, mFilterType));
}

void RSFeedWidget::enableRemove(bool enable)
{
	mEnableRemove = enable;
}

void RSFeedWidget::setSelectionMode(QAbstractItemView::SelectionMode mode)
{
	ui->treeWidget->setSelectionMode(mode);
}

void RSFeedWidget::removeFeedItem(FeedItem *feedItem)
{
	if (!feedItem) {
		return;
	}

	disconnectSignals(feedItem);

	QTreeWidgetItem *treeItem = findTreeWidgetItem(feedItem);
	if (treeItem) {
		delete(treeItem);
	}
}

void RSFeedWidget::feedItemSizeChanged(FeedItem */*feedItem*/)
{
	if (updatesEnabled()) {
		setUpdatesEnabled(false);
		QApplication::processEvents();
		setUpdatesEnabled(true);
	} else {
		QApplication::processEvents();
	}

	ui->treeWidget->doItemsLayout();
}

void RSFeedWidget::feedItemDestroyed(FeedItem *feedItem)
{
	/* No need to disconnect when object will be destroyed */

	QTreeWidgetItem *treeItem = findTreeWidgetItem(feedItem);
	if (treeItem) {
		delete(treeItem);
	}
}

QTreeWidgetItem *RSFeedWidget::findTreeWidgetItem(FeedItem *feedItem)
{
	QTreeWidgetItemIterator it(ui->treeWidget);
	QTreeWidgetItem *treeItem;
	while ((treeItem = *it) != NULL) {
		++it;
		if (feedItemFromTreeItem(treeItem) == feedItem) {
			return treeItem;
		}
	}

	return NULL;
}

bool RSFeedWidget::scrollTo(FeedItem *feedItem, bool focus)
{
	QTreeWidgetItem *item = findTreeWidgetItem(feedItem);
	if (!feedItem) {
		return false;
	}

	ui->treeWidget->scrollToItem(item);
	ui->treeWidget->setCurrentItem(item);

	if (focus) {
		ui->treeWidget->setFocus();
	}

	return true;
}

class RSFeedWidgetCallback
{
public:
	RSFeedWidgetCallback() {}

	virtual void callback(FeedItem *feedItem, const QVariant &data) = 0;
};

void RSFeedWidget::withAll(RSFeedWidgetCallbackFunction callback, const QVariant &data)
{
	if (!callback) {
		return;
	}

	QTreeWidgetItemIterator it(ui->treeWidget);
	QTreeWidgetItem *treeItem;
	while ((treeItem = *it) != NULL) {
		++it;

		FeedItem *feedItem = feedItemFromTreeItem(treeItem);
		if (!feedItem) {
			continue;
		}

		callback(feedItem, data);
	}
}

FeedItem *RSFeedWidget::findFeedItem(RSFeedWidgetFindCallbackFunction callback, const QVariant &data1, const QVariant &data2)
{
	if (!callback) {
		return NULL;
	}

	QTreeWidgetItemIterator it(ui->treeWidget);
	QTreeWidgetItem *treeItem;
	while ((treeItem = *it) != NULL) {
		++it;

		FeedItem *feedItem = feedItemFromTreeItem(treeItem);
		if (!feedItem) {
			continue;
		}

		if (callback(feedItem, data1, data2)) {
			return feedItem;
		}
	}

	return NULL;
}

void RSFeedWidget::selectedFeedItems(QList<FeedItem*> &feedItems)
{
	foreach (QTreeWidgetItem *treeItem, ui->treeWidget->selectedItems()) {
		FeedItem *feedItem = feedItemFromTreeItem(treeItem);
		if (!feedItem) {
			continue;
		}

		feedItems.push_back(feedItem);
	}
}

static bool findGxsFeedItemCallback(FeedItem *feedItem, const QVariant &data1, const QVariant &data2)
{
	if (!data1.canConvert<RsGxsGroupId>() || !data2.canConvert<RsGxsMessageId>()) {
		return false;
	}

	GxsFeedItem *item = dynamic_cast<GxsFeedItem*>(feedItem);
	if (!item) {
		return false;
	}

	if (item->groupId() != data1.value<RsGxsGroupId>() ||
	    item->messageId() != data2.value<RsGxsMessageId>()) {
		return false;
	}

	return true;
}

GxsFeedItem *RSFeedWidget::findGxsFeedItem(const RsGxsGroupId &groupId, const RsGxsMessageId &messageId)
{
	FeedItem *feedItem = findFeedItem(findGxsFeedItemCallback, qVariantFromValue(groupId), qVariantFromValue(messageId));
	return dynamic_cast<GxsFeedItem*>(feedItem);
}
