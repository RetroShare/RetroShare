#ifndef BITDHT_HASH_SPACE_H
#define BITDHT_HASH_SPACE_H

/*
 * bitdht/bdhash.h
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */

#include "bitdht/bdpeer.h"

#include <list>
#include <string>
#include <map>

#define BITDHT_HASH_ENTRY_ADD		1
#define BITDHT_HASH_ENTRY_DELETE	2

class bdHashEntry
{
	public:

	bdHashEntry(std::string value, std::string secret, time_t lifetime, time_t store);
	std::string mValue;
	time_t mStoreTS;

	/* These are nice features that OpenDHT had */
	std::string mSecret;
	time_t mLifetime;
};


class bdHashSet
{
	public:

	bdHashSet(bdNodeId *id);

int     search(std::string key, uint32_t maxAge, std::list<bdHashEntry> &entries);
int     modify(std::string key, bdHashEntry *entry, uint32_t modFlags);
int     printHashSet(std::ostream &out);
int     cleanupHashSet(uint32_t maxAge);

	bdNodeId mId;
	std::multimap<std::string, bdHashEntry> mEntries;
};

class bdHashSpace
{
	public:

	bdHashSpace();

	/* accessors */
int 	search(bdNodeId *id, std::string key, uint32_t maxAge, std::list<bdHashEntry> &entries);
int 	modify(bdNodeId *id, std::string key, bdHashEntry *entry, uint32_t modFlags);

int 	printHashSpace(std::ostream&);
int     cleanHashSpace(bdNodeId *min, bdNodeId *max, time_t maxAge);
int     clear();

	private:

	std::map<bdNodeId, bdHashSet> mHashTable;
};


#endif

