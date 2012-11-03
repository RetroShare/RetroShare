/*
 * libretroshare/src/util: rsmemcache.h
 *
 * Identity interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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
#ifndef RS_UTIL_MEM_CACHE
#define RS_UTIL_MEM_CACHE

#include <map>
#include <time.h>
#include <iostream>
#include <inttypes.h>

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/* Generic Memoory Cache 
 *
 * This is probably crude and crap to start with.
 * Want Least Recently Used (LRU) discard policy, without having to search whole cache.
 * Use two maps:
 *   - mDataMap[key] => data.
 *   - mLruMap[AccessTS] => key (multimap)
 */

#define DEFAULT_MEM_CACHE_SIZE 100

template<class Key, class Value> class RsMemCache
{
	public:

	RsMemCache(uint32_t max_size = DEFAULT_MEM_CACHE_SIZE)
	:mDataCount(0), mMaxSize(max_size) { return; }

	bool is_cached(const Key &key) const;
	bool fetch(const Key &key, Value &data);
	bool store(const Key &key, const Value &data);

	bool resize(); // should be called periodically to cleanup old entries.

	private:

	bool update_lrumap(const Key &key, time_t old_ts, time_t new_ts);
	bool discard_LRU(int count_to_clear);

	// internal class.
	class cache_data
	{
		public:
		cache_data() { return; }
		cache_data(Key in_key, Value in_data, time_t in_ts)
		:key(in_key), data(in_data), ts(in_ts) { return; }
		Key key;
		Value data;
		time_t ts;
	};


        std::map<Key, cache_data > mDataMap;
        std::multimap<time_t, Key> mLruMap;
        uint32_t mDataCount;
	uint32_t mMaxSize;
};


template<class Key, class Value> bool RsMemCache<Key, Value>::is_cached(const Key &key) const
{
	typename std::map<Key,cache_data>::const_iterator it;
	it = mDataMap.find(key);
	if (it == mDataMap.end())
	{
		std::cerr << "RsMemCache::is_cached(" << key << ") false";
		std::cerr << std::endl;

		return false;
	}
	std::cerr << "RsMemCache::is_cached(" << key << ") false";
	std::cerr << std::endl;
	return true;
	
}


template<class Key, class Value> bool RsMemCache<Key, Value>::fetch(const Key &key, Value &data)
{
	typename std::map<Key, cache_data>::iterator it;
	it = mDataMap.find(key);
	if (it == mDataMap.end())
	{
		std::cerr << "RsMemCache::fetch(" << key << ") false";
		std::cerr << std::endl;

		return false;
	}
	
	std::cerr << "RsMemCache::fetch(" << key << ") OK";
	std::cerr << std::endl;

	data = it->second.data;

	/* update ts on data */
        time_t old_ts = it->second.ts;
        time_t new_ts = time(NULL);
        it->second.ts = new_ts;

        update_lrumap(key, old_ts, new_ts);

	return true;
}

template<class Key, class Value> bool RsMemCache<Key, Value>::store(const Key &key, const Value &data)
{
	std::cerr << "RsMemCache::store()";
	std::cerr << std::endl;

	// For consistency
	typename std::map<Key, cache_data>::const_iterator it;
	it = mDataMap.find(key);
	if (it != mDataMap.end())
	{
		// ERROR.
		std::cerr << "RsMemCache::store() ERROR entry exists already";
		std::cerr << std::endl;
		return false;
	}

	/* add new lrumap entry */
        time_t old_ts = 0;
        time_t new_ts = time(NULL);

	mDataMap[key] = cache_data(key, data, new_ts);
        mDataCount++;

        update_lrumap(key, old_ts, new_ts);

	return true;
}


template<class Key, class Value> bool RsMemCache<Key, Value>::update_lrumap(const Key &key, time_t old_ts, time_t new_ts)
{
	if (old_ts == 0)
	{
		std::cerr << "p3IdService::locked_cache_update_lrumap(" << key << ") just insert!";
		std::cerr << std::endl;

		/* new insertion */
		mLruMap.insert(std::make_pair(new_ts, key));
		return true;
	}

	/* find old entry */
	typename std::multimap<time_t, Key>::iterator mit;
	typename std::multimap<time_t, Key>::iterator sit = mLruMap.lower_bound(old_ts);
	typename std::multimap<time_t, Key>::iterator eit = mLruMap.upper_bound(old_ts);

        for(mit = sit; mit != eit; mit++)
	{
		if (mit->second == key)
		{
			mLruMap.erase(mit);
			std::cerr << "p3IdService::locked_cache_update_lrumap(" << key << ") rm old";
			std::cerr << std::endl;

			if (new_ts != 0) // == 0, means remove.
			{	
				std::cerr << "p3IdService::locked_cache_update_lrumap(" << key << ") added new_ts";
				std::cerr << std::endl;
				mLruMap.insert(std::make_pair(new_ts, key));
			}
			return true;
		}
	}
	std::cerr << "p3IdService::locked_cache_update_lrumap(" << key << ") ERROR";
	std::cerr << std::endl;

	return false;
}

template<class Key, class Value> bool RsMemCache<Key, Value>::resize()
{
	std::cerr << "RsMemCache::resize()";
	std::cerr << std::endl;

	int count_to_clear = 0;
	{
		// consistency check.
		if ((mDataMap.size() != mDataCount) ||
			(mLruMap.size() != mDataCount))
		{
			// ERROR.
			std::cerr << "RsMemCache::resize() CONSISTENCY ERROR";
			std::cerr << std::endl;
		}
	
		if (mDataCount > mMaxSize)
		{
			count_to_clear = mDataCount - mMaxSize;
			std::cerr << "RsMemCache::resize() to_clear: " << count_to_clear;
			std::cerr << std::endl;
		}
	}

	if (count_to_clear > 0)
	{
		discard_LRU(count_to_clear);
	}
	return true;
}



template<class Key, class Value> bool RsMemCache<Key, Value>::discard_LRU(int count_to_clear)
{
	while(count_to_clear > 0)
	{
		typename std::multimap<time_t, Key>::iterator mit = mLruMap.begin();
		if (mit != mLruMap.end())
		{
			Key key = mit->second;
			mLruMap.erase(mit);

			/* now clear from real cache */
			//std::map<Key, cache_data<Key, Value> >::iterator it;
			typename std::map<Key, cache_data>::iterator it;
			it = mDataMap.find(key);
			if (it == mDataMap.end())
			{
				// ERROR
				std::cerr << "RsMemCache::discard_LRU(): ERROR Missing key: " << key;
				std::cerr << std::endl;
				return false;
			}
			else
			{
				std::cerr << "RsMemCache::discard_LRU() removing: " << key;
				std::cerr << std::endl;
				mDataMap.erase(it);
				mDataCount--;
			}
		}
		else
		{
			// No More Data, ERROR.
			std::cerr << "RsMemCache::discard_LRU(): INFO more more cache data";
			std::cerr << std::endl;
			return true;
		}
		count_to_clear--;
	}
	return true;
}




#endif // RS_UTIL_MEM_CACHE
