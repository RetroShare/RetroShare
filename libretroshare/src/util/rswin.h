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

#include <windows.h>
#include <string>

// For win32 systems (tested on MingW+Ubuntu)
#define stat64 _stati64

// Should be in Iphlpapi.h, but mingw doesn't seem to have these
// Values copied directly from:
// http://msdn.microsoft.com/en-us/library/aa366845(v=vs.85).aspx
// (Title: MIB_IPADDRROW structure)

#ifndef MIB_IPADDR_DISCONNECTED
#define MIB_IPADDR_DISCONNECTED 0x0008 // Address is on disconnected interface
#endif

#ifndef MIB_IPADDR_DELETED
#define MIB_IPADDR_DELETED      0x0040 // Address is being deleted
#endif

#endif // WINDOWS_SYS

#endif // RSWIN_H_
