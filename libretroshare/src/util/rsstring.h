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

namespace librs { namespace util {

bool ConvertUtf8ToUtf16(const std::string& source, std::wstring& dest);
bool ConvertUtf16ToUtf8(const std::wstring& source, std::string& dest);

} } // librs::util

#endif // RSSTRING_H_
