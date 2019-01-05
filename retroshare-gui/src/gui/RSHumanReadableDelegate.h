/*******************************************************************************
 * gui/RSHumanReadableDelegate.h                                               *
 *                                                                             *
 * Copyright (c) 2010 Cyril Soler      <retroshare.project@gmail.com>          *
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
#include <QPainter>
#include <util/misc.h>

class RSHumanReadableDelegate: public QAbstractItemDelegate
{
	public:
		virtual QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const 
		{
			return QSize(50,17) ;
		}
		virtual void paint(QPainter *painter,const QStyleOptionViewItem & option, const QModelIndex & index) const = 0;

	protected:
		virtual void setPainterOptions(QPainter *painter,QStyleOptionViewItem& option,const QModelIndex& index) const
		{
			// This part of the code is copied from DLListDelegate.cpp
			//
			QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
			QVariant value = index.data(Qt::TextColorRole);

			if(value.isValid() && qvariant_cast<QColor>(value).isValid()) 
				option.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));

			// select pen color
			if(option.state & QStyle::State_Selected)
				painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
			else 
				painter->setPen(option.palette.color(cg, QPalette::Text));

			// draw the background color 
			if(option.showDecorationSelected && (option.state & QStyle::State_Selected)) 
			{
				if(cg == QPalette::Normal && !(option.state & QStyle::State_Active)) 
					cg = QPalette::Inactive;

				painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
			} 
			else 
			{
				value = index.data(Qt::BackgroundColorRole);

				if(value.isValid() && qvariant_cast<QColor>(value).isValid()) 
					painter->fillRect(option.rect, qvariant_cast<QColor>(value));
			}
		}
};

class RSHumanReadableAgeDelegate: public RSHumanReadableDelegate
{
	public:
		virtual void paint(QPainter *painter,const QStyleOptionViewItem & option, const QModelIndex & index) const
		{
			QStyleOptionViewItem opt(option) ;
			setPainterOptions(painter,opt,index) ;

			painter->drawText(opt.rect, Qt::AlignCenter, misc::timeRelativeToNow(index.data().toLongLong())) ;
		}
};

class RSHumanReadableSizeDelegate: public RSHumanReadableDelegate
{
	public:
		virtual void paint(QPainter *painter,const QStyleOptionViewItem & option, const QModelIndex & index) const
		{
			QStyleOptionViewItem opt(option) ;
			setPainterOptions(painter,opt,index) ;

			painter->drawText(opt.rect, Qt::AlignRight, misc::friendlyUnit(index.data().toULongLong()));
		}
};

