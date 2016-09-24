/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#ifndef SFLISTDELEGATE_H
#define SFLISTDELEGATE_H

#include <QAbstractItemDelegate>

// Defines for download list list columns
#define FNAME 0
#define FSIZE 1

#define MAX_CHAR_TMP 128

class QModelIndex;
class QPainter;

class SFListDelegate: public QAbstractItemDelegate {

	Q_OBJECT

	public:
		SFListDelegate(QObject *parent=0);
		~SFListDelegate();
		void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
		QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;

	private:

	public slots:

	signals:
};
#endif
