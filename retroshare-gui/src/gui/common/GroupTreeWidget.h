/*******************************************************************************
 * gui/common/GroupTreeWidget.h                                                *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef GROUPTREEWIDGET_H
#define GROUPTREEWIDGET_H

#include <QTreeWidgetItem>
#include <QDateTime>

class QToolButton;
class RshareSettings;
class RSTreeWidgetItemCompareRole;
class RSTreeWidget;

#define GROUPTREEWIDGET_COLOR_STANDARD   -1
#define GROUPTREEWIDGET_COLOR_CATEGORY   0
#define GROUPTREEWIDGET_COLOR_PRIVATEKEY 1
#define GROUPTREEWIDGET_COLOR_COUNT      2

namespace Ui {
	class GroupTreeWidget;
}

class GroupItemInfo
{
public:
	GroupItemInfo()
	  : popularity(0), publishKey(false), adminKey(false)
	  , subscribeFlags(0), max_visible_posts(0)
	{}

public:
	QString   id;
	QString   name;
	QString   description;
	int       popularity;
	QDateTime lastpost;
	QIcon     icon;
	bool      publishKey;
	bool      adminKey;
    quint32  subscribeFlags;
    quint32  max_visible_posts ;
};

//cppcheck-suppress noConstructor
class GroupTreeWidget : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorCategory READ textColorCategory WRITE setTextColorCategory)
	Q_PROPERTY(QColor textColorPrivateKey READ textColorPrivateKey WRITE setTextColorPrivateKey)

public:
	GroupTreeWidget(QWidget *parent = 0);
	~GroupTreeWidget();

	// Add a tool button to the tool area
	void addToolButton(QToolButton *toolButton);

	// Load and save settings (group must be started from the caller)
	void processSettings(bool load);

	// Add a new category item
	QTreeWidgetItem *addCategoryItem(const QString &name, const QIcon &icon, bool expand);
    // Add a new search item
    void setDistSearchVisible(bool) ; // shows/hides distant search UI parts.
	QTreeWidgetItem *addSearchItem(const QString& search_string, uint32_t id, const QIcon &icon) ;
	void removeSearchItem(QTreeWidgetItem *item);

	// Get id of item
	QString itemId(QTreeWidgetItem *item);
	QString itemIdAt(QPoint &point);
	// Fill items of a group
	void fillGroupItems(QTreeWidgetItem *categoryItem, const QList<GroupItemInfo> &itemList);
	// Set the unread count of an item
	void setUnreadCount(QTreeWidgetItem *item, int unreadCount);

	bool isSearchRequestItem(QPoint &point,uint32_t& search_req_id);
	bool isSearchRequestResult(QPoint &point, QString &group_id, uint32_t& search_req_id);

	QTreeWidgetItem *getItemFromId(const QString &id);
	QTreeWidgetItem *activateId(const QString &id, bool focus);

	bool setWaiting(const QString &id, bool wait);

	RSTreeWidget *treeWidget();

	QColor textColorCategory() const { return mTextColor[GROUPTREEWIDGET_COLOR_CATEGORY]; }
	QColor textColorPrivateKey() const { return mTextColor[GROUPTREEWIDGET_COLOR_PRIVATEKEY]; }

	void setTextColorCategory(QColor color) { mTextColor[GROUPTREEWIDGET_COLOR_CATEGORY] = color; }
	void setTextColorPrivateKey(QColor color) { mTextColor[GROUPTREEWIDGET_COLOR_PRIVATEKEY] = color; }

	bool getGroupName(QString id, QString& name);

	int subscribeFlags(const QString &id);

signals:
	void treeCustomContextMenuRequested(const QPoint &pos);
	void treeCurrentItemChanged(const QString &id);
	void treeItemActivated(const QString &id);
    void distantSearchRequested(const QString&) ;

protected:
	void changeEvent(QEvent *e);

private slots:
	void customContextMenuRequested(const QPoint &pos);
	void currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void itemActivated(QTreeWidgetItem *item, int column);
	void filterChanged();
	void distantSearch();

	void sort();

private:
	// Initialize the display menu for sorting
	void initDisplayMenu(QToolButton *toolButton);
	void calculateScore(QTreeWidgetItem *item, const QString &filterText);
	void resort(QTreeWidgetItem *categoryItem);
	void updateColors();

private:
	QMenu *displayMenu;
	QAction *actionSortAscending;
	QAction *actionSortDescending;
	QAction *actionSortByName;
	QAction *actionSortByPopularity;
	QAction *actionSortByLastPost;
	QAction *actionSortByPosts;
	QAction *actionSortByUnread;

	RSTreeWidgetItemCompareRole *compareRole;

	/* Color definitions (for standard see qss.default) */
	QColor mTextColor[GROUPTREEWIDGET_COLOR_COUNT];

	Ui::GroupTreeWidget *ui;
};

#endif // GROUPTREEWIDGET_H
