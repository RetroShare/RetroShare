/*******************************************************************************
 * bitdht/bdhash.cc                                                            *
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

#include "bitdht/bdhash.h"
#include "bitdht/bdstddht.h"
#include <iostream>

bdHashEntry::bdHashEntry(std::string value, std::string secret, time_t lifetime, time_t store)
	:mValue(value), mStoreTS(store), mSecret(secret), mLifetime(lifetime)
{
	return;
}


/**************************** class bdHashSet ********************************/

bdHashSet::bdHashSet(bdNodeId *id)
	:mId(*id)
{
	return;
}

int 	bdHashSet::search(std::string key, uint32_t maxAge, std::list<bdHashEntry> &entries)
{
	std::multimap<std::string, bdHashEntry>::iterator sit, eit, it;
        sit = mEntries.lower_bound(key);
        eit = mEntries.upper_bound(key);

	time_t now = time(NULL);

	for(it = sit; it != eit; it++)
	{
		time_t age = now - it->second.mStoreTS;
		if (age < (int32_t) maxAge)
		{
			entries.push_back(it->second);
		}
	}
	return (0 < entries.size());
}

/*** 
 * With modification.
 * If there is no secret -> it cannot be deleted (must timeout), but can be extended.
 * If there is a secret -> must include it to modify.
 *
 * Therefore if identical entry without secret comes along - what do I do?
 * -> create duplicate?
 */

int 	bdHashSet::modify(std::string key, bdHashEntry *entry, uint32_t modFlags)
{
	std::multimap<std::string, bdHashEntry>::iterator sit, eit, it;
        sit = mEntries.lower_bound(key);
        eit = mEntries.upper_bound(key);

	time_t now = time(NULL);

	bool updated = false;
	for(it = sit; it != eit; it++)
	{
		/* check it all */
		if (it->second.mValue == entry->mValue)
		{
			bool noSecret = (it->second.mSecret == "");
			bool sameSecret = (it->second.mSecret == entry->mSecret);
			bool update = false;

			if (noSecret && sameSecret)
			{
				/* only allowed to increase lifetime */
				if (modFlags == BITDHT_HASH_ENTRY_ADD)
				{
					time_t existKillTime = it->second.mLifetime + it->second.mStoreTS;
					time_t newKillTime = entry->mLifetime + now;
					if (newKillTime > existKillTime)
					{
						update = true;
					}
				}
			}
			else if (sameSecret)
			{
				if (modFlags == BITDHT_HASH_ENTRY_ADD)
				{
					update = true;
				}
				else if (modFlags == BITDHT_HASH_ENTRY_DELETE)
				{
					/* do it here */
					mEntries.erase(it);
					return 1;
				}
			}
			
			if (update)
			{
				it->second.mStoreTS = now;
				it->second.mLifetime = entry->mLifetime;
				updated = true;
			}
		}
	}

	if ((!updated) && (modFlags == BITDHT_HASH_ENTRY_ADD))
	{
		/* create a new entry */
		bdHashEntry newEntry(entry->mValue, entry->mSecret, entry->mLifetime, now);
	        mEntries.insert(std::pair<std::string, bdHashEntry>(key, newEntry));
		updated = true;
	}
	return updated;
}

int	bdHashSet::printHashSet(std::ostream &out)
{
	time_t now = time(NULL);
	std::multimap<std::string, bdHashEntry>::iterator it;
	out << "Hash: ";
	bdStdPrintNodeId(out, &mId); // Allowing "Std" as we dont need dht functions everywhere.
	out << std::endl;

	for(it = mEntries.begin(); it != mEntries.end();it++)
	{
		time_t age = now - it->second.mStoreTS;
		out << "\tK:" << bdStdConvertToPrintable(it->first);
		out << " V:" << bdStdConvertToPrintable(it->second.mValue);
		out << " A:" << age << " L:" << it->second.mLifetime;
		out << " S:" << bdStdConvertToPrintable(it->second.mSecret);
		out << std::endl;
	}
	return 1;
}

	
	

int	bdHashSet::cleanupHashSet(uint32_t maxAge)
{
	time_t now = time(NULL);

	/* this is nasty... but don't know how to effectively remove from multimaps
	 * * Must do full repeat for each removal.
	 */

	std::multimap<std::string, bdHashEntry>::iterator it;
	for(it = mEntries.begin(); it != mEntries.end();)
	{
		time_t age = now - it->second.mStoreTS;
		if ((age > (int32_t) maxAge) || (age > it->second.mLifetime))
		{
			mEntries.erase(it);
			it = mEntries.begin();
		}
		else
		{
			it++;
		}
	}
	return 1;
}


	



/*******************************  class bdHashSpace ***************************/

bdHashSpace::bdHashSpace()
{
	return;
}

	/* accessors */
int 	bdHashSpace::search(bdNodeId *id, std::string key, uint32_t maxAge, std::list<bdHashEntry> &entries)
{
	std::map<bdNodeId, bdHashSet>::iterator it;
	it = mHashTable.find(*id);
	if (it == mHashTable.end()) 
	{
		/* no entry */
		return 1;
	}

	return it->second.search(key, maxAge, entries);
}

int 	bdHashSpace::modify(bdNodeId *id, std::string key, bdHashEntry *entry, uint32_t modFlags)
{
	std::map<bdNodeId, bdHashSet>::iterator it;
	it = mHashTable.find(*id);
	if (it == mHashTable.end()) 
	{
		if (modFlags == BITDHT_HASH_ENTRY_DELETE)
		{
			/* done already */
			return 1;
		}
			
		//mHashTable[*id] = bdHashSet(id);
	        mHashTable.insert(std::pair<bdNodeId, bdHashSet>(*id, bdHashSet(id)));
		it = mHashTable.find(*id);
	}

	return it->second.modify(key, entry, modFlags);
}

int     bdHashSpace::printHashSpace(std::ostream &out)
{
	std::map<bdNodeId, bdHashSet>::iterator it;
	out << "bdHashSpace::printHashSpace()" << std::endl;
	out << "--------------------------------------------" << std::endl;
	
	for(it = mHashTable.begin(); it != mHashTable.end(); it++)
	{
		it->second.printHashSet(out);
	}
	out << "--------------------------------------------" << std::endl;
	return 1;
}

int     bdHashSpace::cleanHashSpace(bdNodeId *min, bdNodeId *max, time_t maxAge)
{
	std::map<bdNodeId, bdHashSet>::iterator it;
	std::list<bdNodeId> eraseList;
	std::list<bdNodeId>::iterator eit;

	for(it = mHashTable.begin(); it != mHashTable.end(); it++)
	{
		if ((it->first < *min) ||
			(*max < it->first))
		{
			/* schedule for erasure */
			eraseList.push_back(it->first);
		}
		else
		{
			/* clean up Hash Set */
			it->second.cleanupHashSet(maxAge);	
		}
	}

	/* cleanup */
	while(eraseList.size() > 0)
	{
		bdNodeId &eId = eraseList.front();
		it = mHashTable.find(eId);
		if (it != mHashTable.end())
		{
			mHashTable.erase(it);
		}
	}
	return 1;
}


int     bdHashSpace::clear()
{
	mHashTable.clear();
	return 1;
}


