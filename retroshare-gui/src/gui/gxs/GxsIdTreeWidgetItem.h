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

class GxsIdTreeWidgetItem : public QObject, public RSTreeWidgetItem
{
	Q_OBJECT

public:
	GxsIdTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *parent = NULL);
	GxsIdTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent);

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

