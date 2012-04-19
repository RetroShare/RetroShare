#ifndef BITDHT_UTILS_BDSTRING_H
#define BITDHT_UTILS_BDSTRING_H

/****************************************************************
 *  libbitdht is distributed under the following license:
 *
 *  Copyright (C) 2012 RetroShare Team
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

#include <string>

#ifdef WIN32
// for proper handling of %ll
#define bd_snprintf __mingw_snprintf
#define bd_fprintf  __mingw_fprintf
#else
#define bd_snprintf snprintf
#define bd_fprintf  fprintf
#endif

int bd_sprintf(std::string &str, const char *fmt, ...);
int bd_sprintf_append(std::string &str, const char *fmt, ...);

#endif
