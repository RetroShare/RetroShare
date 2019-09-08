/*******************************************************************************
 * libretroshare/src/ft: ftserver.h                                            *
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
#include <functional>
#include <chrono>

#include "ft/ftdata.h"
#include "turtle/turtleclientservice.h"
#include "services/p3service.h"
#include "retroshare/rsfiles.h"

#include "pqi/pqi.h"
#include "pqi/p3cfgmgr.h"

class p3ConnectMgr;
class p3FileDatabase;

class ftFiStore;
class ftFiMonitor;

class ftController;
class ftExtraList;
class ftFileSearch;

class ftDataMultiplex;
class p3turtle;

class p3PeerMgr;
class p3ServiceControl;
class p3FileDatabase;

class ftServer: public p3Service, public RsFiles, public ftDataSend, public RsTurtleClientService, public RsServiceSerializer
{

public:

    /***************************************************************/
    /******************** Setup ************************************/
    /***************************************************************/

    ftServer(p3PeerMgr *peerMgr, p3ServiceControl *serviceCtrl);
    virtual RsServiceInfo getServiceInfo();

    /* Assign important variables */
    void	 setConfigDirectory(std::string path);

    /* add Config Items (Extra, Controller) */
    void addConfigComponents(p3ConfigMgr *mgr);

    const RsPeerId& OwnId();

    /* Final Setup (once everything is assigned) */
    void    SetupFtServer() ;
    virtual void connectToTurtleRouter(p3turtle *p) ;
    virtual void connectToFileDatabase(p3FileDatabase *b);

    // Implements RsTurtleClientService
    //

    uint16_t serviceId() const { return RS_SERVICE_TYPE_FILE_TRANSFER ; }
    virtual bool handleTunnelRequest(const RsFileHash& hash,const RsPeerId& peer_id) ;
    virtual void receiveTurtleData(const RsTurtleGenericTunnelItem *item,const RsFileHash& hash,const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction) ;
	virtual void ftReceiveSearchResult(RsTurtleFTSearchResultItem *item);	// We dont use TurtleClientService::receiveSearchResult() because of backward compatibility.
    virtual RsItem *create_item(uint16_t service,uint8_t item_type) const ;
	virtual RsServiceSerializer *serializer() { return this ; }

    void addVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&,RsTurtleGenericTunnelItem::Direction dir) ;
    void removeVirtualPeer(const TurtleFileHash&, const TurtleVirtualPeerId&) ;

    /***************************************************************/
    /*************** Control Interface *****************************/
    /************** (Implements RsFiles) ***************************/
    /***************************************************************/

    void StartupThreads();
    void StopThreads();

    // member access

    ftDataMultiplex *getMultiplexer() const { return mFtDataplex ; }
    ftController *getController() const { return mFtController ; }

    /**
         * @see RsFiles::getFileData
         */
    bool getFileData(const RsFileHash& hash, uint64_t offset, uint32_t& requested_size,uint8_t *data);

    /***
         * Control of Downloads
         ***/
    virtual bool alreadyHaveFile(const RsFileHash& hash, FileInfo &info);
    virtual bool FileRequest(const std::string& fname, const RsFileHash& hash, uint64_t size, const std::string& dest, TransferRequestFlags flags, const std::list<RsPeerId>& srcIds);
    virtual bool FileCancel(const RsFileHash& hash);
    virtual bool FileControl(const RsFileHash& hash, uint32_t flags);
    virtual bool FileClearCompleted();
    virtual bool setDestinationDirectory(const RsFileHash& hash,const std::string& new_path) ;
    virtual bool setDestinationName(const RsFileHash& hash,const std::string& new_name) ;
    virtual bool setChunkStrategy(const RsFileHash& hash,FileChunksInfo::ChunkStrategy s) ;
    virtual void setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy) ;
    virtual FileChunksInfo::ChunkStrategy defaultChunkStrategy() ;
    virtual uint32_t freeDiskSpaceLimit() const ;
    virtual void setFreeDiskSpaceLimit(uint32_t size_in_mb) ;
    virtual void setDefaultEncryptionPolicy(uint32_t policy) ;	// RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT/PERMISSIVE
    virtual uint32_t defaultEncryptionPolicy() ;
	virtual void setMaxUploadSlotsPerFriend(uint32_t n) ;
	virtual uint32_t getMaxUploadSlotsPerFriend() ;
	virtual void setFilePermDirectDL(uint32_t perm) ;
	virtual uint32_t filePermDirectDL() ;

	/// @see RsFiles
	virtual bool turtleSearchRequest(
	        const std::string& matchString,
	        const std::function<void (const std::list<TurtleFileInfo>& results)>& multiCallback,
	        rstime_t maxWait = 300 );

	virtual TurtleSearchRequestId turtleSearch(const std::string& string_to_match) ;
	virtual TurtleSearchRequestId turtleSearch(const RsRegularExpression::LinearizedExpression& expr) ;

    /***
         * Control of Downloads Priority.
         ***/
    virtual uint32_t getQueueSize() ;
    virtual void setQueueSize(uint32_t s) ;
    virtual bool changeQueuePosition(const RsFileHash& hash, QueueMove queue_mv);
    virtual bool changeDownloadSpeed(const RsFileHash& hash, int speed);
    virtual bool getDownloadSpeed(const RsFileHash& hash, int & speed);
    virtual bool clearDownload(const RsFileHash& hash);
    //virtual void getDwlDetails(std::list<DwlDetails> & details);

    /***
         * Download/Upload Details
         ***/
    virtual void FileDownloads(std::list<RsFileHash> &hashs);
    virtual bool FileUploads(std::list<RsFileHash> &hashs);
    virtual bool FileDetails(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info);
    virtual bool FileDownloadChunksDetails(const RsFileHash& hash,FileChunksInfo& info) ;
    virtual bool FileUploadChunksDetails(const RsFileHash& hash,const RsPeerId& peer_id,CompressedChunkMap& map) ;
    virtual bool isEncryptedSource(const RsPeerId& virtual_peer_id) ;


    /***
         * Extra List Access
         ***/
    virtual bool ExtraFileAdd(std::string fname, const RsFileHash& hash, uint64_t size, uint32_t period, TransferRequestFlags flags);
    virtual bool ExtraFileRemove(const RsFileHash& hash);
	virtual bool ExtraFileHash(std::string localpath, rstime_t period, TransferRequestFlags flags);
    virtual bool ExtraFileStatus(std::string localpath, FileInfo &info);
    virtual bool ExtraFileMove(std::string fname, const RsFileHash& hash, uint64_t size, std::string destpath);


    /***
         * Directory Listing / Search Interface
         ***/
    virtual int RequestDirDetails(void *ref, DirDetails &details, FileSearchFlags flags);

	/// @see RsFiles::RequestDirDetails
	virtual bool requestDirDetails(
	        DirDetails &details, std::uintptr_t handle = 0,
	        FileSearchFlags flags = RS_FILE_HINTS_LOCAL );

    virtual bool findChildPointer(void *ref, int row, void *& result, FileSearchFlags flags) ;
    virtual uint32_t getType(void *ref,FileSearchFlags flags) ;

    virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags);
    virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id);
    virtual int SearchBoolExp(RsRegularExpression::Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags);
    virtual int SearchBoolExp(RsRegularExpression::Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id);
	virtual int getSharedDirStatistics(const RsPeerId& pid, SharedDirStats& stats) ;

    virtual int banFile(const RsFileHash& real_file_hash, const std::string& filename, uint64_t file_size) ;
    virtual int unbanFile(const RsFileHash& real_file_hash);
    virtual bool getPrimaryBannedFilesList(std::map<RsFileHash,BannedFileEntry>& banned_files) ;
	virtual bool isHashBanned(const RsFileHash& hash);

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
    virtual void requestDirUpdate(void *ref) ;			// triggers the update of the given reference. Used when browsing.
	virtual bool setDownloadDirectory(const std::string& path);
	virtual bool setPartialsDirectory(const std::string& path);
    virtual std::string getDownloadDirectory();
    virtual std::string getPartialsDirectory();

    virtual bool	getSharedDirectories(std::list<SharedDirInfo> &dirs);
    virtual bool	setSharedDirectories(const std::list<SharedDirInfo> &dirs);
    virtual bool 	addSharedDirectory(const SharedDirInfo& dir);
    virtual bool   updateShareFlags(const SharedDirInfo& dir); 	// updates the flags. The directory should already exist !
    virtual bool 	removeSharedDirectory(std::string dir);

	virtual bool getIgnoreLists(std::list<std::string>& ignored_prefixes, std::list<std::string>& ignored_suffixes, uint32_t& ignore_flags) ;
	virtual void setIgnoreLists(const std::list<std::string>& ignored_prefixes, const std::list<std::string>& ignored_suffixes,uint32_t ignore_flags) ;

    virtual bool	getShareDownloadDirectory();
    virtual bool 	shareDownloadDirectory(bool share);

    virtual void setWatchPeriod(int minutes) ;
    virtual int watchPeriod() const ;
    virtual void setWatchEnabled(bool b) ;
    virtual bool watchEnabled() ;
	virtual bool followSymLinks() const;
	virtual void setFollowSymLinks(bool b);
	virtual void togglePauseHashingProcess();
	virtual bool hashingProcessPaused();

	virtual void setMaxShareDepth(int depth) ;
	virtual int  maxShareDepth() const;

	virtual bool ignoreDuplicates() ;
	virtual void setIgnoreDuplicates(bool ignore) ;

    static bool encryptHash(const RsFileHash& hash, RsFileHash& hash_of_hash);

    /***************************************************************/
    /*************** Data Transfer Interface ***********************/
    /***************************************************************/
public:
    virtual bool activateTunnels(const RsFileHash& hash,uint32_t default_encryption_policy,TransferRequestFlags flags,bool onoff);

    virtual bool sendData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data);
    virtual bool sendDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize);
    virtual bool sendChunkMapRequest(const RsPeerId& peer_id,const RsFileHash& hash,bool is_client) ;
    virtual bool sendChunkMap(const RsPeerId& peer_id,const RsFileHash& hash,const CompressedChunkMap& cmap,bool is_client) ;
    virtual bool sendSingleChunkCRCRequest(const RsPeerId& peer_id,const RsFileHash& hash,uint32_t chunk_number) ;
    virtual bool sendSingleChunkCRC(const RsPeerId& peer_id,const RsFileHash& hash,uint32_t chunk_number,const Sha1CheckSum& crc) ;

    static void deriveEncryptionKey(const RsFileHash& hash, uint8_t *key);

    bool encryptItem(RsTurtleGenericTunnelItem *clear_item,const RsFileHash& hash,RsTurtleGenericDataItem *& encrypted_item);
    bool decryptItem(const RsTurtleGenericDataItem *encrypted_item, const RsFileHash& hash, RsTurtleGenericTunnelItem *&decrypted_item);

    /*************** Internal Transfer Fns *************************/
    virtual int tick();

    /* Configuration */
    bool	addConfiguration(p3ConfigMgr *cfgmgr);
    bool	ResumeTransfers();

    /*************************** p3 Config Overload ********************/

protected:
    int handleIncoming() ;
    bool handleCacheData() ;

    /*!
     * \brief sendTurtleItem
     * 			Sends the given item into a turtle tunnel, possibly encrypting it if the type of tunnel requires it, which is known from the hash itself.
     * \param peerId Peer id to send to (this is a virtual peer id from turtle service)
     * \param hash   hash of the file. If the item needs to be encrypted
     * \param item	 item to send.
     * \return
     * 			true if everything goes right
     */
    bool sendTurtleItem(const RsPeerId& peerId,const RsFileHash& hash,RsTurtleGenericTunnelItem *item);

    // fnds out what is the real hash of encrypted hash hash
    bool findRealHash(const RsFileHash& hash, RsFileHash& real_hash);
    bool findEncryptedHash(const RsPeerId& virtual_peer_id, RsFileHash& encrypted_hash);

	bool checkUploadLimit(const RsPeerId& pid,const RsFileHash& hash);
private:

    /**** INTERNAL FUNCTIONS ***/
    //virtual int 	reScanDirs();
    //virtual int 	check_dBUpdate();

private:

    /* no need for Mutex protection -
         * as each component is protected independently.
         */
    p3PeerMgr        *mPeerMgr;
    p3ServiceControl *mServiceCtrl;
    p3FileDatabase   *mFileDatabase ;
    ftController     *mFtController;
    ftExtraList      *mFtExtra;
    ftDataMultiplex  *mFtDataplex;
    p3turtle         *mTurtleRouter ;
    ftFileSearch     *mFtSearch;

    RsMutex srvMutex;
    std::string mConfigPath;
    std::string mDownloadPath;
    std::string mPartialsPath;

    std::map<RsFileHash,RsFileHash> mEncryptedHashes ; // This map is such that sha1(it->second) = it->first
    std::map<RsPeerId,RsFileHash> mEncryptedPeerIds ;  // This map holds the hash to be used with each peer id
    std::map<RsPeerId,std::map<RsFileHash,rstime_t> > mUploadLimitMap ;

	/** Store search callbacks with timeout*/
	std::map<
	    TurtleRequestId,
	    std::pair<
	        std::function<void (const std::list<TurtleFileInfo>& results)>,
	        std::chrono::system_clock::time_point >
	 > mSearchCallbacksMap;
	RsMutex mSearchCallbacksMapMutex;

	/// Cleanup mSearchCallbacksMap
	void cleanTimedOutSearches();
};



#endif
