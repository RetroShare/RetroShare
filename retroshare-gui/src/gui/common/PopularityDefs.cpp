/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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

#include <QCoreApplication>

#include "PopularityDefs.h"

QIcon PopularityDefs::icon(int popularity)
{
	if (popularity <= 1) 
		return QIcon(":/images/hot_0.png");
	else if (popularity <= 2) /* 1-1 */
		return QIcon(":/images/hot_1.png");
	else if (popularity <= 5) /* 2-2 */
		return QIcon(":/images/hot_2.png");
	else if (popularity <= 10) /* 3-5 */
		return QIcon(":/images/hot_3.png");
	else if (popularity <= 20) /* 6-10 */
		return QIcon(":/images/hot_4.png");
	else /* >10 */
		return QIcon(":/images/hot_5.png");
}

QString PopularityDefs::tooltip(int popularity)
{
	return QString("%1: %2").arg(qApp->translate("PopularityDefs", "Popularity")).arg(popularity);
}
