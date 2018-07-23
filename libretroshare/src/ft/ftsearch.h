/*******************************************************************************
 * libretroshare/src/ft: ftsearch.h                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef FT_SEARCH_HEADER
#define FT_SEARCH_HEADER

/* 
 * ftSearch
 *
 * This is a generic search interface - used by ft* to find files.
 * The derived class will search for Caches/Local/ExtraList/Remote entries.
 *
 */

#include "retroshare/rsfiles.h" // includes retroshare/rstypes.h too!

class ftSearch
{

	public:
		ftSearch() { return; }
		virtual ~ftSearch() { return; }
        virtual bool	search(const RsFileHash & /*hash*/, FileSearchFlags  /*hintflags*/,const RsPeerId&  /*peer_id*/, FileInfo & /*info*/) const
		{
			std::cerr << "Non overloaded search method called!!!" << std::endl;
			return false;
		}
        virtual bool	search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const = 0;

};

#endif
