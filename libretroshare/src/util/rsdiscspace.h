/*
 * libretroshare/src/util: rsdiscspace.h
 *
 * Universal Networking Header for RetroShare.
 *
 * Copyright 2010-2010 by Cyril Soler.
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <util/rsthreads.h>

class RsDiscSpace
{
	public:
		typedef uint32_t DiscLocation ;

		// Returns false is the disc space is lower than the given size limit, true otherwise.
		// When the size limit is reached, this class calls notify to warn the user (possibly 
		// with a popup window).
		//
		static bool checkForDiscSpace(DiscLocation loc) ;

		// Allow the user to specify his own size limit. Should not be too low, especially not 0 MB ;-)
		// 10MB to 100MB are sensible values.
		//
		static void setFreeSpaceLimit(uint32_t mega_bytes) ;
		static uint32_t freeSpaceLimit() ;

	private:
		static bool crossSystemDiskStats(const char *file, uint32_t& free_blocks, uint32_t& block_size) ;

		static RsMutex _mtx ;

		static time_t _last_check[3] ;
		static uint32_t _size_limit_mb ;
		static uint32_t _current_size[3] ;
		static bool		_last_res[3] ;
};

