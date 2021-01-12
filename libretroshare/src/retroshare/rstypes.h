/*******************************************************************************
 * libretroshare/src/rsserver: rstypes.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_TYPES_GUI_INTERFACE_H
#define RS_TYPES_GUI_INTERFACE_H

#include <list>
#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <stdint.h>

#include <retroshare/rsids.h>
#include <retroshare/rsflags.h>
#include <serialiser/rsserializable.h>
#include <serialiser/rstypeserializer.h>
#include "util/rstime.h"

#define USE_NEW_CHUNK_CHECKING_CODE

typedef Sha1CheckSum  RsFileHash ;

const uint32_t FT_STATE_FAILED			= 0x0000 ;
const uint32_t FT_STATE_OKAY			= 0x0001 ;
const uint32_t FT_STATE_WAITING 		= 0x0002 ;
const uint32_t FT_STATE_DOWNLOADING		= 0x0003 ;
const uint32_t FT_STATE_COMPLETE 		= 0x0004 ;
const uint32_t FT_STATE_QUEUED   		= 0x0005 ;
const uint32_t FT_STATE_PAUSED   		= 0x0006 ;
const uint32_t FT_STATE_CHECKING_HASH	= 0x0007 ;

// These constants are used by RsDiscSpace
//
const uint32_t RS_PARTIALS_DIRECTORY = 0x0000 ;
const uint32_t RS_DOWNLOAD_DIRECTORY = 0x0001 ;
const uint32_t RS_CONFIG_DIRECTORY   = 0x0002 ;
const uint32_t RS_PGP_DIRECTORY      = 0x0003 ;
const uint32_t RS_DIRECTORY_COUNT    = 0x0004 ;

struct TransferInfo : RsSerializable
{
	/**** Need Some of these Fields ****/
	RsPeerId peerId;
	std::string name; /* if has alternative name? */
	double tfRate; /* kbytes */
	int status; /* FT_STATE_... */
	uint64_t transfered ; // used when no chunkmap data is available

	/// @see RsSerializable
	void serial_process(RsGenericSerializer::SerializeJob j,
	                    RsGenericSerializer::SerializeContext& ctx)
	{
		RS_SERIAL_PROCESS(peerId);
		RS_SERIAL_PROCESS(name);
		RS_SERIAL_PROCESS(tfRate);
		RS_SERIAL_PROCESS(status);
		RS_SERIAL_PROCESS(transfered);
	}
};

enum QueueMove { 	QUEUE_TOP 	 = 0x00, 
						QUEUE_UP  	 = 0x01, 
						QUEUE_DOWN	 = 0x02, 
						QUEUE_BOTTOM = 0x03
};

enum DwlSpeed : uint8_t
{
	SPEED_LOW    = 0x00,
	SPEED_NORMAL = 0x01,
	SPEED_HIGH   = 0x02
};




/********************** For Search Interface *****************/

/* This is still rough, implement later! */

	/* text based ones */
const std::string TypeExt  = "ext";
const std::string TypeName = "name";
const std::string TypeHash = "hash";
const std::string TypeSize = "size";

const int OpContains    = 0x001;
const int OpExactMatch  = 0x002;
const int OpLessThan    = 0x003;
const int OpGreaterThan = 0x004;

class Condition
{
	public:

	std::string type;
	int op;
	double value;
	std::string name;
};

struct PeerBandwidthLimits : RsSerializable
{
	PeerBandwidthLimits() : max_up_rate_kbs(0), max_dl_rate_kbs(0) {}

	uint32_t max_up_rate_kbs;
	uint32_t max_dl_rate_kbs;


	/// @see RsSerializable
	void serial_process(RsGenericSerializer::SerializeJob j,
	                    RsGenericSerializer::SerializeContext& ctx)
	{
		RS_SERIAL_PROCESS(max_up_rate_kbs);
		RS_SERIAL_PROCESS(max_dl_rate_kbs);
	}
};

//class SearchRequest // unused stuff.
//{
//	public:
//	int searchId;
//	RsCertId toId;  /* all zeros for everyone! */
//	std::list<Condition> tests;
//};


/********************** For FileCache Interface *****************/

#define DIR_TYPE_UNKNOWN	    0x00
#define DIR_TYPE_ROOT		0x01
#define DIR_TYPE_PERSON  	0x02
#define DIR_TYPE_DIR  		0x04
#define DIR_TYPE_FILE 		0x08
#define DIR_TYPE_EXTRA_FILE 0x10

/* flags for Directry request -
 * two types;
 * (1) Local / Remote (top byte)
 * (2) Request type: Parent / Child - allows reduction in workload.
 *     (TODO)
 */

const FileStorageFlags DIR_FLAGS_PARENT                 	( 0x0001 );
const FileStorageFlags DIR_FLAGS_DETAILS                	( 0x0002 );	// apparently unused
const FileStorageFlags DIR_FLAGS_CHILDREN               	( 0x0004 );	// apparently unused

const FileStorageFlags DIR_FLAGS_ANONYMOUS_DOWNLOAD  	( 0x0080 ); // Flags for directory sharing permissions. The last
//const FileStorageFlags DIR_FLAGS_BROWSABLE_OTHERS     	( 0x0100 ); // one should be the OR of the all four flags.
//const FileStorageFlags DIR_FLAGS_NETWORK_WIDE_GROUPS  	( 0x0200 );
const FileStorageFlags DIR_FLAGS_BROWSABLE     	        ( 0x0400 );
const FileStorageFlags DIR_FLAGS_ANONYMOUS_SEARCH       ( 0x0800 );
const FileStorageFlags DIR_FLAGS_PERMISSIONS_MASK     	( DIR_FLAGS_ANONYMOUS_DOWNLOAD | /*DIR_FLAGS_BROWSABLE_OTHERS
                                                          DIR_FLAGS_NETWORK_WIDE_GROUPS*/   DIR_FLAGS_BROWSABLE | DIR_FLAGS_ANONYMOUS_SEARCH);

const FileStorageFlags DIR_FLAGS_LOCAL                  	( 0x1000 );
const FileStorageFlags DIR_FLAGS_REMOTE                 	( 0x2000 );

struct FileInfo : RsSerializable
{
	FileInfo():
	    mId(0), searchId(0), size(0), avail(0), rank(0), age(0),
	    queue_position(0), transfered(0), tfRate(0), downloadStatus(0),
	    priority(SPEED_NORMAL), lastTS(0) {}

	/// Combination of the four RS_DIR_FLAGS_*. Updated when the file is a local stored file.
	FileStorageFlags storage_permission_flags;

	/// various flags from RS_FILE_HINTS_*
	TransferRequestFlags transfer_info_flags;

	/** allow this to be tweaked by the GUI Model
	 * (GUI) Model Id -> unique number
	 */
	mutable unsigned int mId;


	/// 0 if none
	int searchId;

	std::string path;
	std::string fname;
	RsFileHash hash;

	RS_DEPRECATED std::string ext; /// @deprecated unused

	uint64_t size;
	uint64_t avail; /// how much we have

	double rank;
	int age;
	uint32_t queue_position;

	/* Transfer Stuff */
	uint64_t transfered;
	double tfRate; /// in kbytes
	uint32_t downloadStatus; /// FT_STATE_DOWNLOADING & co. See rstypes.h
	std::vector<TransferInfo> peers;

	DwlSpeed priority;
	rstime_t lastTS;

	std::list<RsNodeGroupId> parent_groups;

	/// @see RsSerializable
	void serial_process(RsGenericSerializer::SerializeJob j,
	                    RsGenericSerializer::SerializeContext& ctx)
	{
		RS_SERIAL_PROCESS(storage_permission_flags);
		RS_SERIAL_PROCESS(transfer_info_flags);
		RS_SERIAL_PROCESS(mId);
		RS_SERIAL_PROCESS(searchId);
		RS_SERIAL_PROCESS(path);
		RS_SERIAL_PROCESS(fname);
		RS_SERIAL_PROCESS(hash);
		RS_SERIAL_PROCESS(ext);
		RS_SERIAL_PROCESS(size);
		RS_SERIAL_PROCESS(avail);
		RS_SERIAL_PROCESS(rank);
		RS_SERIAL_PROCESS(age);
		RS_SERIAL_PROCESS(queue_position);
		RS_SERIAL_PROCESS(transfered);
		RS_SERIAL_PROCESS(tfRate);
		RS_SERIAL_PROCESS(downloadStatus);
		RS_SERIAL_PROCESS(peers);
		RS_SERIAL_PROCESS(priority);
		RS_SERIAL_PROCESS(lastTS);
		RS_SERIAL_PROCESS(parent_groups);
	}
};

/**
 * Pointers in this class have no real meaning as pointers, they are used as
 * indexes, internally by retroshare.
 */
struct DirStub : RsSerializable
{
	DirStub() : type(DIR_TYPE_UNKNOWN), ref(nullptr) {}

	uint8_t type;
	std::string name;
	void *ref;

	/// @see RsSerializable
	void serial_process(RsGenericSerializer::SerializeJob j,
	                    RsGenericSerializer::SerializeContext& ctx)
	{
		RS_SERIAL_PROCESS(type);
		RS_SERIAL_PROCESS(name);

#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif // defined(__GNUC__) && !defined(__clang__)
		// (Cyril) We have to do this because on some systems (MacOS) uintptr_t is unsigned long which is not well defined. It is always
        // preferable to force type serialization to the correct size rather than letting the compiler choose for us.
        // /!\ This structure cannot be sent over the network. The serialization would be inconsistent.

		if(sizeof(ref) == 4)
		{
			std::uint32_t& handle(reinterpret_cast<std::uint32_t&>(ref));
			RS_SERIAL_PROCESS(handle);
		}
		else if(sizeof(ref) == 8)
		{
			std::uint64_t& handle(reinterpret_cast<std::uint64_t&>(ref));
			RS_SERIAL_PROCESS(handle);
		}
		else
			std::cerr << __PRETTY_FUNCTION__ << ": cannot serialize raw pointer of size " << sizeof(ref) << std::endl;

#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic pop
#endif // defined(__GNUC__) && !defined(__clang__)
	}
};

/**
 * Pointers in this class have no real meaning as pointers, they are used as
 * indexes, internally by retroshare.
 */
struct DirDetails : RsSerializable
{
	DirDetails() : parent(nullptr), prow(0), ref(nullptr),
        type(DIR_TYPE_UNKNOWN), size(0), mtime(0), max_mtime(0) {}


	/* G10h4ck do we still need to keep this as void* instead of uint64_t for
	 * retroshare-gui sake? */
	void* parent;

	int prow; /* parent row */

	/* G10h4ck do we still need to keep this as void* instead of uint64_t for
	 * retroshare-gui sake? */
	void* ref;

    uint8_t type;
    RsPeerId id;
    std::string name;
    RsFileHash hash;
    std::string path;		// full path of the parent directory, when it is a file; full path of the dir otherwise.
    uint64_t size;          // total size of directory, or size of the file.
	uint32_t mtime;			// file/directory modification time, according to what the system reports
	FileStorageFlags flags;
	uint32_t max_mtime ;	// maximum modification time of the whole hierarchy below.

    std::vector<DirStub> children;
    std::list<RsNodeGroupId> parent_groups;	// parent groups for the shared directory

	/// @see RsSerializable
	void serial_process(RsGenericSerializer::SerializeJob j,
	                    RsGenericSerializer::SerializeContext& ctx) override
	{
		/* Enforce serialization as uint64_t because void* changes size
		 * depending (usually 4 bytes on 32bit arch and 8 bytes on 64bit archs)
		 */
		uint64_t handle = reinterpret_cast<uint64_t>(ref);
		RS_SERIAL_PROCESS(handle);
		ref = reinterpret_cast<void*>(handle);

		uint64_t parentHandle = reinterpret_cast<uint64_t>(parent);
		RS_SERIAL_PROCESS(parentHandle);
		parent = reinterpret_cast<void*>(parentHandle);

		RS_SERIAL_PROCESS(prow);
		RS_SERIAL_PROCESS(type);
		RS_SERIAL_PROCESS(id);
		RS_SERIAL_PROCESS(name);
		RS_SERIAL_PROCESS(hash);
		RS_SERIAL_PROCESS(path);
        RS_SERIAL_PROCESS(size);
		RS_SERIAL_PROCESS(mtime);
		RS_SERIAL_PROCESS(flags);
		RS_SERIAL_PROCESS(max_mtime);
		RS_SERIAL_PROCESS(children);
		RS_SERIAL_PROCESS(parent_groups);
	}

	~DirDetails() override = default;
};

class FileDetail
{
	public:
        RsPeerId id;
		std::string name;
        RsFileHash hash;
		std::string path;
		uint64_t size;
		uint32_t age;
		uint32_t rank;
};

class CompressedChunkMap ;

struct FileChunksInfo : RsSerializable
{
	enum ChunkState : uint8_t
	{
		CHUNK_OUTSTANDING = 0,
		CHUNK_ACTIVE = 1,
		CHUNK_DONE = 2,
		CHUNK_CHECKING = 3
	};

	enum ChunkStrategy : uint8_t
	{
		CHUNK_STRATEGY_STREAMING,
		CHUNK_STRATEGY_RANDOM,
		CHUNK_STRATEGY_PROGRESSIVE
	};

	struct SliceInfo : RsSerializable
	{
		uint32_t start;
		uint32_t size;
		RsPeerId peer_id;

		/// @see RsSerializable
		void serial_process(RsGenericSerializer::SerializeJob j,
		                    RsGenericSerializer::SerializeContext& ctx) override
		{
			RS_SERIAL_PROCESS(start);
			RS_SERIAL_PROCESS(size);
			RS_SERIAL_PROCESS(peer_id);
		}
	};

	uint64_t file_size; /// real size of the file
	uint32_t chunk_size; /// size of chunks
	ChunkStrategy strategy;

	/// dl state of chunks. Only the last chunk may have size < chunk_size
	std::vector<ChunkState> chunks;

	/// For each source peer, gives the compressed bit map of have/don't have sate
	std::map<RsPeerId, CompressedChunkMap> compressed_peer_availability_maps;

	/// For each chunk (by chunk number), gives the completion of the chunk.
	std::vector<std::pair<uint32_t,uint32_t> > active_chunks;

	/// The list of pending requests, chunk per chunk (by chunk id)
	std::map<uint32_t, std::vector<SliceInfo> > pending_slices;

	/// @see RsSerializable
	void serial_process(RsGenericSerializer::SerializeJob j,
	                    RsGenericSerializer::SerializeContext& ctx)
	{
		RS_SERIAL_PROCESS(file_size);
		RS_SERIAL_PROCESS(chunk_size);
		RS_SERIAL_PROCESS(strategy);
		RS_SERIAL_PROCESS(chunks);
		RS_SERIAL_PROCESS(compressed_peer_availability_maps);
		RS_SERIAL_PROCESS(active_chunks);
		RS_SERIAL_PROCESS(pending_slices);
	}
};

class CompressedChunkMap : public RsSerializable
{
	public:
		CompressedChunkMap() {}

		CompressedChunkMap(const std::vector<FileChunksInfo::ChunkState>& uncompressed_data)
		{
			_map.resize( getCompressedSize(uncompressed_data.size()),0 ) ;

			for(uint32_t i=0;i<uncompressed_data.size();++i)
#ifdef USE_NEW_CHUNK_CHECKING_CODE
				if(uncompressed_data[i]==FileChunksInfo::CHUNK_DONE) 
#else
				if(uncompressed_data[i]==FileChunksInfo::CHUNK_DONE || uncompressed_data[i] == FileChunksInfo::CHUNK_CHECKING) 
#endif
					set(i) ;
		}

		CompressedChunkMap(uint32_t nb_chunks,uint32_t value)
		{
			_map.resize(getCompressedSize(nb_chunks),value) ;
		}

		static uint32_t getCompressedSize(uint32_t size) { return (size>>5) + !!(size&31) ; }

		uint32_t filledChunks(uint32_t nbchks) const
		{
			uint32_t res = 0 ;
			for(uint32_t i=0;i<std::min(nbchks,(uint32_t)_map.size()*32);++i)
				res += operator[](i) ;
			return res ;
		}
		uint64_t computeProgress(uint64_t total_size,uint32_t chunk_size) const
		{
			if(total_size == 0)
				return 0 ;

			uint32_t nbchks = (uint32_t)((total_size + (uint64_t)chunk_size - 1) / (uint64_t)chunk_size) ;
			uint32_t residue = total_size%chunk_size ;

			if(residue && operator[](nbchks-1))
				return (filledChunks(nbchks)-1)*(uint64_t)chunk_size + (total_size%chunk_size) ;
			else
				return filledChunks(nbchks)*(uint64_t)chunk_size ;
		}
		inline bool operator[](uint32_t i) const { return (_map[i >> 5] & (1 << (i & 31))) > 0 ; }

		inline void   set(uint32_t j) { _map[j >> 5] |=  (1 << (j & 31)) ; }
		inline void reset(uint32_t j) { _map[j >> 5] &= ~(1 << (j & 31)) ; }

	/// compressed map, one bit per chunk
	std::vector<uint32_t> _map;

	/// @see RsSerializable
	void serial_process(RsGenericSerializer::SerializeJob j,
	                    RsGenericSerializer::SerializeContext& ctx)
	{ RS_SERIAL_PROCESS(_map); }
};


template<class CRCTYPE> class t_CRCMap
{
	public:
		// Build from a file.
		//
		t_CRCMap(uint64_t file_size,uint32_t chunk_size) 
			: _crcs( file_size/chunk_size + ( (file_size%chunk_size)>0)), _ccmap(file_size/chunk_size + ( (file_size%chunk_size)>0),0)
		{
		}
		t_CRCMap() {}

		inline void set(uint32_t i,const CRCTYPE& val) { _crcs[i] = val ; _ccmap.set(i) ; }
		inline bool isSet(uint32_t i) const { return _ccmap[i] ; }

		inline const CRCTYPE& operator[](int i) const { return _crcs[i] ; }
		inline uint32_t size() const { return _crcs.size() ; }
	private:
		std::vector<CRCTYPE> _crcs;
		CompressedChunkMap _ccmap ;

		friend class RsTurtleFileCrcItem ;
		friend class RsFileItemSerialiser ;
		friend class RsFileCRC32Map ;
};

//typedef t_CRCMap<uint32_t> 		CRC32Map ;
typedef t_CRCMap<Sha1CheckSum> 	Sha1Map ;

/* class which encapsulates download details */
class DwlDetails {
public:
	DwlDetails() { return; }
    DwlDetails(const std::string& fname, const RsFileHash& hash, int count, std::string dest,
			uint32_t flags, std::list<std::string> srcIds, uint32_t queue_pos)
	: fname(fname), hash(hash), count(count), dest(dest), flags(flags),
	srcIds(srcIds), queue_position(queue_pos), retries(0) { return; }

	/* download details */
	std::string fname;
    RsFileHash hash;
	int count;
	std::string dest;
	uint32_t flags;
	std::list<std::string> srcIds;

	/* internally used in download queue */
	uint32_t queue_position;

	/* how many times a failed dwl will be requeued */
	unsigned int retries;
};

/* class for the information about a used library */
class RsLibraryInfo
{
public:
	RsLibraryInfo() {}
	RsLibraryInfo(const std::string &name, const std::string &version) :
	    mName(name), mVersion(version)
	{}

public:
	std::string mName;
	std::string mVersion;
};

#endif
