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

#ifndef _GXS_ID_TREEWIDGET_H
#define _GXS_ID_TREEWIDGET_H

#include <QTreeWidget>
#include <QStyledItemDelegate>

#include "gui/common/RSTreeWidget.h"

/*****
 * To draw multiple fancy Icons, and refresh IDs properly we need
 * to overload QTreeWidget, and provide a QItemDelegate to draw stuff.
 *
 * The ItemDelegate 
 *
 ****/

class GxsIdTreeWidget;
class GxsIdRSTreeWidget;

class GxsIdItemDelegate : public QStyledItemDelegate
{
	public:
	GxsIdItemDelegate(GxsIdTreeWidget *tree, int gxsIdColumn, QObject *parent = 0);
	virtual void	paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
	virtual QSize	sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

	private:
	GxsIdTreeWidget *mTree;
	int mGxsIdColumn;
};


class GxsIdRSItemDelegate : public QStyledItemDelegate
{
	public:
	GxsIdRSItemDelegate(GxsIdRSTreeWidget *tree, int gxsIdColumn, QObject *parent = 0);
	virtual void	paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
	virtual QSize	sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;

	private:
	GxsIdRSTreeWidget *mTree;
	int mGxsIdColumn;
};


class GxsIdTreeWidget : public QTreeWidget
{
public:
	GxsIdTreeWidget(QWidget *parent = NULL);
	virtual ~GxsIdTreeWidget() { return; }

void    setGxsIdColumn(int col);
QString ItemTextFromIndex(const QModelIndex & index, int column ) const;

private:
	GxsIdItemDelegate *mIdDelegate;
};


class GxsIdRSTreeWidget : public RSTreeWidget
{
public:
	GxsIdRSTreeWidget(QWidget *parent = NULL);
	virtual ~GxsIdRSTreeWidget() { return; }

void    setGxsIdColumn(int col);
QString ItemTextFromIndex(const QModelIndex & index, int column ) const;

private:
	GxsIdRSItemDelegate *mIdDelegate;
};

#endif
