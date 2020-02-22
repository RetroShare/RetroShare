/*******************************************************************************
 * libretroshare/src/retroshare: rsfiles.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2008  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#pragma once

#include <list>
#include <iostream>
#include <string>
#include <functional>
#include <chrono>
#include <cstdint>

#include "rstypes.h"
#include "serialiser/rsserializable.h"
#include "rsturtle.h"
#include "util/rstime.h"
#include "retroshare/rsevents.h"

class RsFiles;

/**
 * Pointer to global instance of RsFiles service implementation
 * @jsonapi{development}
 */
extern RsFiles* rsFiles;

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

enum class RsSharedDirectoriesEventCode: uint8_t {
    UNKNOWN                     = 0x00,
    STARTING_DIRECTORY_SWEEP    = 0x01, // (void)
    HASHING_FILE                = 0x02, // mMessage: full path and hashing speed of the file being hashed
    DIRECTORY_SWEEP_ENDED       = 0x03, // (void)
    SAVING_FILE_INDEX           = 0x04, // (void)
};

enum class RsFileTransferEventCode: uint8_t {
    UNKNOWN                     = 0x00,
    DOWNLOAD_COMPLETE           = 0x01,	// mHash: hash of the complete file
    COMPLETED_FILES_REMOVED     = 0x02,	//
};

struct RsSharedDirectoriesEvent: RsEvent
{
	RsSharedDirectoriesEvent()  : RsEvent(RsEventType::SHARED_DIRECTORIES), mEventCode(RsSharedDirectoriesEventCode::UNKNOWN) {}
	~RsSharedDirectoriesEvent() override = default;

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext& ctx ) override
	{
		RsEvent::serial_process(j, ctx);

		RS_SERIAL_PROCESS(mEventCode);
		RS_SERIAL_PROCESS(mMessage);
	}

    RsSharedDirectoriesEventCode mEventCode;
    std::string mMessage;
};

struct RsFileTransferEvent: RsEvent
{
	RsFileTransferEvent()  : RsEvent(RsEventType::FILE_TRANSFER), mFileTransferEventCode(RsFileTransferEventCode::UNKNOWN) {}
	~RsFileTransferEvent() override = default;

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext& ctx ) override
	{
		RsEvent::serial_process(j, ctx);

		RS_SERIAL_PROCESS(mFileTransferEventCode);
		RS_SERIAL_PROCESS(mHash);
	}

    RsFileTransferEventCode mFileTransferEventCode;
    RsFileHash              mHash;
};
struct SharedDirInfo : RsSerializable
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

	std::string filename;
	std::string virtualname;

	/// combnation of DIR_FLAGS_ANONYMOUS_DOWNLOAD | DIR_FLAGS_BROWSABLE | ...
	FileStorageFlags shareflags;
	std::list<RsNodeGroupId> parent_groups;

	/// @see RsSerializable::serial_process
	virtual void serial_process(RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx)
	{
		RS_SERIAL_PROCESS(filename);
		RS_SERIAL_PROCESS(virtualname);
		RS_SERIAL_PROCESS(shareflags);
		RS_SERIAL_PROCESS(parent_groups);
	}
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

	static FileTree *create(const DirDetails& dd, bool remote, bool remove_top_dirs = true) ;
	static FileTree *create(const std::string& radix64_string) ;

	virtual std::string toRadix64() const =0 ;

	// These methods allow the user to browse the hierarchy

	struct FileData {
		std::string name ;
		uint64_t    size ;
		RsFileHash  hash ;
	};

	virtual uint32_t root() const { return 0;}
	virtual bool getDirectoryContent(uint32_t index,std::string& name,std::vector<uint32_t>& subdirs,std::vector<FileData>& subfiles) const = 0;

	virtual void print() const=0;

	uint32_t mTotalFiles ;
	uint64_t mTotalSize ;
};

struct BannedFileEntry : RsSerializable
{
	BannedFileEntry() : mFilename(""), mSize(0), mBanTimeStamp(0) {}

	std::string mFilename;
	uint64_t mSize;
	rstime_t mBanTimeStamp;

	/// @see RsSerializable::serial_process
	virtual void serial_process(RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx)
	{
		RS_SERIAL_PROCESS(mFilename);
		RS_SERIAL_PROCESS(mSize);
		RS_SERIAL_PROCESS(mBanTimeStamp);
	}
};

struct DeepFilesSearchResult;

struct TurtleFileInfoV2 : RsSerializable
{
	TurtleFileInfoV2() : fSize(0), fWeight(0) {}

	TurtleFileInfoV2(const TurtleFileInfo& oldInfo) :
	    fSize(oldInfo.size), fHash(oldInfo.hash), fName(oldInfo.name),
	    fWeight(0) {}

#ifdef RS_DEEP_FILES_INDEX
	TurtleFileInfoV2(const DeepFilesSearchResult& dRes);
#endif // def RS_DEEP_FILES_INDEX

	uint64_t fSize;    /// File size
	RsFileHash fHash;  /// File hash
	std::string fName; /// File name

	/** @brief Xapian weight of the file which matched the search criteria
	 * This field is optional (its value is 0 when not specified).
	 * Given that Xapian weight for the same file is usually different on
	 * different nodes, it should not be used as an absolute refence, but just
	 * as an hint of how much the given file match the search criteria.
	 */
	float fWeight;

	/** @brief Xapian snippet of the file which matched the search criteria
	 * This field is optional (its value is an empty string when not specified).
	 */
	std::string fSnippet;


	/// @see RsSerializable::serial_process
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(fSize);
		RS_SERIAL_PROCESS(fHash);
		RS_SERIAL_PROCESS(fName);
		RS_SERIAL_PROCESS(fWeight);
		RS_SERIAL_PROCESS(fSnippet);
	}

	~TurtleFileInfoV2() override;
};

class RsFiles
{
public:
	RsFiles() {}
	virtual ~RsFiles() {}

	/**
	 * @brief Provides file data for the GUI, media streaming or API clients.
	 * It may return unverified chunks. This allows streaming without having to
	 * wait for hashes or completion of the file.
	 * This function returns an unspecified amount of bytes. Either as much data
	 * as available or a sensible maximum. Expect a block size of around 1MiB.
	 * To get more data, call this function repeatedly with different offsets.
	 *
	 * @jsonapi{development,manualwrapper}
	 * note the wrapper for this is written manually not autogenerated
	 * @see JsonApiServer.
	 *
	 * @param[in] hash hash of the file. The file has to be available on this node
	 * 	or it has to be in downloading state.
	 * @param[in] offset where the desired block starts
	 * @param[inout] requested_size size of pre-allocated data. Will be updated
	 * 	by the function.
	 * @param[out] data pre-allocated memory chunk of size 'requested_size' by the
	 * 	client
	 * @return Returns false in case
	 * 	- the files is not available on the local node
	 * 	- not downloading
	 * 	- the requested data was not downloaded yet
	 * 	- end of file was reached
	 */
	virtual bool getFileData( const RsFileHash& hash, uint64_t offset,
	                          uint32_t& requested_size, uint8_t* data ) = 0;

	/**
	 * @brief Check if we already have a file
	 * @jsonapi{development}
	 * @param[in] hash file identifier
	 * @param[out] info storage for the possibly found file information
	 * @return true if the file is already present, false otherwise
	 */
	virtual bool alreadyHaveFile(const RsFileHash& hash, FileInfo &info) = 0;

	/**
	 * @brief Initiate downloading of a file
	 * @jsonapi{development}
	 * @param[in] fileName
	 * @param[in] hash
	 * @param[in] size
	 * @param[in] destPath in not empty specify a destination path
	 * @param[in] flags you usually want RS_FILE_REQ_ANONYMOUS_ROUTING
	 * @param[in] srcIds eventually specify known sources
	 * @return false if we already have the file, true otherwhise
	 */
	virtual bool FileRequest(
	        const std::string& fileName, const RsFileHash& hash, uint64_t size,
	        const std::string& destPath, TransferRequestFlags flags,
	        const std::list<RsPeerId>& srcIds ) = 0;

	/**
	 * @brief Cancel file downloading
	 * @jsonapi{development}
	 * @param[in] hash
	 * @return false if the file is not in the download queue, true otherwhise
	 */
	virtual bool FileCancel(const RsFileHash& hash) = 0;

	/**
	 * @brief Set destination directory for given file
	 * @jsonapi{development}
	 * @param[in] hash file identifier
	 * @param[in] newPath
	 * @return false if some error occurred, true otherwise
	 */
	virtual bool setDestinationDirectory(
	        const RsFileHash& hash, const std::string& newPath ) = 0;

	/**
	 * @brief Set name for dowloaded file
	 * @jsonapi{development}
	 * @param[in] hash file identifier
	 * @param[in] newName
	 * @return false if some error occurred, true otherwise
	 */
	virtual bool setDestinationName(
	        const RsFileHash& hash, const std::string& newName ) = 0;

	/**
	 * @brief Set chunk strategy for file, useful to set streaming mode to be
	 * able of see video or other media preview while it is still downloading
	 * @jsonapi{development}
	 * @param[in] hash file identifier
	 * @param[in] newStrategy
	 * @return false if some error occurred, true otherwise
	 */
	virtual bool setChunkStrategy(
	        const RsFileHash& hash,
	        FileChunksInfo::ChunkStrategy newStrategy ) = 0;

	/**
	 * @brief Set default chunk strategy
	 * @jsonapi{development}
	 * @param[in] strategy
	 */
	virtual void setDefaultChunkStrategy(
	        FileChunksInfo::ChunkStrategy strategy ) = 0;

	/**
	 * @brief Get default chunk strategy
	 * @jsonapi{development}
	 * @return current default chunck strategy
	 */
	virtual FileChunksInfo::ChunkStrategy defaultChunkStrategy() = 0;

	/**
	 * @brief Get free disk space limit
	 * @jsonapi{development}
	 * @return current minimum free space on disk in MB
	 */
	virtual uint32_t freeDiskSpaceLimit() const = 0;

	/**
	 * @brief Set minimum free disk space limit
	 * @jsonapi{development}
	 * @param[in] minimumFreeMB minimum free space in MB
	 */
	virtual void setFreeDiskSpaceLimit(uint32_t minimumFreeMB) = 0;

	/**
	 * @brief Controls file transfer
	 * @jsonapi{development}
	 * @param[in] hash file identifier
	 * @param[in] flags action to perform. Pict into { RS_FILE_CTRL_PAUSE, RS_FILE_CTRL_START, RS_FILE_CTRL_FORCE_CHECK } }
	 * @return false if error occured such as unknown hash.
	 */
	virtual bool FileControl(const RsFileHash& hash, uint32_t flags) = 0;

	/**
	 * @brief Clear completed downloaded files list
	 * @jsonapi{development}
	 * @return false on error, true otherwise
	 */
	virtual bool FileClearCompleted() = 0;

		virtual void setDefaultEncryptionPolicy(uint32_t policy)=0;	// RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT/PERMISSIVE
		virtual uint32_t defaultEncryptionPolicy()=0;
		virtual void setMaxUploadSlotsPerFriend(uint32_t n)=0;
		virtual uint32_t getMaxUploadSlotsPerFriend()=0;
		virtual void setFilePermDirectDL(uint32_t perm)=0;
		virtual uint32_t filePermDirectDL()=0;

	/**
	 * @brief Request remote files search
	 * @jsonapi{development}
	 * @param[in] matchString string to look for in the search. If files deep
	 *	indexing is enabled at compile time support advanced features described
	 *	at https://xapian.org/docs/queryparser.html
	 * @param multiCallback function that will be called each time a search
	 * result is received
	 * @param[in] maxWait maximum wait time in seconds for search results
	 * @return false on error, true otherwise
	 */
	virtual bool turtleSearchRequest(
	        const std::string& matchString,
	        const std::function<void (const std::vector<TurtleFileInfoV2>& results)>& multiCallback,
	        rstime_t maxWait = 300 ) = 0;

	virtual TurtleRequestId turtleSearch(const std::string& string_to_match) = 0;
	virtual TurtleRequestId turtleSearch(
	        const RsRegularExpression::LinearizedExpression& expr) = 0;

		/***
		 * Control of Downloads Priority.
		 ***/
		virtual uint32_t getQueueSize() = 0 ;
		virtual void setQueueSize(uint32_t s) = 0 ;
		virtual bool changeQueuePosition(const RsFileHash& hash, QueueMove mv) = 0;
		virtual bool changeDownloadSpeed(const RsFileHash& hash, int speed) = 0;
		virtual bool getDownloadSpeed(const RsFileHash& hash, int & speed) = 0;
		virtual bool clearDownload(const RsFileHash& hash) = 0;

	/**
	 * @brief Get incoming files list
	 * @jsonapi{development}
	 * @param[out] hashs storage for files identifiers list
	 */
	virtual void FileDownloads(std::list<RsFileHash>& hashs) = 0;

	/**
	 * @brief Get outgoing files list
	 * @jsonapi{development}
	 * @param[out] hashs storage for files identifiers list
	 * @return false if some error occurred, true otherwise
	 */
	virtual bool FileUploads(std::list<RsFileHash>& hashs) = 0;

	/**
	 * @brief Get file details
	 * @jsonapi{development}
	 * @param[in] hash file identifier
	 * @param[in] hintflags filtering hint (RS_FILE_HINTS_EXTRA|...|RS_FILE_HINTS_LOCAL)
	 * @param[out] info storage for file information
	 * @return true if file found, false otherwise
	 */
	virtual bool FileDetails(
	        const RsFileHash& hash, FileSearchFlags hintflags, FileInfo& info ) = 0;

        virtual bool isEncryptedSource(const RsPeerId& virtual_peer_id) =0;

	/**
	 * @brief Get chunk details about the downloaded file with given hash.
	 * @jsonapi{development}
	 * @param[in] hash file identifier
	 * @param[out] info storage for file information
	 * @return true if file found, false otherwise
	 */
	virtual bool FileDownloadChunksDetails(
	        const RsFileHash& hash, FileChunksInfo& info) = 0;

	/**
	 * @brief Get details about the upload with given hash
	 * @jsonapi{development}
	 * @param[in] hash file identifier
	 * @param[in] peerId peer identifier
	 * @param[out] map storage for chunk info
	 * @return true if file found, false otherwise
	 */
	virtual bool FileUploadChunksDetails(
	        const RsFileHash& hash, const RsPeerId& peerId,
	        CompressedChunkMap& map ) = 0;

	/**
	 * @brief Remove file from extra fila shared list
	 * @jsonapi{development}
	 * @param[in] hash hash of the file to remove
	 * @return return false on error, true otherwise
	 */
	virtual bool ExtraFileRemove(const RsFileHash& hash) = 0;

	/**
	 * @brief Add file to extra shared file list
	 * @jsonapi{development}
	 * @param[in] localpath path of the file
	 * @param[in] period how much time the file will be kept in extra list in
	 *                   seconds
	 * @param[in] flags sharing policy flags ex: RS_FILE_REQ_ANONYMOUS_ROUTING
	 * @return false on error, true otherwise
	 */
	virtual bool ExtraFileHash(
	        std::string localpath, rstime_t period, TransferRequestFlags flags
	        ) = 0;

	/**
	 * @brief Get extra file information
	 * @jsonapi{development}
	 * @param[in] localpath path of the file
	 * @param[out] info storage for the file information
	 * @return false on error, true otherwise
	 */
	virtual bool ExtraFileStatus(std::string localpath, FileInfo &info) = 0;

	virtual bool ExtraFileMove(
	        std::string fname, const RsFileHash& hash, uint64_t size,
	        std::string destpath ) = 0;

	/**
	 * @brief Request directory details, subsequent multiple call may be used to
	 * explore a whole directory tree.
	 * @jsonapi{development}
	 * @param[out] details Storage for directory details
	 * @param[in] handle element handle 0 for root, pass the content of
	 *	DirDetails::child[x].ref after first call to explore deeper, be aware
	 *	that is not a real pointer but an index used internally by RetroShare.
	 * @param[in] flags file search flags RS_FILE_HINTS_*
	 * @return false if error occurred, true otherwise
	 */
	virtual bool requestDirDetails(
	        DirDetails &details, std::uintptr_t handle = 0,
	        FileSearchFlags flags = RS_FILE_HINTS_LOCAL ) = 0;

	/***
	 * Directory Listing / Search Interface
	 */
	/**
	 * Kept for retrocompatibility, it was originally written for easier
	 * interaction with Qt. As soon as you can, you should prefer to use the
	 * version of this methodn which take `std::uintptr_t handle` as paramether.
	 */
	virtual int RequestDirDetails(
	        void* handle, DirDetails& details, FileSearchFlags flags ) = 0;

        virtual bool findChildPointer(void *ref, int row, void *& result, FileSearchFlags flags) =0;
        virtual uint32_t getType(void *ref,FileSearchFlags flags) = 0;

		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags) = 0;
		virtual int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id) = 0;
        virtual int SearchBoolExp(RsRegularExpression::Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags) = 0;
        virtual int SearchBoolExp(RsRegularExpression::Expression * exp, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id) = 0;
		virtual int getSharedDirStatistics(const RsPeerId& pid, SharedDirStats& stats) =0;

	/**
	 * @brief Ban unwanted file from being, searched and forwarded by this node
	 * @jsonapi{development}
	 * @param[in] realFileHash this is what will really enforce banning
	 * @param[in] filename expected name of the file, for the user to read
	 * @param[in] fileSize expected file size, for the user to read
	 * @return meaningless value
	 */
	virtual int banFile( const RsFileHash& realFileHash,
	                     const std::string& filename, uint64_t fileSize ) = 0;

	/**
	 * @brief Remove file from unwanted list
	 * @jsonapi{development}
	 * @param[in] realFileHash hash of the file
	 * @return meaningless value
	 */
	virtual int unbanFile(const RsFileHash& realFileHash) = 0;

	/**
	 * @brief Get list of banned files
	 * @jsonapi{development}
	 * @param[out] bannedFiles storage for banned files information
	 * @return meaningless value
	 */
	virtual bool getPrimaryBannedFilesList(
	        std::map<RsFileHash,BannedFileEntry>& bannedFiles ) = 0;

	/**
	 * @brief Check if a file is on banned list
	 * @jsonapi{development}
	 * @param[in] hash hash of the file
	 * @return true if the hash is on the list, false otherwise
	 */
	virtual bool isHashBanned(const RsFileHash& hash) = 0;

		/***
		 * Utility Functions.
		 ***/
		virtual bool ConvertSharedFilePath(std::string path, std::string &fullpath) = 0;

	/**
	 * @brief Force shared directories check
	 * @param[in] add_safe_delay Schedule the check 20 seconds from now, to ensure to capture files written just now.
	 * @jsonapi{development}
	 */
	virtual void ForceDirectoryCheck(bool add_safe_delay=false) = 0;

	virtual void updateSinceGroupPermissionsChanged() = 0;
		virtual bool InDirectoryCheck() = 0;
		virtual bool copyFile(const std::string& source,const std::string& dest) = 0;

		/***
		 * Directory Control
		 ***/
        virtual void requestDirUpdate(void *ref) =0 ;			// triggers the update of the given reference. Used when browsing.

	/**
	 * @brief Set default complete downloads directory
	 * @jsonapi{development}
	 * @param[in] path directory path
	 * @return false if something failed, true otherwhise
	 */
	virtual bool setDownloadDirectory(const std::string& path) = 0;

	/**
	 * @brief Set partial downloads directory
	 * @jsonapi{development}
	 * @param[in] path directory path
	 * @return false if something failed, true otherwhise
	 */
	virtual bool setPartialsDirectory(const std::string& path) = 0;

	/**
	 * @brief Get default complete downloads directory
	 * @jsonapi{development}
	 * @return default completed downloads directory path
	 */
	virtual std::string getDownloadDirectory() = 0;

	/**
	 * @brief Get partial downloads directory
	 * @jsonapi{development}
	 * @return partials downloads directory path
	 */
	virtual std::string getPartialsDirectory() = 0;

	/**
	 * @brief Get list of current shared directories
	 * @jsonapi{development}
	 * @param[out] dirs storage for the list of share directories
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getSharedDirectories(std::list<SharedDirInfo>& dirs) = 0;

	/**
	 * @brief Set shared directories
	 * @jsonapi{development}
	 * @param[in] dirs list of shared directories with share options
	 * @return false if something failed, true otherwhise
	 */
	virtual bool setSharedDirectories(const std::list<SharedDirInfo>& dirs) = 0;

	/**
	 * @brief Add shared directory
	 * @jsonapi{development}
	 * @param[in] dir directory to share with sharing options
	 * @return false if something failed, true otherwhise
	 */
	virtual bool addSharedDirectory(const SharedDirInfo& dir) = 0;

	/**
	 * @brief Updates shared directory sharing flags.
	 * The directory should be already shared!
	 * @jsonapi{development}
	 * @param[in] dir Shared directory with updated sharing options
	 * @return false if something failed, true otherwhise
	 */
	virtual bool updateShareFlags(const SharedDirInfo& dir) = 0;

	/**
	 * @brief Remove directory from shared list
	 * @jsonapi{development}
	 * @param[in] dir Path of the directory to remove from shared list
	 * @return false if something failed, true otherwhise
	 */
	virtual bool removeSharedDirectory(std::string dir) = 0;

	/**
	 * @brief Get list of ignored file name prefixes and suffixes
	 * @param[out] ignoredPrefixes storage for ingored prefixes
	 * @param[out] ignoredSuffixes storage for ingored suffixes
	 * @param flags RS_FILE_SHARE_FLAGS_IGNORE_*
	 * @return false if something failed, true otherwhise
	 */
	virtual bool getIgnoreLists(
	        std::list<std::string>& ignoredPrefixes,
	        std::list<std::string>& ignoredSuffixes,
	        uint32_t& flags ) = 0;

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
