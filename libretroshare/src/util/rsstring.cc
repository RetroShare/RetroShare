/*******************************************************************************
 * libretroshare/src/util: rsstd.h                                             *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (c) 2010, Thomas Kister                                           *
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
#include "rsstring.h"

#ifdef WINDOWS_SYS
#include <windows.h>
#include <malloc.h>
#else
#include <vector>
#include <stdarg.h>
#include <stdlib.h>
#endif
#include <stdint.h>
#include <stdio.h>

namespace librs { namespace util {

bool ConvertUtf8ToUtf16(const std::string& source, std::wstring& dest)
{
	if (source.empty()) {
		dest.clear();
		return true;
	}

#ifdef WINDOWS_SYS
	int nbChars = MultiByteToWideChar(CP_UTF8, 0, source.c_str(), -1, 0, 0);
	if(nbChars == 0) {
		return false;
	}

	wchar_t* utf16Name = new wchar_t[nbChars];
	if( MultiByteToWideChar(CP_UTF8, 0, source.c_str(), -1, utf16Name, nbChars) == 0) {
		delete[] utf16Name;
		return false;
	}

	dest = utf16Name;
	delete[] utf16Name;
#else
	std::vector<wchar_t> res;
	std::string::size_type len = source.length();

	unsigned int i = 0;
	unsigned long temp;

	while (i < len) {
		char src = source[i];
		if ((src & 0x80) == 0) { // ASCII : 0000 0000-0000 007F 0xxxxxxx
			temp = src;
			++i;
		} else if ((src & 0xE0) == 0xC0) { // 0000 0080-0000 07FF 110xxxxx 10xxxxxx
			temp = (src & 0x1F);
			temp <<= 6;
			temp += (source[i + 1] & 0x3F);
			i += 2;
		} else if ((src & 0xF0) == 0xE0) { // 0000 0800-0000 FFFF 1110xxxx 10xxxxxx 10xxxxxx
			temp = (src & 0x0F);
			temp <<= 6;
			temp += (source[i + 1] & 0x3F);
			temp <<= 6;
			temp += (source[i + 2] & 0x3F);
			i += 3;
		} else if ((src & 0xF8) == 0xF0) { // 0001 0000-001F FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			temp = (src & 0x07);
			temp <<= 6;
			temp += (source[i + 1] & 0x3F);
			temp <<= 6;
			temp += (source[i + 2] & 0x3F);
			temp <<= 6;
			temp += (source[i + 3] & 0x3F);
			i += 4;
		} else if ((src & 0xFC) == 0xF8) { // 0020 0000-03FF FFFF 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			temp = (src & 0x03);
			temp <<= 6;
			temp += (source[i + 1] & 0x3F);
			temp <<= 6;
			temp += (source[i + 2] & 0x3F);
			temp <<= 6;
			temp += (source[i + 3] & 0x3F);
			temp <<= 6;
			temp += (source[i + 4] & 0x3F);
			i += 5;
		} else if ((src & 0xFE) == 0xFC) { // 0400 0000-7FFF FFFF 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			temp = (src & 0x01);
			temp <<= 6;
			temp += (source[i + 1] & 0x3F);
			temp <<= 6;
			temp += (source[i + 2] & 0x3F);
			temp <<= 6;
			temp += (source[i + 3] & 0x3F);
			temp <<= 6;
			temp += (source[i + 4] & 0x3F);
			temp <<= 6;
			temp += (source[i + 5] & 0x3F);
			i += 6;
		} else {
			temp = '?';
		}
		// Need to transform to UTF-16 > handle surrogates
		if (temp > 0xFFFF) {
			// First high surrogate
			res.push_back(0xD800 + wchar_t(temp >> 10));
			// Now low surrogate
			res.push_back(0xDC00 + wchar_t(temp & 0x3FF));
		} else {
			res.push_back(wchar_t(temp));
		}
	}
	// Check whether first wchar_t is the BOM 0xFEFF
	if (res[0] == 0xFEFF) {
		dest.append(&res[1], res.size() - 1);
	} else {
		dest.append(&res[0], res.size());
	}
#endif

	return true;
}

bool ConvertUtf16ToUtf8(const std::wstring& source, std::string& dest)
{
#ifdef WINDOWS_SYS
	int nbChars = WideCharToMultiByte(CP_UTF8, 0, source.c_str(), -1, 0, 0, 0, 0);
	if(nbChars == 0) {
		return false;
	}

	char* utf8Name = new char[nbChars];
	if( WideCharToMultiByte(CP_UTF8, 0, source.c_str(), -1, utf8Name, nbChars, 0, 0) == 0) {
		delete[] utf8Name;
		return false;
	}

	dest = utf8Name;
	delete[] utf8Name;
#else
	std::vector<char> res;

	std::wstring::size_type len = source.length();
	unsigned int i = 0;
	unsigned long temp;

	while (i < len) {
		if ((source[i] & 0xD800) == 0xD800) { // surrogate
			temp = (source[i] - 0xD800);
			temp <<= 10;
			temp += (source[i + 1] - 0xDC00);
			i += 2;
		} else {
			temp = source[i];
			++i;
		}
		if (temp < 0x00000080) { // ASCII : 0000 0000-0000 007F 0xxxxxxx
			res.push_back(char(temp));
		} else if (temp < 0x00000800) { // 0000 0080-0000 07FF 110xxxxx 10xxxxxx
			res.push_back(char(0xC0 | (temp >> 6)));
			res.push_back(char(0x80 | (temp & 0x3F)));
		} else if (temp < 0x00010000) { // 0000 0800-0000 FFFF 1110xxxx 10xxxxxx 10xxxxxx
			res.push_back(char(0xE0 | (temp >> 12)));
			res.push_back(char(0x80 | ((temp >> 6) & 0x3F)));
			res.push_back(char(0x80 | (temp & 0x3F)));
		} else if (temp < 0x00200000) { // 0001 0000-001F FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			res.push_back(char(0xF0 | (temp >> 18)));
			res.push_back(char(0x80 | ((temp >> 12) & 0x3F)));
			res.push_back(char(0x80 | ((temp >> 6) & 0x3F)));
			res.push_back(char(0x80 | (temp & 0x3F)));
		} else if (temp < 0x04000000) { // 0020 0000-03FF FFFF 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			res.push_back(char(0xF8 | (temp >> 24)));
			res.push_back(char(0x80 | ((temp >> 18) & 0x3F)));
			res.push_back(char(0x80 | ((temp >> 12) & 0x3F)));
			res.push_back(char(0x80 | ((temp >> 6) & 0x3F)));
			res.push_back(char(0x80 | (temp & 0x3F)));
		} else if (temp < 0x80000000) { // 0400 0000-7FFF FFFF 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			res.push_back(char(0xFC | (temp >> 30)));
			res.push_back(char(0x80 | ((temp >> 24) & 0x3F)));
			res.push_back(char(0x80 | ((temp >> 18) & 0x3F)));
			res.push_back(char(0x80 | ((temp >> 12) & 0x3F)));
			res.push_back(char(0x80 | ((temp >> 6) & 0x3F)));
			res.push_back(char(0x80 | (temp & 0x3F)));
		}
	}

	dest.append(&res[0], res.size());
#endif

	return true;
}

#if 0
bool is_alphanumeric(char c)
{
	return (c>='0' && c<='9') || (c>='a' && c<='z') || (c>='A' && c<='Z');
}

bool is_alphanumeric(const std::string& s)
{
	for( uint32_t i=0; i < s.size(); ++i)
		if(!is_alphanumeric(s[i])) return false;
	return true;
}
#endif

} } // librs::util

#ifdef WINDOWS_SYS
// asprintf() and vasprintf() are missing in Win32
static int vasprintf(char **sptr, const char *fmt, va_list argv)
{
	int wanted = __mingw_vsnprintf(*sptr = NULL, 0, fmt, argv);
	if ((wanted > 0) && ((*sptr = (char*) malloc(wanted + 1)) != NULL)) {
		return __mingw_vsprintf(*sptr, fmt, argv);
	}

	return wanted;
}

//static int asprintf(char **sptr, const char *fmt, ...)
//{
//	int retval;
//	va_list argv;
//	va_start( argv, fmt );
//	retval = vasprintf(sptr, fmt, argv);
//	va_end(argv);
//	return retval;
//}
#endif

int rs_sprintf_args(std::string &str, const char *fmt, va_list ap)
{
	char *buffer = NULL;

	int retval = vasprintf(&buffer, fmt, ap);

	if (retval >= 0) {
		if (buffer) {
			str = buffer;
			free(buffer);
		} else {
			str.clear();
		}
	} else {
		str.clear();
	}

	return retval;
}

int rs_sprintf(std::string &str, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	int retval = rs_sprintf_args(str, fmt, ap);
	va_end(ap);

	return retval;
}

int rs_sprintf_append_args(std::string &str, const char *fmt, va_list ap)
{
	char *buffer = NULL;

	int retval = vasprintf(&buffer, fmt, ap);

	if (retval >= 0) {
		if (buffer) {
			str.append(buffer);
			free(buffer);
		}
	}

	return retval;
}

int rs_sprintf_append(std::string &str, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	int retval = rs_sprintf_append_args(str, fmt, ap);
	va_end(ap);

	return retval;
}

void stringToUpperCase(const std::string& s, std::string &upper)
{
	upper = s ;

	for(uint32_t i=0;i<upper.size();++i)
		if(upper[i] > 96 && upper[i] < 123)
			upper[i] -= 97-65 ;
}

void stringToLowerCase(const std::string& s, std::string &lower)
{
	lower = s ;

	for(uint32_t i=0;i<lower.size();++i)
		if(lower[i] > 64 && lower[i] < 91)
			lower[i] += 97-65 ;
}



bool isHexaString(const std::string& s)
{
	for(uint32_t i=0;i<s.length();++i)
		if(!((s[i] >= 'A' && s[i] <= 'F') || (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f')))
		return false ;

	return true ;
}


