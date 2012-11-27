/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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

#ifndef GROUPTREEWIDGET_H
#define GROUPTREEWIDGET_H

#include <QWidget>
#include <QIcon>
#include <QTreeWidgetItem>
#include <QDateTime>

class QPushButton;
class RshareSettings;
class RSTreeWidgetItemCompareRole;

namespace Ui {
	class GroupTreeWidget;
}

class GroupItemInfo
{
public:
	GroupItemInfo()
	{
		popularity = 0;
		privatekey = false;
	}

public:
	QString   id;
	QString   name;
	QString   description;
	int       popularity;
	QDateTime lastpost;
	QIcon     icon;
	bool      privatekey;
};

class GroupTreeWidget : public QWidget
{
	Q_OBJECT

public:
	GroupTreeWidget(QWidget *parent = 0);
	~GroupTreeWidget();

	// Load and save settings (group must be startet from the caller)
	void processSettings(RshareSettings *settings, bool load);
	// Initialize the display menu for sorting
	void initDisplayMenu(QPushButton *pushButton);

	// Add a new category item
	QTreeWidgetItem *addCategoryItem(const QString &name, const QIcon &icon, bool expand);
	// Get id of item
	QString itemId(QTreeWidgetItem *item);
	// Fill items of a group
	void fillGroupItems(QTreeWidgetItem *categoryItem, const QList<GroupItemInfo> &itemList);
	// Set the unread count of an item
	void setUnreadCount(QTreeWidgetItem *item, int unreadCount);

	QTreeWidgetItem *getItemFromId(const QString &id);
	QTreeWidgetItem *activateId(const QString &id, bool focus);

signals:
	void treeCustomContextMenuRequested(const QPoint &pos);
	void treeCurrentItemChanged(const QString &id);
	void treeItemClicked(const QString &id);

protected:
	void changeEvent(QEvent *e);

private slots:
	void customContextMenuRequested(const QPoint &pos);
	void currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void itemClicked(QTreeWidgetItem *item, int column);
	void filterChanged();

	void sort();

private:
	void calculateScore(QTreeWidgetItem *item, const QString &filterText);
	void resort(QTreeWidgetItem *categoryItem);

private:
	QMenu *displayMenu;
	QAction *actionSortAscending;
//	QAction *actionSortDescending;
	QAction *actionSortByName;
	QAction *actionSortByPopularity;
	QAction *actionSortByLastPost;

	RSTreeWidgetItemCompareRole *compareRole;

	Ui::GroupTreeWidget *ui;
};

#endif // GROUPTREEWIDGET_H
