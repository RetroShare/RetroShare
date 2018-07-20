/*******************************************************************************
 * libretroshare/src/util: rsdebug.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#pragma once

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#	define RS_DEPRECATED __attribute__((__deprecated__))
#elif defined(_MSC_VER) && (_MSC_VER >= 1300)
#	define RS_DEPRECATED __declspec(deprecated)
#else
#	define RS_DEPRECATED
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#	define RS_DEPRECATED_FOR(f) __attribute__((__deprecated__("Use '" #f "' instead")))
#elif defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#	define RS_DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#	define RS_DEPRECATED_FOR(f) RS_DEPRECATED
#endif
