/*******************************************************************************
 * libretroshare/src/rsserver: rstypes.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare.project@gmail.com>         *
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

/* Insides of RetroShare interface.
 * only prints stuff out at the moment
 */

#include "retroshare/rstypes.h"
#include <iostream>
#include <iomanip>
#include "util/rstime.h"


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
