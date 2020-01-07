/*******************************************************************************
 * libretroshare/src/ft: ftserver.cc                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <retroshare@lunamutt.com>                   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "crypto/chacha20.h"
//const int ftserverzone = 29539;

#include "file_sharing/p3filelists.h"
#include "ft/ftcontroller.h"
#include "ft/ftdatamultiplex.h"
//#include "ft/ftdwlqueue.h"
#include "ft/ftextralist.h"
#include "ft/ftfileprovider.h"
#include "ft/ftfilesearch.h"
#include "ft/ftserver.h"
#include "ft/ftturtlefiletransferitem.h"

#include "pqi/p3linkmgr.h"
#include "pqi/p3notify.h"
#include "pqi/pqi.h"

#include "retroshare/rstypes.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsinit.h"

#include "rsitems/rsfiletransferitems.h"
#include "rsitems/rsserviceids.h"

#include "rsserver/p3face.h"
#include "turtle/p3turtle.h"

#include "util/rsdebug.h"
#include "util/rsdir.h"
#include "util/rsprint.h"

#include <iostream>
#include "util/rstime.h"

#ifdef RS_DEEP_FILES_INDEX
#	include "deep_search/filesindex.hpp"
#endif // def RS_DEEP_FILES_INDEX

/***
 * #define SERVER_DEBUG       1
 * #define SERVER_DEBUG_CACHE 1
 ***/

#define FTSERVER_DEBUG() std::cerr << time(NULL) << " : FILE_SERVER : " << __FUNCTION__ << " : "
#define FTSERVER_ERROR() std::cerr << "(EE) FILE_SERVER ERROR : "

static const rstime_t FILE_TRANSFER_LOW_PRIORITY_TASKS_PERIOD = 5 ;           // low priority tasks handling every 5 seconds
static const rstime_t FILE_TRANSFER_MAX_DELAY_BEFORE_DROP_USAGE_RECORD = 10 ; // keep usage records for 10 secs at most.

#ifdef RS_DEEP_FILES_INDEX
TurtleFileInfoV2::TurtleFileInfoV2(const DeepFilesSearchResult& dRes) :
    fHash(dRes.mFileHash), fWeight(static_cast<float>(dRes.mWeight)),
    fSnippet(dRes.mSnippet)
{
	FileInfo fInfo;
	rsFiles->FileDetails(fHash, RS_FILE_HINTS_LOCAL, fInfo);

	fSize = fInfo.size;
	fName = fInfo.fname;
}
#endif // def  RS_DEEP_FILES_INDEX

TurtleFileInfoV2::~TurtleFileInfoV2() = default;

/* Setup */
ftServer::ftServer(p3PeerMgr *pm, p3ServiceControl *sc):
    p3Service(),
    // should be FT, but this is for backward compatibility
    RsServiceSerializer(RS_SERVICE_TYPE_TURTLE),
      mPeerMgr(pm), mServiceCtrl(sc),
      mFileDatabase(NULL),
      mFtController(NULL), mFtExtra(NULL),
      mFtDataplex(NULL), mFtSearch(NULL), srvMutex("ftServer"),
      mSearchCallbacksMapMutex("ftServer callbacks map")
{
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
	//std::string localcachedir = mConfigPath + "/cache/local";
	//std::string remotecachedir = mConfigPath + "/cache/remote";
	RsPeerId ownId = mServiceCtrl->getOwnId();

	/* search/extras List */
	mFtExtra = new ftExtraList();
	mFtSearch = new ftFileSearch();

	/* Transport */
	mFtDataplex = new ftDataMultiplex(ownId, this, mFtSearch);

	/* make Controller */
	mFtController = new ftController(mFtDataplex, mServiceCtrl, getServiceInfo().mServiceType);
	mFtController -> setFtSearchNExtra(mFtSearch, mFtExtra);

	std::string emergencySaveDir = RsAccounts::AccountDirectory();
	std::string emergencyPartialsDir = RsAccounts::AccountDirectory();

	if (emergencySaveDir != "")
	{
		emergencySaveDir += "/";
		emergencyPartialsDir += "/";
	}
	emergencySaveDir += "Downloads";
	emergencyPartialsDir += "Partials";

	mFtController->setDownloadDirectory(emergencySaveDir);
	mFtController->setPartialsDirectory(emergencyPartialsDir);

	/* complete search setup */
	mFtSearch->addSearchMode(mFtExtra, RS_FILE_HINTS_EXTRA);

	mServiceCtrl->registerServiceMonitor(mFtController, getServiceInfo().mServiceType);

	return;
}

void ftServer::connectToFileDatabase(p3FileDatabase *fdb)
{
	mFileDatabase = fdb ;

	mFtSearch->addSearchMode(fdb, RS_FILE_HINTS_LOCAL);	// due to a bug in addSearchModule, modules can only be added one by one. Using | between flags wont work.
	mFtSearch->addSearchMode(fdb, RS_FILE_HINTS_REMOTE);

    mFileDatabase->setExtraList(mFtExtra);
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
	mFileDatabase->startThreads();

	/* Controller thread */
	mFtController->start("ft ctrl");

	/* Dataplex */
	mFtDataplex->start("ft dataplex");
}

void ftServer::StopThreads()
{
	/* stop Dataplex */
	mFtDataplex->fullstop();

	/* stop Controller thread */
	mFtController->fullstop();

	/* self contained threads */
	/* stop ExtraList Thread */
	mFtExtra->fullstop();

	delete (mFtDataplex);
	mFtDataplex = nullptr;

	delete (mFtController);
	mFtController = nullptr;

	delete (mFtExtra);
	mFtExtra = nullptr;

	/* stop Monitor Thread */
	mFileDatabase->stopThreads();
	delete mFileDatabase;
	mFileDatabase = nullptr;
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

bool ftServer::getFileData(const RsFileHash& hash, uint64_t offset, uint32_t& requested_size,uint8_t *data)
{
	return mFtDataplex->getFileData(hash, offset, requested_size,data);
}

bool ftServer::alreadyHaveFile(const RsFileHash& hash, FileInfo &info)
{
	return mFileDatabase->search(hash, RS_FILE_HINTS_LOCAL, info);
}

bool ftServer::FileRequest(const std::string& fname, const RsFileHash& hash, uint64_t size, const std::string& dest, TransferRequestFlags flags, const std::list<RsPeerId>& srcIds)
{
#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "Requesting " << fname << std::endl ;
#endif

	if(!mFtController->FileRequest(fname, hash, size, dest, flags, srcIds))
		return false ;

	return true ;
}

bool ftServer::activateTunnels(const RsFileHash& hash,uint32_t encryption_policy,TransferRequestFlags flags,bool onoff)
{
	RsFileHash hash_of_hash ;

	encryptHash(hash,hash_of_hash) ;
	mEncryptedHashes.insert(std::make_pair(hash_of_hash,hash)) ;

	if(onoff)
	{
#ifdef SERVER_DEBUG
		FTSERVER_DEBUG() << "Activating tunnels for hash " << hash << std::endl;
#endif
		if(flags & RS_FILE_REQ_ENCRYPTED)
		{
#ifdef SERVER_DEBUG
			FTSERVER_DEBUG() << "  flags require end-to-end encryption. Requesting hash of hash " << hash_of_hash << std::endl;
#endif
			mTurtleRouter->monitorTunnels(hash_of_hash,this,true) ;
		}
		if((flags & RS_FILE_REQ_UNENCRYPTED) && (encryption_policy != RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT))
		{
#ifdef SERVER_DEBUG
			FTSERVER_DEBUG() << "  flags require no end-to-end encryption. Requesting hash " << hash << std::endl;
#endif
			mTurtleRouter->monitorTunnels(hash,this,true) ;
		}
	}
	else
	{
		mTurtleRouter->stopMonitoringTunnels(hash_of_hash);
		mTurtleRouter->stopMonitoringTunnels(hash);
	}
	return true ;
}

bool ftServer::setDestinationDirectory(const RsFileHash& hash,const std::string& directory)
{
	return mFtController->setDestinationDirectory(hash,directory);
}
bool ftServer::setDestinationName(const RsFileHash& hash,const std::string& name)
{
	return mFtController->setDestinationName(hash,name);
}

bool ftServer::setChunkStrategy(const RsFileHash& hash,FileChunksInfo::ChunkStrategy s)
{
	return mFtController->setChunkStrategy(hash,s);
}
void ftServer::setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy s)
{
	mFtController->setDefaultChunkStrategy(s) ;
}
FileChunksInfo::ChunkStrategy ftServer::defaultChunkStrategy()
{
	return mFtController->defaultChunkStrategy() ;
}

uint32_t ftServer::freeDiskSpaceLimit()const
{
	return mFtController->freeDiskSpaceLimit() ;
}
void ftServer::setFreeDiskSpaceLimit(uint32_t s)
{
	mFtController->setFreeDiskSpaceLimit(s) ;
}

void ftServer::setDefaultEncryptionPolicy(uint32_t s)
{
	mFtController->setDefaultEncryptionPolicy(s) ;
}
uint32_t ftServer::defaultEncryptionPolicy()
{
	return mFtController->defaultEncryptionPolicy() ;
}

void ftServer::setMaxUploadSlotsPerFriend(uint32_t n)
{
    mFtController->setMaxUploadsPerFriend(n) ;
}
uint32_t ftServer::getMaxUploadSlotsPerFriend()
{
    return mFtController->getMaxUploadsPerFriend() ;
}

void ftServer::setFilePermDirectDL(uint32_t perm)
{
	mFtController->setFilePermDirectDL(perm);
}
uint32_t ftServer::filePermDirectDL()
{
	return mFtController->filePermDirectDL() ;
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

void ftServer::requestDirUpdate(void *ref)
{
	mFileDatabase->requestDirUpdate(ref) ;
}

/* Directory Handling */
bool ftServer::setDownloadDirectory(const std::string& path)
{
	return mFtController->setDownloadDirectory(path);
}

std::string ftServer::getDownloadDirectory()
{
	return mFtController->getDownloadDirectory();
}

bool ftServer::setPartialsDirectory(const std::string& path)
{
	return mFtController->setPartialsDirectory(path);
}

std::string ftServer::getPartialsDirectory()
{
	return mFtController->getPartialsDirectory();
}

/***************************************************************/
/************************* Other Access ************************/
/***************************************************************/

bool ftServer::copyFile(const std::string& source, const std::string& dest)
{
	return RsDirUtil::copyFile(source, dest);
}

void ftServer::FileDownloads(std::list<RsFileHash> &hashs)
{
	mFtController->FileDownloads(hashs);
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
			if(mFtController->FileDetails(hash, info2))
				info.fname = info2.fname ;

			return true ;
		}

	if(hintflags & ~(RS_FILE_HINTS_UPLOAD | RS_FILE_HINTS_DOWNLOAD))
		if(mFtSearch->search(hash, hintflags, info))
			return true ;

	return false;
}

RsItem *ftServer::create_item(uint16_t service, uint8_t item_type) const
{
#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "p3turtle: deserialising packet: " << std::endl ;
#endif

	RsServiceType serviceType = static_cast<RsServiceType>(service);
	switch (serviceType)
	{
	/* This one is here for retro-compatibility as turtle routing and file
	 * trasfer services were just one service before turle service was
	 * generalized */
	case RsServiceType::TURTLE: break;
	case RsServiceType::FILE_TRANSFER: break;
	default:
		RsErr() << __PRETTY_FUNCTION__ << " Wrong service type: " << service
		        << std::endl;
		return nullptr;
	}

	try
	{
		switch(item_type)
		{
		case RS_TURTLE_SUBTYPE_FILE_REQUEST 			:	return new RsTurtleFileRequestItem();
		case RS_TURTLE_SUBTYPE_FILE_DATA    			:	return new RsTurtleFileDataItem();
		case RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST		    :	return new RsTurtleFileMapRequestItem();
		case RS_TURTLE_SUBTYPE_FILE_MAP     			:	return new RsTurtleFileMapItem();
		case RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST		:	return new RsTurtleChunkCrcRequestItem();
		case RS_TURTLE_SUBTYPE_CHUNK_CRC     			:	return new RsTurtleChunkCrcItem();
		case static_cast<uint8_t>(RsFileItemType::FILE_SEARCH_REQUEST):
			return new RsFileSearchRequestItem();
		case static_cast<uint8_t>(RsFileItemType::FILE_SEARCH_RESULT):
			return new RsFileSearchResultItem();
		default:
			return nullptr;
		}
	}
	catch(std::exception& e)
	{
		FTSERVER_ERROR() << "(EE) deserialisation error in " << __PRETTY_FUNCTION__ << ": " << e.what() << std::endl;

		return nullptr;
	}
}

bool ftServer::isEncryptedSource(const RsPeerId& virtual_peer_id)
{
	RS_STACK_MUTEX(srvMutex) ;

	return  mEncryptedPeerIds.find(virtual_peer_id) != mEncryptedPeerIds.end();
}

void ftServer::addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir)
{
#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "adding virtual peer. Direction=" << dir << ", hash=" << hash << ", vpid=" << virtual_peer_id << std::endl;
#endif
	RsFileHash real_hash ;

	{
		if(findRealHash(hash,real_hash))
		{
			RS_STACK_MUTEX(srvMutex) ;
			mEncryptedPeerIds[virtual_peer_id] = hash ;
		}
		else
			real_hash = hash;
	}

	if(dir == RsTurtleGenericTunnelItem::DIRECTION_SERVER)
	{
#ifdef SERVER_DEBUG
		FTSERVER_DEBUG() << "  direction is SERVER. Adding file source for end-to-end encrypted tunnel for real hash " << real_hash << ", virtual peer id = " << virtual_peer_id << std::endl;
#endif
		mFtController->addFileSource(real_hash,virtual_peer_id) ;
	}
}

void ftServer::removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id)
{
	RsFileHash real_hash ;
	if(findRealHash(hash,real_hash))
		mFtController->removeFileSource(real_hash,virtual_peer_id) ;
	else
		mFtController->removeFileSource(hash,virtual_peer_id) ;

	RS_STACK_MUTEX(srvMutex) ;
	mEncryptedPeerIds.erase(virtual_peer_id) ;
}

bool ftServer::handleTunnelRequest(const RsFileHash& hash,const RsPeerId& peer_id)
{
	FileInfo info ;
	RsFileHash real_hash ;
	bool found = false ;

	if(FileDetails(hash, RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_SPEC_ONLY, info))
	{
		if(info.transfer_info_flags & RS_FILE_REQ_ENCRYPTED)
		{
#ifdef SERVER_DEBUG
			FTSERVER_DEBUG() << "handleTunnelRequest: openning encrypted FT tunnel for H(H(F))=" << hash << " and H(F)=" << info.hash << std::endl;
#endif

			RS_STACK_MUTEX(srvMutex) ;
			mEncryptedHashes[hash] = info.hash;

			real_hash = info.hash ;
		}
		else
			real_hash = hash ;

		found = true ;
	}
	else	// try to see if we're already swarming the file
	{
		{
			RS_STACK_MUTEX(srvMutex) ;
			std::map<RsFileHash,RsFileHash>::const_iterator it = mEncryptedHashes.find(hash) ;

			if(it != mEncryptedHashes.end())
				real_hash = it->second ;
			else
				real_hash = hash ;
		}

		if(FileDetails(real_hash,RS_FILE_HINTS_DOWNLOAD,info))
		{
			// This file is currently being downloaded. Let's look if we already have a chunk or not. If not, no need to
			// share the file!

			FileChunksInfo info2 ;
			if(rsFiles->FileDownloadChunksDetails(hash, info2))
				for(uint32_t i=0;i<info2.chunks.size();++i)
					if(info2.chunks[i] == FileChunksInfo::CHUNK_DONE)
					{
						found = true ;

						if(info.transfer_info_flags & RS_FILE_REQ_ANONYMOUS_ROUTING)
							info.storage_permission_flags = DIR_FLAGS_ANONYMOUS_DOWNLOAD ; // this is to allow swarming

						break ;
					}
		}
	}

	if(found && mFtController->defaultEncryptionPolicy() == RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT && hash == real_hash)
	{
#ifdef SERVER_DEBUG
		std::cerr << "(WW) rejecting file transfer for hash " << hash << " because the hash is not encrypted and encryption policy requires it." << std::endl;
#endif
		return false ;
	}

#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "ftServer: performing local hash search for hash " << hash << std::endl;

	if(found)
	{
		FTSERVER_DEBUG() << "Found hash: " << std::endl;
		FTSERVER_DEBUG() << "   hash  = " << real_hash << std::endl;
		FTSERVER_DEBUG() << "   peer  = " << peer_id << std::endl;
		FTSERVER_DEBUG() << "   flags = " << info.storage_permission_flags << std::endl;
		FTSERVER_DEBUG() << "   groups= " ;
		for(std::list<RsNodeGroupId>::const_iterator it(info.parent_groups.begin());it!=info.parent_groups.end();++it)
			FTSERVER_DEBUG() << (*it) << ", " ;
		FTSERVER_DEBUG() << std::endl;
		FTSERVER_DEBUG() << "   clear = " << rsPeers->computePeerPermissionFlags(peer_id,info.storage_permission_flags,info.parent_groups) << std::endl;
	}
#endif

	// The call to computeHashPeerClearance() return a combination of RS_FILE_HINTS_NETWORK_WIDE and RS_FILE_HINTS_BROWSABLE
	// This is an additional computation cost, but the way it's written here, it's only called when res is true.
	//
	found = found && (RS_FILE_HINTS_NETWORK_WIDE & rsPeers->computePeerPermissionFlags(peer_id,info.storage_permission_flags,info.parent_groups)) ;

	return found ;
}

/***************************************************************/
/******************* ExtraFileList Access **********************/
/***************************************************************/

bool  ftServer::ExtraFileAdd(std::string fname, const RsFileHash& hash, uint64_t size, uint32_t period, TransferRequestFlags flags)
{
	return mFtExtra->addExtraFile(fname, hash, size, period, flags);
}

bool ftServer::ExtraFileRemove(const RsFileHash& hash)
{ return mFileDatabase->removeExtraFile(hash); }

bool ftServer::ExtraFileHash(
        std::string localpath, rstime_t period, TransferRequestFlags flags )
{
	return mFtExtra->hashExtraFile(
	            localpath, static_cast<uint32_t>(period), flags );
}

bool ftServer::ExtraFileStatus(std::string localpath, FileInfo &info)
{
	return mFtExtra->hashExtraFileDone(localpath, info);
}

bool ftServer::ExtraFileMove(std::string fname, const RsFileHash& hash, uint64_t size, std::string destpath)
{
	return mFtExtra->moveExtraFile(fname, hash, size, destpath);
}

/***************************************************************/
/******************** Directory Listing ************************/
/***************************************************************/

bool ftServer::findChildPointer(void *ref, int row, void *& result, FileSearchFlags flags)
{
	return mFileDatabase->findChildPointer(ref,row,result,flags) ;
}

bool ftServer::requestDirDetails(
        DirDetails &details, std::uintptr_t handle, FileSearchFlags flags )
{ return RequestDirDetails(reinterpret_cast<void*>(handle), details, flags); }

int ftServer::RequestDirDetails(void *ref, DirDetails &details, FileSearchFlags flags)
{
	return mFileDatabase->RequestDirDetails(ref,details,flags) ;
}

uint32_t ftServer::getType(void *ref, FileSearchFlags flags)
{
	return mFileDatabase->getType(ref,flags) ;
}
/***************************************************************/
/******************** Search Interface *************************/
/***************************************************************/

int ftServer::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags)
{
	return mFileDatabase->SearchKeywords(keywords, results,flags,RsPeerId());
}
int ftServer::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id)
{
	return mFileDatabase->SearchKeywords(keywords, results,flags,peer_id);
}

int ftServer::SearchBoolExp(RsRegularExpression::Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags)
{
	return mFileDatabase->SearchBoolExp(exp, results,flags,RsPeerId());
}
int ftServer::SearchBoolExp(RsRegularExpression::Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id)
{
	return mFileDatabase->SearchBoolExp(exp,results,flags,peer_id) ;
}
int ftServer::getSharedDirStatistics(const RsPeerId& pid, SharedDirStats& stats)
{
    return mFileDatabase->getSharedDirStatistics(pid,stats) ;
}

/***************************************************************/
/*************** Local Shared Dir Interface ********************/
/***************************************************************/

bool ftServer::ConvertSharedFilePath(std::string path, std::string &fullpath)
{
	return mFileDatabase->convertSharedFilePath(path, fullpath);
}

void    ftServer::updateSinceGroupPermissionsChanged()
{
	mFileDatabase->forceSyncWithPeers();
}
void    ftServer::ForceDirectoryCheck()
{
	mFileDatabase->forceDirectoryCheck();
	return;
}

bool    ftServer::InDirectoryCheck()
{
	return mFileDatabase->inDirectoryCheck();
}

bool	ftServer::getSharedDirectories(std::list<SharedDirInfo> &dirs)
{
	mFileDatabase->getSharedDirectories(dirs);
	return true;
}

bool	ftServer::setSharedDirectories(const std::list<SharedDirInfo>& dirs)
{
	mFileDatabase->setSharedDirectories(dirs);
	return true;
}

bool 	ftServer::addSharedDirectory(const SharedDirInfo& dir)
{
	SharedDirInfo _dir = dir;
	_dir.filename = RsDirUtil::convertPathToUnix(_dir.filename);

	std::list<SharedDirInfo> dirList;
	mFileDatabase->getSharedDirectories(dirList);

	// check that the directory is not already in the list.
	for(std::list<SharedDirInfo>::const_iterator it(dirList.begin());it!=dirList.end();++it)
		if((*it).filename == _dir.filename)
			return false ;

	// ok then, add the shared directory.
	dirList.push_back(_dir);

	mFileDatabase->setSharedDirectories(dirList);
	return true;
}

bool ftServer::updateShareFlags(const SharedDirInfo& info)
{
	mFileDatabase->updateShareFlags(info);

	return true ;
}

bool 	ftServer::removeSharedDirectory(std::string dir)
{
	dir = RsDirUtil::convertPathToUnix(dir);

	std::list<SharedDirInfo> dirList;
	std::list<SharedDirInfo>::iterator it;

#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "ftServer::removeSharedDirectory(" << dir << ")" << std::endl;
#endif

	mFileDatabase->getSharedDirectories(dirList);

#ifdef SERVER_DEBUG
	for(it = dirList.begin(); it != dirList.end(); ++it)
		FTSERVER_DEBUG() << " existing: " << (*it).filename << std::endl;
#endif

	for(it = dirList.begin();it!=dirList.end() && (*it).filename != dir;++it) ;

	if(it == dirList.end())
	{
		FTSERVER_ERROR() << "(EE) ftServer::removeSharedDirectory(): Cannot Find Directory... Fail" << std::endl;
		return false;
	}


#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << " Updating Directories" << std::endl;
#endif

	dirList.erase(it);
	mFileDatabase->setSharedDirectories(dirList);

	return true;
}

bool ftServer::getIgnoreLists(std::list<std::string>& ignored_prefixes, std::list<std::string>& ignored_suffixes,uint32_t& ignore_flags)
{
	return mFileDatabase->getIgnoreLists(ignored_prefixes,ignored_suffixes,ignore_flags) ;
}
void ftServer::setIgnoreLists(const std::list<std::string>& ignored_prefixes, const std::list<std::string>& ignored_suffixes, uint32_t ignore_flags)
{
	mFileDatabase->setIgnoreLists(ignored_prefixes,ignored_suffixes,ignore_flags) ;
}

bool ftServer::watchEnabled()                      { return mFileDatabase->watchEnabled() ; }
int  ftServer::watchPeriod() const                 { return mFileDatabase->watchPeriod()/60 ; }
bool ftServer::followSymLinks() const              { return mFileDatabase->followSymLinks() ; }
bool ftServer::ignoreDuplicates()                  { return mFileDatabase->ignoreDuplicates() ; }
int  ftServer::maxShareDepth() const               { return mFileDatabase->maxShareDepth() ; }

void ftServer::setWatchEnabled(bool b)             { mFileDatabase->setWatchEnabled(b) ; }
void ftServer::setWatchPeriod(int minutes)         { mFileDatabase->setWatchPeriod(minutes*60) ; }
void ftServer::setFollowSymLinks(bool b)           { mFileDatabase->setFollowSymLinks(b) ; }
void ftServer::setIgnoreDuplicates(bool ignore)    { mFileDatabase->setIgnoreDuplicates(ignore); }
void ftServer::setMaxShareDepth(int depth)         { mFileDatabase->setMaxShareDepth(depth) ; }

void ftServer::togglePauseHashingProcess()  { mFileDatabase->togglePauseHashingProcess() ; }
bool ftServer::hashingProcessPaused() { return mFileDatabase->hashingProcessPaused() ; }

bool ftServer::getShareDownloadDirectory()
{
	std::list<SharedDirInfo> dirList;
	mFileDatabase->getSharedDirectories(dirList);

	std::string dir = mFtController->getDownloadDirectory();

	// check if the download directory is in the list.
	for (std::list<SharedDirInfo>::const_iterator it(dirList.begin()); it != dirList.end(); ++it)
		if ((*it).filename == dir)
			return true;

	return false;
}

bool ftServer::shareDownloadDirectory(bool share)
{
	if (share)
	{
		/* Share */
		SharedDirInfo inf ;
		inf.filename = mFtController->getDownloadDirectory();
		inf.shareflags = DIR_FLAGS_ANONYMOUS_DOWNLOAD ;

		return addSharedDirectory(inf);
	}
	else
	{
		/* Unshare */
		std::string dir = mFtController->getDownloadDirectory();
		return removeSharedDirectory(dir);
	}
}

/***************************************************************/
/****************** End of RsFiles Interface *******************/
/***************************************************************/

//bool  ftServer::loadConfigMap(std::map<std::string, std::string> &/*configMap*/)
//{
//	return true;
//}

/***************************************************************/
/**********************     Data Flow     **********************/
/***************************************************************/

bool ftServer::sendTurtleItem(const RsPeerId& peerId,const RsFileHash& hash,RsTurtleGenericTunnelItem *item)
{
	// we cannot look in the encrypted hash map, since the same hash--on this side of the FT--can be used with both
	// encrypted and unencrypted peers ids. So the information comes from the virtual peer Id.

	RsFileHash encrypted_hash;

	if(findEncryptedHash(peerId,encrypted_hash))
	{
		// we encrypt the item

#ifdef SERVER_DEBUG
		FTSERVER_DEBUG() << "Sending turtle item to peer ID " << peerId << " using encrypted tunnel." << std::endl;
#endif

		RsTurtleGenericDataItem *encrypted_item ;

		if(!encryptItem(item, hash, encrypted_item))
			return false ;

		delete item ;

		mTurtleRouter->sendTurtleData(peerId,encrypted_item) ;
	}
	else
	{
#ifdef SERVER_DEBUG
		FTSERVER_DEBUG() << "Sending turtle item to peer ID " << peerId << " using non uncrypted tunnel." << std::endl;
#endif
		mTurtleRouter->sendTurtleData(peerId,item) ;
	}

	return true ;
}

/* Client Send */
bool	ftServer::sendDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "ftServer::sendDataRequest() to peer " << peerId << " for hash " << hash << ", offset=" << offset << ", chunk size="<< chunksize << std::endl;
#endif
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleFileRequestItem *item = new RsTurtleFileRequestItem ;

		item->chunk_offset = offset ;
		item->chunk_size = chunksize ;

		sendTurtleItem(peerId,hash,item) ;
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
	FTSERVER_DEBUG() << "ftServer::sendChunkMapRequest() to peer " << peerId << " for hash " << hash << std::endl;
#endif
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleFileMapRequestItem *item = new RsTurtleFileMapRequestItem ;
		sendTurtleItem(peerId,hash,item) ;
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
	FTSERVER_DEBUG() << "ftServer::sendChunkMap() to peer " << peerId << " for hash " << hash << std::endl;
#endif
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleFileMapItem *item = new RsTurtleFileMapItem ;
		item->compressed_map = map ;
		sendTurtleItem(peerId,hash,item) ;
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
	FTSERVER_DEBUG() << "ftServer::sendSingleCRCRequest() to peer " << peerId << " for hash " << hash << ", chunk number=" << chunk_number << std::endl;
#endif
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleChunkCrcRequestItem *item = new RsTurtleChunkCrcRequestItem;
		item->chunk_number = chunk_number ;

		sendTurtleItem(peerId,hash,item) ;
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
	FTSERVER_DEBUG() << "ftServer::sendSingleCRC() to peer " << peerId << " for hash " << hash << ", chunk number=" << chunk_number << std::endl;
#endif
	if(mTurtleRouter->isTurtlePeer(peerId))
	{
		RsTurtleChunkCrcItem *item = new RsTurtleChunkCrcItem;
		item->chunk_number = chunk_number ;
		item->check_sum = crc ;

		sendTurtleItem(peerId,hash,item) ;
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
	FTSERVER_DEBUG() << "ftServer::sendData() to " << peerId << ", hash: " << hash << " offset: " << baseoffset << " chunk: " << chunksize << " data: " << data << std::endl;
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

			sendTurtleItem(peerId,hash,item) ;
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
			FTSERVER_DEBUG() << "ftServer::sendData() Packet: " << " offset: " << rfd->fd.file_offset << " chunk: " << chunk << " len: " << rfd->fd.binData.bin_len << " data: " << rfd->fd.binData.bin_data << std::endl;
#endif
		}

		offset += chunk;
		tosend -= chunk;
	}

	/* clean up data */
	free(data);

	return true;
}

// Encrypts the given item using aead-chacha20-poly1305
//
// The format is the following
//
//     [encryption format] [random initialization vector] [encrypted data size] [encrypted data] [authentication tag]
//            4 bytes                 12 bytes                   4 bytes            variable           16 bytes
//
//                         +-------------------- authenticated data part ----------------------+
//
//
// Encryption format:
//     ae ad 01 01		:  encryption using AEAD, format 01 (authed with Poly1305   ), version 01
//     ae ad 02 01		:  encryption using AEAD, format 02 (authed with HMAC Sha256), version 01
//
//

void ftServer::deriveEncryptionKey(const RsFileHash& hash, uint8_t *key)
{
	// The encryption key is simply the 256 hash of the
	SHA256_CTX sha_ctx ;

	if(SHA256_DIGEST_LENGTH != 32)
		throw std::runtime_error("Warning: can't compute Sha1Sum with sum size != 32") ;

	SHA256_Init(&sha_ctx);
	SHA256_Update(&sha_ctx, hash.toByteArray(), hash.SIZE_IN_BYTES);
	SHA256_Final (key, &sha_ctx);
}

#ifdef USE_NEW_METHOD
static const uint32_t ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE = 12 ;
static const uint32_t ENCRYPTED_FT_AUTHENTICATION_TAG_SIZE    = 16 ;
static const uint32_t ENCRYPTED_FT_HEADER_SIZE                =  4 ;
static const uint32_t ENCRYPTED_FT_EDATA_SIZE                 =  4 ;

static const uint8_t  ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_POLY1305 = 0x01 ;
static const uint8_t  ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_SHA256   = 0x02 ;
#endif //USE_NEW_METHOD

bool ftServer::encryptItem(RsTurtleGenericTunnelItem *clear_item,const RsFileHash& hash,RsTurtleGenericDataItem *& encrypted_item)
{
#ifndef USE_NEW_METHOD
    uint32_t item_serialized_size = size(clear_item) ;

	RsTemporaryMemory data(item_serialized_size) ;

	if(data == NULL)
		return false ;

    serialise(clear_item, data, &item_serialized_size);

	uint8_t encryption_key[32] ;
	deriveEncryptionKey(hash,encryption_key) ;

	return p3turtle::encryptData(data,item_serialized_size,encryption_key,encrypted_item) ;
#else
	uint8_t initialization_vector[ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE] ;

	RSRandom::random_bytes(initialization_vector,ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE) ;

#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "ftServer::Encrypting ft item." << std::endl;
	FTSERVER_DEBUG() << "  random nonce    : " << RsUtil::BinToHex(initialization_vector,ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE) << std::endl;
#endif

    uint32_t item_serialized_size = size(clear_item) ;
	uint32_t total_data_size = ENCRYPTED_FT_HEADER_SIZE + ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_FT_EDATA_SIZE + item_serialized_size + ENCRYPTED_FT_AUTHENTICATION_TAG_SIZE  ;

#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "  clear part size : " << size(clear_item) << std::endl;
	FTSERVER_DEBUG() << "  total item size : " << total_data_size << std::endl;
#endif

	encrypted_item = new RsTurtleGenericDataItem ;
	encrypted_item->data_bytes = rs_malloc( total_data_size ) ;
	encrypted_item->data_size  = total_data_size ;

	if(encrypted_item->data_bytes == NULL)
		return false ;

	uint8_t *edata = (uint8_t*)encrypted_item->data_bytes ;
	uint32_t edata_size = item_serialized_size;
	uint32_t offset = 0;

	edata[0] = 0xae ;
	edata[1] = 0xad ;
	edata[2] = ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_SHA256 ;	// means AEAD_chacha20_sha256
	edata[3] = 0x01 ;

	offset += ENCRYPTED_FT_HEADER_SIZE;
	uint32_t aad_offset = offset ;
	uint32_t aad_size = ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_FT_EDATA_SIZE ;

	memcpy(&edata[offset], initialization_vector, ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE) ;
	offset += ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE ;

	edata[offset+0] = (edata_size >>  0) & 0xff ;
	edata[offset+1] = (edata_size >>  8) & 0xff ;
	edata[offset+2] = (edata_size >> 16) & 0xff ;
	edata[offset+3] = (edata_size >> 24) & 0xff ;

	offset += ENCRYPTED_FT_EDATA_SIZE ;

	uint32_t ser_size = (uint32_t)((int)total_data_size - (int)offset);

    serialise(clear_item,&edata[offset], &ser_size);

#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "  clear item      : " << RsUtil::BinToHex(&edata[offset],std::min(50,(int)total_data_size-(int)offset)) << "(...)" << std::endl;
#endif

	uint32_t clear_item_offset = offset ;
	offset += edata_size ;

	uint32_t authentication_tag_offset = offset ;
	assert(ENCRYPTED_FT_AUTHENTICATION_TAG_SIZE + offset == total_data_size) ;

	uint8_t encryption_key[32] ;
	deriveEncryptionKey(hash,encryption_key) ;

	if(edata[2] == ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_POLY1305)
		librs::crypto::AEAD_chacha20_poly1305(encryption_key,initialization_vector,&edata[clear_item_offset],edata_size, &edata[aad_offset],aad_size, &edata[authentication_tag_offset],true) ;
	else if(edata[2] == ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_SHA256)
		librs::crypto::AEAD_chacha20_sha256  (encryption_key,initialization_vector,&edata[clear_item_offset],edata_size, &edata[aad_offset],aad_size, &edata[authentication_tag_offset],true) ;
	else
		return false ;

#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "  encryption key  : " << RsUtil::BinToHex(encryption_key,32) << std::endl;
	FTSERVER_DEBUG() << "  authen. tag     : " << RsUtil::BinToHex(&edata[authentication_tag_offset],ENCRYPTED_FT_AUTHENTICATION_TAG_SIZE) << std::endl;
	FTSERVER_DEBUG() << "  final item      : " << RsUtil::BinToHex(&edata[0],std::min(50u,total_data_size)) << "(...)" << std::endl;
#endif

	return true ;
#endif
}

// Decrypts the given item using aead-chacha20-poly1305

bool ftServer::decryptItem(const RsTurtleGenericDataItem *encrypted_item,const RsFileHash& hash,RsTurtleGenericTunnelItem *& decrypted_item)
{
#ifndef USE_NEW_METHOD
	unsigned char *data = NULL ;
	uint32_t data_size = 0 ;

	uint8_t encryption_key[32] ;
	deriveEncryptionKey(hash,encryption_key) ;

	if(!p3turtle::decryptItem(encrypted_item,encryption_key,data,data_size))
	{
		FTSERVER_ERROR() << "Cannot decrypt data!" << std::endl;

		if(data)
			free(data) ;
		return false ;
	}
	decrypted_item = dynamic_cast<RsTurtleGenericTunnelItem*>(deserialise(data,&data_size)) ;
	free(data);

	return (decrypted_item != NULL);

#else
	uint8_t encryption_key[32] ;
	deriveEncryptionKey(hash,encryption_key) ;

	uint8_t *edata = (uint8_t*)encrypted_item->data_bytes ;
	uint32_t offset = 0;

	if(encrypted_item->data_size < ENCRYPTED_FT_HEADER_SIZE + ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_FT_EDATA_SIZE) return false ;

	if(edata[0] != 0xae) return false ;
	if(edata[1] != 0xad) return false ;
	if(edata[2] != ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_POLY1305 && edata[2] != ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_SHA256) return false ;
	if(edata[3] != 0x01) return false ;

	offset += ENCRYPTED_FT_HEADER_SIZE ;
	uint32_t aad_offset = offset ;
	uint32_t aad_size = ENCRYPTED_FT_EDATA_SIZE + ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE ;

	uint8_t *initialization_vector = &edata[offset] ;

#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "ftServer::decrypting ft item." << std::endl;
	FTSERVER_DEBUG() << "  item data       : " << RsUtil::BinToHex(edata,std::min(50u,encrypted_item->data_size)) << "(...)" << std::endl;
	FTSERVER_DEBUG() << "  hash            : " << hash << std::endl;
	FTSERVER_DEBUG() << "  encryption key  : " << RsUtil::BinToHex(encryption_key,32) << std::endl;
	FTSERVER_DEBUG() << "  random nonce    : " << RsUtil::BinToHex(initialization_vector,ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE) << std::endl;
#endif

	offset += ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE ;

	uint32_t edata_size = 0 ;
	edata_size += ((uint32_t)edata[offset+0]) <<  0 ;
	edata_size += ((uint32_t)edata[offset+1]) <<  8 ;
	edata_size += ((uint32_t)edata[offset+2]) << 16 ;
	edata_size += ((uint32_t)edata[offset+3]) << 24 ;

	if(edata_size + ENCRYPTED_FT_EDATA_SIZE + ENCRYPTED_FT_AUTHENTICATION_TAG_SIZE + ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_FT_HEADER_SIZE != encrypted_item->data_size)
	{
		FTSERVER_ERROR() << "  ERROR: encrypted data size is " << edata_size << ", should be " << encrypted_item->data_size - (ENCRYPTED_FT_EDATA_SIZE + ENCRYPTED_FT_AUTHENTICATION_TAG_SIZE + ENCRYPTED_FT_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_FT_HEADER_SIZE ) << std::endl;
		return false ;
	}

	offset += ENCRYPTED_FT_EDATA_SIZE ;
	uint32_t clear_item_offset = offset ;

	uint32_t authentication_tag_offset = offset + edata_size ;
#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "  authen. tag     : " << RsUtil::BinToHex(&edata[authentication_tag_offset],ENCRYPTED_FT_AUTHENTICATION_TAG_SIZE) << std::endl;
#endif

	bool result ;

	if(edata[2] == ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_POLY1305)
		result = librs::crypto::AEAD_chacha20_poly1305(encryption_key,initialization_vector,&edata[clear_item_offset],edata_size, &edata[aad_offset],aad_size, &edata[authentication_tag_offset],false) ;
	else if(edata[2] == ENCRYPTED_FT_FORMAT_AEAD_CHACHA20_SHA256)
		result = librs::crypto::AEAD_chacha20_sha256  (encryption_key,initialization_vector,&edata[clear_item_offset],edata_size, &edata[aad_offset],aad_size, &edata[authentication_tag_offset],false) ;
	else
		return false ;

#ifdef SERVER_DEBUG
	FTSERVER_DEBUG() << "  authen. result  : " << result << std::endl;
	FTSERVER_DEBUG() << "  decrypted daya  : " << RsUtil::BinToHex(&edata[clear_item_offset],std::min(50u,edata_size)) << "(...)" << std::endl;
#endif

	if(!result)
	{
		FTSERVER_ERROR() << "(EE) decryption/authentication went wrong." << std::endl;
		return false ;
	}

	decrypted_item = dynamic_cast<RsTurtleGenericTunnelItem*>(deserialise(&edata[clear_item_offset],&edata_size)) ;

	if(decrypted_item == NULL)
		return false ;

	return true ;
#endif
}

bool ftServer::encryptHash(const RsFileHash& hash, RsFileHash& hash_of_hash)
{
	hash_of_hash = RsDirUtil::sha1sum(hash.toByteArray(),hash.SIZE_IN_BYTES);
	return true ;
}

bool ftServer::findEncryptedHash(const RsPeerId& virtual_peer_id, RsFileHash& encrypted_hash)
{
	RS_STACK_MUTEX(srvMutex);

	std::map<RsPeerId,RsFileHash>::const_iterator it = mEncryptedPeerIds.find(virtual_peer_id) ;

	if(it != mEncryptedPeerIds.end())
	{
		encrypted_hash = it->second ;
		return true ;
	}
	else
		return false ;
}

bool ftServer::findRealHash(const RsFileHash& hash, RsFileHash& real_hash)
{
	RS_STACK_MUTEX(srvMutex);
	std::map<RsFileHash,RsFileHash>::const_iterator it = mEncryptedHashes.find(hash) ;

	if(it != mEncryptedHashes.end())
	{
		real_hash = it->second ;
		return true ;
	}
	else
		return false ;
}

TurtleSearchRequestId ftServer::turtleSearch(const std::string& string_to_match)
{
    return mTurtleRouter->turtleSearch(string_to_match) ;
}
TurtleSearchRequestId ftServer::turtleSearch(const RsRegularExpression::LinearizedExpression& expr)
{
    return mTurtleRouter->turtleSearch(expr) ;
}

#warning we should do this here, but for now it is done by turtle router.
//   // Dont delete the item. The client (p3turtle) is doing it after calling this.
//   //
//   void ftServer::receiveSearchResult(RsTurtleSearchResultItem *item)
//   {
//       RsTurtleFTSearchResultItem *ft_sr = dynamic_cast<RsTurtleFTSearchResultItem*>(item) ;
//
//       if(ft_sr == NULL)
//       {
//   		FTSERVER_ERROR() << "(EE) ftServer::receiveSearchResult(): item cannot be cast to a RsTurtleFTSearchResultItem" << std::endl;
//           return ;
//       }
//
//   	RsServer::notify()->notifyTurtleSearchResult(ft_sr->request_id,ft_sr->result) ;
//   }

// Dont delete the item. The client (p3turtle) is doing it after calling this.
//
void ftServer::receiveTurtleData(const RsTurtleGenericTunnelItem *i,
                                 const RsFileHash& hash,
                                 const RsPeerId& virtual_peer_id,
                                 RsTurtleGenericTunnelItem::Direction direction)
{
	if(i->PacketSubType() == RS_TURTLE_SUBTYPE_GENERIC_DATA)
	{
#ifdef SERVER_DEBUG
		FTSERVER_DEBUG() << "Received encrypted data item. Trying to decrypt" << std::endl;
#endif

		RsFileHash real_hash ;

		if(!findRealHash(hash,real_hash))
		{
			FTSERVER_ERROR() << "(EE) Cannot find real hash for encrypted data item with H(H(F))=" << hash << ". This is unexpected." << std::endl;
			return ;
		}

		RsTurtleGenericTunnelItem *decrypted_item ;
		if(!decryptItem(dynamic_cast<const RsTurtleGenericDataItem *>(i),real_hash,decrypted_item))
		{
			FTSERVER_ERROR() << "(EE) decryption error." << std::endl;
			return ;
		}

		receiveTurtleData(decrypted_item, real_hash, virtual_peer_id,direction) ;

		delete decrypted_item ;
		return ;
	}

	switch(i->PacketSubType())
	{
	case RS_TURTLE_SUBTYPE_FILE_REQUEST:
	{
		const RsTurtleFileRequestItem *item = dynamic_cast<const RsTurtleFileRequestItem *>(i) ;
		if (item)
		{
#ifdef SERVER_DEBUG
			FTSERVER_DEBUG() << "ftServer::receiveTurtleData(): received file data request for " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
			getMultiplexer()->recvDataRequest(virtual_peer_id,hash,0,item->chunk_offset,item->chunk_size) ;
		}
	}
		break ;

	case RS_TURTLE_SUBTYPE_FILE_DATA :
	{
		const RsTurtleFileDataItem *item = dynamic_cast<const RsTurtleFileDataItem *>(i) ;
		if (item)
		{
#ifdef SERVER_DEBUG
			FTSERVER_DEBUG() << "ftServer::receiveTurtleData(): received file data for " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
			getMultiplexer()->recvData(virtual_peer_id,hash,0,item->chunk_offset,item->chunk_size,item->chunk_data) ;

			const_cast<RsTurtleFileDataItem*>(item)->chunk_data = NULL ;	// this prevents deletion in the destructor of RsFileDataItem, because data will be deleted
			// down _ft_server->getMultiplexer()->recvData()...in ftTransferModule::recvFileData
		}
	}
		break ;

	case RS_TURTLE_SUBTYPE_FILE_MAP :
	{
		const RsTurtleFileMapItem *item = dynamic_cast<const RsTurtleFileMapItem *>(i) ;
		if (item)
		{
#ifdef SERVER_DEBUG
			FTSERVER_DEBUG() << "ftServer::receiveTurtleData(): received chunk map for hash " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
			getMultiplexer()->recvChunkMap(virtual_peer_id,hash,item->compressed_map,direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
		}
	}
		break ;

	case RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST:
	{
		//RsTurtleFileMapRequestItem *item = dynamic_cast<RsTurtleFileMapRequestItem *>(i) ;
#ifdef SERVER_DEBUG
		FTSERVER_DEBUG() << "ftServer::receiveTurtleData(): received chunkmap request for hash " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
		getMultiplexer()->recvChunkMapRequest(virtual_peer_id,hash,direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
	}
		break ;

	case RS_TURTLE_SUBTYPE_CHUNK_CRC :
	{
		const RsTurtleChunkCrcItem *item = dynamic_cast<const RsTurtleChunkCrcItem *>(i) ;
		if (item)
		{
#ifdef SERVER_DEBUG
			FTSERVER_DEBUG() << "ftServer::receiveTurtleData(): received single chunk CRC for hash " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
			getMultiplexer()->recvSingleChunkCRC(virtual_peer_id,hash,item->chunk_number,item->check_sum) ;
		}
	}
		break ;

	case RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST:
	{
		const RsTurtleChunkCrcRequestItem *item = dynamic_cast<const RsTurtleChunkCrcRequestItem *>(i) ;
		if (item)
		{
#ifdef SERVER_DEBUG
			FTSERVER_DEBUG() << "ftServer::receiveTurtleData(): received single chunk CRC request for hash " << hash << " from peer " << virtual_peer_id << std::endl;
#endif
			getMultiplexer()->recvSingleChunkCRCRequest(virtual_peer_id,hash,item->chunk_number) ;
		}
	}
		break ;
	default:
		FTSERVER_ERROR() << "WARNING: Unknown packet type received: sub_id=" << reinterpret_cast<void*>(i->PacketSubType()) << ". Is somebody trying to poison you ?" << std::endl ;
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

	static rstime_t last_law_priority_tasks_handling_time = 0 ;
	rstime_t now = time(NULL) ;

	if(last_law_priority_tasks_handling_time + FILE_TRANSFER_LOW_PRIORITY_TASKS_PERIOD < now)
	{
		last_law_priority_tasks_handling_time = now ;

		mFtDataplex->deleteUnusedServers() ;
		mFtDataplex->handlePendingCrcRequests() ;
		mFtDataplex->dispatchReceivedChunkCheckSum() ;
		cleanTimedOutSearches();
	}

	return moreToTick;
}

bool ftServer::checkUploadLimit(const RsPeerId& pid,const RsFileHash& hash)
{
    // No need for this extra cost if the value means "unlimited"

#ifdef SERVER_DEBUG
    std::cerr << "Checking upload limit for friend " << pid << " and hash " << hash << ": " ;
#endif

    uint32_t max_ups = mFtController->getMaxUploadsPerFriend() ;

	RS_STACK_MUTEX(srvMutex) ;

    if(max_ups == 0)
    {
#ifdef SERVER_DEBUG
        std::cerr << " no limit! returning true." << std::endl;
#endif
        return true ;
    }
#ifdef SERVER_DEBUG
	std::cerr << " max=" << max_ups ;
#endif

    // Find the latest records for this pid.

    std::map<RsFileHash,rstime_t>& tmap(mUploadLimitMap[pid]) ;
    std::map<RsFileHash,rstime_t>::iterator it ;

	rstime_t now = time(NULL) ;

    // If the limit has been decresed, we arbitrarily drop some ongoing slots.

    while(tmap.size() > max_ups)
        tmap.erase(tmap.begin()) ;

    // Look in the upload record map. If it's not full, directly allocate a slot. If full, re-use an existing slot if a file is already cited.

    if(tmap.size() < max_ups || (tmap.size()==max_ups && tmap.end() != (it = tmap.find(hash))))
    {
#ifdef SERVER_DEBUG
        std::cerr << " allocated slot for this hash => true" << std::endl;
#endif

        tmap[hash] = now ;
        return true ;
    }

    // There's no room in the used slots, but maybe some of them are not used anymore, in which case we remove them, which freeze a slot.
    uint32_t cleaned = 0 ;

    for(it = tmap.begin();it!=tmap.end() && cleaned<2;)
        if(it->second + FILE_TRANSFER_MAX_DELAY_BEFORE_DROP_USAGE_RECORD < now)
		{
			std::map<RsFileHash,rstime_t>::iterator tmp(it) ;
            ++tmp;
 			tmap.erase(it) ;
            it = tmp;
            ++cleaned ;
		}
		else
			++it ;

    if(cleaned > 0)
    {
#ifdef SERVER_DEBUG
        std::cerr << " cleaned up " << cleaned << " old hashes => true" << std::endl;
#endif
		tmap[hash] = now ;
		return true ;
    }

#ifdef SERVER_DEBUG
	std::cerr << " no slot for this hash => false" << std::endl;
#endif
    return false ;
}

int ftServer::handleIncoming()
{
	// now File Input.
	int nhandled = 0 ;

	RsItem *item = NULL ;

	while(NULL != (item = recvItem()))
	{
		nhandled++ ;

		switch(item->PacketSubType())
		{
		case RS_PKT_SUBTYPE_FT_DATA_REQUEST:
		{
			RsFileTransferDataRequestItem *f = dynamic_cast<RsFileTransferDataRequestItem*>(item) ;

			if (f && checkUploadLimit(f->PeerId(),f->file.hash))
			{
#ifdef SERVER_DEBUG
				FTSERVER_DEBUG() << "ftServer::handleIncoming: received data request for hash " << f->file.hash << ", offset=" << f->fileoffset << ", chunk size=" << f->chunksize << std::endl;
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
				FTSERVER_DEBUG() << "ftServer::handleIncoming: received data for hash " << f->fd.file.hash << ", offset=" << f->fd.file_offset << ", chunk size=" << f->fd.binData.bin_len << std::endl;
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
				FTSERVER_DEBUG() << "ftServer::handleIncoming: received chunkmap request for hash " << f->hash << ", client=" << f->is_client << std::endl;
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
				FTSERVER_DEBUG() << "ftServer::handleIncoming: received chunkmap for hash " << f->hash << ", client=" << f->is_client << /*", map=" << f->compressed_map <<*/ std::endl;
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
				FTSERVER_DEBUG() << "ftServer::handleIncoming: received single chunk crc req for hash " << f->hash << ", chunk number=" << f->chunk_number << std::endl;
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
				FTSERVER_DEBUG() << "ftServer::handleIncoming: received single chunk crc req for hash " << f->hash << ", chunk number=" << f->chunk_number << ", checksum = " << f->check_sum << std::endl;
#endif
				mFtDataplex->recvSingleChunkCRC(f->PeerId(), f->hash,f->chunk_number,f->check_sum);
			}
		}
			break ;
		}

		delete item ;
	}

	return nhandled;
}

/**********************************
 **********************************
 **********************************
 *********************************/

void ftServer::ftReceiveSearchResult(RsTurtleFTSearchResultItem *item)
{
	bool hasCallback = false;

	{
		RS_STACK_MUTEX(mSearchCallbacksMapMutex);
		auto cbpt = mSearchCallbacksMap.find(item->request_id);
		if(cbpt != mSearchCallbacksMap.end())
		{
			hasCallback = true;

			std::vector<TurtleFileInfoV2> cRes;
			for( const auto& tfiold : item->result)
				cRes.push_back(tfiold);

			cbpt->second.first(cRes);
		}
	} // end RS_STACK_MUTEX(mSearchCallbacksMapMutex);

	if(!hasCallback)
		RsServer::notify()->notifyTurtleSearchResult(item->PeerId(),item->request_id, item->result );
}

bool ftServer::receiveSearchRequest(
        unsigned char* searchRequestData, uint32_t searchRequestDataLen,
        unsigned char*& searchResultData, uint32_t& searchResultDataLen,
        uint32_t& maxAllowsHits )
{
#ifdef RS_DEEP_FILES_INDEX
	std::unique_ptr<RsItem> recvItem(
	            RsServiceSerializer::deserialise(
	                searchRequestData, &searchRequestDataLen ) );

	if(!recvItem)
	{
		RsWarn() << __PRETTY_FUNCTION__ << " Search request deserialization "
		         << "failed" << std::endl;
		return false;
	}

	std::unique_ptr<RsFileSearchRequestItem> sReqItPtr(
	            dynamic_cast<RsFileSearchRequestItem*>(recvItem.get()) );
	if(!sReqItPtr)
	{
		RsWarn() << __PRETTY_FUNCTION__ << " Received an invalid search request"
		         << " " << *recvItem << std::endl;
		return false;
	}
	recvItem.release();

	RsFileSearchRequestItem& searchReq(*sReqItPtr);

	std::vector<DeepFilesSearchResult> dRes;
	DeepFilesIndex dfi(DeepFilesIndex::dbDefaultPath());
	if(dfi.search(searchReq.queryString, dRes, maxAllowsHits) > 0)
	{
		RsFileSearchResultItem resIt;

		for(const auto& dMatch : dRes)
			resIt.mResults.push_back(TurtleFileInfoV2(dMatch));

		searchResultDataLen = RsServiceSerializer::size(&resIt);
		searchResultData = static_cast<uint8_t*>(malloc(searchResultDataLen));
		return RsServiceSerializer::serialise(
		            &resIt, searchResultData, &searchResultDataLen );
	}
#endif // def RS_DEEP_FILES_INDEX

	searchResultData = nullptr;
	searchResultDataLen = 0;
	return false;
}

void ftServer::receiveSearchResult(
        TurtleSearchRequestId requestId, unsigned char* searchResultData,
        uint32_t searchResultDataLen )
{
	if(!searchResultData || !searchResultDataLen)
	{
		RsWarn() << __PRETTY_FUNCTION__ << " got null paramethers "
		         << "searchResultData: " << static_cast<void*>(searchResultData)
		         << " searchResultDataLen: " << searchResultDataLen
		         << " seems someone else in the network have a buggy RetroShare"
		         << " implementation" << std::endl;
		return;
	}

	RS_STACK_MUTEX(mSearchCallbacksMapMutex);
	auto cbpt = mSearchCallbacksMap.find(requestId);
	if(cbpt != mSearchCallbacksMap.end())
	{
		RsItem* recvItem = RsServiceSerializer::deserialise(
		            searchResultData, &searchResultDataLen );

		if(!recvItem)
		{
			RsWarn() << __PRETTY_FUNCTION__ << " Search result deserialization "
			         << "failed" << std::endl;
			return;
		}

		std::unique_ptr<RsFileSearchResultItem> resItPtr(
		            dynamic_cast<RsFileSearchResultItem*>(recvItem) );

		if(!resItPtr)
		{
			RsWarn() << __PRETTY_FUNCTION__ << " Received invalid search result"
			         << std::endl;
			delete recvItem;
			return;
		}

		cbpt->second.first(resItPtr->mResults);
	}
}

/***************************** CONFIG ****************************/

bool    ftServer::addConfiguration(p3ConfigMgr *cfgmgr)
{
	/* add all the subbits to config mgr */
	cfgmgr->addConfiguration("ft_database.cfg" , mFileDatabase);
	cfgmgr->addConfiguration("ft_extra.cfg"    , mFtExtra     );
	cfgmgr->addConfiguration("ft_transfers.cfg", mFtController);

	return true;
}

#ifdef RS_DEEP_FILES_INDEX
static std::vector<std::string> xapianQueryKeywords =
{
    " AND ", " OR ", " NOT ", " XOR ", " +", " -", " ( ", " ) ", " NEAR ",
    " ADJ ", " \"", "\" "
};
#endif

bool ftServer::turtleSearchRequest(
        const std::string& matchString,
        const std::function<void (const std::vector<TurtleFileInfoV2>& results)>& multiCallback,
        rstime_t maxWait )
{
	if(matchString.empty())
	{
		RsWarn() << __PRETTY_FUNCTION__ << " match string can't be empty!"
		         << std::endl;
		return false;
	}

#ifdef RS_DEEP_FILES_INDEX
	RsFileSearchRequestItem sItem;
	sItem.queryString = matchString;

	uint32_t iSize = RsServiceSerializer::size(&sItem);
	uint8_t* iBuf = static_cast<uint8_t*>(malloc(iSize));
	RsServiceSerializer::serialise(&sItem, iBuf, &iSize);

	Dbg3() << __PRETTY_FUNCTION__ << " sending search request:" << sItem
	       << std::endl;

	TurtleRequestId xsId = mTurtleRouter->turtleSearch(iBuf, iSize, this);

	{ RS_STACK_MUTEX(mSearchCallbacksMapMutex);
		mSearchCallbacksMap.emplace(
		            xsId,
		            std::make_pair(
		                multiCallback,
		                std::chrono::system_clock::now() +
		                std::chrono::seconds(maxWait) ) );
	} // RS_STACK_MUTEX(mSearchCallbacksMapMutex);

	/* Trick to keep receiving more or less usable results from old peers */
	std::string strippedQuery = matchString;
	for(const std::string& xKeyword : xapianQueryKeywords)
	{
		std::string::size_type pos = std::string::npos;
		while( (pos  = strippedQuery.find(xKeyword)) != std::string::npos )
			strippedQuery.replace(pos, xKeyword.length(), " ");
	}

	Dbg3() << __PRETTY_FUNCTION__ << " sending stripped query for "
	       << "retro-compatibility: " << strippedQuery << std::endl;

	TurtleRequestId sId = mTurtleRouter->turtleSearch(strippedQuery);
#else  // def RS_DEEP_FILES_INDEX
	TurtleRequestId sId = mTurtleRouter->turtleSearch(matchString);
#endif // def RS_DEEP_FILES_INDEX

	{
		RS_STACK_MUTEX(mSearchCallbacksMapMutex);
		mSearchCallbacksMap.emplace(
		            sId,
		            std::make_pair(
		                multiCallback,
		                std::chrono::system_clock::now() +
		                std::chrono::seconds(maxWait) ) );
	}

	return true;
}

void ftServer::cleanTimedOutSearches()
{
	RS_STACK_MUTEX(mSearchCallbacksMapMutex);
	auto now = std::chrono::system_clock::now();
	for( auto cbpt = mSearchCallbacksMap.begin();
	     cbpt != mSearchCallbacksMap.end(); )
		if(cbpt->second.second <= now)
			cbpt = mSearchCallbacksMap.erase(cbpt);
		else ++cbpt;
}

// Offensive content file filtering

int ftServer::banFile(const RsFileHash& real_file_hash, const std::string& filename, uint64_t file_size)
{
    return mFileDatabase->banFile(real_file_hash,filename,file_size) ;
}
int ftServer::unbanFile(const RsFileHash& real_file_hash)
{
    return mFileDatabase->unbanFile(real_file_hash) ;
}
bool ftServer::getPrimaryBannedFilesList(std::map<RsFileHash,BannedFileEntry>& banned_files)
{
    return mFileDatabase->getPrimaryBannedFilesList(banned_files) ;
}

bool ftServer::isHashBanned(const RsFileHash& hash)
{
    return mFileDatabase->isFileBanned(hash);
}

RsFileItem::~RsFileItem() = default;

RsFileItem::RsFileItem(RsFileItemType subtype) :
    RsItem( RS_PKT_VERSION_SERVICE,
            static_cast<uint16_t>(RsServiceType::FILE_TRANSFER),
            static_cast<uint8_t>(subtype) ) {}

void RsFileSearchRequestItem::clear() { queryString.clear(); }

void RsFileSearchResultItem::clear() { mResults.clear(); }
