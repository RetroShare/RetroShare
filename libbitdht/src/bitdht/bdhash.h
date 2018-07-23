/*******************************************************************************
 * bitdht/bdhash.h                                                             *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef BITDHT_HASH_SPACE_H
#define BITDHT_HASH_SPACE_H

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

