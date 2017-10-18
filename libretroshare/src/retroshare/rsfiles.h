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

namespace RsRegularExpression { class Expression; }

/* These are used mainly by ftController at the moment */
const uint32_t RS_FILE_CTRL_PAUSE	 		= 0x00000100;
const uint32_t RS_FILE_CTRL_START	 		= 0x00000200;
const uint32_t RS_FILE_CTRL_FORCE_CHECK	    = 0x00000400;

const uint32_t RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT     = 0x00000001 ;
const uint32_t RS_FILE_CTRL_ENCRYPTION_POLICY_PERMISSIVE = 0x00000002 ;

const uint32_t RS_FILE_PERM_DIRECT_DL_YES      = 0x00000001 ;
const uint32_t RS_FILE_PERM_DIRECT_DL_NO       = 0x00000002 ;
const uint32_t RS_FILE_PERM_DIRECT_DL_PER_USER = 0x00000003 ;

const uint32_t RS_FILE_RATE_TRICKLE	 = 0x00000001;
const uint32_t RS_FILE_RATE_SLOW	 = 0x00000002;
const uint32_t RS_FILE_RATE_STANDARD	 = 0x00000003;
const uint32_t RS_FILE_RATE_FAST	 = 0x00000004;
const uint32_t RS_FILE_RATE_STREAM_AUDIO = 0x00000005;
const uint32_t RS_FILE_RATE_STREAM_VIDEO = 0x00000006;

const uint32_t RS_FILE_PEER_ONLINE 	 = 0x00001000;
const uint32_t RS_FILE_PEER_OFFLINE 	 = 0x00002000;

const uint32_t RS_FILE_SHARE_FLAGS_IGNORE_PREFIXES          = 0x0001 ;
const uint32_t RS_FILE_SHARE_FLAGS_IGNORE_SUFFIXES          = 0x0002 ;
const uint32_t RS_FILE_SHARE_FLAGS_IGNORE_DUPLICATES        = 0x0004 ;

const uint32_t RS_FILE_SHARE_PARAMETER_DEFAULT_MAXIMUM_DEPTH = 8;

/************************************
 * Used To indicate where to search.
 *
 * The Order of these is very important,
 * it specifies the search order too.
 *
 */

// Flags used when requesting info about transfers, mostly to filter out the result.
//
const FileSearchFlags RS_FILE_HINTS_CACHE_deprecated       ( 0x00000001 );
const FileSearchFlags RS_FILE_HINTS_EXTRA                  ( 0x00000002 );
const FileSearchFlags RS_FILE_HINTS_LOCAL                  ( 0x00000004 );
const FileSearchFlags RS_FILE_HINTS_REMOTE                 ( 0x00000008 );
const FileSearchFlags RS_FILE_HINTS_DOWNLOAD               ( 0x00000010 );
const FileSearchFlags RS_FILE_HINTS_UPLOAD                 ( 0x00000020 );
const FileSearchFlags RS_FILE_HINTS_SPEC_ONLY              ( 0x01000000 );

const FileSearchFlags RS_FILE_HINTS_NETWORK_WIDE           ( 0x00000080 );// can be downloaded anonymously 
const FileSearchFlags RS_FILE_HINTS_BROWSABLE              ( 0x00000100 );// browsable by friends
const FileSearchFlags RS_FILE_HINTS_SEARCHABLE             ( 0x00000200 );// can be searched anonymously
const FileSearchFlags RS_FILE_HINTS_PERMISSION_MASK        ( 0x00000380 );// OR of the last tree flags. Used to filter out.

// Flags used when requesting a transfer
//
const TransferRequestFlags RS_FILE_REQ_ANONYMOUS_ROUTING   ( 0x00000040 ); // Use to ask turtle router to download the file.
const TransferRequestFlags RS_FILE_REQ_ENCRYPTED           ( 0x00000080 ); // Asks for end-to-end encryption of file at the level of ftServer
const TransferRequestFlags RS_FILE_REQ_UNENCRYPTED         ( 0x00000100 ); // Asks for no end-to-end encryption of file at the level of ftServer
const TransferRequestFlags RS_FILE_REQ_ASSUME_AVAILABILITY ( 0x00000200 ); // Assume full source availability. Used for cache files.
const TransferRequestFlags RS_FILE_REQ_CACHE_deprecated    ( 0x00000400 ); // Old stuff used for cache files. Not used anymore.
const TransferRequestFlags RS_FILE_REQ_EXTRA               ( 0x00000800 );
const TransferRequestFlags RS_FILE_REQ_MEDIA               ( 0x00001000 );
const TransferRequestFlags RS_FILE_REQ_BACKGROUND          ( 0x00002000 ); // To download slowly.
const TransferRequestFlags RS_FILE_REQ_NO_SEARCH           ( 0x02000000 );	// disable searching for potential direct sources.

// const uint32_t RS_FILE_HINTS_SHARE_FLAGS_MASK	 = 	RS_FILE_HINTS_NETWORK_WIDE_OTHERS | RS_FILE_HINTS_BROWSABLE_OTHERS
// 																	 | RS_FILE_HINTS_NETWORK_WIDE_GROUPS | RS_FILE_HINTS_BROWSABLE_GROUPS ;

/* Callback Codes */

const uint32_t RS_FILE_EXTRA_DELETE	 = 0x0010;

struct SharedDirInfo
{
	static bool sameLists(const std::list<RsNodeGroupId>& l1,const std::list<RsNodeGroupId>& l2)
	{
		std::list<RsNodeGroupId>::const_iterator it1(l1.begin()) ;
		std::list<RsNodeGroupId>::const_iterator it2(l2.begin()) ;

		for(; (it1!=l1.end() && it2!=l2.end());++it1,++it2)
			if(*it1 != *it2)
				return false ;

		return it1 == l1.end() && it2 == l2.end() ;
	}

	std::string filename ;
	std::string virtualname ;
    FileStorageFlags shareflags ;		// combnation of DIR_FLAGS_ANONYMOUS_DOWNLOAD | DIR_FLAGS_BROWSABLE | ...
    std::list<RsNodeGroupId> parent_groups ;
};

struct SharedDirStats
{
    uint32_t total_number_of_files ;
    uint64_t total_shared_size ;
};

// This class represents a tree of directories and files, only with their names size and hash. It is used to create collection links in the GUI
// and to transmit directory information between services. This class is independent from the existing FileHierarchy classes used in storage because
// we need a very copact serialization and storage size since we create links with it. Besides, we cannot afford to risk the leak of other local information
// by using the orignal classes.

class FileTree
{
public:
	virtual ~FileTree() {}

	static FileTree *create(const DirDetails& dd, bool remote) ;
	static FileTree *create(const std::string& radix64_string) ;

	virtual std::string toRadix64() const =0 ;

	// These methods allow the user to browse the hierarchy

	struct FileData {
		std::string name ;
		uint64_t    size ;
		RsFileHash  hash ;
	};

	virtual uint32_t root() const { return 0;}
	virtual bool getDirectoryContent(uint32_t index,std::vector<uint32_t>& subdirs,std::vector<FileData>& subfiles) const = 0;

	virtual void print() const=0;

	uint32_t mTotalFiles ;
	uint64_t mTotalSize ;
};

class RsFiles
{
	public:

		RsFiles() { return; }
		virtual ~RsFiles() { return; }

        /**
         * Provides file data for the gui: media streaming or rpc clients.
         * It may return unverified chunks. This allows streaming without having to wait for hashes or completion of the file.
         * This function returns an unspecified amount of bytes. Either as much data as available or a sensible maximum. Expect a block size of around 1MiB.
         * To get more data, call this function repeatedly with different offsets.
         * Returns false in case
         * - the files is not available on the local node
         * - not downloading
         * - the requested data was not downloaded yet
         * - end of file was reached
         * @param hash hash of a file. The file has to be available on this node or it has to be in downloading state.
         * @param offset where the desired block starts
     * @param requested_size size of pre-allocated data. Will be updated by the function.
         * @param data pre-allocated memory chunk of size 'requested_size' by the client
         */
        virtual bool getFileData(const RsFileHash& hash, uint64_t offset, uint32_t& requested_size,uint8_t *data)=0;

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
		virtual void setDefaultEncryptionPolicy(uint32_t policy)=0;	// RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT/PERMISSIVE
		virtual uint32_t defaultEncryptionPolicy()=0;
		virtual void setMaxUploadSlotsPerFriend(uint32_t n)=0;
		virtual uint32_t getMaxUploadSlotsPerFriend()=0;
		virtual void setFilePermDirectDL(uint32_t perm)=0;
		virtual uint32_t filePermDirectDL()=0;

		/***
		 * Control of Downloads Priority.
		 ***/
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
        virtual void FileDownloads(std::list<RsFileHash> &hashs) = 0;
		virtual bool FileUploads(std::list<RsFileHash> &hashs) = 0;
		virtual bool FileDetails(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) = 0;
        virtual bool isEncryptedSource(const RsPeerId& virtual_peer_id) =0;

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
        virtual bool findChildPointer(void *ref, int row, void *& result, FileSearchFlags flags) =0;
        virtual uint32_t getType(void *ref,FileSearchFlags flags) = 0;

		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags) = 0;
		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id) = 0;
        virtual int SearchBoolExp(RsRegularExpression::Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags) = 0;
        virtual int SearchBoolExp(RsRegularExpression::Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id) = 0;
		virtual int getSharedDirStatistics(const RsPeerId& pid, SharedDirStats& stats) =0;

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
        virtual void requestDirUpdate(void *ref) =0 ;			// triggers the update of the given reference. Used when browsing.

		virtual void    setDownloadDirectory(std::string path) = 0;
		virtual void    setPartialsDirectory(std::string path) = 0;
		virtual std::string getDownloadDirectory() = 0;
		virtual std::string getPartialsDirectory() = 0;

        virtual bool    getSharedDirectories(std::list<SharedDirInfo>& dirs) = 0;
        virtual bool    setSharedDirectories(const std::list<SharedDirInfo>& dirs) = 0;
        virtual bool    addSharedDirectory(const SharedDirInfo& dir) = 0;
		virtual bool    updateShareFlags(const SharedDirInfo& dir) = 0;	// updates the flags. The directory should already exist !
		virtual bool    removeSharedDirectory(std::string dir) = 0;

		virtual bool getIgnoreLists(std::list<std::string>& ignored_prefixes, std::list<std::string>& ignored_suffixes,uint32_t& flags) =0;
		virtual void setIgnoreLists(const std::list<std::string>& ignored_prefixes, const std::list<std::string>& ignored_suffixes,uint32_t flags) =0;

        virtual void setWatchPeriod(int minutes) =0;
        virtual void setWatchEnabled(bool b) =0;
        virtual int  watchPeriod() const =0;
        virtual bool watchEnabled() =0;
        virtual bool followSymLinks() const=0;
        virtual void setFollowSymLinks(bool b)=0 ;
		virtual void togglePauseHashingProcess() =0;		// pauses/resumes the hashing process.
		virtual bool hashingProcessPaused() =0;

		virtual bool	getShareDownloadDirectory() = 0;
		virtual bool 	shareDownloadDirectory(bool share) = 0;

        virtual void setMaxShareDepth(int depth) =0;
        virtual int  maxShareDepth() const=0;

		virtual bool	ignoreDuplicates() = 0;
		virtual void 	setIgnoreDuplicates(bool ignore) = 0;

};


#endif
