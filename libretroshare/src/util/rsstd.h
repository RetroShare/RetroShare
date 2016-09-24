/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2015, RetroShare Team
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

#ifndef RSSTD_H_
#define RSSTD_H_

namespace rsstd {

template<typename _IIter>
void delete_all(_IIter iter_begin, _IIter iter_end)
{
	for (_IIter it = iter_begin; it != iter_end; ++it) {
		delete(*it);
	}
}

}

#endif // RSSTD_H_
