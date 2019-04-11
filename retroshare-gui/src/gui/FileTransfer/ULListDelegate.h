/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/ULListDelegate.h                        *
 *                                                                             *
 * Copyright (c) 2007 Crypton         <retroshare.project@gmail.com>           *
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

#ifndef ULLISTDELEGATE_H
#define ULLISTDELEGATE_H

#include <QAbstractItemDelegate>

// Defines for upload list list columns
#define COLUMN_UNAME        0
#define COLUMN_UPEER        1
#define COLUMN_USIZE        2
#define COLUMN_UTRANSFERRED 3
#define COLUMN_ULSPEED      4
#define COLUMN_UPROGRESS    5
#define COLUMN_UHASH        6
#define COLUMN_UCOUNT       7


#define MAX_CHAR_TMP 128

class QModelIndex;
class QPainter;


class ULListDelegate: public QAbstractItemDelegate {

	Q_OBJECT

	public:
		ULListDelegate(QObject *parent=0);
		~ULListDelegate();
		void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
		QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;

	private:

	public slots:

	signals:
};
#endif

