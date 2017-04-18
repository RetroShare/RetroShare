/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, Thomas Kister
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

#ifndef RSSTRING_H_
#define RSSTRING_H_

#include <string>
#include <stdarg.h>
#include <sstream>

namespace librs { namespace util {

bool ConvertUtf8ToUtf16(const std::string& source, std::wstring& dest);
bool ConvertUtf16ToUtf8(const std::wstring& source, std::string& dest);

} } // librs::util

#ifdef WIN32
#define INT64FMT "%I64d"
#define UINT64FMT "%I64u"
#else
#define INT64FMT "%lld"
#define UINT64FMT "%llu"
#endif

int rs_sprintf_args(std::string &str, const char *fmt, va_list ap);
int rs_sprintf(std::string &str, const char *fmt, ...);
int rs_sprintf_append_args(std::string &str, const char *fmt, va_list ap);
int rs_sprintf_append(std::string &str, const char *fmt, ...);

void stringToUpperCase(const std::string& s, std::string &upper);
void stringToLowerCase(const std::string& s, std::string &lower);

bool isHexaString(const std::string& s);

template < typename T > std::string rs_to_string( const T& n )
{
#if _GLIBCXX_USE_C99_STDIO
	return std::to_string(n);
#else
	std::ostringstream stm ;
	stm << n ;
	return stm.str() ;
#endif
}


#endif // RSSTRING_H_
