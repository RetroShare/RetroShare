/*******************************************************************************
 * gui/common/RSFeedWidget.h                                                   *
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

#ifndef _RSFEEDTREEWIDGET_H
#define _RSFEEDTREEWIDGET_H

#include <QAbstractItemView>
#include <QWidget>
#include <QMap>

#define FEED_TREEWIDGET_SORTROLE Qt::UserRole

class FeedItem;
class QTreeWidgetItem;
class RSTreeWidgetItemCompareRole;

namespace Ui {
class RSFeedWidget;
}

typedef void (*RSFeedWidgetCallbackFunction)(FeedItem *feedItem, void *data);
typedef bool (*RSFeedWidgetFindCallbackFunction)(FeedItem *feedItem, void *data);
typedef bool (*RSFeedWidgetFilterCallbackFunction)(FeedItem *feedItem, const QString &text, int filter);

class RSFeedWidget : public QWidget
{
	Q_OBJECT

public:
	RSFeedWidget(QWidget *parent = 0);
	virtual ~RSFeedWidget();

	QString placeholderText();
	void setPlaceholderText(const QString &placeholderText);

	void addFeedItem(FeedItem *feedItem, Qt::ItemDataRole sortRole, const QVariant &value);
	void addFeedItem(FeedItem *feedItem, const QMap<Qt::ItemDataRole, QVariant> &sort);

	void setSort(FeedItem *feedItem, Qt::ItemDataRole sortRole, const QVariant &value);
	void setSort(FeedItem *feedItem, const QMap<Qt::ItemDataRole, QVariant> &sort);

	int feedItemCount();
	FeedItem *feedItem(int index);
	void removeFeedItem(FeedItem *feedItem);

	void setSortRole(Qt::ItemDataRole role, Qt::SortOrder order);
	void setSortingEnabled(bool enable);
	void setFilterCallback(RSFeedWidgetFilterCallbackFunction callback);

	void enableRemove(bool enable);
	void enableCountChangedSignal(bool enable);
	void setSelectionMode(QAbstractItemView::SelectionMode mode);

	bool scrollTo(FeedItem *feedItem, bool focus);

	void withAll(RSFeedWidgetCallbackFunction callback, void *data);
	FeedItem *findFeedItem(const std::string &identifier);

	void selectedFeedItems(QList<FeedItem*> &feedItems);

signals:
	void feedCountChanged();

public slots:
	void clear();
	void setFilter(const QString &text, int type);
	void setFilterText(const QString &text);
	void setFilterType(int type);

protected:
	bool eventFilter(QObject *object, QEvent *event);
	virtual void feedAdded(FeedItem *feedItem, QTreeWidgetItem *treeItem);
	virtual void feedRemoved(FeedItem *feedItem);
	virtual void feedsCleared();

private slots:
	void feedItemDestroyed(FeedItem *feedItem);
	void feedItemSizeChanged(FeedItem *feedItem);

private:
	void connectSignals(FeedItem *feedItem);
	void disconnectSignals(FeedItem *feedItem);
	FeedItem *feedItemFromTreeItem(QTreeWidgetItem *treeItem);
	QTreeWidgetItem *findTreeWidgetItem(FeedItem *feedItem);
	void filterItems();
	void filterItem(QTreeWidgetItem *treeItem, FeedItem *feedItem);

private:
	/* Sort */
	RSTreeWidgetItemCompareRole *mFeedCompareRole;

	/* Filter */
	RSFeedWidgetFilterCallbackFunction mFilterCallback;
	QString mFilterText;
	int mFilterType;

	/* Remove */
	bool mEnableRemove;

	/* Options */
	int mCountChangedDisabled;

	/* Items */
	QMap<FeedItem*, QTreeWidgetItem*> mItems;

	Ui::RSFeedWidget *ui;
};

#endif
