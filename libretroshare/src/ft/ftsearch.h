/*
 * libretroshare/src/ft: ftsearch.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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
		virtual bool	search(const std::string & /*hash*/, FileSearchFlags  /*hintflags*/,const std::string&  /*peer_id*/, FileInfo & /*info*/) const 
		{
			std::cerr << "Non overloaded search method called!!!" << std::endl;
			return false;
		}
		virtual bool	search(const std::string &hash, FileSearchFlags hintflags, FileInfo &info) const = 0;

};

#endif
