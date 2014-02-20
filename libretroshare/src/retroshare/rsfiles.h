#ifndef RS_FILES_GUI_INTERFACE_H
#define RS_FILES_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsfiles.h
 *
 * RetroShare C++ Interface.
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


#include <list>
#include <iostream>
#include <string>

#include "rstypes.h"

class RsFiles;
extern RsFiles  *rsFiles;

class Expression;
class CacheStrapper ;
class CacheTransfer;

/* These are used mainly by ftController at the moment */
const uint32_t RS_FILE_CTRL_PAUSE	 		= 0x00000100;
const uint32_t RS_FILE_CTRL_START	 		= 0x00000200;
const uint32_t RS_FILE_CTRL_FORCE_CHECK	= 0x00000400;

const uint32_t RS_FILE_RATE_TRICKLE	 = 0x00000001;
const uint32_t RS_FILE_RATE_SLOW	 = 0x00000002;
const uint32_t RS_FILE_RATE_STANDARD	 = 0x00000003;
const uint32_t RS_FILE_RATE_FAST	 = 0x00000004;
const uint32_t RS_FILE_RATE_STREAM_AUDIO = 0x00000005;
const uint32_t RS_FILE_RATE_STREAM_VIDEO = 0x00000006;

const uint32_t RS_FILE_PEER_ONLINE 	 = 0x00001000;
const uint32_t RS_FILE_PEER_OFFLINE 	 = 0x00002000;

/************************************
 * Used To indicate where to search.
 *
 * The Order of these is very important,
 * it specifies the search order too.
 *
 */

// Flags used when requesting info about transfers, mostly to filter out the result.
//
const FileSearchFlags RS_FILE_HINTS_CACHE	 		       	 ( 0x00000001 );
const FileSearchFlags RS_FILE_HINTS_EXTRA	 		       	 ( 0x00000002 );
const FileSearchFlags RS_FILE_HINTS_LOCAL	 		       	 ( 0x00000004 );
const FileSearchFlags RS_FILE_HINTS_REMOTE	 		       ( 0x00000008 );
const FileSearchFlags RS_FILE_HINTS_DOWNLOAD		       	 ( 0x00000010 );
const FileSearchFlags RS_FILE_HINTS_UPLOAD	 		       ( 0x00000020 );
const FileSearchFlags RS_FILE_HINTS_SPEC_ONLY	          ( 0x01000000 );

const FileSearchFlags RS_FILE_HINTS_NETWORK_WIDE           ( 0x00000080 );// anonymously shared over network
const FileSearchFlags RS_FILE_HINTS_BROWSABLE              ( 0x00000100 );// browsable by friends
const FileSearchFlags RS_FILE_HINTS_PERMISSION_MASK        ( 0x00000180 );// OR of the last two flags. Used to filter out.

// Flags used when requesting a transfer
//
const TransferRequestFlags RS_FILE_REQ_ANONYMOUS_ROUTING   ( 0x00000040 ); // Use to ask turtle router to download the file.
const TransferRequestFlags RS_FILE_REQ_ASSUME_AVAILABILITY ( 0x00000200 ); // Assume full source availability. Used for cache files.
const TransferRequestFlags RS_FILE_REQ_CACHE               ( 0x00000400 ); // Assume full source availability. Used for cache files.
const TransferRequestFlags RS_FILE_REQ_EXTRA               ( 0x00000800 );
const TransferRequestFlags RS_FILE_REQ_MEDIA	              ( 0x00001000 );
const TransferRequestFlags RS_FILE_REQ_BACKGROUND	        ( 0x00002000 ); // To download slowly.
const TransferRequestFlags RS_FILE_REQ_NO_SEARCH           ( 0x02000000 );	// disable searching for potential direct sources.

// const uint32_t RS_FILE_HINTS_SHARE_FLAGS_MASK	 = 	RS_FILE_HINTS_NETWORK_WIDE_OTHERS | RS_FILE_HINTS_BROWSABLE_OTHERS 
// 																	 | RS_FILE_HINTS_NETWORK_WIDE_GROUPS | RS_FILE_HINTS_BROWSABLE_GROUPS ;

/* Callback Codes */

const uint32_t RS_FILE_EXTRA_DELETE	 = 0x0010;

struct SharedDirInfo
{
	std::string filename ;
	std::string virtualname ;
	FileStorageFlags shareflags ;		// DIR_FLAGS_NETWORK_WIDE_OTHERS | DIR_FLAGS_BROWSABLE_GROUPS | ...
	std::list<std::string> parent_groups ;
};

class RsFiles
{
	public:

		RsFiles() { return; }
		virtual ~RsFiles() { return; }

		/****************************************/
		/* download */


		/***
		 *  Control of Downloads.
		 ***/

		virtual bool alreadyHaveFile(const RsFileHash& hash, FileInfo &info) = 0;
		/// Returns false is we already have the file. Otherwise, initiates the dl and returns true.
		virtual bool FileRequest(const std::string& fname, const RsFileHash& hash, uint64_t size, const std::string& dest, TransferRequestFlags flags, const std::list<RsPeerId>& srcIds) = 0;
		virtual bool FileCancel(const RsFileHash& hash) = 0;
		virtual bool setDestinationDirectory(const RsFileHash& hash,const std::string& new_path) = 0;
		virtual bool setDestinationName(const RsFileHash& hash,const std::string& new_name) = 0;
		virtual bool setChunkStrategy(const RsFileHash& hash,FileChunksInfo::ChunkStrategy) = 0;
		virtual void setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy) = 0;
		virtual FileChunksInfo::ChunkStrategy defaultChunkStrategy() = 0;
		virtual uint32_t freeDiskSpaceLimit() const =0;
		virtual void setFreeDiskSpaceLimit(uint32_t size_in_mb) =0;
		virtual bool FileControl(const RsFileHash& hash, uint32_t flags) = 0;
		virtual bool FileClearCompleted() = 0;

		/***
		 * Control of Downloads Priority.
		 ***/
		virtual uint32_t getMinPrioritizedTransfers() = 0 ;
		virtual void setMinPrioritizedTransfers(uint32_t s) = 0 ;
		virtual uint32_t getQueueSize() = 0 ;
		virtual void setQueueSize(uint32_t s) = 0 ;
		virtual bool changeQueuePosition(const RsFileHash& hash, QueueMove mv) = 0;
		virtual bool changeDownloadSpeed(const RsFileHash& hash, int speed) = 0;
		virtual bool getDownloadSpeed(const RsFileHash& hash, int & speed) = 0;
		virtual bool clearDownload(const RsFileHash& hash) = 0;
//		virtual void getDwlDetails(std::list<DwlDetails> & details) = 0;

		/***
		 * Download / Upload Details.
		 ***/
		virtual bool FileDownloads(std::list<RsFileHash> &hashs) = 0;
		virtual bool FileUploads(std::list<RsFileHash> &hashs) = 0;
		virtual bool FileDetails(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) = 0;

		/// Gives chunk details about the downloaded file with given hash.
		virtual bool FileDownloadChunksDetails(const RsFileHash& hash,FileChunksInfo& info) = 0 ;

		/// details about the upload with given hash
		virtual bool FileUploadChunksDetails(const RsFileHash& hash,const RsPeerId& peer_id,CompressedChunkMap& map) = 0 ;

		/***
		 * Extra List Access
		 ***/
		//virtual bool ExtraFileAdd(std::string fname, std::string hash, uint64_t size, uint32_t period, TransferRequestFlags flags) = 0;
		virtual bool ExtraFileRemove(const RsFileHash& hash, TransferRequestFlags flags) = 0;
		virtual bool ExtraFileHash(std::string localpath, uint32_t period, TransferRequestFlags flags) = 0;
		virtual bool ExtraFileStatus(std::string localpath, FileInfo &info) = 0;
		virtual bool ExtraFileMove(std::string fname, const RsFileHash& hash, uint64_t size, std::string destpath) = 0;



		/***
		 * Directory Listing / Search Interface
		 */
		virtual int RequestDirDetails(const RsPeerId& uid, const std::string& path, DirDetails &details) = 0;
		virtual int RequestDirDetails(void *ref, DirDetails &details, FileSearchFlags flags) = 0;
		virtual uint32_t getType(void *ref,FileSearchFlags flags) = 0;

		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags) = 0;
		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id) = 0;
		virtual int SearchBoolExp(Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags) = 0;
		virtual int SearchBoolExp(Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id) = 0;

		/***
		 * Utility Functions.
		 ***/
		virtual bool ConvertSharedFilePath(std::string path, std::string &fullpath) = 0;
		virtual void ForceDirectoryCheck() = 0;
		virtual void updateSinceGroupPermissionsChanged() = 0;
		virtual bool InDirectoryCheck() = 0;
		virtual bool copyFile(const std::string& source,const std::string& dest) = 0;

		/***
		 * Directory Control
		 ***/
		virtual void    setDownloadDirectory(std::string path) = 0;
		virtual void    setPartialsDirectory(std::string path) = 0;
		virtual std::string getDownloadDirectory() = 0;
		virtual std::string getPartialsDirectory() = 0;

		virtual bool    getSharedDirectories(std::list<SharedDirInfo> &dirs) = 0;
		virtual bool    addSharedDirectory(const SharedDirInfo& dir) = 0;
		virtual bool    updateShareFlags(const SharedDirInfo& dir) = 0;	// updates the flags. The directory should already exist !
		virtual bool    removeSharedDirectory(std::string dir) = 0;
		virtual void	setRememberHashFilesDuration(uint32_t days) = 0 ;
		virtual uint32_t rememberHashFilesDuration() const = 0 ;
		virtual void   clearHashCache() = 0 ;
				virtual bool rememberHashFiles() const =0;
		virtual void setRememberHashFiles(bool) =0;
				virtual void setWatchPeriod(int minutes) =0;
		virtual int watchPeriod() const =0;

		virtual CacheStrapper *getCacheStrapper() =0;
		virtual CacheTransfer *getCacheTransfer() =0;

		virtual bool	getShareDownloadDirectory() = 0;
		virtual bool 	shareDownloadDirectory(bool share) = 0;

};


#endif
