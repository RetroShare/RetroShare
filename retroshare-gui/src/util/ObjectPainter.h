/*******************************************************************************
 * util/ObjectPainter.h                                                        *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team   <retroshare.project@gmail.com>         *
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

#ifndef OBJECTPAINTER_H
#define OBJECTPAINTER_H

class QString;
class QPixmap;

namespace ObjectPainter
{
	void drawButton(const QString &text, const QString &styleSheet, QPixmap &pixmap);
}

#endif	//MOUSEEVENTFILTER_H
