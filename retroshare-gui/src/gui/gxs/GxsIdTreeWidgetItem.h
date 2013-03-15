/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

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

