/*******************************************************************************
 * libretroshare/src/retroshare: rsversion.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2018 by Retroshare Team <retroshare.team@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#define RS_MAJOR_VERSION     0
#define RS_MINOR_VERSION     6
#define RS_BUILD_NUMBER      4
#define RS_BUILD_NUMBER_ADD  "" // <-- do we need this?
// The revision number should be the 4 first bytes of the git revision hash, which is obtained using:
//    git log --pretty="%H" | head -1 | cut -c1-8
//
// Do not forget the 0x, since the RS_REVISION_NUMBER should be an integer.
//
#define RS_REVISION_STRING   "01234567"
#define RS_REVISION_NUMBER   0x01234567
