/*******************************************************************************
 * util/bdstring.cc                                                            *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright (C) 2010 Retroshare Team <retroshare.project@gmail.com>           *
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
#ifndef BITDHT_UTILS_BDSTRING_H
#define BITDHT_UTILS_BDSTRING_H

#ifdef WIN32
// for proper handling of %ll
#define bd_snprintf __mingw_snprintf
#define bd_fprintf  __mingw_fprintf
#else
#define bd_snprintf snprintf
#define bd_fprintf  fprintf
#endif

#ifdef __cplusplus
	// These definitions are only available for C++ Modules.
	#include <string>

	int bd_sprintf(std::string &str, const char *fmt, ...);
	int bd_sprintf_append(std::string &str, const char *fmt, ...);
#endif

#endif
