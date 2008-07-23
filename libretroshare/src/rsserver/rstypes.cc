
/*
 * "$Id: rstypes.cc,v 1.2 2007-04-07 08:41:00 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2006 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


/* Insides of RetroShare interface.
 * only prints stuff out at the moment
 */

#include "rsiface/rstypes.h"
#include <iostream>
#include <sstream>
#include <iomanip>


/**********************************************************************
 * NOTE NOTE NOTE ...... XXX
 * BUG in MinGW .... %hhx in sscanf overwrites 32bits, instead of 8bits.
 * this means that scanf(.... &(data[15])) is running off the 
 * end of the buffer, and hitting data[15-18]...
 * To work around this bug we are reading into proper int32s
 * and then copying the data over...
 *
**********************************************************************/


std::ostream &operator<<(std::ostream &out, const FileInfo &info)
{
	out << "FileInfo: path: " << info.path;
	out << std::endl;
	out << "File: " << info.fname;
	out << " Size: " << info.size;
	out << std::endl;
	out << "Hash: " << info.hash;
	return out;
}


