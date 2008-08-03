/*
 * libretroshare/src/ft: ftfilesearch.cc
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

#include "ft/ftfilesearch.h"

const uint32_t MAX_SEARCHS = 24; /* lower 24 bits of hint */

ftFileSearch::ftFileSearch()
	:mSearchs(MAX_SEARCHS)
{
	uint32_t i;
	for(i = 0; i < MAX_SEARCHS; i++)
	{
		mSearchs[i] = NULL;		
	}
}

bool	ftFileSearch::addSearchMode(ftSearch *search, uint32_t hintflags)
{
	hintflags &= 0x00ffffff;

#ifndef DEBUG_SEARCH
	std::cerr << "ftSearchDummy::addSearchMode() : " << hintflags;
	std::cerr << std::endl;
#endif

	uint32_t i;
	for  (i = 0; i < MAX_SEARCHS; i++)
	{
		uint32_t hints = hintflags >> i;
		if (hints & 0x0001)
		{
			/* has the flag */
			mSearchs[i] = search;

#ifndef DEBUG_SEARCH
			std::cerr << "ftSearchDummy::addSearchMode() to slot ";
			std::cerr << i;
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifndef DEBUG_SEARCH
	std::cerr << "ftSearchDummy::addSearchMode() Failed";
	std::cerr << std::endl;
#endif

	return false;
}

bool	ftFileSearch::search(std::string hash, uint64_t size, uint32_t hintflags, FileInfo &info) const
{
	uint32_t hints, i;

#ifndef DEBUG_SEARCH
	std::cerr << "ftFileSearch::search(" << hash << ", " << size; 
	std::cerr << ", " << hintflags << ");";
	std::cerr << std::endl;
#endif
	
	for  (i = 0; i < MAX_SEARCHS; i++)
	{
		hints = hintflags >> i;
		if (hints & 0x0001)
		{
			/* has the flag */
			ftSearch *search = mSearchs[i];
			if (search)
			{
#ifndef DEBUG_SEARCH
				std::cerr << "ftFileSearch::search() SLOT: ";
				std::cerr << i;
				std::cerr << std::endl;
#endif
				if (search->search(hash, size, hintflags, info))
				{
					return true;
				}
			}
		}
	}

	/* if we haven't found it by now! - check if SPEC_ONLY flag is set */
	if (hintflags & RS_FILE_HINTS_SPEC_ONLY)
	{
#ifndef DEBUG_SEARCH
		std::cerr << "ftFileSearch::search() SPEC_ONLY: Failed";
		std::cerr << std::endl;
#endif
		return false;
	}

#ifndef DEBUG_SEARCH
	std::cerr << "ftSearchDummy::search() Searching Others:";
	std::cerr << std::endl;
#endif

	/* if we don't have the SPEC_ONLY flag, 
	 * we check through all the others 
	 */
	for  (i = 0; i < MAX_SEARCHS; i++)
	{
		hints = hintflags >> i;
		if (hints & 0x0001)
		{
			continue;
		}

		/* has the flag */
		ftSearch *search = mSearchs[i];
		if (search)
		{

#ifndef DEBUG_SEARCH
			std::cerr << "ftFileSearch::search() SLOT: " << i;
			std::cerr << std::endl;
#endif
			if (search->search(hash, size, hintflags, info))
			{
				return true;
			}
		}
	}
	/* found nothing */
	return false;
}


bool 	ftSearchDummy::search(std::string hash, uint64_t size, uint32_t hintflags, FileInfo &info) const
{
#ifndef DEBUG_SEARCH
	std::cerr << "ftSearchDummy::search(" << hash << ", " << size; 
	std::cerr << ", " << hintflags << ");";
	std::cerr << std::endl;
#endif
        return false;
}


