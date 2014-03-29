/*
 * libretroshare/src/ft: ftdbase.cc
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

#include "ft/ftdbase.h"
#include "util/rsdir.h"

#include "serialiser/rsconfigitems.h"

//#define DB_DEBUG 1

ftFiStore::ftFiStore(CacheStrapper *cs, CacheTransfer *cft, p3PeerMgr *pm,
                        RsPeerId ownid, std::string cachedir)
	:FileIndexStore(cs, cft, pm, ownid, cachedir)
{
	return;
}

bool ftFiStore::search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const
{
	/* could use hintflags to specify which bits of fileinfo to use additionally.
	   eg. hintflags & FT_SEARCH_PEER_ID, then only return matching peers + hash.
	   eg. hintflags & FT_SEARCH_NAME, then only return matching name + hash.
	 *
	 * Still to see if concept is worthwhle
	 */

	/* remove unused parameter warnings */
	(void) hintflags;

#ifdef DB_DEBUG
	std::cerr << "ftFiStore::search(" << hash << "," << hintflags;
	std::cerr << ")";
	std::cerr << std::endl;
#endif

	std::list<FileDetail> results;
	std::list<FileDetail>::iterator it;

	if (SearchHash(hash, results))
	{
		bool first = true;
		for(it = results.begin(); it != results.end(); it++)
		{
#ifdef DB_DEBUG
			std::cerr << "ftFiStore::search() found: ";
			std::cerr << it->name << " (" << it->size;
			std::cerr << ") @ " << it->id << " = " << hash;
			std::cerr << std::endl;
#endif
			bool fullmatch = true;

//			if (it->size != size)
//				fullmatch = false;


#if 0
			if (hintflags & FT_SEARCH_PEER_ID)
			{
				pit = std::find(info.srcIds.begin(), 
					info.srcId.end(). it->id);
				if (pit  == info.srcIds.end())
				{
					fullmatch = false;
				}
			}
#endif


			if (fullmatch)
			{
				if (first)
				{
					first = false;
					info.fname = it->name;
					info.size = it->size;
					info.hash = it->hash;
			
				}

				TransferInfo ti;
				ti.peerId = it->id;
				ti.name = it->name;
				ti.tfRate = 0;
				info.peers.push_back(ti);
			}
		}

		/****	DEPENDS ON SOURCES!
		info.downloadStatus = FT_STATE_COMPLETE:
		****/

		/* if the first flag is cleared, we've definitely
		 * had a full match!.
		 */

		if (!first)
			return true;
	}
	return false;
}

		
ftFiMonitor::ftFiMonitor(CacheStrapper *cs,std::string cachedir, const RsPeerId& pid,const std::string& config_dir)
	:FileIndexMonitor(cs,cachedir, pid,config_dir), p3Config(CONFIG_TYPE_FT_SHARED)
{
	return;
}

bool ftFiMonitor::search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const
{
	return search(hash,hintflags,RsPeerId(),info) ;
}
bool ftFiMonitor::search(const RsFileHash &hash, FileSearchFlags hintflags, const RsPeerId& peer_id,FileInfo &info) const
{
#ifdef DB_DEBUG
	std::cerr << "ftFiMonitor::search(" << hash << "," << hintflags;
	std::cerr << ")";
	std::cerr << std::endl;
#endif

	// Setup search flags according to hintflags. Originally flags was 0. I (cyril) don't know
	// why we don't just pass hintflags there, so I tried to keep the idea.
	//
	FileSearchFlags flags = hintflags ;
	flags &= (RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE);
	
	if(findLocalFile(hash, flags,peer_id,info.path, info.size,info.storage_permission_flags,info.parent_groups))
	{
		/* fill in details */
#ifdef DB_DEBUG
		std::cerr << "ftFiMonitor::search() found: ";
		std::cerr << path;
		std::cerr << " = " << hash << "," << fsize;
		std::cerr << std::endl;
#endif
		info.fname = RsDirUtil::getTopDir(info.path);

		return true;
	}

	return false;
};

int ftFiMonitor::watchPeriod() const
{
	return getPeriod() ;
}
void ftFiMonitor::setWatchPeriod(int seconds)
{
	setPeriod(seconds) ;// call FileIndexMonitor
	IndicateConfigChanged() ;
}

void	ftFiMonitor::setRememberHashCacheDuration(uint32_t days) 
{
	setRememberHashFilesDuration(days) ;	// calls FileIndexMonitor
	IndicateConfigChanged() ;
}
uint32_t ftFiMonitor::rememberHashCacheDuration() const 
{
	return rememberHashFilesDuration() ; // calls FileIndexMonitor
}
void	ftFiMonitor::setRememberHashCache(bool b) 
{
	setRememberHashFiles(b) ; // calls FileIndexMonitor
	IndicateConfigChanged() ;
}
bool ftFiMonitor::rememberHashCache() 
{
	return rememberHashFiles() ; // calls FileIndexMonitor
}
void ftFiMonitor::clearHashCache()
{
	clearHashFiles() ;
}

/******* LOAD / SAVE CONFIG List.
 *
 *
 *
 *
 */

RsSerialiser *ftFiMonitor::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	/* add in the types we need! */
	rss->addSerialType(new RsFileConfigSerialiser());
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss;
}

const std::string hash_cache_duration_ss("HASH_CACHE_DURATION");
const std::string hash_cache_ss("HASH_CACHE");
const std::string watch_file_duration_ss("WATCH_FILES_DELAY");

bool ftFiMonitor::saveList(bool &cleanup, std::list<RsItem *>& sList)
{


	cleanup = true;

#ifdef  DB_DEBUG
	std::cerr << "ftFiMonitor::saveList()";
	std::cerr << std::endl;
#endif

	/* get list of directories */
	std::list<SharedDirInfo> dirList;
	std::list<SharedDirInfo>::iterator it;

	getSharedDirectories(dirList);

	for(it = dirList.begin(); it != dirList.end(); it++)
	{
		RsFileConfigItem *fi = new RsFileConfigItem();
		fi->file.path = (*it).filename ;
		fi->file.name = (*it).virtualname ;
		fi->flags = (*it).shareflags.toUInt32() ;
		fi->parent_groups = (*it).parent_groups ;

		sList.push_back(fi);
	}

	std::map<std::string, std::string> configMap;

	/* basic control parameters */
	{
		std::string s ;
		rs_sprintf(s, "%lu", rememberHashFilesDuration()) ;

		configMap[hash_cache_duration_ss] = s ;
	}
	configMap[hash_cache_ss] = rememberHashFiles()?"YES":"NO" ;

	{
		std::string s ;
		rs_sprintf(s, "%d", watchPeriod()) ;

		configMap[watch_file_duration_ss] = s ;
	}

	RsConfigKeyValueSet *rskv = new RsConfigKeyValueSet();

	/* Convert to TLV */
	for(std::map<std::string,std::string>::const_iterator mit = configMap.begin(); mit != configMap.end(); mit++)
	{
		RsTlvKeyValue kv;
		kv.key = mit->first;
		kv.value = mit->second;

		rskv->tlvkvs.pairs.push_back(kv);
	}

	/* Add KeyValue to saveList */
	sList.push_back(rskv);

	return true;
}


bool    ftFiMonitor::loadList(std::list<RsItem *>& load)
{
	/* for each item, check it exists .... 
	 * - remove any that are dead (or flag?) 
	 */
	static const FileStorageFlags PERMISSION_MASK = DIR_FLAGS_BROWSABLE_OTHERS | DIR_FLAGS_NETWORK_WIDE_OTHERS | DIR_FLAGS_BROWSABLE_GROUPS | DIR_FLAGS_NETWORK_WIDE_GROUPS ;

#ifdef  DEBUG_ELIST
	std::cerr << "ftFiMonitor::loadList()";
	std::cerr << std::endl;
#endif

	std::list<SharedDirInfo> dirList;

	std::list<RsItem *>::iterator it;
	for(it = load.begin(); it != load.end(); it++)
	{
		RsConfigKeyValueSet *rskv ;
				/* switch on type */
		if (NULL != (rskv = dynamic_cast<RsConfigKeyValueSet *>(*it)))
		{
			/* make into map */
			std::map<std::string, std::string> configMap;
			std::map<std::string, std::string>::const_iterator mit ;

			for(std::list<RsTlvKeyValue>::const_iterator kit = rskv->tlvkvs.pairs.begin(); kit != rskv->tlvkvs.pairs.end(); kit++)
			{
				configMap[kit->key] = kit->value;
			}

			if (configMap.end() != (mit = configMap.find(hash_cache_duration_ss)))
			{
				uint32_t t=0 ;
				if(sscanf(mit->second.c_str(),"%d",&t) == 1)
					setRememberHashFilesDuration(t);
			}
			if(configMap.end() != (mit = configMap.find(hash_cache_ss)))
				setRememberHashFiles( mit->second == "YES") ;

			if(configMap.end() != (mit = configMap.find(watch_file_duration_ss)))
			{
				int t=0 ;
				if(sscanf(mit->second.c_str(),"%d",&t) == 1)
					setWatchPeriod(t);
			}
		}

		RsFileConfigItem *fi = dynamic_cast<RsFileConfigItem *>(*it);
		if (!fi)
		{
			delete (*it);
			continue;
		}

		/* ensure that it exists? */

		SharedDirInfo info ;
		info.filename = RsDirUtil::convertPathToUnix(fi->file.path);
		info.virtualname = fi->file.name;
		info.parent_groups = fi->parent_groups;
		info.shareflags = FileStorageFlags(fi->flags) ;
		info.shareflags &= PERMISSION_MASK ;
		info.shareflags &= ~DIR_FLAGS_NETWORK_WIDE_GROUPS ;	// disabling this flag for know, for consistency reasons

		dirList.push_back(info) ;
	}

	/* set directories */
	setSharedDirectories(dirList);
	return true;
}

void	ftFiMonitor::updateShareFlags(const SharedDirInfo& info)
{
	std::cerr << "Updating share flags:" << std::endl;
	std::cerr << "  Directory : " << info.filename << std::endl;
	std::cerr << "  Virtual   : " << info.virtualname << std::endl;
	std::cerr << "  Flags     : " << info.shareflags << std::endl;

	FileIndexMonitor::updateShareFlags(info);

	/* flag for config */
	IndicateConfigChanged();
}

void	ftFiMonitor::setSharedDirectories(const std::list<SharedDirInfo>& dirList)
{
	FileIndexMonitor::setSharedDirectories(dirList);

	/* flag for config */
	IndicateConfigChanged();
}



ftCacheStrapper::ftCacheStrapper(p3ServiceControl *sc, uint32_t ftServiceId)
        :CacheStrapper(sc, ftServiceId)
{
	return;
}

	/* overloaded search function */
bool ftCacheStrapper::search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const
{
	/* remove unused parameter warnings */
	(void) hintflags;

#ifdef DB_DEBUG
	std::cerr << "ftCacheStrapper::search(" << hash << "," << hintflags;
	std::cerr << ")";
	std::cerr << std::endl;
#endif

	RsCacheData data;
	if (findCache(hash, data))
	{
#ifdef DB_DEBUG
		std::cerr << "ftCacheStrapper::search() found: ";
		std::cerr << data.path << "/" << data.name;
		std::cerr << " = " << data.hash << "," << data.size;
		std::cerr << std::endl;
#endif

		/* ... */
		info.size = data.size;
		info.fname = data.name;
		info.path = data.path + "/" + data.name;

		return true;
	}
	return false;
}

