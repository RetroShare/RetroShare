/*
 * "$Id: filedexserver.cc,v 1.24 2007-05-05 16:10:06 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

#include "server/filedexserver.h"
#include <fstream>
#include <time.h>

#include "pqi/pqibin.h"
#include "pqi/pqiarchive.h"

#include "util/rsdebug.h"
#include "util/rsdir.h"

#include <sstream>
#include <iomanip>


/* New FileCache Stuff */
#include "server/ftfiler.h"
#include "dbase/cachestrapper.h"
#include "dbase/fimonitor.h"
#include "dbase/fistore.h"

#include "pqi/p3connmgr.h"
#include "pqi/p3authmgr.h"

#include "serialiser/rsserviceids.h"
#include "serialiser/rsconfigitems.h"


#include <sstream>

const int fldxsrvrzone = 47659;

/****
#define SERVER_DEBUG 1
#define DEBUG_TICK 1
****/
#define SERVER_DEBUG 1
#define DEBUG_TICK 1

filedexserver::filedexserver()
	:p3Config(CONFIG_TYPE_FSERVER), 
	 pqisi(NULL), mAuthMgr(NULL), mConnMgr(NULL),
	save_dir("."),
        mCacheStrapper(NULL), ftFiler(NULL), fiStore(NULL), fimon(NULL)
{
	initialiseFileStore();
}

int	filedexserver::setSearchInterface(P3Interface *si, p3AuthMgr *am, p3ConnectMgr *cm)
{
	pqisi = si;
	mAuthMgr = am;
	mConnMgr = cm;
	return 1;
}

std::list<RsFileTransfer *> filedexserver::getTransfers()
{
	return ftFiler->getStatus();
}


int	filedexserver::tick()
{
	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::tick()");

	/* the new Cache Hack() */
	FileStoreTick();

	if (pqisi == NULL)
	{
		std::ostringstream out;
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
			"filedexserver::tick() Invalid Interface()");

		return 1;
	}

	int moreToTick = 0;

	if (0 < pqisi -> tick())
	{
		moreToTick = 1;
#ifdef DEBUG_TICK
		std::cerr << "filedexserver::tick() moreToTick from pqisi" << std::endl;
#endif
	}

	if (0 < handleInputQueues())
	{
		moreToTick = 1;
#ifdef DEBUG_TICK
		std::cerr << "filedexserver::tick() moreToTick from InputQueues" << std::endl;
#endif
	}

	if (0 < handleOutputQueues())
	{
		moreToTick = 1;
#ifdef DEBUG_TICK
		std::cerr << "filedexserver::tick() moreToTick from OutputQueues" << std::endl;
#endif
	}
	return moreToTick;
}


int	filedexserver::status()
{
	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::status()");

	if (pqisi == NULL)
	{
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
			"filedexserver::status() Invalid Interface()");
		return 1;
	}

	pqisi -> status();

	return 1;
}

std::string	filedexserver::getSaveDir()
{
	return save_dir;
}

void	filedexserver::setSaveDir(std::string d)
{
	save_dir = d;
	if (ftFiler)
		ftFiler -> setSaveBasePath(save_dir);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}

void    filedexserver::setEmergencySaveDir(std::string s)
{
	if (ftFiler)
	{
		ftFiler -> setEmergencyBasePath(s);
	}
}

bool 	filedexserver::getSaveIncSearch()
{
	return save_inc;
}

void	filedexserver::setSaveIncSearch(bool v)
{
	save_inc = v;

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}

int     filedexserver::addSearchDirectory(std::string dir)
{
	dbase_dirs.push_back(dir);
	reScanDirs();

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	return 1;
}

int     filedexserver::removeSearchDirectory(std::string dir)
{
	std::list<std::string>::iterator it;
	for(it = dbase_dirs.begin(); (it != dbase_dirs.end()) 
			&& (dir != *it); it++);
	if (it != dbase_dirs.end())
	{
		dbase_dirs.erase(it);
	}

	reScanDirs();

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	return 1;
}

std::list<std::string> &filedexserver::getSearchDirectories()
{
	return dbase_dirs;
}


int     filedexserver::reScanDirs()
{
	if (fimon)
		fimon->setSharedDirectories(dbase_dirs);

	return 1;
}

bool    filedexserver::ConvertSharedFilePath(std::string path, std::string &fullpath)
{
	if (fimon)
		return fimon->convertSharedFilePath(path, fullpath);

	return false;
}

void    filedexserver::ForceDirectoryCheck()
{
	if (fimon)
		fimon->forceDirectoryCheck();
	return;
}

bool    filedexserver::InDirectoryCheck()
{
	if (fimon)
		return fimon->inDirectoryCheck();
	return false;
}


/*************************************** NEW File Cache Stuff ****************************/

void filedexserver::initialiseFileStore()
{

}

const std::string LOCAL_CACHE_FILE_KEY = "LCF_NAME";
const std::string LOCAL_CACHE_HASH_KEY = "LCF_HASH";
const std::string LOCAL_CACHE_SIZE_KEY = "LCF_SIZE";

void    filedexserver::setFileCallback(std::string ownId, CacheStrapper *strapper, ftfiler *ft, NotifyBase *cb)
{
	mCacheStrapper = strapper;
	ftFiler = ft; 

	/* setup FiStore/Monitor */
	std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";
	fiStore = new FileIndexStore(strapper, ftFiler, cb, ownId, remotecachedir);

	/* now setup the FiMon */
	fimon = new FileIndexMonitor(strapper, localcachedir, ownId);

	/* setup ftFiler
	 * to find peer info / savedir 
	 */
	FileHashSearch *fhs = new FileHashSearch(fiStore, fimon);
	ftFiler -> setFileHashSearch(fhs);
	ftFiler -> setSaveBasePath(save_dir);

	/* now add the set to the cachestrapper */

	CachePair cp(fimon, fiStore, CacheId(RS_SERVICE_TYPE_FILE_INDEX, 0));
	mCacheStrapper -> addCachePair(cp);

	return;
}


void    filedexserver::StartupMonitor()
{
	std::cerr << "filedexserver::StartupMonitor() err" << std::endl;

	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::StartupMonitor()");

	/* startup the FileMonitor (after cache load) */
	fimon->setPeriod(100); /* 10 minutes */
        /* start it up */
	
	fimon->setSharedDirectories(dbase_dirs);
	fimon->start();


	std::list<RsFileTransfer *>::iterator tit;
	for(tit = mResumeTransferList.begin(); 
		tit != mResumeTransferList.end(); tit++)
	{
		RsFileTransfer      *rsft = (*tit);

		/* only add in ones which have a hash (filters old versions) */
		if (rsft->file.hash != "")
		{
			ftFiler -> getFile(
				rsft->file.name, 
				rsft->file.hash,
				rsft->file.filesize, "");
		}
		delete rsft;
	}
	mResumeTransferList.clear();
}



int filedexserver::FileCacheSave()
{
	/************ TMP HACK SAVE until new serialiser is finished */

	RsPeerId pid;
	std::map<CacheId, CacheData> ids;
	std::map<CacheId, CacheData>::iterator it;

#ifdef SERVER_DEBUG 
	std::cerr << "filedexserver::FileCacheSave() listCaches:" << std::endl;
	fimon->listCaches(std::cerr);
	fimon->cachesAvailable(pid, ids);
#endif

	std::string localCacheFile;
	std::string localCacheHash;
	std::string localCacheSize;

	if (ids.size() == 1)
	{
		it = ids.begin();
		localCacheFile = (it->second).name;
		localCacheHash = (it->second).hash;
		std::ostringstream out;
		out << (it->second).size;
		localCacheSize = out.str();
	}

	/* extract the details of the local cache */
	//getSSLRoot()->setSetting(LOCAL_CACHE_FILE_KEY, localCacheFile);
	//getSSLRoot()->setSetting(LOCAL_CACHE_HASH_KEY, localCacheHash);
	//getSSLRoot()->setSetting(LOCAL_CACHE_SIZE_KEY, localCacheSize);

	/************ TMP HACK SAVE until new serialiser is finished */
	return 1;
}

// Transfer control.
void filedexserver::saveFileTransferStatus()
{
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}


// Transfer control.
int filedexserver::getFile(std::string fname, std::string hash,
                        uint32_t size, std::string dest)

{
	int ret = ftFiler -> getFile(fname, hash, size, dest);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return ret;
}

void filedexserver::clear_old_transfers()
{
	ftFiler -> clearFailedTransfers();

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}

void filedexserver::cancelTransfer(std::string fname, std::string hash, uint32_t size)
{
	ftFiler -> cancelFile(hash);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}


int filedexserver::RequestDirDetails(std::string uid, std::string path,
                                        DirDetails &details)
{
	return fiStore->RequestDirDetails(uid, path, details);
}

int filedexserver::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags)
{
	return fiStore->RequestDirDetails(ref, details, flags);
}

int filedexserver::SearchKeywords(std::list<std::string> keywords, 
					std::list<FileDetail> &results)
{
	return fiStore->SearchKeywords(keywords, results);
}

int filedexserver::SearchBoolExp(Expression * exp, std::list<FileDetail> &results)
{
	return fiStore->searchBoolExp(exp, results);
}


int filedexserver::FileStoreTick()
{
	ftFiler -> tick();
	return 1;
}


// This function needs to be divided up.
int     filedexserver::handleInputQueues()
{
	// get all the incoming results.. and print to the screen.
	RsCacheRequest *cr;
	RsCacheItem    *ci;
	RsFileRequest *fr;
	RsFileData *fd;

	// Loop through Search Results.
	int i = 0;
	int i_init = 0;

	//std::cerr << "filedexserver::handleInputQueues()" << std::endl;
	while((ci = pqisi -> GetSearchResult()) != NULL)
	{
		//std::cerr << "filedexserver::handleInputQueues() Recvd SearchResult (CacheResponse!)" << std::endl;
		std::ostringstream out;
		if (i++ == i_init)
		{
			out << "Recieved Search Results:" << std::endl;
		}
		ci -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());

		/* these go to the CacheStrapper! */
		CacheData data;
		data.cid = CacheId(ci->cacheType, ci->cacheSubId);
		data.hash = ci->file.hash;
		data.size = ci->file.filesize;
		data.name = ci->file.name;
		data.path = ci->file.path;
		data.pid = ci->PeerId();
		data.pname = mAuthMgr->getName(ci->PeerId());
		mCacheStrapper->recvCacheResponse(data, time(NULL));

		delete ci;
	}

	// now requested Searches.
	i_init = i;
	while((cr = pqisi -> RequestedSearch()) != NULL)
	{
		/* just delete these */
		std::ostringstream out;
		out << "Requested Search:" << std::endl;
		cr -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());
		delete cr;
	}


	// Now handle it replacement (pushed cache results) 
	{
		std::list<std::pair<RsPeerId, CacheData> > cacheUpdates;
		std::list<std::pair<RsPeerId, CacheData> >::iterator it;

		mCacheStrapper->getCacheUpdates(cacheUpdates);
		for(it = cacheUpdates.begin(); it != cacheUpdates.end(); it++)
		{
			/* construct reply */
			RsCacheItem *ci = new RsCacheItem();
	
			/* id from incoming */
			ci -> PeerId(it->first);

			ci -> file.hash = (it->second).hash;
			ci -> file.name = (it->second).name;
			ci -> file.path = ""; // (it->second).path;
			ci -> file.filesize = (it->second).size;
			ci -> cacheType  = (it->second).cid.type;
			ci -> cacheSubId =  (it->second).cid.subid;

#ifdef SERVER_DEBUG 
			std::ostringstream out2;
			out2 << "Outgoing CacheStrapper Update -> RsCacheItem:" << std::endl;
			ci -> print(out2);
			std::cerr << out2.str() << std::endl;
#endif

			//pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out2.str());
			pqisi -> SendSearchResult(ci);
		}
	}

	// now File Input.
	i_init = i;
	while((fr = pqisi -> GetFileRequest()) != NULL )
	{
#ifdef SERVER_DEBUG 
		std::cerr << "filedexserver::handleInputQueues() Recvd ftFiler Request" << std::endl;
		std::ostringstream out;
		if (i == i_init)
		{
			out << "Incoming(Net) File Item:" << std::endl;
		}
		fr -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());
#endif
		i++; /* count */

		/* This bit is for debugging only! (not really needed) */

		/* request */
		ftFileRequest *ffr = new ftFileRequest(fr->PeerId(), 
			fr->file.hash,  fr->file.filesize, 
			fr->fileoffset, fr->chunksize);
		ftFiler->recvFileInfo(ffr);

		delete fr;
	}

	// now File Data.
	i_init = i;
	while((fd = pqisi -> GetFileData()) != NULL )
	{
#ifdef SERVER_DEBUG 
		//std::cerr << "filedexserver::handleInputQueues() Recvd ftFiler Data" << std::endl;
		std::ostringstream out;
		if (i == i_init)
		{
			out << "Incoming(Net) File Data:" << std::endl;
		}
		fd -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());
#endif
		i++; /* count */

		/* incoming data */
		ftFileData *ffd = new ftFileData(fd->PeerId(), 
			fd->fd.file.hash, fd->fd.file.filesize, 
			fd->fd.file_offset, 
			fd->fd.binData.bin_len, 
			fd->fd.binData.bin_data, FT_FILEDATA_FLAG_NOFREE);

		ftFiler->recvFileInfo(ffd);
		delete fd;
	}

	if (i > 0)
	{
		return 1;
	}
	return 0;
}


// This function needs to be divided up.
int     filedexserver::handleOutputQueues()
{
	// get all the incoming results.. and print to the screen.
	//std::cerr << "filedexserver::handleOutputQueues()" << std::endl;
	int i = 0;

	/* now see if the filer has any data */
	ftFileRequest *ftr;
	while((ftr = ftFiler -> sendFileInfo()) != NULL)
	{
		//std::cerr << "filedexserver::handleOutputQueues() ftFiler Data for: " << ftr->id << std::endl;

		/* decide if its data or request */
		ftFileData *ftd = dynamic_cast<ftFileData *>(ftr);
		if (ftd)
		{
			SendFileData(ftd, ftr->id);
		}
		else
		{
			SendFileRequest(ftr, ftr->id);
		}

		std::ostringstream out;
		if (i++ == 0)
		{
			out << "Outgoing filer -> PQFileItem:" << std::endl;
		}
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());

		/* clean up */
		delete ftr;
	}



	if (i > 0)
	{
		return 1;
	}
	return 0;
}

void filedexserver::SendFileRequest(ftFileRequest *ftr, std::string pid)
{
	RsFileRequest *rfi = new RsFileRequest();

	/* id */
	rfi->PeerId(pid);

	/* file info */
	rfi->file.filesize   = ftr->size;
	rfi->file.hash       = ftr->hash;

	/* offsets */
	rfi->fileoffset = ftr->offset;
	rfi->chunksize  = ftr->chunk;

	pqisi -> SendFileRequest(rfi);
}

#define MAX_FT_CHUNK 4096

void filedexserver::SendFileData(ftFileData *ftd, std::string pid)
{
	uint32_t tosend = ftd->chunk;
	uint32_t baseoffset = ftd->offset;
	uint32_t offset = 0;
	uint32_t chunk;


	while(tosend > 0)
	{
		/* workout size */
		chunk = MAX_FT_CHUNK;
		if (chunk > tosend)
		{
			chunk = tosend;
		}

		/******** New Serialiser Type *******/

		RsFileData *rfd = new RsFileData();

		/* set id */
		rfd->PeerId(pid);

		/* file info */
		rfd->fd.file.filesize = ftd->size;
		rfd->fd.file.hash     = ftd->hash;
		rfd->fd.file.name     = ""; /* blank other data */
		rfd->fd.file.path     = "";
		rfd->fd.file.pop      = 0;
		rfd->fd.file.age      = 0;

		rfd->fd.file_offset = baseoffset + offset;

		/* file data */
		rfd->fd.binData.setBinData(
			&(((uint8_t *) ftd->data)[offset]), chunk);

		pqisi -> SendFileData(rfd);

		offset += chunk;
		tosend -= chunk;
	}
}


/***************************************************************************/
/****************************** CONFIGURATION HANDLING *********************/
/***************************************************************************/


/**** OVERLOADED FROM p3Config ****/



static const std::string fdex_dir("FDEX_DIR");
static const std::string save_dir_ss("SAVE_DIR");
static const std::string save_inc_ss("SAVE_INC");

RsSerialiser *filedexserver::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	/* add in the types we need! */
	rss->addSerialType(new RsFileTransferSerialiser());
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss;
}


std::list<RsItem *> filedexserver::saveList(bool &cleanup)
{
	std::list<RsItem *> saveData;

	/* it can delete them! */
	cleanup = true;

	/* create a key/value set for most of the parameters */
	std::map<std::string, std::string> configMap;
	std::map<std::string, std::string>::iterator mit;
	std::list<std::string>::iterator it;

	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"fildexserver::save_config()");

	/* basic control parameters */
	configMap[save_dir_ss] = getSaveDir();
	if (getSaveIncSearch())
	{
		configMap[save_inc_ss] = "true";
	}
	else
	{
		configMap[save_inc_ss] = "false";
	}

	int i;
	for(it = dbase_dirs.begin(), i = 0; (it != dbase_dirs.end()) 
		&& (i < 1000); it++, i++)
	{
		std::string name = fdex_dir;
		int d1, d2, d3;
		d1 = i / 100;
		d2 = (i - d1 * 100) / 10;
		d3 = i - d1 * 100 - d2 * 10;

		name += '0'+d1;
		name += '0'+d2;
		name += '0'+d3;

		configMap[name] = (*it);
	}

	RsConfigKeyValueSet *rskv = new RsConfigKeyValueSet();

	/* Convert to TLV */
	for(mit = configMap.begin(); mit != configMap.end(); mit++)
	{
		RsTlvKeyValue kv;
		kv.key = mit->first;
		kv.value = mit->second;

		rskv->tlvkvs.pairs.push_back(kv);
	}

	/* Add KeyValue to saveList */
	saveData.push_back(rskv);

	std::list<RsFileTransfer *>::iterator fit;
	std::list<RsFileTransfer *> ftlist = ftFiler -> getStatus();
	for(fit = ftlist.begin(); fit != ftlist.end(); fit++)
	{
		/* only write out the okay/uncompleted (with hash) files */
		if (((*fit)->state == FT_STATE_FAILED) || 
		    ((*fit)->state == FT_STATE_COMPLETE) || 
		    ((*fit)->in == false) ||
		    ((*fit)->file.hash == ""))
		{
			/* ignore */
			/* cleanup */
			delete(*fit);
		}
		else 
		{
			saveData.push_back(*fit);
		}
	}

	/* list completed! */
	return saveData;
}


bool filedexserver::loadList(std::list<RsItem *> load)
{
	std::list<RsItem *>::iterator it;
	std::list<RsTlvKeyValue>::iterator kit;
	RsConfigKeyValueSet *rskv;
	RsFileTransfer      *rsft;

#ifdef SERVER_DEBUG 
	std::cerr << "filedexserver::loadList() Item Count: " << load.size();
	std::cerr << std::endl;
#endif

	for(it = load.begin(); it != load.end(); it++)
	{
		/* switch on type */
		if (NULL != (rskv = dynamic_cast<RsConfigKeyValueSet *>(*it)))
		{
			/* make into map */
			std::map<std::string, std::string> configMap;
			for(kit = rskv->tlvkvs.pairs.begin();
				kit != rskv->tlvkvs.pairs.end(); kit++)
			{
				configMap[kit->key] = kit->value;
			}

			loadConfigMap(configMap);
			/* cleanup */
			delete (*it);

		}
		else if (NULL != (rsft = dynamic_cast<RsFileTransfer *>(*it)))
		{
			/* save to the preLoad list */
			mResumeTransferList.push_back(rsft);
		}
		else
		{
			/* cleanup */
			delete (*it);
		}
	}

	return true;

}

bool  filedexserver::loadConfigMap(std::map<std::string, std::string> &configMap)
{
	std::map<std::string, std::string>::iterator mit;

	int i;
	std::string str_true("true");
	std::string empty("");
	std::string dir = "notempty";

	if (configMap.end() != (mit = configMap.find(save_dir_ss)))
	{
		setSaveDir(mit->second);
	}

	if (configMap.end() != (mit = configMap.find(save_inc_ss)))
	{
		setSaveIncSearch(mit->second == str_true);
	}

	dbase_dirs.clear();

	for(i = 0; (i < 1000) && (dir != empty); i++)
	{
		std::string name = fdex_dir;
		int d1, d2, d3;
		d1 = i / 100;
		d2 = (i - d1 * 100) / 10;
		d3 = i - d1 * 100 - d2 * 10;

		name += '0'+d1;
		name += '0'+d2;
		name += '0'+d3;

		if (configMap.end() != (mit = configMap.find(name)))
		{
			dir = mit->second;
			dbase_dirs.push_back(mit->second);
		}
	}
	if (dbase_dirs.size() > 0)
	{
		std::ostringstream out;
		out << "Loading " << dbase_dirs.size();
		out << " Directories" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());

		reScanDirs();
	}

	return true;
}

