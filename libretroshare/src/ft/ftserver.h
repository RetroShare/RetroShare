/*
 * libretroshare/src/ft: ftserver.h
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

#ifndef FT_SERVER_HEADER
#define FT_SERVER_HEADER

/*
 * ftServer.
 *
 * Top level File Transfer interface.
 * (replaces old filedexserver)
 *
 * sets up the whole File Transfer class structure.
 * sets up the File Indexing side of cache system too.
 *
 * provides rsFiles interface for external control.
 *
 */

#include <map>
#include <list>
#include <iostream>

#include "ft/ftdata.h"
#include "turtle/turtleclientservice.h"
#include "retroshare/rsfiles.h"
//#include "dbase/cachestrapper.h"

#include "pqi/pqi.h"
#include "pqi/p3cfgmgr.h"

class p3ConnectMgr;

class CacheStrapper;
class CacheTransfer;

class NotifyBase; /* needed by FiStore */
class ftCacheStrapper;
class ftFiStore;
class ftFiMonitor;

class ftController;
class ftExtraList;
class ftFileSearch;

class ftDataMultiplex;
class p3turtle;

class ftDwlQueue;

class p3PeerMgr;
class p3LinkMgr;

class ftServer: public RsFiles, public ftDataSend, public RsTurtleClientService, public RsThread
{

	public:

		/***************************************************************/
		/******************** Setup ************************************/
		/***************************************************************/

		ftServer(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr);

		/* Assign important variables */
		void	setConfigDirectory(std::string path);

		void	setP3Interface(P3Interface *pqi);

		/* add Config Items (Extra, Controller) */
		void	addConfigComponents(p3ConfigMgr *mgr);

		virtual CacheStrapper *getCacheStrapper();
		virtual CacheTransfer *getCacheTransfer();

		std::string 	OwnId();

		/* Final Setup (once everything is assigned) */
		//void	SetupFtServer();
		void    SetupFtServer(NotifyBase *cb);
		virtual void    connectToTurtleRouter(p3turtle *p) ;

		void	StartupThreads();
		void	StopThreads();

		/* own thread */
		virtual void	run();

		// Checks that the given hash is well formed. Used to chase 
		// string bugs.
		static bool checkHash(const std::string& hash,std::string& error_string) ;

		// Implements RsTurtleClientService
		//
		virtual bool handleTunnelRequest(const std::string& hash,const std::string& peer_id) ;
		virtual void receiveTurtleData(RsTurtleGenericTunnelItem *item,const std::string& hash,const std::string& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction) ;
		virtual RsTurtleGenericTunnelItem *deserialiseItem(void *data,uint32_t size) const ;

		void addVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&,RsTurtleGenericTunnelItem::Direction dir) ;
		void removeVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&) ;

		/***************************************************************/
		/*************** Control Interface *****************************/
		/************** (Implements RsFiles) ***************************/
		/***************************************************************/

		// member access

		ftDataMultiplex *getMultiplexer() const { return mFtDataplex ; }
		ftController *getController() const { return mFtController ; }

		/***
		 * Control of Downloads
		 ***/
		virtual bool alreadyHaveFile(const std::string& hash, FileInfo &info);
		virtual bool FileRequest(const std::string& fname, const std::string& hash, uint64_t size, const std::string& dest, TransferRequestFlags flags, const std::list<std::string>& srcIds);
		virtual bool FileCancel(const std::string& hash);
		virtual bool FileControl(const std::string& hash, uint32_t flags);
		virtual bool FileClearCompleted();
		virtual bool setDestinationDirectory(const std::string& hash,const std::string& new_path) ;
		virtual bool setDestinationName(const std::string& hash,const std::string& new_name) ;
		virtual bool setChunkStrategy(const std::string& hash,FileChunksInfo::ChunkStrategy s) ;
		virtual void setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy) ;
		virtual FileChunksInfo::ChunkStrategy defaultChunkStrategy() ;
		virtual uint32_t freeDiskSpaceLimit() const ;
		virtual void setFreeDiskSpaceLimit(uint32_t size_in_mb) ;


		/***
		 * Control of Downloads Priority.
		 ***/
		virtual uint32_t getMinPrioritizedTransfers() ;
		virtual void setMinPrioritizedTransfers(uint32_t s) ;
		virtual uint32_t getQueueSize() ;
		virtual void setQueueSize(uint32_t s) ;
		virtual bool changeQueuePosition(const std::string hash, QueueMove queue_mv);
		virtual bool changeDownloadSpeed(const std::string hash, int speed);
		virtual bool getDownloadSpeed(const std::string hash, int & speed);
		virtual bool clearDownload(const std::string hash);
		//virtual void getDwlDetails(std::list<DwlDetails> & details);

		/***
		 * Download/Upload Details
		 ***/
		virtual bool FileDownloads(std::list<std::string> &hashs);
		virtual bool FileUploads(std::list<std::string> &hashs);
		virtual bool FileDetails(const std::string &hash, FileSearchFlags hintflags, FileInfo &info);
		virtual bool FileDownloadChunksDetails(const std::string& hash,FileChunksInfo& info) ;
		virtual bool FileUploadChunksDetails(const std::string& hash,const std::string& peer_id,CompressedChunkMap& map) ;


		/***
		 * Extra List Access
		 ***/
		virtual bool ExtraFileAdd(std::string fname, std::string hash, uint64_t size, uint32_t period, TransferRequestFlags flags);
		virtual bool ExtraFileRemove(std::string hash, TransferRequestFlags flags);
		virtual bool ExtraFileHash(std::string localpath, uint32_t period, TransferRequestFlags flags);
		virtual bool ExtraFileStatus(std::string localpath, FileInfo &info);
		virtual bool ExtraFileMove(std::string fname, std::string hash, uint64_t size, std::string destpath);


		/***
		 * Directory Listing / Search Interface
		 ***/
		virtual int RequestDirDetails(const std::string& uid, const std::string& path, DirDetails &details);
		virtual int RequestDirDetails(void *ref, DirDetails &details, FileSearchFlags flags);
		virtual uint32_t getType(void *ref,FileSearchFlags flags) ;

		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags);
		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const std::string& peer_id);
		virtual int SearchBoolExp(Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags);
		virtual int SearchBoolExp(Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags,const std::string& peer_id);

		/***
		 * Utility Functions
		 ***/
		virtual bool ConvertSharedFilePath(std::string path, std::string &fullpath);
		virtual void ForceDirectoryCheck();
		virtual void updateSinceGroupPermissionsChanged() ;
		virtual bool InDirectoryCheck();
		virtual bool copyFile(const std::string& source, const std::string& dest);

		/***
		 * Directory Handling
		 ***/
		virtual void	setDownloadDirectory(std::string path);
		virtual void	setPartialsDirectory(std::string path);
		virtual std::string getDownloadDirectory();
		virtual std::string getPartialsDirectory();

		virtual bool	getSharedDirectories(std::list<SharedDirInfo> &dirs);
		virtual bool	setSharedDirectories(std::list<SharedDirInfo> &dirs);
		virtual bool 	addSharedDirectory(const SharedDirInfo& dir);
		virtual bool   updateShareFlags(const SharedDirInfo& dir); 	// updates the flags. The directory should already exist !
		virtual bool 	removeSharedDirectory(std::string dir);

		virtual bool	getShareDownloadDirectory();
		virtual bool 	shareDownloadDirectory(bool share);

		virtual void	setRememberHashFilesDuration(uint32_t days) ;
		virtual uint32_t rememberHashFilesDuration() const ;
		virtual bool rememberHashFiles() const ;
		virtual void setRememberHashFiles(bool) ;
		virtual void   clearHashCache() ;
		virtual void setWatchPeriod(int minutes) ;
		virtual int watchPeriod() const ;

		/***************************************************************/
		/*************** Control Interface *****************************/
		/***************************************************************/

		/***************************************************************/
		/*************** Data Transfer Interface ***********************/
		/***************************************************************/
	public:
		virtual bool sendData(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data);
		virtual bool sendDataRequest(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize);
		virtual bool sendChunkMapRequest(const std::string& peer_id,const std::string& hash,bool is_client) ;
		virtual bool sendChunkMap(const std::string& peer_id,const std::string& hash,const CompressedChunkMap& cmap,bool is_client) ;
		virtual bool sendCRC32MapRequest(const std::string&, const std::string&) ;
		virtual bool sendCRC32Map(const std::string&, const std::string&, const CRC32Map&) ;
		virtual bool sendSingleChunkCRCRequest(const std::string& peer_id,const std::string& hash,uint32_t chunk_number) ;
		virtual bool sendSingleChunkCRC(const std::string& peer_id,const std::string& hash,uint32_t chunk_number,const Sha1CheckSum& crc) ;

		/*************** Internal Transfer Fns *************************/
		virtual int tick();

		/* Configuration */
		bool	addConfiguration(p3ConfigMgr *cfgmgr);
		bool	ResumeTransfers();

	private:
		bool	handleInputQueues();
		bool	handleCacheData();
		bool	handleFileData();

		/******************* p3 Config Overload ************************/
	protected:
		/* Key Functions to be overloaded for Full Configuration */
		virtual RsSerialiser *setupSerialiser();
		virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
		virtual bool    loadList(std::list<RsItem *>& load);

	private:
		bool  loadConfigMap(std::map<std::string, std::string> &configMap);
		/******************* p3 Config Overload ************************/

		/*************************** p3 Config Overload ********************/

	private:

		/**** INTERNAL FUNCTIONS ***/
		//virtual int 	reScanDirs();
		//virtual int 	check_dBUpdate();

	private:

		/* no need for Mutex protection -
		 * as each component is protected independently.
		 */

		P3Interface *mP3iface;     /* XXX THIS NEEDS PROTECTION */

		p3PeerMgr *mPeerMgr;
		p3LinkMgr *mLinkMgr;

		ftCacheStrapper *mCacheStrapper;
		ftFiStore 	*mFiStore;
		ftFiMonitor   	*mFiMon;

		ftController  *mFtController;
		ftExtraList   *mFtExtra;

		ftDataMultiplex *mFtDataplex;
		p3turtle *mTurtleRouter ;


		ftFileSearch   *mFtSearch;

		ftDwlQueue *mFtDwlQueue;

		RsMutex srvMutex;
		std::string mConfigPath;
		std::string mDownloadPath;
		std::string mPartialsPath;

};



#endif
