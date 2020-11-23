/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/DLListDelegate.h                        *
 *                                                                             *
 * Copyright 2007 by Crypton         <retroshare.project@gmail.com>            *
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

#ifndef DLLISTDELEGATE_H
#define DLLISTDELEGATE_H

#include <QAbstractItemDelegate>
#include "xprogressbar.h"


// Defines for download list list columns
#define COLUMN_NAME         0
#define COLUMN_SIZE         1
#define COLUMN_COMPLETED    2
#define COLUMN_DLSPEED      3
#define COLUMN_PROGRESS     4
#define COLUMN_SOURCES      5
#define COLUMN_STATUS       6
#define COLUMN_PRIORITY     7
#define COLUMN_REMAINING    8
#define COLUMN_DOWNLOADTIME 9
#define COLUMN_ID          10
#define COLUMN_LASTDL      11
#define COLUMN_PATH        12
#define COLUMN_COUNT       13

#define PRIORITY_NULL     0.0
#define PRIORITY_FASTER   0.1
#define PRIORITY_AVERAGE  0.2
#define PRIORITY_SLOWER   0.3

#define MAX_CHAR_TMP 128

class QModelIndex;
class QPainter;


class DLListDelegate: public QAbstractItemDelegate {

	Q_OBJECT

	public:
		DLListDelegate(QObject *parent=0);
        virtual ~DLListDelegate(){}
		void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
        QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;

	private:

	public slots:

	signals:
};
#endif

