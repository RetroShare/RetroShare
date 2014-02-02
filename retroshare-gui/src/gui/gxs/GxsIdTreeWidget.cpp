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

#include "GxsIdTreeWidget.h"
#include "GxsIdDetails.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include <iostream>

#include <QPainter>

static void paintGxsId( QPainter * painter, 
	const QStyleOptionViewItem & option /*, const QRect &rect */, const RsGxsId &id )
{
	QString desc;
	std::list<QIcon> icons;
	if (!GxsIdDetails::MakeIdDesc(id, true, desc, icons))
	{
		/* flag for reloading */
	}

	const QRect &rect = option.rect;
	int x = rect.left();
	int y = rect.top();
	int height = rect.height();
	int width = rect.width();
	

	std::list<QIcon>::iterator it;
	const int IconSize = 15;
	int i = 0;
	for(it = icons.begin(); it != icons.end(); it++, i++)
	{
		it->paint(painter, x, y, IconSize, IconSize);
		x += IconSize;
	}

#define DELTA_X 4
	QRect textRect = rect.adjusted(DELTA_X + IconSize * i, 0, 0, 0);
	painter->drawText(textRect, 0, desc, NULL);
}
		

GxsIdItemDelegate::GxsIdItemDelegate(GxsIdTreeWidget *tree, int col, QObject *parent)
:QStyledItemDelegate(parent), mTree(tree), mGxsIdColumn(col)
{
	return;
}

void GxsIdItemDelegate::paint( QPainter * painter, 
	const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	std::cerr << "GxsIdItemDelegate::paint()";
	std::cerr << std::endl;
		
	RsGxsId id = mTree->ItemTextFromIndex(index, mGxsIdColumn).toStdString();
	paintGxsId(painter, option, id);
}

QSize GxsIdItemDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const	
{
	std::cerr << "GxsIdItemDelegate::sizeHint()";
	std::cerr << std::endl;

	return QStyledItemDelegate::sizeHint(option, index);
}

/******************************************************************/

GxsIdRSItemDelegate::GxsIdRSItemDelegate(GxsIdRSTreeWidget *tree, int col, QObject *parent)
:QStyledItemDelegate(parent), mTree(tree), mGxsIdColumn(col)
{
	return;
}

void GxsIdRSItemDelegate::paint( QPainter * painter, 
	const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	std::cerr << "GxsIdRSItemDelegate::paint()";
	std::cerr << std::endl;
		
	RsGxsId id = mTree->ItemTextFromIndex(index, mGxsIdColumn).toStdString();
	paintGxsId(painter, option, id);
}

QSize GxsIdRSItemDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const	
{
	std::cerr << "GxsIdRSItemDelegate::sizeHint()";
	std::cerr << std::endl;

	return QStyledItemDelegate::sizeHint(option, index);
}

/******************************************************************/

GxsIdTreeWidget::GxsIdTreeWidget(QWidget *parent)
:QTreeWidget(parent), mIdDelegate(NULL)
{
	return;
}


void    GxsIdTreeWidget::setGxsIdColumn(int col)
{
	mIdDelegate = new GxsIdItemDelegate(this, col, this);
	setItemDelegateForColumn(col, mIdDelegate);
}


QString GxsIdTreeWidget::ItemTextFromIndex(const QModelIndex & index, int column ) const
{
	// get real item.
	QTreeWidgetItem *item = itemFromIndex(index);
	if (!item)
	{
		std::cerr << "GxsIdTreeWidget::ItemTextFromIndex() Invalid Item";
		std::cerr << std::endl;
		QString text;
		return text;
	}
	return item->text(column);
}


GxsIdRSTreeWidget::GxsIdRSTreeWidget(QWidget *parent)
:RSTreeWidget(parent), mIdDelegate(NULL)
{
	return;
}

void    GxsIdRSTreeWidget::setGxsIdColumn(int col)
{
	mIdDelegate = new GxsIdRSItemDelegate(this, col, this);
	setItemDelegateForColumn(col, mIdDelegate);
}


QString GxsIdRSTreeWidget::ItemTextFromIndex(const QModelIndex & index, int column ) const
{
	// get real item.
	QTreeWidgetItem *item = itemFromIndex(index);
	if (!item)
	{
		std::cerr << "GxsIdTreeWidget::ItemTextFromIndex() Invalid Item";
		std::cerr << std::endl;
		QString text;
		return text;
	}
	return item->text(column);
}

