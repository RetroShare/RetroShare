/*******************************************************************************
 * retroshare-gui/src/util/RsQtVersion.cpp                                     *
 *                                                                             *
 * Copyright (C) 2025 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <QtGlobal>

// Functions to compile with Qt 4, Qt 5 and Qt 6

#if QT_VERSION < QT_VERSION_CHECK (5, 15, 0)
#include <QLabel>
QPixmap QLabel_pixmap(QLabel* label)
{
	const QPixmap *pixmap = label->pixmap();
	if (pixmap) {
		return *pixmap;
	}

	return QPixmap();
}
#endif
