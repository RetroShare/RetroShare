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

#include "rsiface/rstypes.h"

class RsFiles;
extern RsFiles  *rsFiles;

class Expression;

/* These are used mainly by ftController at the moment */
const uint32_t RS_FILE_CTRL_PAUSE	 = 0x00000100;
const uint32_t RS_FILE_CTRL_START	 = 0x00000200;

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

const uint32_t RS_FILE_HINTS_MASK	             = 0x00ffffff;

const uint32_t RS_FILE_HINTS_CACHE	 		       = 0x00000001;
const uint32_t RS_FILE_HINTS_EXTRA	 		       = 0x00000002;
const uint32_t RS_FILE_HINTS_LOCAL	 		       = 0x00000004;
const uint32_t RS_FILE_HINTS_REMOTE	 		       = 0x00000008;
const uint32_t RS_FILE_HINTS_DOWNLOAD		       = 0x00000010;
const uint32_t RS_FILE_HINTS_UPLOAD	 		       = 0x00000020;

const uint32_t RS_FILE_HINTS_NETWORK_WIDE        = 0x00000080;	// anonymously shared over network
const uint32_t RS_FILE_HINTS_BROWSABLE 	       = 0x00000100;	// browsable by friends
const uint32_t RS_FILE_HINTS_ASSUME_AVAILABILITY = 0x00000200; // Assume full source availability. Used for cache files.

const uint32_t RS_FILE_HINTS_SPEC_ONLY	 = 0x01000000;
const uint32_t RS_FILE_HINTS_NO_SEARCH   = 0x02000000;

/* Callback Codes */
//const uint32_t RS_FILE_HINTS_CACHE	 = 0x00000001; // ALREADY EXISTS
const uint32_t RS_FILE_HINTS_MEDIA	 = 0x00001000;

const uint32_t RS_FILE_HINTS_BACKGROUND	 = 0x00002000; // To download slowly.

const uint32_t RS_FILE_EXTRA_DELETE	 = 0x0010;

const uint32_t CB_CODE_CACHE = 0x0001;
const uint32_t CB_CODE_EXTRA = 0x0002;
const uint32_t CB_CODE_MEDIA = 0x0004;

struct SharedDirInfo
{
	std::string filename ;
	uint32_t shareflags ;		// RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_BROWSABLE
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

		/// Returns false is we already have the file. Otherwise, initiates the dl and returns true.
		virtual bool FileRequest(std::string fname, std::string hash, uint64_t size, std::string dest, uint32_t flags, std::list<std::string> srcIds) = 0;
		virtual bool FileCancel(std::string hash) = 0;
		virtual bool setChunkStrategy(const std::string& hash,FileChunksInfo::ChunkStrategy) = 0;
		virtual bool FileControl(std::string hash, uint32_t flags) = 0;
		virtual bool FileClearCompleted() = 0;

		/***
		 * Control of Downloads Priority.
		 ***/
		virtual bool changeQueuePriority(const std::string hash, int priority) = 0;
		virtual bool changeDownloadSpeed(const std::string hash, int speed) = 0;
		virtual bool getQueuePriority(const std::string hash, int & priority) = 0;
		virtual bool getDownloadSpeed(const std::string hash, int & speed) = 0;
		virtual bool clearDownload(const std::string hash) = 0;
		virtual void clearQueue() = 0;
		virtual void getDwlDetails(std::list<DwlDetails> & details) = 0;

		/***
		 * Download / Upload Details.
		 ***/
		virtual bool FileDownloads(std::list<std::string> &hashs) = 0;
		virtual bool FileUploads(std::list<std::string> &hashs) = 0;
		virtual bool FileDetails(std::string hash, uint32_t hintflags, FileInfo &info) = 0;

		/// Gives chunk details about the downloaded file with given hash.
		virtual bool FileDownloadChunksDetails(const std::string& hash,FileChunksInfo& info) = 0 ;

		/// details about the upload with given hash
		virtual bool FileUploadChunksDetails(const std::string& hash,const std::string& peer_id,CompressedChunkMap& map) = 0 ;

		/***
		 * Extra List Access
		 ***/
		virtual bool ExtraFileAdd(std::string fname, std::string hash, uint64_t size,
				uint32_t period, uint32_t flags) = 0;
		virtual bool ExtraFileRemove(std::string hash, uint32_t flags) = 0;
		virtual bool ExtraFileHash(std::string localpath,
				uint32_t period, uint32_t flags) = 0;
		virtual bool ExtraFileStatus(std::string localpath, FileInfo &info) = 0;
		virtual bool ExtraFileMove(std::string fname, std::string hash, uint64_t size,
				std::string destpath) = 0;



		/***
		 * Directory Listing / Search Interface
		 */
		virtual int RequestDirDetails(std::string uid, std::string path, DirDetails &details) = 0;
		virtual int RequestDirDetails(void *ref, DirDetails &details, uint32_t flags) = 0;

		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,uint32_t flags) = 0;
		virtual int SearchBoolExp(Expression * exp, std::list<DirDetails> &results,uint32_t flags) = 0;

		/***
		 * Utility Functions.
		 ***/
		virtual bool ConvertSharedFilePath(std::string path, std::string &fullpath) = 0;
		virtual void ForceDirectoryCheck() = 0;
		virtual bool InDirectoryCheck() = 0;

		/***
		 * Directory Control
		 ***/
		virtual void    setDownloadDirectory(std::string path) = 0;
		virtual void    setPartialsDirectory(std::string path) = 0;
		virtual std::string getDownloadDirectory() = 0;
		virtual std::string getPartialsDirectory() = 0;

		virtual bool    getSharedDirectories(std::list<SharedDirInfo> &dirs) = 0;
		virtual bool    addSharedDirectory(SharedDirInfo dir) = 0;
		virtual bool    updateShareFlags(const SharedDirInfo& dir) = 0;	// updates the flags. The directory should already exist !
		virtual bool    removeSharedDirectory(std::string dir) = 0;

		virtual void	setShareDownloadDirectory(bool value) = 0;
		virtual bool	getShareDownloadDirectory() = 0;
		virtual bool 	shareDownloadDirectory() = 0;
		virtual bool 	unshareDownloadDirectory() = 0;

};


#endif
