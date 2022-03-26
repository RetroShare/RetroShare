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

