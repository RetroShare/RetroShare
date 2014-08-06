/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

#ifndef GXSCHANNELFILESWIDGET_H
#define GXSCHANNELFILESWIDGET_H

#include <QWidget>

#include "retroshare/rsgxsifacetypes.h"

class RsGxsChannelPost;
class RSTreeWidgetItemCompareRole;
class QTreeWidgetItem;
class GxsFeedItem;

namespace Ui {
class GxsChannelFilesWidget;
}

class GxsChannelFilesWidget : public QWidget
{
	Q_OBJECT

public:
	explicit GxsChannelFilesWidget(QWidget *parent = 0);
	~GxsChannelFilesWidget();

	void addFiles(const RsGxsChannelPost &post, bool related);
	void clear();

public slots:
	void setFilter(const QString &text, int type);
	void setFilterText(const QString &text);
	void setFilterType(int type);

private:
//	QTreeWidgetItem *findFile(const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, const RsFileHash &fileHash);
	void removeItems(const RsGxsGroupId &groupId, const RsGxsMessageId &messageId);
	void closeFeedItem();
	void filterItems();
	void filterItem(QTreeWidgetItem *treeItem);

private slots:
	void currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
	/* Sort */
	RSTreeWidgetItemCompareRole *mCompareRole;

	/* Filter */
	QString mFilterText;
	int mFilterType;

	GxsFeedItem *mFeedItem;

	Ui::GxsChannelFilesWidget *ui;
};

#endif // GXSCHANNELFILESWIDGET_H
