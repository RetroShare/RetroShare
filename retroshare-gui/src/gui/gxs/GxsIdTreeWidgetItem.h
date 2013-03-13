/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License
 *  as published by the Free Software Foundation; either version 2.1
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


#ifndef _GXS_ID_TREEWIDGETITEM_H
#define _GXS_ID_TREEWIDGETITEM_H

#include <QTimer>
#include <retroshare/rsidentity.h>

#include "gui/common/RSTreeWidgetItem.h"

/*****
 * NOTE: I have investigated why the Timer doesn't work in GxsForums.
 * It appears that the Timer events occur in the Thread they were created in.
 * GxsForums uses a short term thread to fill the ForumThread - this finishes, 
 * and so the Timer Events don't happen.
 *
 * The Timer events work fine in Wiki and Comments Dialog.
 * because they don't use an additional thread.
 *
 ***/


class GxsIdRSTreeWidgetItem : public QObject, public RSTreeWidgetItem
{
	Q_OBJECT

public:
	GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *parent = NULL);
	GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent);

	void setId(const RsGxsId &id, int column);
	bool getId(RsGxsId &id);

private slots:
	void loadId();

private:
	void init();

	QTimer *mTimer;
	RsGxsId mId;
	int mCount;
	int mColumn;
};



class GxsIdTreeWidgetItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

public:
	GxsIdTreeWidgetItem(QTreeWidget *parent = NULL);
	GxsIdTreeWidgetItem(QTreeWidgetItem *parent);

	void setId(const RsGxsId &id, int column);
	bool getId(RsGxsId &id);

private slots:
	void loadId();

private:
	void init();

	QTimer *mTimer;
	RsGxsId mId;
	int mCount;
	int mColumn;
};

#endif

