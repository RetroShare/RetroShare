/*******************************************************************************
 * util/bdfile.cc                                                              *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <bitdht@lunamutt.com>                       *
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
#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "bdfile.h"

namespace librs { 
	namespace util {
		bool ConvertUtf8ToUtf16(const std::string& source, std::wstring& dest) ;
	} 
}

bool bdFile::renameFile(const std::string& from, const std::string& to)
{
	int loops = 0;

#ifdef WIN32
	std::wstring f;
	librs::util::ConvertUtf8ToUtf16(from, f);
	std::wstring t;
	librs::util::ConvertUtf8ToUtf16(to, t);

	while (!MoveFileEx(f.c_str(), t.c_str(), MOVEFILE_REPLACE_EXISTING))
#else
	std::string f(from),t(to) ;

	while (rename(from.c_str(), to.c_str()) < 0)
#endif
	{
#ifdef WIN32
		if (GetLastError() != ERROR_ACCESS_DENIED)
#else
		if (errno != EACCES)
#endif
			/* set errno? */
			return false ;
#ifdef WIN32
		Sleep(100000);				/* us */
#else
		usleep(100000);				/* us */
#endif

		if (loops >= 30)
			return false ;

		loops++;
	}

	return true ;
}

