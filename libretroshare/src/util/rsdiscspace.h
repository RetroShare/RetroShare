/*******************************************************************************
 * libretroshare/src/util: rsdiscspace.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2010-2010 by Cyril Soler <csoler@users.sourceforge.net>           *
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
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <util/rsthreads.h>
#include <retroshare/rstypes.h>

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

		static void setPartialsPath(const std::string& path) ;
		static void setDownloadPath(const std::string& path) ;
	private:
		static bool crossSystemDiskStats(const char *file, uint64_t& free_blocks, uint64_t& block_size) ;

		static RsMutex _mtx ;

		static rstime_t _last_check[RS_DIRECTORY_COUNT] ;
		static uint32_t _size_limit_mb ;
		static uint32_t _current_size[RS_DIRECTORY_COUNT] ;
		static bool		_last_res[RS_DIRECTORY_COUNT] ;

		static std::string _partials_path ;
		static std::string _download_path ;
};

