/*
 * RetroShare FileCache Module: cachetest.h
 *   
 * Copyright 2004-2007 by Robert Fernie.
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

#ifndef MRK_TEST_CACHE_H
#define MRK_TEST_CACHE_H

#include "cachestrapper.h"

#define TESTID 0xffff
#define TESTID2 0xffee

class CacheTestSource: public CacheSource
{
	public:
	CacheTestSource(CacheStrapper *cs, std::string dir)
	:CacheSource(cs, TESTID, false, dir) { return; }
};

class CacheTestStore: public CacheStore
{
	public:
	CacheTestStore(CacheTransfer *cft, std::string dir)
	:CacheStore(TESTID, false, cft, dir) { return; }
};


class CacheTestMultiSource: public CacheSource
{
	public:
	CacheTestMultiSource(CacheStrapper *cs, std::string dir)
	:CacheSource(cs, TESTID2, true, dir) { return; }
};

class CacheTestMultiStore: public CacheStore
{
	public:
	CacheTestMultiStore(CacheTransfer *cft, std::string dir)
	:CacheStore(TESTID2, true, cft, dir) { return; }
};


#endif

