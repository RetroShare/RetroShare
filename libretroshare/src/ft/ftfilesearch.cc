/*******************************************************************************
 * libretroshare/src/ft: ftfilesearch.cc                                       *
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
#include "ft/ftfilesearch.h"

const uint32_t MAX_SEARCHS = 24; /* lower 24 bits of hint */

//#define DEBUG_SEARCH 1

ftFileSearch::ftFileSearch()
	:mSearchs(MAX_SEARCHS)
{
	uint32_t i;
	for(i = 0; i < MAX_SEARCHS; i++)
	{
		mSearchs[i] = NULL;		
	}
}

bool	ftFileSearch::addSearchMode(ftSearch *search, FileSearchFlags hintflags)
{
	hintflags &= FileSearchFlags(0x000000ff);

#ifdef DEBUG_SEARCH
	std::cerr << "ftFileSearch::addSearchMode() : " << hintflags;
	std::cerr << std::endl;
#endif

	uint32_t i;
	for  (i = 0; i < MAX_SEARCHS; i++)
	{
		uint32_t hints = hintflags.toUInt32() >> i;
		if (hints & 0x0001)
		{
			/* has the flag */
			mSearchs[i] = search;

#ifdef DEBUG_SEARCH
			std::cerr << "ftFileSearch::addSearchMode() to slot ";
			std::cerr << i;
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifdef DEBUG_SEARCH
	std::cerr << "ftFileSearch::addSearchMode() Failed";
	std::cerr << std::endl;
#endif

	return false;
}

bool	ftFileSearch::search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const
{
	uint32_t hints, i;

#ifdef DEBUG_SEARCH
	std::cerr << "ftFileSearch::search(" << hash ;
	std::cerr << ", " << hintflags << ");";
	std::cerr << std::endl;
#endif
	
	for  (i = 0; i < MAX_SEARCHS; i++)
	{
		hints = hintflags.toUInt32() >> i;
		if (hints & 0x0001)
		{
			/* has the flag */
			ftSearch *search = mSearchs[i];
			if (search)
			{
#ifdef DEBUG_SEARCH
				std::cerr << "ftFileSearch::search() SLOT: ";
				std::cerr << i;
				std::cerr << std::endl;
#endif
				if (search->search(hash, hintflags, info))
				{
#ifdef DEBUG_SEARCH
					std::cerr << "ftFileSearch::search() SLOT: ";
					std::cerr << i << " success!";
					std::cerr << std::endl;
#endif
					return true;
				}
				else
				{
#ifdef DEBUG_SEARCH
					std::cerr << "ftFileSearch::search() SLOT: ";
					std::cerr << i << " no luck";
					std::cerr << std::endl;
#endif
				}
			}
		}
	}

	/* if we haven't found it by now! - check if SPEC_ONLY flag is set */
	if (hintflags & RS_FILE_HINTS_SPEC_ONLY)
	{
#ifdef DEBUG_SEARCH
		std::cerr << "ftFileSearch::search() SPEC_ONLY: Failed";
		std::cerr << std::endl;
#endif
		return false;
	}

#ifdef DEBUG_SEARCH
	std::cerr << "ftFileSearch::search() Searching Others (no SPEC ONLY):";
	std::cerr << std::endl;
#endif

	/* if we don't have the SPEC_ONLY flag, 
	 * we check through all the others 
	 */
	for  (i = 0; i < MAX_SEARCHS; i++)
	{
		hints = hintflags.toUInt32() >> i;
		if (hints & 0x0001)
		{
			continue;
		}

		/* has the flag */
		ftSearch *search = mSearchs[i];
		if (search)
		{

#ifdef DEBUG_SEARCH
			std::cerr << "ftFileSearch::search() SLOT: " << i;
			std::cerr << std::endl;
#endif
			if (search->search(hash, hintflags, info))
			{
#ifdef DEBUG_SEARCH
				std::cerr << "ftFileSearch::search() SLOT: ";
				std::cerr << i << " success!";
				std::cerr << std::endl;
#endif
				return true;
			}
			else
			{
#ifdef DEBUG_SEARCH
				std::cerr << "ftFileSearch::search() SLOT: ";
				std::cerr << i << " no luck";
				std::cerr << std::endl;
#endif
			}

		}
	}
	/* found nothing */
	return false;
}

