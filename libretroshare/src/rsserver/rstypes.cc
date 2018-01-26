
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

#include "retroshare/rstypes.h"
#include <iostream>
#include <iomanip>
#include <time.h>


/**********************************************************************
 * NOTE NOTE NOTE ...... XXX
 * BUG in MinGW .... %hhx in sscanf overwrites 32bits, instead of 8bits.
 * this means that scanf(.... &(data[15])) is running off the 
 * end of the buffer, and hitting data[15-18]...
 * To work around this bug we are reading into proper int32s
 * and then copying the data over...
 *
**********************************************************************/

std::ostream &operator<<(std::ostream &out, const DirDetails& d)
{
    std::cerr << "====DIR DETAILS====" << std::endl;
    std::cerr << "  parent pointer: " << d.parent << std::endl;
    std::cerr << "  current pointer: " << d.ref << std::endl;
    std::cerr << "  parent row    : " << d.prow << std::endl;
    std::cerr << "  type          : " << (int)d.type << std::endl;
    std::cerr << "  PeerId        : " << d.id << std::endl;
    std::cerr << "  Name          : " << d.name << std::endl;
    std::cerr << "  Hash          : " << d.hash << std::endl;
    std::cerr << "  Path          : " << d.path << std::endl;
    std::cerr << "  Count         : " << d.count << std::endl;
    std::cerr << "  Age           : " << time(NULL) - (int)d.mtime << std::endl;
    std::cerr << "  Min age       : " << time(NULL) - (int)d.max_mtime << std::endl;
    std::cerr << "  Flags         : " << d.flags << std::endl;
    std::cerr << "  Parent groups : " ; for(std::list<RsNodeGroupId>::const_iterator it(d.parent_groups.begin());it!=d.parent_groups.end();++it) std::cerr << (*it) << " "; std::cerr << std::endl;
    std::cerr << "  Children      : " ; for(uint32_t i=0;i<d.children.size();++i) std::cerr << (void*)(intptr_t)d.children[i].ref << " "; std::cerr << std::endl;
    std::cerr << "===================" << std::endl;
    return out;
}
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

RS_REGISTER_SERIALIZABLE_TYPE_DEF(CompressedChunkMap)
