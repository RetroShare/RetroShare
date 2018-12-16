/*******************************************************************************
 * libretroshare/src/retroshare: rsversion.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2018 by Retroshare Team <retroshare.project@gmail.com>       *
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
#	define RS_MINI_VERSION  5
#endif

/**
 * @def RS_EXTRA_VERSION
 * An extra string to append to the version to make it more descriptive.
 * Customize it trough qmake command line @see retroshare.pri
 */
#ifndef RS_EXTRA_VERSION
#	define RS_EXTRA_VERSION "alpha"
#endif


/**
 * Use this macro to check in your code if version of RetroShare is at least the
 * specified.
 */
#define RS_VERSION_AT_LEAST(A,B,C) (RS_MAJOR_VERSION > (A) || \
	(RS_MAJOR_VERSION == (A) && \
	(RS_MINOR_VERSION > (B) || \
	(RS_MINOR_VERSION == (B) && RS_MINI_VERSION >= (C)))))


#define __RS_PRIVATE_STRINGIFY2(X) #X
#define __RS_PRIVATE_STRINGIFY(X) __RS_PRIVATE_STRINGIFY2(X)

/**
 * Human readable string describing RetroShare version
 */
constexpr auto RS_HUMAN_READABLE_VERSION =
        __RS_PRIVATE_STRINGIFY(RS_MAJOR_VERSION) "." \
        __RS_PRIVATE_STRINGIFY(RS_MINOR_VERSION) "." \
        __RS_PRIVATE_STRINGIFY(RS_MINI_VERSION) RS_EXTRA_VERSION;
