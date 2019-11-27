/*******************************************************************************
 * libretroshare/src/retroshare: rsversion.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2018  Retroshare Team <contact@retroshare.cc>            *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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


#define RS_PRIVATE_STRINGIFY2(X) #X
#define RS_PRIVATE_STRINGIFY(X) RS_PRIVATE_STRINGIFY2(X)

/**
 * Human readable string describing RetroShare version
 */
constexpr auto RS_HUMAN_READABLE_VERSION =
        RS_PRIVATE_STRINGIFY(RS_MAJOR_VERSION) "." \
        RS_PRIVATE_STRINGIFY(RS_MINOR_VERSION) "." \
        RS_PRIVATE_STRINGIFY(RS_MINI_VERSION) RS_EXTRA_VERSION;


#include <cstdint>
#include <string>

/**
 * Helper to expose version information to JSON API.
 * From C++ you should use directly the macro and constants defined upstair
 * @jsonapi{development}
 */
class RsVersion
{
public:
	/**
	 * @brief Write version information to given paramethers
	 * @jsonapi{development,unauthenticated}
	 * @param[out] major storage
	 * @param[out] minor storage
	 * @param[out] mini storage
	 * @param[out] extra storage
	 * @param[out] human storage
	 */
	static void version( uint32_t& major, uint32_t& minor, uint32_t& mini,
	                     std::string& extra, std::string& human );
};

/**
 * Pointer to global instance of RsVersion, for the sake of JSON API, from C++
 * you can use directly the macro and constants defined upstair
 * @jsonapi{development}
 */
extern RsVersion* rsVersion;
