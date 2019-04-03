/*******************************************************************************
 * gui/common/RsBanListDefs.h                                                  *
 *                                                                             *
 * Copyright (C) 2015, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef _RSBANLISTDEFS_H
#define _RSBANLISTDEFS_H

#include <QString>

class RsBanListDefs
{
public:
//	static const QString resultState(uint32_t result);
	static const QString resultString(uint32_t result);
};

#endif
