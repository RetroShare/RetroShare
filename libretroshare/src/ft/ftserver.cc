/*
 * libretroshare/src/ft: ftserver.cc
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

#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "rsiface/rstypes.h"
const int ftserverzone = 29539;

#include "ft/ftserver.h"
#include "ft/ftextralist.h"
#include "ft/ftfilesearch.h"
#include "ft/ftcontroller.h"
#include "ft/ftfileprovider.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftdwlqueue.h"
#include "turtle/p3turtle.h"


// Includes CacheStrapper / FiMonitor / FiStore for us.

#include "ft/ftdbase.h"

#include "pqi/pqi.h"
#include "pqi/p3connmgr.h"

#include "serialiser/rsserviceids.h"

#include <iostream>
#include <sstream>

/***
 * #define SERVER_DEBUG 1
 * #define DEBUG_TICK   1
 ***/

	/* Setup */
ftServer::ftServer(p3ConnectMgr *connMgr)
        :       mP3iface(NULL),
                mConnMgr(connMgr),
		mCacheStrapper(NULL),
		mFiStore(NULL), mFiMon(NULL),
		mFtController(NULL), mFtExtra(NULL),
		mFtDataplex(NULL), mFtSearch(NULL)
{
        mCacheStrapper = new ftCacheStrapper(connMgr);
}

void	ftServer::setConfigDirectory(std::string path)
{
	mConfigPath = path;

	/* Must update the sub classes ... if they exist
	 * TODO.
	 */

	std::string localcachedir = mConfigPath + "/cache/local";
	std::string remotecachedir = mConfigPath + "/cache/remote";

	RsDirUtil::checkCreateDirectory(localcachedir) ;
	RsDirUtil::checkCreateDirectory(remotecachedir) ;

	//mFiStore -> setCacheDir(remotecachedir);
        //mFiMon -> setCacheDir(localcachedir);

}

void	ftServer::setP3Interface(P3Interface *pqi)
{
	mP3iface = pqi;
}

	/* Control Interface */

	/* add Config Items (Extra, Controller) */
void	ftServer::addConfigComponents(p3ConfigMgr *mgr)
{
	/* NOT SURE ABOUT THIS ONE */
}

std::string ftServer::OwnId()
{
	std::string ownId;
	if (mConnMgr)
		ownId = mConnMgr->getOwnId();
	return ownId;
}

	/* Final Setup (once everything is assigned) */
void ftServer::SetupFtServer(NotifyBase *cb)
{

	/* setup FiStore/Monitor */
	std::string localcachedir = mConfigPath + "/cache/local";
	std::string remotecachedir = mConfigPath + "/cache/remote";
	std::string ownId = mConnMgr->getOwnId();

	/* search/extras List */
	mFtExtra = new ftExtraList();
	mFtSearch = new ftFileSearch();

	/* Transport */
	mFtDataplex = new ftDataMultiplex(ownId, this, mFtSearch);

	/* make Controller */
	mFtController = new ftController(mCacheStrapper, mFtDataplex, mConfigPath);
	mFtController -> setFtSearchNExtra(mFtSearch, mFtExtra);
	std::string tmppath = ".";
	mFtController->setPartialsDirectory(tmppath);
	mFtController->setDownloadDirectory(tmppath);


	/* Make Cache Source/Store */
	mFiStore = new ftFiStore(mCacheStrapper, mFtController, cb, ownId, remotecachedir);
	mFiMon = new ftFiMonitor(mCacheStrapper,cb, localcachedir, ownId);

	/* now add the set to the cachestrapper */
	CachePair cp(mFiMon, mFiStore, CacheId(RS_SERVICE_TYPE_FILE_INDEX, 0));
	mCacheStrapper -> addCachePair(cp);

	/* complete search setup */
	mFtSearch->addSearchMode(mCacheStrapper, RS_FILE_HINTS_CACHE);
	mFtSearch->addSearchMode(mFtExtra, RS_FILE_HINTS_EXTRA);
	mFtSearch->addSearchMode(mFiMon, RS_FILE_HINTS_LOCAL);
	mFtSearch->addSearchMode(mFiStore, RS_FILE_HINTS_REMOTE);

	mConnMgr->addMonitor(mFtController);
        mConnMgr->addMonitor(mCacheStrapper);

    mFtDwlQueue = new ftDwlQueue(mFtController);

	return;
}

void ftServer::connectToTurtleRouter(p3turtle *fts)
{
	mTurtleRouter = fts ;

//	mFtSearch->addSearchMode(fts, RS_FILE_HINTS_TURTLE);
	mFtController->setTurtleRouter(fts) ;
}

void    ftServer::StartupThreads()
{
	/* start up order - important for dependencies */

	/* self contained threads */
	/* startup ExtraList Thread */
	mFtExtra->start();

	/* startup Monitor Thread */
	/* startup the FileMonitor (after cache load) */
	mFiMon->setPeriod(600); /* 10 minutes */
	/* start it up */
	//mFiMon->setSharedDirectories(dbase_dirs);
	mFiMon->start();

	/* Controller thread */
	mFtController->start();

	/* Dataplex */
	mFtDataplex->start();

	/* Download Queue */
	mFtDwlQueue->start();

	/* start own thread */
	start();
}

CacheStrapper *ftServer::getCacheStrapper()
{
	return mCacheStrapper;
}

CacheTransfer *ftServer::getCacheTransfer()
{
	return mFtController;
}

void	ftServer::run()
{
	while(1)
	{
	        //scan the uploads list in ftdatamultiplex and delete the items which time out
         	time_t now = time(NULL);
                FileInfo info;
                std::list<std::string> toDels;
                std::map<std::string, ftFileProvider *>::iterator sit;
                for(sit = mFtDataplex->mServers.begin(); sit != mFtDataplex->mServers.end(); sit++)
                {
                  if (FileDetails(sit->first,RS_FILE_HINTS_UPLOAD,info))
                  {
                    if ((now - info.lastTS) > 10)
                    {
#ifdef SERVER_DEBUG
							  std::cout << "info.lastTS = " << info.lastTS << ", now=" << now << std::endl ;
#endif
                      toDels.push_back(sit->first);
                    }
                  }
                }

					 if(!toDels.empty())
						 mFtDataplex->deleteServers(toDels) ;

#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
}


	/***************************************************************/
	/********************** RsFiles Interface **********************/
	/***************************************************************/


	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool ftServer::FileRequest(std::string fname, std::string hash, uint64_t size, std::string dest, uint32_t flags, std::list<std::string> srcIds)
{
   std::cerr << "Requesting " << fname << std::endl ;

	if(mFtController->alreadyHaveFile(hash))
		return false ;

	const DwlDetails details(fname, hash, size, dest, flags, srcIds, PRIORITY_NORMAL);
	mFtDwlQueue->insertDownload(details);

	return true ;
}

bool ftServer::setChunkStrategy(const std::string& hash,FileChunksInfo::ChunkStrategy s)
{
	return mFtController->setChunkStrategy(hash,s);
}

bool ftServer::FileCancel(std::string hash)
{
	// Remove from both queue and ftController, by default.
	//
	mFtDwlQueue->clearDownload(hash);
	mFtController->FileCancel(hash);

	return true ;
}

bool ftServer::FileControl(std::string hash, uint32_t flags)
{
	return mFtController->FileControl(hash, flags);
}

bool ftServer::FileClearCompleted()
{
	return mFtController->FileClearCompleted();
}

	/* Control of Downloads Priority. */
bool ftServer::changeQueuePriority(const std::string hash, int priority)
{
	return mFtDwlQueue->changePriority(hash,(DwlPriority)priority) ;
}
bool ftServer::changeDownloadSpeed(const std::string hash, int speed)
{
	mFtController->setPriority(hash, (DwlSpeed)speed);
	return true ;
}
bool ftServer::getDownloadSpeed(const std::string hash, int & speed)
{
	DwlSpeed _speed;
	int ret = mFtController->getPriority(hash, _speed);
	if (ret) 
		speed = _speed;

	return ret;
}
bool ftServer::getQueuePriority(const std::string hash, int & priority)
{
	DwlPriority _priority;
	int ret = mFtDwlQueue->getPriority(hash, _priority);
	if (ret) 
		priority = _priority;

	return ret;
}
bool ftServer::clearDownload(const std::string hash)
{
   return true ;
}

void ftServer::clearQueue()
{
	mFtDwlQueue->clearQueue();
}

void ftServer::getDwlDetails(std::list<DwlDetails> & details)
{
	mFtDwlQueue->getDwlDetails(details);
}

bool ftServer::FileDownloadChunksDetails(const std::string& hash,FileChunksInfo& info)
{
	return mFtController->getFileDownloadChunksDetails(hash,info);
}

	/* Directory Handling */
void ftServer::setDownloadDirectory(std::string path)
{
	mFtController->setDownloadDirectory(path);
}

std::string ftServer::getDownloadDirectory()
{
	return mFtController->getDownloadDirectory();
}

void ftServer::setPartialsDirectory(std::string path)
{
	mFtController->setPartialsDirectory(path);
}

std::string ftServer::getPartialsDirectory()
{
	return mFtController->getPartialsDirectory();
}


	/***************************************************************/
	/************************* Other Access ************************/
	/***************************************************************/

bool ftServer::FileDownloads(std::list<std::string> &hashs)
{
	return mFtController->FileDownloads(hashs);
	/* this only contains downloads.... not completed */
	//return mFtDataplex->FileDownloads(hashs);
}

bool ftServer::FileUploadChunksDetails(const std::string& hash,const std::string& peer_id,CompressedChunkMap& cmap)
{
	return mFtDataplex->getClientChunkMap(hash,peer_id,cmap);
}

bool ftServer::FileUploads(std::list<std::string> &hashs)
{
	return mFtDataplex->FileUploads(hashs);
}

bool ftServer::FileDetails(std::string hash, uint32_t hintflags, FileInfo &info)
{
	if (hintflags & RS_FILE_HINTS_DOWNLOAD)
		if(mFtController->FileDetails(hash, info))
			return true ;

	if(hintflags & RS_FILE_HINTS_UPLOAD)
		if(mFtDataplex->FileDetails(hash, hintflags, info))
			return true ;

	if(hintflags & ~(RS_FILE_HINTS_UPLOAD | RS_FILE_HINTS_DOWNLOAD)) 
		if(mFtSearch->search(hash, hintflags, info))
			return true ;

	return false;
}

	/***************************************************************/
	/******************* ExtraFileList Access **********************/
	/***************************************************************/

bool  ftServer::ExtraFileAdd(std::string fname, std::string hash, uint64_t size,
						uint32_t period, uint32_t flags)
{
	return mFtExtra->addExtraFile(fname, hash, size, period, flags);
}

bool ftServer::ExtraFileRemove(std::string hash, uint32_t flags)
{
	return mFtExtra->removeExtraFile(hash, flags);
}

bool ftServer::ExtraFileHash(std::string localpath, uint32_t period, uint32_t flags)
{
	return mFtExtra->hashExtraFile(localpath, period, flags);
}

bool ftServer::ExtraFileStatus(std::string localpath, FileInfo &info)
{
	return mFtExtra->hashExtraFileDone(localpath, info);
}

bool ftServer::ExtraFileMove(std::string fname, std::string hash, uint64_t size,
                                std::string destpath)
{
	return mFtExtra->moveExtraFile(fname, hash, size, destpath);
}


	/***************************************************************/
	/******************** Directory Listing ************************/
	/***************************************************************/

int ftServer::RequestDirDetails(std::string uid, std::string path, DirDetails &details)
{
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::RequestDirDetails(uid:" << uid;
	std::cerr << ", path:" << path << ", ...) -> mFiStore";
	std::cerr << std::endl;

	if (!mFiStore)
	{
		std::cerr << "mFiStore not SET yet = FAIL";
		std::cerr << std::endl;
	}
#endif
	return mFiStore->RequestDirDetails(uid, path, details);
}

int ftServer::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags)
{
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::RequestDirDetails(ref:" << ref;
	std::cerr << ", flags:" << flags << ", ...) -> mFiStore";
	std::cerr << std::endl;

	if (!mFiStore)
	{
		std::cerr << "mFiStore not SET yet = FAIL";
		std::cerr << std::endl;
	}

#endif
	if(flags & DIR_FLAGS_LOCAL)
		return mFiMon->RequestDirDetails(ref, details, flags);
	else
		return mFiStore->RequestDirDetails(ref, details, flags);
}

	/***************************************************************/
	/******************** Search Interface *************************/
	/***************************************************************/


int ftServer::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,uint32_t flags)
{
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::SearchKeywords()";
	std::cerr << std::endl;

	if (!mFiStore)
	{
		std::cerr << "mFiStore not SET yet = FAIL";
		std::cerr << std::endl;
	}

#endif
	if(flags & DIR_FLAGS_LOCAL)
		return mFiMon->SearchKeywords(keywords, results,flags);
	else
		return mFiStore->SearchKeywords(keywords, results,flags);
}

int ftServer::SearchBoolExp(Expression * exp, std::list<DirDetails> &results,uint32_t flags)
{
	if(flags & DIR_FLAGS_LOCAL)
		return mFiMon->SearchBoolExp(exp,results,flags) ;
	else
		return mFiStore->searchBoolExp(exp, results);
}


	/***************************************************************/
	/*************** Local Shared Dir Interface ********************/
	/***************************************************************/

bool    ftServer::ConvertSharedFilePath(std::string path, std::string &fullpath)
{
	return mFiMon->convertSharedFilePath(path, fullpath);
}

void    ftServer::ForceDirectoryCheck()
{
	mFiMon->forceDirectoryCheck();
	return;
}

bool    ftServer::InDirectoryCheck()
{
	return mFiMon->inDirectoryCheck();
}

bool	ftServer::getSharedDirectories(std::list<SharedDirInfo> &dirs)
{
	mFiMon->getSharedDirectories(dirs);
	return true;
}

bool	ftServer::setSharedDirectories(std::list<SharedDirInfo> &dirs)
{
	mFiMon->setSharedDirectories(dirs);
	return true;
}

bool 	ftServer::addSharedDirectory(SharedDirInfo dir)
{
	std::list<SharedDirInfo> dirList;
	mFiMon->getSharedDirectories(dirList);

	// check that the directory is not already in the list.
	for(std::list<SharedDirInfo>::const_iterator it(dirList.begin());it!=dirList.end();++it)
		if((*it).filename == dir.filename)
			return false ;

	// ok then, add the shared directory.
	dirList.push_back(dir);

	mFiMon->setSharedDirectories(dirList);
	return true;
}

bool ftServer::updateShareFlags(const SharedDirInfo& info)
{
	mFiMon->updateShareFlags(info);

	return true ;
}

bool 	ftServer::removeSharedDirectory(std::string dir)
{
	std::list<SharedDirInfo> dirList;
	std::list<SharedDirInfo>::iterator it;

#ifdef SERVER_DEBUG
	std::cerr << "ftServer::removeSharedDirectory(" << dir << ")";
	std::cerr << std::endl;
#endif

	mFiMon->getSharedDirectories(dirList);

#ifdef SERVER_DEBUG
	for(it = dirList.begin(); it != dirList.end(); it++)
	{
		std::cerr << "ftServer::removeSharedDirectory()";
		std::cerr << " existing: " << (*it).filename;
		std::cerr << std::endl;
	}
#endif

	for(it = dirList.begin();it!=dirList.end() && (*it).filename != dir;++it) ;

	if(it == dirList.end())
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::removeSharedDirectory()";
		std::cerr << " Cannot Find Directory... Fail";
		std::cerr << std::endl;
#endif

		return false;
	}


#ifdef SERVER_DEBUG
	std::cerr << "ftServer::removeSharedDirectory()";
	std::cerr << " Updating Directories";
	std::cerr << std::endl;
#endif

	dirList.erase(it);
	mFiMon->setSharedDirectories(dirList);

	return true;
}

void ftServer::setShareDownloadDirectory(bool value)
{
	mFtController->setShareDownloadDirectory(value);
}

bool ftServer::getShareDownloadDirectory()
{
	return mFtController->getShareDownloadDirectory();
}

bool ftServer::shareDownloadDirectory()
{
	SharedDirInfo inf ;
	inf.filename = mFtController->getDownloadDirectory();
	inf.shareflags = RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_BROWSABLE ;

	return addSharedDirectory(inf);
}

bool ftServer::unshareDownloadDirectory()
{
	std::string dir = mFtController->getDownloadDirectory();
	return removeSharedDirectory(dir);
}

	/***************************************************************/
	/****************** End of RsFiles Interface *******************/
	/***************************************************************/


	/***************************************************************/
	/**************** Config Interface *****************************/
	/***************************************************************/

        /* Key Functions to be overloaded for Full Configuration */
RsSerialiser *ftServer::setupSerialiser()
{
	return NULL;
}

std::list<RsItem *> ftServer::saveList(bool &cleanup)
{
	std::list<RsItem *> list;
	return list;
}

bool    ftServer::loadList(std::list<RsItem *> load)
{
	return true;
}

bool  ftServer::loadConfigMap(std::map<std::string, std::string> &configMap)
{
	return true;
}


	/***************************************************************/
	/**********************     Data Flow     **********************/
	/***************************************************************/

	/* Client Send */
bool	ftServer::sendDataRequest(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
		mTurtleRouter->sendDataRequest(peerId,hash,size,offset,chunksize) ;
	else
	{
		/* create a packet */
		/* push to networking part */
		RsFileRequest *rfi = new RsFileRequest();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->file.filesize   = size;
		rfi->file.hash       = hash; /* ftr->hash; */

		/* offsets */
		rfi->fileoffset = offset; /* ftr->offset; */
		rfi->chunksize  = chunksize; /* ftr->chunk; */

		mP3iface->SendFileRequest(rfi);
	}

	return true;
}

bool ftServer::sendChunkMapRequest(const std::string& peerId,const std::string& hash)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
		mTurtleRouter->sendChunkMapRequest(peerId,hash) ;

	// We only send chunkmap requests to turtle peers. This will be a problem at display time for
	// direct friends, so I'll see later whether I code it or not.
	return true ;
}

bool ftServer::sendChunkMap(const std::string& peerId,const std::string& hash,const CompressedChunkMap& map)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
		mTurtleRouter->sendChunkMap(peerId,hash,map) ;

	// We only send chunkmap requests to turtle peers. This will be a problem at display time for
	// direct friends, so I'll see later whether I code it or not.
	return true ;
}


//const uint32_t	MAX_FT_CHUNK  = 32 * 1024; /* 32K */
//const uint32_t	MAX_FT_CHUNK  = 16 * 1024; /* 16K */
const uint32_t	MAX_FT_CHUNK  = 32 * 1024; /* 16K */

	/* Server Send */
bool	ftServer::sendData(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t baseoffset, uint32_t chunksize, void *data)
{
	/* create a packet */
	/* push to networking part */
	uint32_t tosend = chunksize;
	uint64_t offset = 0;
	uint32_t chunk;

#ifdef SERVER_DEBUG
	std::cerr << "ftServer::sendData() to " << peerId << std::endl;
	std::cerr << "hash: " << hash;
	std::cerr << " offset: " << baseoffset;
	std::cerr << " chunk: " << chunksize;
	std::cerr << " data: " << data;
	std::cerr << std::endl;
#endif

	while(tosend > 0)
	{
		/* workout size */
		chunk = MAX_FT_CHUNK;
		if (chunk > tosend)
		{
			chunk = tosend;
		}

		/******** New Serialiser Type *******/

		if(mTurtleRouter->isTurtlePeer(peerId))
			mTurtleRouter->sendFileData(peerId,hash,size,baseoffset+offset,chunk,&(((uint8_t *) data)[offset])) ;
		else
		{
			RsFileData *rfd = new RsFileData();

			/* set id */
			rfd->PeerId(peerId);

			/* file info */
			rfd->fd.file.filesize = size;
			rfd->fd.file.hash     = hash;
			rfd->fd.file.name     = ""; /* blank other data */
			rfd->fd.file.path     = "";
			rfd->fd.file.pop      = 0;
			rfd->fd.file.age      = 0;

			rfd->fd.file_offset = baseoffset + offset;

			/* file data */
			rfd->fd.binData.setBinData( &(((uint8_t *) data)[offset]), chunk);

			mP3iface->SendFileData(rfd);

			/* print the data pointer */
#ifdef SERVER_DEBUG
			std::cerr << "ftServer::sendData() Packet: " << std::endl;
			std::cerr << " offset: " << rfd->fd.file_offset;
			std::cerr << " chunk: " << chunk;
			std::cerr << " len: " << rfd->fd.binData.bin_len;
			std::cerr << " data: " << rfd->fd.binData.bin_data;
			std::cerr << std::endl;
#endif
		}

		offset += chunk;
		tosend -= chunk;
	}

	/* clean up data */
	free(data);

	return true;
}


/* NB: The rsCore lock must be activated before calling this.
 * This Lock should be moved lower into the system...
 * most likely destination is in ftServer.
 */
int	ftServer::tick()
{
	rslog(RSL_DEBUG_BASIC, ftserverzone,
		"filedexserver::tick()");

	if (mP3iface == NULL)
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::tick() ERROR: mP3iface == NULL";
#endif

		std::ostringstream out;
		rslog(RSL_DEBUG_BASIC, ftserverzone,
			"filedexserver::tick() Invalid Interface()");

		return 1;
	}

	int moreToTick = 0;

	if (0 < mP3iface -> tick())
	{
		moreToTick = 1;
#ifdef DEBUG_TICK
		std::cerr << "filedexserver::tick() moreToTick from mP3iface" << std::endl;
#endif
	}

	if (0 < handleInputQueues())
	{
		moreToTick = 1;
#ifdef DEBUG_TICK
		std::cerr << "filedexserver::tick() moreToTick from InputQueues" << std::endl;
#endif
	}
	return moreToTick;
}


// This function needs to be divided up.
bool    ftServer::handleInputQueues()
{
	bool moreToTick = false;

	if (handleCacheData())
		moreToTick = true;

	if (handleFileData())
		moreToTick = true;

	return moreToTick;
}

bool     ftServer::handleCacheData()
{
	// get all the incoming results.. and print to the screen.
	RsCacheRequest *cr;
	RsCacheItem    *ci;

	// Loop through Search Results.
	int i = 0;
	int i_init = 0;

#ifdef SERVER_DEBUG
	//std::cerr << "ftServer::handleCacheData()" << std::endl;
#endif
	while((ci = mP3iface -> GetSearchResult()) != NULL)
	{

#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleCacheData() Recvd SearchResult (CacheResponse!)" << std::endl;
		std::ostringstream out;
		if (i++ == i_init)
		{
			out << "Recieved Search Results:" << std::endl;
		}
		ci -> print(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out.str());
#endif

		/* these go to the CacheStrapper! */
		CacheData data;
		data.cid = CacheId(ci->cacheType, ci->cacheSubId);
		data.hash = ci->file.hash;
		data.size = ci->file.filesize;
		data.name = ci->file.name;
		data.path = ci->file.path;
		data.pid = ci->PeerId();
                peerConnectState pca;
                mConnMgr->getFriendNetStatus(ci->PeerId(), pca);
                data.pname = pca.name;
		mCacheStrapper->recvCacheResponse(data, time(NULL));

		delete ci;
	}

	// now requested Searches.
	i_init = i;
	while((cr = mP3iface -> RequestedSearch()) != NULL)
	{
#ifdef SERVER_DEBUG
		/* just delete these */
		std::ostringstream out;
		out << "Requested Search:" << std::endl;
		cr -> print(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out.str());
#endif
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

			//rslog(RSL_DEBUG_BASIC, ftserverzone, out2.str());
			mP3iface -> SendSearchResult(ci);

			i++;
		}
	}
	return (i > 0);
}


bool    ftServer::handleFileData()
{
	// now File Input.
	RsFileRequest *fr;
	RsFileData *fd;

	int i_init = 0;
	int i = 0;

	i_init = i;
	while((fr = mP3iface -> GetFileRequest()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ftFiler Request" << std::endl;
		std::ostringstream out;
		if (i == i_init)
		{
			out << "Incoming(Net) File Item:" << std::endl;
		}
		fr -> print(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out.str());
#endif

		i++; /* count */
		mFtDataplex->recvDataRequest(fr->PeerId(),
				fr->file.hash,  fr->file.filesize,
				fr->fileoffset, fr->chunksize);

FileInfo(ffr);
		delete fr;
	}

	// now File Data.
	i_init = i;
	while((fd = mP3iface -> GetFileData()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ftFiler Data" << std::endl;
		std::cerr << "hash: " << fd->fd.file.hash;
		std::cerr << " length: " << fd->fd.binData.bin_len;
		std::cerr << " data: " << fd->fd.binData.bin_data;
		std::cerr << std::endl;

		std::ostringstream out;
		if (i == i_init)
		{
			out << "Incoming(Net) File Data:" << std::endl;
		}
		fd -> print(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out.str());
#endif
		i++; /* count */

		/* incoming data */
		mFtDataplex->recvData(fd->PeerId(),
				 fd->fd.file.hash,  fd->fd.file.filesize,
				fd->fd.file_offset,
				fd->fd.binData.bin_len,
				fd->fd.binData.bin_data);

		/* we've stolen the data part -> so blank before delete
		 */
		fd->fd.binData.TlvShallowClear();
		delete fd;
	}

	if (i > 0)
	{
		return 1;
	}
	return 0;
}

/**********************************
 **********************************
 **********************************
 *********************************/

 /***************************** CONFIG ****************************/

bool    ftServer::addConfiguration(p3ConfigMgr *cfgmgr)
{
	/* add all the subbits to config mgr */
	cfgmgr->addConfiguration("ft_shared.cfg", mFiMon);
	cfgmgr->addConfiguration("ft_extra.cfg", mFtExtra);
	cfgmgr->addConfiguration("ft_transfers.cfg", mFtController);
	cfgmgr->addConfiguration("ft_dwlqueue.cfg", mFtDwlQueue);

	return true;
}

bool	ftServer::ResumeTransfers()
{
	mFtController->activate();

	return true;
}

