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


/**
 * This file provides helper functions for the windows environment
 */


#ifndef RSWIN_H_
#define RSWIN_H_


#ifdef WINDOWS_SYS

#ifdef _WIN32_WINNT
#error "Please include \"util/rswin.h\" *before* any other one as _WIN32_WINNT needs to predefined"
#endif

// This defines the platform to be WinXP or later and is needed for getaddrinfo
// It must be declared before pthread.h includes windows.h
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <string>

// For win32 systems (tested on MingW+Ubuntu)
#define stat64 _stati64 


namespace librs { namespace util {


bool ConvertUtf8ToUtf16(const std::string& source, std::wstring& dest);

bool ConvertUtf16ToUtf8(const std::wstring& source, std::string& dest);


} } // librs::util


#endif // WINDOWS_SYS


#endif // RSWIN_H_
