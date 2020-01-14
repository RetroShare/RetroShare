/*******************************************************************************
 * gui/common/RSFeedWidget.cpp                                                 *
 *                                                                             *
 * Copyright (C) 2014, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QKeyEvent>
#include <QScrollBar>

#include "RSFeedWidget.h"
#include "ui_RSFeedWidget.h"
#include "RSTreeWidgetItem.h"
#include "gui/feeds/FeedItem.h"

#include <iostream>

#define COLUMN_FEED       0
#define COLUMN_IDENTIFIER 1

#define SINGLE_STEP  15

/* Redefine single step for srolling */
class RSFeedWidgetScrollBar : public QScrollBar
{
public:
	RSFeedWidgetScrollBar(QWidget *parent = 0) : QScrollBar(parent) {}

	void sliderChange(SliderChange change)
	{
		if (change == SliderStepsChange) {
			if (singleStep() > SINGLE_STEP) {
				/* Set our own value */
				setSingleStep(SINGLE_STEP);
				return;
			}
		}

		QScrollBar::sliderChange(change);
	}
};

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

	/* Options */
	mCountChangedDisabled = 0;

	ui->treeWidget->installEventFilter(this);

	ui->treeWidget->setVerticalScrollBar(new RSFeedWidgetScrollBar);
	ui->treeWidget->setColumnCount(2);
	ui->treeWidget->setColumnHidden(COLUMN_IDENTIFIER,true);
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

					if (!mCountChangedDisabled) {
						emit feedCountChanged();
					}

					return true; // eat event
				}
			}
		}
	}

	/* Pass the event on to the parent class */
	return QWidget::eventFilter(object, event);
}

void RSFeedWidget::feedAdded(FeedItem *feedItem, QTreeWidgetItem *treeItem)
{
}

void RSFeedWidget::feedsCleared()
{
}

void RSFeedWidget::connectSignals(FeedItem *feedItem)
{
	connect(feedItem, SIGNAL(feedItemNeedsClosing(qulonglong)), this, SLOT(feedItemDestroyed(qulonglong)));
	connect(feedItem, SIGNAL(sizeChanged(FeedItem*)), this, SLOT(feedItemSizeChanged(FeedItem*)));
}

void RSFeedWidget::disconnectSignals(FeedItem *feedItem)
{
	disconnect(feedItem, SIGNAL(feedItemNeedsClosing(qulonglong)), this, SLOT(feedItemDestroyed(qulonglong)));
	disconnect(feedItem, SIGNAL(sizeChanged(FeedItem*)), this, SLOT(feedItemSizeChanged(FeedItem*)));
}

QString RSFeedWidget::placeholderText()
{
	return ui->treeWidget->placeholderText();
}

void RSFeedWidget::setPlaceholderText(const QString &placeholderText)
{
	ui->treeWidget->setPlaceholderText(placeholderText);
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
	treeItem->setData(COLUMN_IDENTIFIER, Qt::DisplayRole, QString("%1").arg(feedItem->uniqueIdentifier(),8,16,QChar('0')));

	ui->treeWidget->addTopLevelItem(treeItem);
	ui->treeWidget->setItemWidget(treeItem, 0, feedItem);

	feedAdded(feedItem, treeItem);

	connectSignals(feedItem);

	filterItem(treeItem, feedItem);

	if (!mCountChangedDisabled) {
		emit feedCountChanged();
	}
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
	treeItem->setData(COLUMN_IDENTIFIER, Qt::DisplayRole, QString("%1").arg(feedItem->uniqueIdentifier(),8,16,QChar('0')));

	ui->treeWidget->addTopLevelItem(treeItem);
	ui->treeWidget->setItemWidget(treeItem, 0, feedItem);

	feedAdded(feedItem, treeItem);

	connectSignals(feedItem);

	filterItem(treeItem, feedItem);

	if (!mCountChangedDisabled) {
		emit feedCountChanged();
	}
}

void RSFeedWidget::setSort(FeedItem *feedItem, Qt::ItemDataRole sortRole, const QVariant &value)
{
	if (!feedItem)
		return;

	QTreeWidgetItem *treeItem = findTreeWidgetItem(feedItem->uniqueIdentifier());

	if (!treeItem)
		return;

	treeItem->setData(COLUMN_FEED, sortRole, value);
}

void RSFeedWidget::setSort(FeedItem *feedItem, const QMap<Qt::ItemDataRole, QVariant> &sort)
{
	if (!feedItem) {
		return;
	}

	QTreeWidgetItem *treeItem = findTreeWidgetItem(feedItem->uniqueIdentifier());
	if (!treeItem)
		return;

	QMap<Qt::ItemDataRole, QVariant>::const_iterator it;
	for (it = sort.begin(); it != sort.end(); ++it) {
		treeItem->setData(COLUMN_FEED, it.key(), it.value());
	}
}

void RSFeedWidget::clear()
{
	/* Disconnect signals */
	QTreeWidgetItemIterator it(ui->treeWidget);
	QTreeWidgetItem *treeItem;
	while ((treeItem = *it) != NULL) {
		++it;

		FeedItem *feedItem = feedItemFromTreeItem(treeItem);
		if (feedItem) {
			disconnectSignals(feedItem);
		}
	}

	feedsCleared();

	/* Clear items */
	ui->treeWidget->clear();

	if (!mCountChangedDisabled) {
		emit feedCountChanged();
	}
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

void RSFeedWidget::enableCountChangedSignal(bool enable)
{
	if (enable) {
		--mCountChangedDisabled;
		if (mCountChangedDisabled < 0) {
			std::cerr << "RSFeedWidget::enableCountChangedSignal error disable count change signal" << std::endl;
			mCountChangedDisabled = 0;
		}
	} else {
		++mCountChangedDisabled;
	}
}

void RSFeedWidget::setSelectionMode(QAbstractItemView::SelectionMode mode)
{
	ui->treeWidget->setSelectionMode(mode);
}

int RSFeedWidget::feedItemCount()
{
	return ui->treeWidget->topLevelItemCount();
}

FeedItem *RSFeedWidget::feedItem(int index)
{
	QTreeWidgetItem *treeItem = ui->treeWidget->topLevelItem(index);
	if (!treeItem) {
		return NULL;
	}

	return feedItemFromTreeItem(treeItem);
}

void RSFeedWidget::removeFeedItem(FeedItem *feedItem)
{
	if (!feedItem) {
		return;
	}

	QTreeWidgetItem *treeItem = findTreeWidgetItem(feedItem->uniqueIdentifier());

	if (treeItem)
    {
        int treeItem_index = ui->treeWidget->indexOfTopLevelItem(treeItem);

        if(treeItem_index < 0)
        {
            std::cerr << "(EE) Cannot remove designated item \"" << feedItem->uniqueIdentifier() << "\": not found!" << std::endl;
            return ;
		}

		disconnectSignals(feedItem);

		delete ui->treeWidget->takeTopLevelItem(treeItem_index);
    }

	if (!mCountChangedDisabled) {
		emit feedCountChanged();
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

void RSFeedWidget::feedItemDestroyed(qulonglong id)
{
	/* No need to disconnect when object will be destroyed */

	QTreeWidgetItem *treeItem = findTreeWidgetItem(id);

	if(treeItem)
		delete(treeItem);

	if (!mCountChangedDisabled)
		emit feedCountChanged();
}

QTreeWidgetItem *RSFeedWidget::findTreeWidgetItem(uint64_t identifier)
{
    QList<QTreeWidgetItem*> list = ui->treeWidget->findItems(QString("%1").arg(identifier,8,16,QChar('0')),Qt::MatchExactly,COLUMN_IDENTIFIER);

    if(list.empty())
        return nullptr;
    else if(list.size() == 1)
        return list.front();
    else
    {
        std::cerr << "(EE) More than a single item with identifier \"" << identifier << "\" in the feed tree widget. This shouldn't happen!" << std::endl;
        return nullptr;
    }
}

bool RSFeedWidget::scrollTo(FeedItem *feedItem, bool focus)
{
	QTreeWidgetItem *item = findTreeWidgetItem(feedItem->uniqueIdentifier());
	if (!feedItem)
		return false;

	ui->treeWidget->scrollToItem(item);
	ui->treeWidget->setCurrentItem(item);

	if (focus)
		ui->treeWidget->setFocus();

	return true;
}

void RSFeedWidget::withAll(RSFeedWidgetCallbackFunction callback, void *data)
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

FeedItem *RSFeedWidget::findFeedItem(uint64_t identifier)
{
	QTreeWidgetItemIterator it(ui->treeWidget);
	QTreeWidgetItem *treeItem=NULL;

    // this search could probably be automatised by giving the tree items the identifier as data for some specific role, then calling QTreeWidget::findItems()
#warning TODO
	while ((treeItem = *it) != NULL) {
		++it;

		FeedItem *feedItem = feedItemFromTreeItem(treeItem);
		if (!feedItem)
			continue;

        uint64_t id = feedItem->uniqueIdentifier();

		if (id == identifier)
			return feedItem;
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
