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

#include <unistd.h>		/* for (u)sleep() */
#include <iostream>
#include <time.h>
#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "retroshare/rstypes.h"
#include "retroshare/rspeers.h"
const int ftserverzone = 29539;

#include "ft/ftturtlefiletransferitem.h"
#include "ft/ftserver.h"
#include "ft/ftextralist.h"
#include "ft/ftfilesearch.h"
#include "ft/ftcontroller.h"
#include "ft/ftfileprovider.h"
#include "ft/ftdatamultiplex.h"
//#include "ft/ftdwlqueue.h"
#include "turtle/p3turtle.h"
#include "pqi/p3notify.h"
#include "rsserver/p3face.h"


// Includes CacheStrapper / FiMonitor / FiStore for us.

#include "ft/ftdbase.h"

#include "pqi/pqi.h"
#include "pqi/p3linkmgr.h"

#include "serialiser/rsserviceids.h"

/***
 * #define SERVER_DEBUG 1
 * #define DEBUG_TICK   1
 ***/

	/* Setup */
ftServer::ftServer(p3PeerMgr *pm, p3LinkMgr *lm)
        :       mP3iface(NULL), mPeerMgr(pm),
                mLinkMgr(lm),
		mCacheStrapper(NULL),
		mFiStore(NULL), mFiMon(NULL),
		mFtController(NULL), mFtExtra(NULL),
		mFtDataplex(NULL), mFtSearch(NULL), srvMutex("ftServer")
{
        mCacheStrapper = new ftCacheStrapper(lm);
}

void	ftServer::setConfigDirectory(std::string path)
{
	mConfigPath = path;

	/* Must update the sub classes ... if they exist
	 * TODO.
	 */

	std::string basecachedir = mConfigPath + "/cache";
	std::string localcachedir = mConfigPath + "/cache/local";
	std::string remotecachedir = mConfigPath + "/cache/remote";

	RsDirUtil::checkCreateDirectory(basecachedir) ;
	RsDirUtil::checkCreateDirectory(localcachedir) ;
	RsDirUtil::checkCreateDirectory(remotecachedir) ;
}

void	ftServer::setP3Interface(P3Interface *pqi)
{
	mP3iface = pqi;
}

	/* Control Interface */

	/* add Config Items (Extra, Controller) */
void	ftServer::addConfigComponents(p3ConfigMgr */*mgr*/)
{
	/* NOT SURE ABOUT THIS ONE */
}

std::string ftServer::OwnId()
{
	std::string ownId;
	if (mLinkMgr)
		ownId = mLinkMgr->getOwnId();
	return ownId;
}

	/* Final Setup (once everything is assigned) */
void ftServer::SetupFtServer()
{

	/* setup FiStore/Monitor */
	std::string localcachedir = mConfigPath + "/cache/local";
	std::string remotecachedir = mConfigPath + "/cache/remote";
	std::string ownId = mLinkMgr->getOwnId();

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
	mFiStore = new ftFiStore(mCacheStrapper, mFtController, mPeerMgr, ownId, remotecachedir);
	mFiMon = new ftFiMonitor(mCacheStrapper,localcachedir, ownId,mConfigPath);

	/* now add the set to the cachestrapper */
	CachePair cp(mFiMon, mFiStore, CacheId(RS_SERVICE_TYPE_FILE_INDEX, 0));
	mCacheStrapper -> addCachePair(cp);

	/* complete search setup */
	mFtSearch->addSearchMode(mCacheStrapper, RS_FILE_HINTS_CACHE);
	mFtSearch->addSearchMode(mFtExtra, RS_FILE_HINTS_EXTRA);
	mFtSearch->addSearchMode(mFiMon, RS_FILE_HINTS_LOCAL);
	mFtSearch->addSearchMode(mFiStore, RS_FILE_HINTS_REMOTE);

	mLinkMgr->addMonitor(mFtController);
	mLinkMgr->addMonitor(mCacheStrapper);

	return;
}

void ftServer::connectToTurtleRouter(p3turtle *fts)
{
	mTurtleRouter = fts ;

	mFtController->setTurtleRouter(fts) ;
	mFtController->setFtServer(this) ;

	mTurtleRouter->registerTunnelService(this) ;
}

void    ftServer::StartupThreads()
{
	/* start up order - important for dependencies */

	/* self contained threads */
	/* startup ExtraList Thread */
	mFtExtra->start();

	/* startup Monitor Thread */
	/* startup the FileMonitor (after cache load) */
	/* start it up */
	
	mFiMon->start();

	/* Controller thread */
	mFtController->start();

	/* Dataplex */
	mFtDataplex->start();

	/* start own thread */
	start();
}

void ftServer::StopThreads()
{
	/* stop own thread */
	join();

	/* stop Dataplex */
	mFtDataplex->join();

	/* stop Controller thread */
	mFtController->join();

	/* stop Monitor Thread */
	mFiMon->join();

	/* self contained threads */
	/* stop ExtraList Thread */
	mFtExtra->join();

	delete (mFtDataplex);
	mFtDataplex = NULL;

	delete (mFtController);
	mFtController = NULL;

	delete (mFiMon);
	mFiMon = NULL;

	delete (mFtExtra);
	mFtExtra = NULL;
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
	while(isRunning())
	{
		mFtDataplex->deleteUnusedServers() ;
		mFtDataplex->handlePendingCrcRequests() ;
		mFtDataplex->dispatchReceivedChunkCheckSum() ;
#ifdef WIN32
		Sleep(5000);
#else
		sleep(5);
#endif
	}
}


	/***************************************************************/
	/********************** RsFiles Interface **********************/
	/***************************************************************/


	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool ftServer::checkHash(const std::string& hash,std::string& error_string)
{
	static const uint32_t HASH_LENGTH = 40 ;

	if(hash.length() != HASH_LENGTH)
	{
		rs_sprintf(error_string, "Line too long : %u chars, %ld expected.", hash.length(), HASH_LENGTH) ;
		return false ;
	}

	for(uint32_t i=0;i<hash.length();++i)
		if(!((hash[i] > 47 && hash[i] < 58) || (hash[i] > 96 && hash[i] < 103)))
		{
			rs_sprintf(error_string, "unexpected char code=%d '%c'", (int)hash[i], hash[i]) ;
			return false ;
		}

	return true ;
}

bool ftServer::alreadyHaveFile(const std::string& hash, FileInfo &info)
{
	return mFtController->alreadyHaveFile(hash, info);
}

bool ftServer::FileRequest(const std::string& fname, const std::string& hash, uint64_t size, const std::string& dest, TransferRequestFlags flags, const std::list<std::string>& srcIds)
{
	std::string error_string ;

	if(!checkHash(hash,error_string))
	{
		RsServer::notify()->notifyErrorMsg(0,0,"Error handling hash \""+hash+"\". This hash appears to be invalid(Error string=\""+error_string+"\"). This is probably due an bad handling of strings.") ;
		return false ;
	}

   std::cerr << "Requesting " << fname << std::endl ;

	if(!mFtController->FileRequest(fname, hash, size, dest, flags, srcIds))
		return false ;

	return true ;
}

bool ftServer::setDestinationName(const std::string& hash,const std::string& name)
{
	return mFtController->setDestinationName(hash,name);
}
bool ftServer::setDestinationDirectory(const std::string& hash,const std::string& directory)
{
	return mFtController->setDestinationDirectory(hash,directory);
}
bool ftServer::setChunkStrategy(const std::string& hash,FileChunksInfo::ChunkStrategy s)
{
	return mFtController->setChunkStrategy(hash,s);
}
uint32_t ftServer::freeDiskSpaceLimit()const
{
	return mFtController->freeDiskSpaceLimit() ;
}
void ftServer::setFreeDiskSpaceLimit(uint32_t s)
{
	mFtController->setFreeDiskSpaceLimit(s) ;
}
void ftServer::setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy s) 
{
	mFtController->setDefaultChunkStrategy(s) ;
}
FileChunksInfo::ChunkStrategy ftServer::defaultChunkStrategy() 
{
	return mFtController->defaultChunkStrategy() ;
}
bool ftServer::FileCancel(const std::string& hash)
{
	// Remove from both queue and ftController, by default.
	//
	mFtController->FileCancel(hash);

	return true ;
}

bool ftServer::FileControl(const std::string& hash, uint32_t flags)
{
	return mFtController->FileControl(hash, flags);
}

bool ftServer::FileClearCompleted()
{
	return mFtController->FileClearCompleted();
}
void ftServer::setMinPrioritizedTransfers(uint32_t s)
{
	mFtController->setMinPrioritizedTransfers(s) ;
}
uint32_t ftServer::getMinPrioritizedTransfers()
{
	return mFtController->getMinPrioritizedTransfers() ;
}
void ftServer::setQueueSize(uint32_t s)
{
	mFtController->setQueueSize(s) ;
}
uint32_t ftServer::getQueueSize()
{
	return mFtController->getQueueSize() ;
}
	/* Control of Downloads Priority. */
bool ftServer::changeQueuePosition(const std::string hash, QueueMove mv)
{
	mFtController->moveInQueue(hash,mv) ;
	return true ;
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
bool ftServer::clearDownload(const std::string /*hash*/)
{
   return true ;
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

bool ftServer::FileDetails(const std::string &hash, FileSearchFlags hintflags, FileInfo &info)
{
	if (hintflags & RS_FILE_HINTS_DOWNLOAD)
		if(mFtController->FileDetails(hash, info))
			return true ;

	if(hintflags & RS_FILE_HINTS_UPLOAD)
		if(mFtDataplex->FileDetails(hash, hintflags, info))
		{
			// We also check if the file is a DL as well. In such a case we use
			// the DL as the file name, to replace the hash. If the file is a cache
			// file, we skip the call to fileDetails() for efficiency reasons.
			//
			FileInfo info2 ;
			if( (!(info.transfer_info_flags & RS_FILE_REQ_CACHE)) && mFtController->FileDetails(hash, info2))
				info.fname = info2.fname ;

			return true ;
		}

	if(hintflags & ~(RS_FILE_HINTS_UPLOAD | RS_FILE_HINTS_DOWNLOAD)) 
		if(mFtSearch->search(hash, hintflags, info))
			return true ;

	return false;
}

RsTurtleGenericTunnelItem *ftServer::deserialiseItem(void *data,uint32_t size) const
{
	uint32_t rstype = getRsItemId(data);

#ifdef SERVER_DEBUG
	std::cerr << "p3turtle: deserialising packet: " << std::endl ;
#endif
#ifdef SERVER_DEBUG
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_TURTLE != getRsItemService(rstype))) 
	{
#ifdef SERVER_DEBUG
		std::cerr << "  Wrong type !!" << std::endl ;
#endif
		return NULL; /* wrong type */
	}
#endif

	switch(getRsItemSubType(rstype))
	{
		case RS_TURTLE_SUBTYPE_FILE_REQUEST 			:	return new RsTurtleFileRequestItem(data,size) ;
		case RS_TURTLE_SUBTYPE_FILE_DATA    			:	return new RsTurtleFileDataItem(data,size) ;
		case RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST		:	return new RsTurtleFileMapRequestItem(data,size) ;
		case RS_TURTLE_SUBTYPE_FILE_MAP     			:	return new RsTurtleFileMapItem(data,size) ;
		case RS_TURTLE_SUBTYPE_FILE_CRC_REQUEST		:	return new RsTurtleFileCrcRequestItem(data,size) ;
		case RS_TURTLE_SUBTYPE_FILE_CRC     			:	return new RsTurtleFileCrcItem(data,size) ;
		case RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST		:	return new RsTurtleChunkCrcRequestItem(data,size) ;
		case RS_TURTLE_SUBTYPE_CHUNK_CRC     			:	return new RsTurtleChunkCrcItem(data,size) ;

		default:
																		return NULL ;
	}
}

void ftServer::addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir) 
{
	if(dir == RsTurtleGenericTunnelItem::DIRECTION_SERVER)
		mFtController->addFileSource(hash,virtual_peer_id) ;
}
void ftServer::removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id) 
{
	mFtController->removeFileSource(hash,virtual_peer_id) ;
}

bool ftServer::handleTunnelRequest(const std::string& hash,const std::string& peer_id)
{
	FileInfo info ;
	bool res = FileDetails(hash, RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_SPEC_ONLY | RS_FILE_HINTS_DOWNLOAD, info);

#ifdef SERVER_DEBUG
	std::cerr << "ftServer: performing local hash search for hash " << hash << std::endl;

	if(res)
	{
		std::cerr << "Found hash: " << std::endl;
		std::cerr << "   hash  = " << hash << std::endl;
		std::cerr << "   peer  = " << peer_id << std::endl;
		std::cerr << "   flags = " << info.storage_permission_flags << std::endl;
		std::cerr << "   local = " << rsFiles->FileDetails(hash, RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_SPEC_ONLY | RS_FILE_HINTS_DOWNLOAD, info) << std::endl;
		std::cerr << "   groups= " ; for(std::list<std::string>::const_iterator it(info.parent_groups.begin());it!=info.parent_groups.end();++it) std::cerr << (*it) << ", " ; std::cerr << std::endl;
		std::cerr << "   clear = " << rsPeers->computePeerPermissionFlags(peer_id,info.storage_permission_flags,info.parent_groups) << std::endl;
	}
#endif

	// The call to computeHashPeerClearance() return a combination of RS_FILE_HINTS_NETWORK_WIDE and RS_FILE_HINTS_BROWSABLE
	// This is an additional computation cost, but the way it's written here, it's only called when res is true.
	//
	res = res && (RS_FILE_HINTS_NETWORK_WIDE & rsPeers->computePeerPermissionFlags(peer_id,info.storage_permission_flags,info.parent_groups)) ;

	return res ;
}

	/***************************************************************/
	/******************* ExtraFileList Access **********************/
	/***************************************************************/

bool  ftServer::ExtraFileAdd(std::string fname, std::string hash, uint64_t size,
						uint32_t period, TransferRequestFlags flags)
{
	return mFtExtra->addExtraFile(fname, hash, size, period, flags);
}

bool ftServer::ExtraFileRemove(std::string hash, TransferRequestFlags flags)
{
	return mFtExtra->removeExtraFile(hash, flags);
}

bool ftServer::ExtraFileHash(std::string localpath, uint32_t period, TransferRequestFlags flags)
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

int ftServer::RequestDirDetails(const std::string& uid, const std::string& path, DirDetails &details)
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
	if(uid == mLinkMgr->getOwnId())
		return mFiMon->RequestDirDetails(path, details);
	else
		return mFiStore->RequestDirDetails(uid, path, details);
}

int ftServer::RequestDirDetails(void *ref, DirDetails &details, FileSearchFlags flags)
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
	if(flags & RS_FILE_HINTS_LOCAL)
		return mFiMon->RequestDirDetails(ref, details, flags);
	else
		return mFiStore->RequestDirDetails(ref, details, flags);
}
uint32_t ftServer::getType(void *ref, FileSearchFlags flags)
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
	if(flags & RS_FILE_HINTS_LOCAL)
		return mFiMon->getType(ref);
	else
		return mFiStore->getType(ref);
}
	/***************************************************************/
	/******************** Search Interface *************************/
	/***************************************************************/


int ftServer::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags)
{
	if(flags & RS_FILE_HINTS_LOCAL)
		return mFiMon->SearchKeywords(keywords, results,flags,"");
	else
		return mFiStore->SearchKeywords(keywords, results,flags);
	return 0 ;
}
int ftServer::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const std::string& peer_id)
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
	if(flags & RS_FILE_HINTS_LOCAL)
		return mFiMon->SearchKeywords(keywords, results,flags,peer_id);
	else
		return mFiStore->SearchKeywords(keywords, results,flags);
}

int ftServer::SearchBoolExp(Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags)
{
	if(flags & RS_FILE_HINTS_LOCAL)
		return mFiMon->SearchBoolExp(exp,results,flags,"") ;
	else
		return mFiStore->searchBoolExp(exp, results);
	return 0 ;
}
int ftServer::SearchBoolExp(Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags,const std::string& peer_id)
{
	if(flags & RS_FILE_HINTS_LOCAL)
		return mFiMon->SearchBoolExp(exp,results,flags,peer_id) ;
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

void    ftServer::updateSinceGroupPermissionsChanged()
{
	mFiMon->forceDirListsRebuildAndSend();
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

bool ftServer::copyFile(const std::string& source, const std::string& dest)
{
	return mFtController->copyFile(source, dest);
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

bool 	ftServer::addSharedDirectory(const SharedDirInfo& dir)
{
	SharedDirInfo _dir = dir;
	_dir.filename = RsDirUtil::convertPathToUnix(_dir.filename);

	std::list<SharedDirInfo> dirList;
	mFiMon->getSharedDirectories(dirList);

	// check that the directory is not already in the list.
	for(std::list<SharedDirInfo>::const_iterator it(dirList.begin());it!=dirList.end();++it)
		if((*it).filename == _dir.filename)
			return false ;

	// ok then, add the shared directory.
	dirList.push_back(_dir);

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
	dir = RsDirUtil::convertPathToUnix(dir);

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
void	ftServer::setWatchPeriod(int minutes) 
{
	mFiMon->setWatchPeriod(minutes*60) ;
}
int ftServer::watchPeriod() const
{
	return mFiMon->watchPeriod()/60 ;
}

void	ftServer::setRememberHashFiles(bool b) 
{
	mFiMon->setRememberHashCache(b) ;
}
bool ftServer::rememberHashFiles() const
{
	return mFiMon->rememberHashCache() ;
}
void	ftServer::setRememberHashFilesDuration(uint32_t days) 
{
	mFiMon->setRememberHashCacheDuration(days) ;
}
uint32_t ftServer::rememberHashFilesDuration() const 
{
	return mFiMon->rememberHashCacheDuration() ;
}
void   ftServer::clearHashCache() 
{
	mFiMon->clearHashCache() ;
}

bool ftServer::getShareDownloadDirectory()
{
	std::list<SharedDirInfo> dirList;
	mFiMon->getSharedDirectories(dirList);

	std::string dir = mFtController->getDownloadDirectory();

	// check if the download directory is in the list.
	for (std::list<SharedDirInfo>::const_iterator it(dirList.begin()); it != dirList.end(); ++it)
		if ((*it).filename == dir)
			return true;

	return false;
}

bool ftServer::shareDownloadDirectory(bool share)
{
	if (share) {
		/* Share */
		SharedDirInfo inf ;
		inf.filename = mFtController->getDownloadDirectory();
		inf.shareflags = DIR_FLAGS_NETWORK_WIDE_OTHERS ;

		return addSharedDirectory(inf);
	}

	/* Unshare */
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

bool ftServer::saveList(bool &/*cleanup*/, std::list<RsItem *>& /*list*/)
{

	return true;
}

bool    ftServer::loadList(std::list<RsItem *>& /*load*/)
{
	return true;
}

bool  ftServer::loadConfigMap(std::map<std::string, std::string> &/*configMap*/)
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
	{
		RsTurtleFileRequestItem *item = new RsTurtleFileRequestItem ;

		item->chunk_offset = offset ;
		item->chunk_size = chunksize ;

		mTurtleRouter->sendTurtleData(peerId,item) ;
	}
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

bool ftServer::sendChunkMapRequest(const std::string& peerId,const std::string& hash,bool is_client)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleFileMapRequestItem *item = new RsTurtleFileMapRequestItem ;
		mTurtleRouter->sendTurtleData(peerId,item) ;
	}
	else
	{
		/* create a packet */
		/* push to networking part */
		RsFileChunkMapRequest *rfi = new RsFileChunkMapRequest();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->is_client = is_client ;

		mP3iface->SendFileChunkMapRequest(rfi);
	}

	return true ;
}

bool ftServer::sendChunkMap(const std::string& peerId,const std::string& hash,const CompressedChunkMap& map,bool is_client)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleFileMapItem *item = new RsTurtleFileMapItem ;
		item->compressed_map = map ;
		mTurtleRouter->sendTurtleData(peerId,item) ;
	}
	else
	{
		/* create a packet */
		/* push to networking part */
		RsFileChunkMap *rfi = new RsFileChunkMap();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->is_client = is_client; /* ftr->hash; */
		rfi->compressed_map = map; /* ftr->hash; */

		mP3iface->SendFileChunkMap(rfi);
	}

	return true ;
}
bool ftServer::sendCRC32MapRequest(const std::string& peerId,const std::string& hash)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleFileCrcRequestItem *item = new RsTurtleFileCrcRequestItem;

		mTurtleRouter->sendTurtleData(peerId,item) ;
	}
	else
	{
		/* create a packet */
		/* push to networking part */
		RsFileCRC32MapRequest *rfi = new RsFileCRC32MapRequest();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */

		mP3iface->SendFileCRC32MapRequest(rfi);
	}

	return true ;
}
bool ftServer::sendSingleChunkCRCRequest(const std::string& peerId,const std::string& hash,uint32_t chunk_number)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleChunkCrcRequestItem *item = new RsTurtleChunkCrcRequestItem;
		item->chunk_number = chunk_number ;

		mTurtleRouter->sendTurtleData(peerId,item) ;
	}
	else
	{
		/* create a packet */
		/* push to networking part */
		RsFileSingleChunkCrcRequest *rfi = new RsFileSingleChunkCrcRequest();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->chunk_number = chunk_number ;

		mP3iface->SendFileSingleChunkCrcRequest(rfi);
	}

	return true ;
}

bool ftServer::sendCRC32Map(const std::string& peerId,const std::string& hash,const CRC32Map& crcmap)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleFileCrcItem *item = new RsTurtleFileCrcItem ;
		item->crc_map = crcmap ;

		mTurtleRouter->sendTurtleData(peerId,item) ;
	}
	else
	{
		/* create a packet */
		/* push to networking part */
		RsFileCRC32Map *rfi = new RsFileCRC32Map();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->crc_map = crcmap; /* ftr->hash; */

		mP3iface->SendFileCRC32Map(rfi);
	}

	return true ;
}
bool ftServer::sendSingleChunkCRC(const std::string& peerId,const std::string& hash,uint32_t chunk_number,const Sha1CheckSum& crc)
{
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleChunkCrcItem *item = new RsTurtleChunkCrcItem;
		item->chunk_number = chunk_number ;
		item->check_sum = crc ;

		mTurtleRouter->sendTurtleData(peerId,item) ;
	}
	else
	{
		/* create a packet */
		/* push to networking part */
		RsFileSingleChunkCrc *rfi = new RsFileSingleChunkCrc();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->check_sum = crc; 
		rfi->chunk_number = chunk_number; 

		mP3iface->SendFileSingleChunkCrc(rfi);
	}

	return true ;
}

//const uint32_t	MAX_FT_CHUNK  = 32 * 1024; /* 32K */
//const uint32_t	MAX_FT_CHUNK  = 16 * 1024; /* 16K */
const uint32_t	MAX_FT_CHUNK  = 8 * 1024; /* 16K */

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
		{
			RsTurtleFileDataItem *item = new RsTurtleFileDataItem ;

			item->chunk_offset = offset+baseoffset ;
			item->chunk_size = chunk;
			item->chunk_data = malloc(chunk) ;

			if(item->chunk_data == NULL)
			{
				std::cerr << "p3turtle: Warning: failed malloc of " << chunk << " bytes for sending data packet." << std::endl ;
				delete item;
				return false;
			}
			memcpy(item->chunk_data,&(((uint8_t *) data)[offset]),chunk) ;

			mTurtleRouter->sendTurtleData(peerId,item) ;
		}
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

void ftServer::receiveTurtleData(RsTurtleGenericTunnelItem *i,
											const std::string& hash,
											const std::string& virtual_peer_id,
											RsTurtleGenericTunnelItem::Direction direction) 
{
	switch(i->PacketSubType())
	{
		case RS_TURTLE_SUBTYPE_FILE_REQUEST: 		
			{
				RsTurtleFileRequestItem *item = dynamic_cast<RsTurtleFileRequestItem *>(i) ;
				getMultiplexer()->recvDataRequest(virtual_peer_id,hash,0,item->chunk_offset,item->chunk_size) ;
			}
			break ;

		case RS_TURTLE_SUBTYPE_FILE_DATA : 	
			{
				RsTurtleFileDataItem *item = dynamic_cast<RsTurtleFileDataItem *>(i) ;
				getMultiplexer()->recvData(virtual_peer_id,hash,0,item->chunk_offset,item->chunk_size,item->chunk_data) ;

				item->chunk_data = NULL ;	// this prevents deletion in the destructor of RsFileDataItem, because data will be deleted
				// down _ft_server->getMultiplexer()->recvData()...in ftTransferModule::recvFileData

			}
			break ;

		case RS_TURTLE_SUBTYPE_FILE_MAP : 	
			{
				RsTurtleFileMapItem *item = dynamic_cast<RsTurtleFileMapItem *>(i) ;
				getMultiplexer()->recvChunkMap(virtual_peer_id,hash,item->compressed_map,direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
			}
			break ;

		case RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST:	
			{
				RsTurtleFileMapRequestItem *item = dynamic_cast<RsTurtleFileMapRequestItem *>(i) ;
				getMultiplexer()->recvChunkMapRequest(virtual_peer_id,hash,direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
			}
			break ;

		case RS_TURTLE_SUBTYPE_FILE_CRC : 			
			{
				RsTurtleFileCrcItem *item = dynamic_cast<RsTurtleFileCrcItem *>(i) ;
				getMultiplexer()->recvCRC32Map(virtual_peer_id,hash,item->crc_map) ;
			}
			break ;

		case RS_TURTLE_SUBTYPE_FILE_CRC_REQUEST:
			{
				getMultiplexer()->recvCRC32MapRequest(virtual_peer_id,hash) ;
			}
			break ;

		case RS_TURTLE_SUBTYPE_CHUNK_CRC : 			
			{
				RsTurtleChunkCrcItem *item = dynamic_cast<RsTurtleChunkCrcItem *>(i) ;
				getMultiplexer()->recvSingleChunkCRC(virtual_peer_id,hash,item->chunk_number,item->check_sum) ;
			}
			break ;

		case RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST:	
			{
				RsTurtleChunkCrcRequestItem *item = dynamic_cast<RsTurtleChunkCrcRequestItem *>(i) ;
				getMultiplexer()->recvSingleChunkCRCRequest(virtual_peer_id,hash,item->chunk_number) ;
			}
			break ;
		default:
			std::cerr << "WARNING: Unknown packet type received: sub_id=" << reinterpret_cast<void*>(i->PacketSubType()) << ". Is somebody trying to poison you ?" << std::endl ;
	}
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
		std::string out;
		if (i++ == i_init)
		{
			out += "Recieved Search Results:\n";
		}
		ci -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
#endif

		/* these go to the CacheStrapper! */
		RsCacheData data;
		data.pid = ci->PeerId();
		data.cid = CacheId(ci->cacheType, ci->cacheSubId);
		data.path = ci->file.path;
		data.name = ci->file.name;
		data.hash = ci->file.hash;
		data.size = ci->file.filesize;
		data.recvd = time(NULL) ;

		mCacheStrapper->recvCacheResponse(data, time(NULL));

		delete ci;
	}

	// now requested Searches.
	i_init = i;
	while((cr = mP3iface -> RequestedSearch()) != NULL)
	{
#ifdef SERVER_DEBUG
		/* just delete these */
		std::string out = "Requested Search:\n";
		cr -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
#endif
		delete cr;
	}


	// Now handle it replacement (pushed cache results)
	{
		std::list<std::pair<RsPeerId, RsCacheData> > cacheUpdates;
		std::list<std::pair<RsPeerId, RsCacheData> >::iterator it;

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
			std::string out2 = "Outgoing CacheStrapper Update -> RsCacheItem:\n";
			ci -> print_string(out2);
			std::cerr << out2 << std::endl;
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
	RsFileChunkMapRequest *fcmr;
	RsFileChunkMap *fcm;
	RsFileCRC32MapRequest *fccrcmr;
	RsFileCRC32Map *fccrcm;
	RsFileSingleChunkCrcRequest *fscrcr;
	RsFileSingleChunkCrc *fscrc;

	int i_init = 0;
	int i = 0;

	i_init = i;
	while((fr = mP3iface -> GetFileRequest()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ftFiler Request" << std::endl;
		std::string out;
		if (i == i_init)
		{
			out += "Incoming(Net) File Item:\n";
		}
		fr -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
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

		std::string out;
		if (i == i_init)
		{
			out += "Incoming(Net) File Data:\n";
		}
		fd -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
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
	// now file chunkmap requests
	i_init = i;
	while((fcmr = mP3iface -> GetFileChunkMapRequest()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ChunkMap request" << std::endl;
		std::string out;
		if (i == i_init)
		{
			out += "Incoming(Net) File Data:\n";
		}
		fcmr -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
#endif
		i++; /* count */

		/* incoming data */
		mFtDataplex->recvChunkMapRequest(fcmr->PeerId(), fcmr->hash,fcmr->is_client) ;

		delete fcmr;
	}
	// now file chunkmaps 
	i_init = i;
	while((fcm = mP3iface -> GetFileChunkMap()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ChunkMap request" << std::endl;
		std::string out;
		if (i == i_init)
		{
			out += "Incoming(Net) File Data:\n";
		}
		fcm -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
#endif
		i++; /* count */

		/* incoming data */
		mFtDataplex->recvChunkMap(fcm->PeerId(), fcm->hash,fcm->compressed_map,fcm->is_client) ;

		delete fcm;
	}
	// now file chunkmap requests
	i_init = i;
	while((fccrcmr = mP3iface -> GetFileCRC32MapRequest()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ChunkMap request" << std::endl;
		std::string out;
		if (i == i_init)
		{
			out += "Incoming(Net) File Data:\n";
		}
		fccrcmr -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
#endif
		i++; /* count */

		/* incoming data */
		mFtDataplex->recvCRC32MapRequest(fccrcmr->PeerId(), fccrcmr->hash) ;

		delete fccrcmr;
	}
	// now file chunkmaps 
	i_init = i;
	while((fccrcm = mP3iface -> GetFileCRC32Map()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ChunkMap request" << std::endl;
		std::string out;
		if (i == i_init)
		{
			out += "Incoming(Net) File Data:\n";
		}
		fccrcm -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
#endif
		i++; /* count */

		/* incoming data */
		mFtDataplex->recvCRC32Map(fccrcm->PeerId(), fccrcm->hash,fccrcm->crc_map); 

		delete fccrcm;
	}
	// now file chunk crc requests
	i_init = i;
	while((fscrcr = mP3iface -> GetFileSingleChunkCrcRequest()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ChunkMap request" << std::endl;
		std::string out;
		if (i == i_init)
		{
			out += "Incoming(Net) File CRC Request:\n";
		}
		fscrcr -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
#endif
		i++; /* count */

		/* incoming data */
		mFtDataplex->recvSingleChunkCRCRequest(fscrcr->PeerId(), fscrcr->hash,fscrcr->chunk_number) ;

		delete fscrcr;
	}
	// now file chunkmaps 
	i_init = i;
	while((fscrc = mP3iface -> GetFileSingleChunkCrc()) != NULL )
	{
#ifdef SERVER_DEBUG
		std::cerr << "ftServer::handleFileData() Recvd ChunkMap request" << std::endl;
		std::string out;
		if (i == i_init)
		{
			out += "Incoming(Net) File Data:\n";
		}
		fscrc -> print_string(out);
		rslog(RSL_DEBUG_BASIC, ftserverzone, out);
#endif
		i++; /* count */

		/* incoming data */
		mFtDataplex->recvSingleChunkCRC(fscrc->PeerId(), fscrc->hash,fscrc->chunk_number,fscrc->check_sum); 

		delete fscrcr;
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

	return true;
}

bool	ftServer::ResumeTransfers()
{
	mFtController->activate();

	return true;
}

