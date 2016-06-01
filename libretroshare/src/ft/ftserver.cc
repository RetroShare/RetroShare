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

#include "serialiser/rsfiletransferitems.h"
#include "serialiser/rsserviceids.h"

/***
 * #define SERVER_DEBUG       1
 * #define SERVER_DEBUG_CACHE 1
 ***/

static const time_t FILE_TRANSFER_LOW_PRIORITY_TASKS_PERIOD = 5 ; // low priority tasks handling every 5 seconds

	/* Setup */
ftServer::ftServer(p3PeerMgr *pm, p3ServiceControl *sc)
        :       p3Service(),
	 	mPeerMgr(pm),
                mServiceCtrl(sc),
		mCacheStrapper(NULL),
		mFiStore(NULL), mFiMon(NULL),
		mFtController(NULL), mFtExtra(NULL),
		mFtDataplex(NULL), mFtSearch(NULL), srvMutex("ftServer")
{
	mCacheStrapper = new ftCacheStrapper(sc, getServiceInfo().mServiceType);

	addSerialType(new RsFileTransferSerialiser()) ;
}

const std::string FILE_TRANSFER_APP_NAME = "ft";
const uint16_t FILE_TRANSFER_APP_MAJOR_VERSION	= 	1;
const uint16_t FILE_TRANSFER_APP_MINOR_VERSION  = 	0;
const uint16_t FILE_TRANSFER_MIN_MAJOR_VERSION  = 	1;
const uint16_t FILE_TRANSFER_MIN_MINOR_VERSION	=	0;

RsServiceInfo ftServer::getServiceInfo()
{
	return RsServiceInfo(RS_SERVICE_TYPE_FILE_TRANSFER, 
		FILE_TRANSFER_APP_NAME,
		FILE_TRANSFER_APP_MAJOR_VERSION, 
		FILE_TRANSFER_APP_MINOR_VERSION, 
		FILE_TRANSFER_MIN_MAJOR_VERSION, 
		FILE_TRANSFER_MIN_MINOR_VERSION);
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

	/* Control Interface */

	/* add Config Items (Extra, Controller) */
void	ftServer::addConfigComponents(p3ConfigMgr */*mgr*/)
{
	/* NOT SURE ABOUT THIS ONE */
}

const RsPeerId& ftServer::OwnId()
{
	static RsPeerId null_id ;

	if (mServiceCtrl)
		return  mServiceCtrl->getOwnId();
	else
		return null_id ;
}

	/* Final Setup (once everything is assigned) */
void ftServer::SetupFtServer()
{

	/* setup FiStore/Monitor */
	std::string localcachedir = mConfigPath + "/cache/local";
	std::string remotecachedir = mConfigPath + "/cache/remote";
	RsPeerId ownId = mServiceCtrl->getOwnId();

	/* search/extras List */
	mFtExtra = new ftExtraList();
	mFtSearch = new ftFileSearch();

	/* Transport */
	mFtDataplex = new ftDataMultiplex(ownId, this, mFtSearch);

	/* make Controller */
	mFtController = new ftController(mCacheStrapper, mFtDataplex, mServiceCtrl, getServiceInfo().mServiceType);
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

	mServiceCtrl->registerServiceMonitor(mFtController, getServiceInfo().mServiceType);
	mServiceCtrl->registerServiceMonitor(mCacheStrapper, getServiceInfo().mServiceType);

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
	mFtExtra->start("ft extra lst");

	/* startup Monitor Thread */
	/* startup the FileMonitor (after cache load) */
	/* start it up */
	mFiMon->start("ft monitor");

	/* Controller thread */
	mFtController->start("ft ctrl");

	/* Dataplex */
	mFtDataplex->start("ft dataplex");
}

void ftServer::StopThreads()
{
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


/***************************************************************/
/********************** RsFiles Interface **********************/
/***************************************************************/

/***************************************************************/
/********************** Controller Access **********************/
/***************************************************************/

bool	ftServer::ResumeTransfers()
{
	mFtController->activate();

	return true;
}

bool ftServer::checkHash(const RsFileHash& hash,std::string& error_string)
{
    return true ;
}

bool ftServer::getFileData(const RsFileHash& hash, uint64_t offset, uint32_t& requested_size,uint8_t *data)
{
    return mFtDataplex->getFileData(hash, offset, requested_size,data);
}

bool ftServer::alreadyHaveFile(const RsFileHash& hash, FileInfo &info)
{
	return mFtController->alreadyHaveFile(hash, info);
}

bool ftServer::FileRequest(const std::string& fname, const RsFileHash& hash, uint64_t size, const std::string& dest, TransferRequestFlags flags, const std::list<RsPeerId>& srcIds)
{
	std::string error_string ;

	if(!checkHash(hash,error_string))
	{
        RsServer::notify()->notifyErrorMsg(0,0,"Error handling hash \""+hash.toStdString()+"\". This hash appears to be invalid(Error string=\""+error_string+"\"). This is probably due an bad handling of strings.") ;
		return false ;
	}

   std::cerr << "Requesting " << fname << std::endl ;

	if(!mFtController->FileRequest(fname, hash, size, dest, flags, srcIds))
		return false ;

	return true ;
}

bool ftServer::setDestinationName(const RsFileHash& hash,const std::string& name)
{
	return mFtController->setDestinationName(hash,name);
}
bool ftServer::setDestinationDirectory(const RsFileHash& hash,const std::string& directory)
{
	return mFtController->setDestinationDirectory(hash,directory);
}
bool ftServer::setChunkStrategy(const RsFileHash& hash,FileChunksInfo::ChunkStrategy s)
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
bool ftServer::FileCancel(const RsFileHash& hash)
{
	// Remove from both queue and ftController, by default.
	//
	mFtController->FileCancel(hash);

	return true ;
}

bool ftServer::FileControl(const RsFileHash& hash, uint32_t flags)
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
bool ftServer::changeQueuePosition(const RsFileHash& hash, QueueMove mv)
{
	mFtController->moveInQueue(hash,mv) ;
	return true ;
}
bool ftServer::changeDownloadSpeed(const RsFileHash& hash, int speed)
{
	mFtController->setPriority(hash, (DwlSpeed)speed);
	return true ;
}
bool ftServer::getDownloadSpeed(const RsFileHash& hash, int & speed)
{
	DwlSpeed _speed;
	int ret = mFtController->getPriority(hash, _speed);
	if (ret) 
		speed = _speed;

	return ret;
}
bool ftServer::clearDownload(const RsFileHash& /*hash*/)
{
   return true ;
}

bool ftServer::FileDownloadChunksDetails(const RsFileHash& hash,FileChunksInfo& info)
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

void ftServer::FileDownloads(std::list<RsFileHash> &hashs)
{
    mFtController->FileDownloads(hashs);
	/* this only contains downloads.... not completed */
	//return mFtDataplex->FileDownloads(hashs);
}

bool ftServer::FileUploadChunksDetails(const RsFileHash& hash,const RsPeerId& peer_id,CompressedChunkMap& cmap)
{
	return mFtDataplex->getClientChunkMap(hash,peer_id,cmap);
}

bool ftServer::FileUploads(std::list<RsFileHash> &hashs)
{
	return mFtDataplex->FileUploads(hashs);
}

bool ftServer::FileDetails(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info)
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
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_TURTLE != getRsItemService(rstype))) 
	{
		std::cerr << "  Wrong type !!" << std::endl ;
		return NULL; /* wrong type */
	}

    try
    {
	switch(getRsItemSubType(rstype))
	{
		case RS_TURTLE_SUBTYPE_FILE_REQUEST 			:	return new RsTurtleFileRequestItem(data,size) ;
		case RS_TURTLE_SUBTYPE_FILE_DATA    			:	return new RsTurtleFileDataItem(data,size) ;
		case RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST		:	return new RsTurtleFileMapRequestItem(data,size) ;
		case RS_TURTLE_SUBTYPE_FILE_MAP     			:	return new RsTurtleFileMapItem(data,size) ;
		case RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST		:	return new RsTurtleChunkCrcRequestItem(data,size) ;
		case RS_TURTLE_SUBTYPE_CHUNK_CRC     			:	return new RsTurtleChunkCrcItem(data,size) ;

		default:
																		return NULL ;
	}
    }
    catch(std::exception& e)
    {
        std::cerr << "(EE) deserialisation error in " << __PRETTY_FUNCTION__ << ": " << e.what() << std::endl;
        
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

bool ftServer::handleTunnelRequest(const RsFileHash& hash,const RsPeerId& peer_id)
{
    FileInfo info ;
    bool res = FileDetails(hash, RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_SPEC_ONLY, info);

    if( (!res) && FileDetails(hash,RS_FILE_HINTS_DOWNLOAD,info))
    {
        // This file is currently being downloaded. Let's look if we already have a chunk or not. If not, no need to
        // share the file!

        FileChunksInfo info2 ;
        if(rsFiles->FileDownloadChunksDetails(hash, info2))
            for(uint32_t i=0;i<info2.chunks.size();++i)
                if(info2.chunks[i] == FileChunksInfo::CHUNK_DONE)
                {
                    res = true ;
                    break ;
                }
    }
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

bool  ftServer::ExtraFileAdd(std::string fname, const RsFileHash& hash, uint64_t size,
						uint32_t period, TransferRequestFlags flags)
{
	return mFtExtra->addExtraFile(fname, hash, size, period, flags);
}

bool ftServer::ExtraFileRemove(const RsFileHash& hash, TransferRequestFlags flags)
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

bool ftServer::ExtraFileMove(std::string fname, const RsFileHash& hash, uint64_t size,
                                std::string destpath)
{
	return mFtExtra->moveExtraFile(fname, hash, size, destpath);
}


	/***************************************************************/
	/******************** Directory Listing ************************/
	/***************************************************************/

int ftServer::RequestDirDetails(const RsPeerId& uid, const std::string& path, DirDetails &details)
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
	if(uid == mServiceCtrl->getOwnId())
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
		return mFiMon->SearchKeywords(keywords, results,flags,RsPeerId());
	else
		return mFiStore->SearchKeywords(keywords, results,flags);
	return 0 ;
}
int ftServer::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id)
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
		return mFiMon->SearchBoolExp(exp,results,flags,RsPeerId()) ;
	else
		return mFiStore->searchBoolExp(exp, results);
	return 0 ;
}
int ftServer::SearchBoolExp(Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id)
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
	for(it = dirList.begin(); it != dirList.end(); ++it)
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
	RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsFileTransferSerialiser) ;

	//rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss ;
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
bool	ftServer::sendDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::sendDataRequest() to peer " << peerId << " for hash " << hash << ", offset=" << offset << ", chunk size="<< chunksize << std::endl;
#endif
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
		RsFileTransferDataRequestItem *rfi = new RsFileTransferDataRequestItem();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->file.filesize   = size;
		rfi->file.hash       = hash; /* ftr->hash; */

		/* offsets */
		rfi->fileoffset = offset; /* ftr->offset; */
		rfi->chunksize  = chunksize; /* ftr->chunk; */

		sendItem(rfi);
	}

	return true;
}

bool ftServer::sendChunkMapRequest(const RsPeerId& peerId,const RsFileHash& hash,bool is_client)
{
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::sendChunkMapRequest() to peer " << peerId << " for hash " << hash << std::endl;
#endif
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleFileMapRequestItem *item = new RsTurtleFileMapRequestItem ;
		mTurtleRouter->sendTurtleData(peerId,item) ;
	}
	else
	{
		/* create a packet */
		/* push to networking part */
		RsFileTransferChunkMapRequestItem *rfi = new RsFileTransferChunkMapRequestItem();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->is_client = is_client ;

		sendItem(rfi);
	}

	return true ;
}

bool ftServer::sendChunkMap(const RsPeerId& peerId,const RsFileHash& hash,const CompressedChunkMap& map,bool is_client)
{
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::sendChunkMap() to peer " << peerId << " for hash " << hash << std::endl;
#endif
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
		RsFileTransferChunkMapItem *rfi = new RsFileTransferChunkMapItem();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->is_client = is_client; /* ftr->hash; */
		rfi->compressed_map = map; /* ftr->hash; */

		sendItem(rfi);
	}

	return true ;
}

bool ftServer::sendSingleChunkCRCRequest(const RsPeerId& peerId,const RsFileHash& hash,uint32_t chunk_number)
{
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::sendSingleCRCRequest() to peer " << peerId << " for hash " << hash << ", chunk number=" << chunk_number << std::endl;
#endif
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
		RsFileTransferSingleChunkCrcRequestItem *rfi = new RsFileTransferSingleChunkCrcRequestItem();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->chunk_number = chunk_number ;

		sendItem(rfi);
	}

	return true ;
}

bool ftServer::sendSingleChunkCRC(const RsPeerId& peerId,const RsFileHash& hash,uint32_t chunk_number,const Sha1CheckSum& crc)
{
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::sendSingleCRC() to peer " << peerId << " for hash " << hash << ", chunk number=" << chunk_number << std::endl;
#endif
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
		RsFileTransferSingleChunkCrcItem *rfi = new RsFileTransferSingleChunkCrcItem();

		/* id */
		rfi->PeerId(peerId);

		/* file info */
		rfi->hash = hash; /* ftr->hash; */
		rfi->check_sum = crc; 
		rfi->chunk_number = chunk_number; 

		sendItem(rfi);
	}

	return true ;
}

	/* Server Send */
bool	ftServer::sendData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t baseoffset, uint32_t chunksize, void *data)
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
		//static const uint32_t	MAX_FT_CHUNK  = 32 * 1024; /* 32K */
		//static const uint32_t	MAX_FT_CHUNK  = 16 * 1024; /* 16K */
		//
		static const uint32_t	MAX_FT_CHUNK  = 8 * 1024; /* 16K */

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
			item->chunk_data = rs_malloc(chunk) ;

			if(item->chunk_data == NULL)
			{
				delete item;
				return false;
			}
			memcpy(item->chunk_data,&(((uint8_t *) data)[offset]),chunk) ;

			mTurtleRouter->sendTurtleData(peerId,item) ;
		}
		else
		{
			RsFileTransferDataItem *rfd = new RsFileTransferDataItem();

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

			sendItem(rfd);

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

// Dont delete the item. The client (p3turtle) is doing it after calling this.
//
void ftServer::receiveTurtleData(RsTurtleGenericTunnelItem *i,
											const RsFileHash& hash,
											const RsPeerId& virtual_peer_id,
											RsTurtleGenericTunnelItem::Direction direction) 
{
	switch(i->PacketSubType())
	{
		case RS_TURTLE_SUBTYPE_FILE_REQUEST: 		
			{
				RsTurtleFileRequestItem *item = dynamic_cast<RsTurtleFileRequestItem *>(i) ;
				if (item)
				{
#ifdef SERVER_DEBUG
					std::cerr << "ftServer::receiveTurtleData(): received file data request for " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
					getMultiplexer()->recvDataRequest(virtual_peer_id,hash,0,item->chunk_offset,item->chunk_size) ;
				}
			}
			break ;

		case RS_TURTLE_SUBTYPE_FILE_DATA : 	
			{
				RsTurtleFileDataItem *item = dynamic_cast<RsTurtleFileDataItem *>(i) ;
				if (item)
				{
#ifdef SERVER_DEBUG
					std::cerr << "ftServer::receiveTurtleData(): received file data for " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
					getMultiplexer()->recvData(virtual_peer_id,hash,0,item->chunk_offset,item->chunk_size,item->chunk_data) ;

					item->chunk_data = NULL ;	// this prevents deletion in the destructor of RsFileDataItem, because data will be deleted
					// down _ft_server->getMultiplexer()->recvData()...in ftTransferModule::recvFileData
				}
			}
			break ;

		case RS_TURTLE_SUBTYPE_FILE_MAP : 	
			{
				RsTurtleFileMapItem *item = dynamic_cast<RsTurtleFileMapItem *>(i) ;
				if (item)
				{
#ifdef SERVER_DEBUG
					std::cerr << "ftServer::receiveTurtleData(): received chunk map for hash " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
					getMultiplexer()->recvChunkMap(virtual_peer_id,hash,item->compressed_map,direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
				}
			}
			break ;

		case RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST:	
			{
				//RsTurtleFileMapRequestItem *item = dynamic_cast<RsTurtleFileMapRequestItem *>(i) ;
#ifdef SERVER_DEBUG
				std::cerr << "ftServer::receiveTurtleData(): received chunkmap request for hash " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
				getMultiplexer()->recvChunkMapRequest(virtual_peer_id,hash,direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
			}
			break ;

		case RS_TURTLE_SUBTYPE_CHUNK_CRC : 			
			{
				RsTurtleChunkCrcItem *item = dynamic_cast<RsTurtleChunkCrcItem *>(i) ;
				if (item)
				{
#ifdef SERVER_DEBUG
					std::cerr << "ftServer::receiveTurtleData(): received single chunk CRC for hash " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
					getMultiplexer()->recvSingleChunkCRC(virtual_peer_id,hash,item->chunk_number,item->check_sum) ;
				}
			}
			break ;

		case RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST:	
			{
				RsTurtleChunkCrcRequestItem *item = dynamic_cast<RsTurtleChunkCrcRequestItem *>(i) ;
				if (item)
				{
#ifdef SERVER_DEBUG
					std::cerr << "ftServer::receiveTurtleData(): received single chunk CRC request for hash " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
					getMultiplexer()->recvSingleChunkCRCRequest(virtual_peer_id,hash,item->chunk_number) ;
				}
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
	bool moreToTick = false ;

	if(handleIncoming())
		moreToTick = true;	

	if(handleCacheData())
		moreToTick = true;	

	static time_t last_law_priority_tasks_handling_time = 0 ;
	time_t now = time(NULL) ;

	if(last_law_priority_tasks_handling_time + FILE_TRANSFER_LOW_PRIORITY_TASKS_PERIOD < now)
	{
		last_law_priority_tasks_handling_time = now ;

		mFtDataplex->deleteUnusedServers() ;
		mFtDataplex->handlePendingCrcRequests() ;
		mFtDataplex->dispatchReceivedChunkCheckSum() ;
	}

	return moreToTick;
}

bool ftServer::handleCacheData()
{
	std::list<std::pair<RsPeerId, RsCacheData> > cacheUpdates;
	std::list<std::pair<RsPeerId, RsCacheData> >::iterator it;
	int i=0 ;

#ifdef SERVER_DEBUG_CACHE
	std::cerr << "handleCacheData() " << std::endl;
#endif
	mCacheStrapper->getCacheUpdates(cacheUpdates);
	for(it = cacheUpdates.begin(); it != cacheUpdates.end(); ++it)
	{
		/* construct reply */
		RsFileTransferCacheItem *ci = new RsFileTransferCacheItem();

		/* id from incoming */
		ci -> PeerId(it->first);

		ci -> file.hash = (it->second).hash;
		ci -> file.name = (it->second).name;
		ci -> file.path = ""; // (it->second).path;
		ci -> file.filesize = (it->second).size;
		ci -> cacheType  = (it->second).cid.type;
		ci -> cacheSubId =  (it->second).cid.subid;

#ifdef SERVER_DEBUG_CACHE
		std::string out2 = "Outgoing CacheStrapper Update -> RsCacheItem:\n";
		ci -> print_string(out2);
		std::cerr << out2 << std::endl;
#endif

		sendItem(ci);

		i++;
	}

	return i>0 ;
}

int ftServer::handleIncoming()
{
	// now File Input.
	int nhandled = 0 ;

	RsItem *item = NULL ;
#ifdef SERVER_DEBUG
	std::cerr << "ftServer::handleIncoming() " << std::endl;
#endif

	while(NULL != (item = recvItem()))
	{
		nhandled++ ;

		switch(item->PacketSubType())
		{
			case RS_PKT_SUBTYPE_FT_DATA_REQUEST: 
				{
					RsFileTransferDataRequestItem *f = dynamic_cast<RsFileTransferDataRequestItem*>(item) ;
					if (f)
					{
#ifdef SERVER_DEBUG
						std::cerr << "ftServer::handleIncoming: received data request for hash " << f->file.hash << ", offset=" << f->fileoffset << ", chunk size=" << f->chunksize << std::endl;
#endif
						mFtDataplex->recvDataRequest(f->PeerId(), f->file.hash,  f->file.filesize, f->fileoffset, f->chunksize);
					}
				}
				break ;

			case RS_PKT_SUBTYPE_FT_DATA:
				{
					RsFileTransferDataItem *f = dynamic_cast<RsFileTransferDataItem*>(item) ;
					if (f)
					{
#ifdef SERVER_DEBUG
						std::cerr << "ftServer::handleIncoming: received data for hash " << f->fd.file.hash << ", offset=" << f->fd.file_offset << ", chunk size=" << f->fd.binData.bin_len << std::endl;
#endif
						mFtDataplex->recvData(f->PeerId(), f->fd.file.hash,  f->fd.file.filesize, f->fd.file_offset, f->fd.binData.bin_len, f->fd.binData.bin_data);

						/* we've stolen the data part -> so blank before delete
						 */
						f->fd.binData.TlvShallowClear();
					}
				}
				break ;

			case RS_PKT_SUBTYPE_FT_CHUNK_MAP_REQUEST:
				{
					RsFileTransferChunkMapRequestItem *f = dynamic_cast<RsFileTransferChunkMapRequestItem*>(item) ;
					if (f)
					{
#ifdef SERVER_DEBUG
						std::cerr << "ftServer::handleIncoming: received chunkmap request for hash " << f->hash << ", client=" << f->is_client << std::endl;
#endif
						mFtDataplex->recvChunkMapRequest(f->PeerId(), f->hash,f->is_client) ;
					}
				}
				break ;

			case RS_PKT_SUBTYPE_FT_CHUNK_MAP:
				{
					RsFileTransferChunkMapItem *f = dynamic_cast<RsFileTransferChunkMapItem*>(item) ;
					if (f)
					{
#ifdef SERVER_DEBUG
						std::cerr << "ftServer::handleIncoming: received chunkmap for hash " << f->hash << ", client=" << f->is_client << /*", map=" << f->compressed_map <<*/ std::endl;
#endif
						mFtDataplex->recvChunkMap(f->PeerId(), f->hash,f->compressed_map,f->is_client) ;
					}
				}
				break ;

			case RS_PKT_SUBTYPE_FT_CHUNK_CRC_REQUEST:
				{
					RsFileTransferSingleChunkCrcRequestItem *f = dynamic_cast<RsFileTransferSingleChunkCrcRequestItem*>(item) ;
					if (f)
					{
#ifdef SERVER_DEBUG
						std::cerr << "ftServer::handleIncoming: received single chunk crc req for hash " << f->hash << ", chunk number=" << f->chunk_number << std::endl;
#endif
						mFtDataplex->recvSingleChunkCRCRequest(f->PeerId(), f->hash,f->chunk_number) ;
					}
				}
				break ;

			case RS_PKT_SUBTYPE_FT_CHUNK_CRC:
				{
					RsFileTransferSingleChunkCrcItem *f = dynamic_cast<RsFileTransferSingleChunkCrcItem *>(item) ;
					if (f)
					{
#ifdef SERVER_DEBUG
						std::cerr << "ftServer::handleIncoming: received single chunk crc req for hash " << f->hash << ", chunk number=" << f->chunk_number << ", checksum = " << f->check_sum << std::endl;
#endif
						mFtDataplex->recvSingleChunkCRC(f->PeerId(), f->hash,f->chunk_number,f->check_sum);
					}
				}
				break ;

			case RS_PKT_SUBTYPE_FT_CACHE_ITEM:
				{
					RsFileTransferCacheItem *ci = dynamic_cast<RsFileTransferCacheItem*>(item) ;
					if (ci)
					{
#ifdef SERVER_DEBUG_CACHE
						std::cerr << "ftServer::handleIncoming: received cache item hash=" << ci->file.hash << ". from peer " << ci->PeerId() << std::endl;
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
					}
				}
				break ;

//			case RS_PKT_SUBTYPE_FT_CACHE_REQUEST:
//				{
//					// do nothing
//					RsFileTransferCacheRequestItem *cr = dynamic_cast<RsFileTransferCacheRequestItem*>(item) ;
//					if (cr)
//					{
//#ifdef SERVER_DEBUG_CACHE
//						std::cerr << "ftServer::handleIncoming: received cache request from peer " << cr->PeerId() << std::endl;
//#endif
//					}
//				}
//				break ;
		}

		delete item ;
	}

	return nhandled;
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

