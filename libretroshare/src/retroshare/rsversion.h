/*******************************************************************************
 * libretroshare/src/retroshare: rsversion.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2018  Retroshare Team <contact@retroshare.cc>            *
 * Copyright (C) 2018-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2020  Asociación Civil Altermundi <info@altermundi.net>       *
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

/**
 * @def RS_MINI_VERSION
 * First number of RetroShare versioning scheme
 * Customize it trough qmake command line @see retroshare.pri
 */
#ifndef RS_MAJOR_VERSION
#	define RS_MAJOR_VERSION 0
#endif

/**
 * @def RS_MINI_VERSION
 * Second number of RetroShare versioning scheme
 * Customize it trough qmake command line @see retroshare.pri
 */
#ifndef RS_MINOR_VERSION
#	define RS_MINOR_VERSION 6
#endif

/**
 * @def RS_MINI_VERSION
 * Third number of RetroShare versioning scheme
 * Customize it trough qmake command line @see retroshare.pri
 */
#ifndef RS_MINI_VERSION
#	define RS_MINI_VERSION  6
#endif

/**
 * @def RS_EXTRA_VERSION
 * An extra string to append to the version to make it more descriptive.
 * Customize it trough qmake command line @see retroshare.pri
 */
#ifndef RS_EXTRA_VERSION
#	define RS_EXTRA_VERSION "-RC1"
#endif


/**
 * Use this macro to check in your code if version of RetroShare is at least the
 * specified.
 */
#define RS_VERSION_AT_LEAST(A,B,C) (RS_MAJOR_VERSION > (A) || \
	(RS_MAJOR_VERSION == (A) && \
	(RS_MINOR_VERSION > (B) || \
	(RS_MINOR_VERSION == (B) && RS_MINI_VERSION >= (C)))))


#define RS_PRIVATE_STRINGIFY2(X) #X
#define RS_PRIVATE_STRINGIFY(X) RS_PRIVATE_STRINGIFY2(X)

/**
 * Human readable string describing RetroShare version
 */
constexpr auto RS_HUMAN_READABLE_VERSION =
        RS_PRIVATE_STRINGIFY(RS_MAJOR_VERSION) "." \
        RS_PRIVATE_STRINGIFY(RS_MINOR_VERSION) "." \
        RS_PRIVATE_STRINGIFY(RS_MINI_VERSION) RS_EXTRA_VERSION;

/* Because RetroShare-gui include this file in gui/images/retroshare_win.rc
 * including any C++ things like `#include <string>` will break compilation of
 * RetroShare-gui on Windows. Therefore this file must be kept minimal. */
