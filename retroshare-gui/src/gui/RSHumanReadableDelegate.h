/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2010 Cyril Soler
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


/** Use this class for displaying dates and sizes in a readable format, while allowing to read the 
 *  real size in the column.
 *
 *  To use:
 *
 *  	- in the QABstractItemView constructor, do a 
 *
 *				myView->setItemDelegateForColumn(SR_SIZE_COL,new RSHumanReadableSizeDelegate()) ;
 *
 *  	- each field must be filled with a string that allows a proper sorting based on lexicographic
 *  	 order. For Sizes, use this:
 *
 *  			myView->setText(SR_SIZE_COL, QString("%1").arg(dir.count,(int)15,(int)10));	
 *
 *  Note: there's no .cpp file, because the code here is really simple.
 */

#include <QItemDelegate>
#include <util/misc.h>

class RSHumanReadableDelegate: public QAbstractItemDelegate
{
	public:
		virtual QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const 
		{
			return QSize(50,17) ;
		}
		virtual void paint(QPainter *painter,const QStyleOptionViewItem & option, const QModelIndex & index) const = 0;
};

class RSHumanReadableAgeDelegate: public RSHumanReadableDelegate
{
	public:
		virtual void paint(QPainter *painter,const QStyleOptionViewItem & option, const QModelIndex & index) const
		{
			painter->drawText(option.rect, Qt::AlignCenter, misc::userFriendlyDuration(index.data().toLongLong())) ;
		}
};

class RSHumanReadableSizeDelegate: public RSHumanReadableDelegate
{
	public:
		virtual void paint(QPainter *painter,const QStyleOptionViewItem & option, const QModelIndex & index) const
		{
			painter->drawText(option.rect, Qt::AlignRight, misc::friendlyUnit(index.data().toULongLong()));
		}
};

