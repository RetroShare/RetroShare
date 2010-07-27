#ifndef RS_TYPES_GUI_INTERFACE_H
#define RS_TYPES_GUI_INTERFACE_H

/*
 * "$Id: rstypes.h,v 1.7 2007-05-05 16:10:05 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2006 by Robert Fernie.
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
#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <stdint.h>

typedef std::string   RsCertId;
typedef std::string   RsChanId;
typedef std::string   RsMsgId;
typedef std::string   RsAuthId;

const uint32_t FT_STATE_FAILED			= 0x0000 ;
const uint32_t FT_STATE_OKAY				= 0x0001 ;
const uint32_t FT_STATE_WAITING 			= 0x0002 ;
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

class TransferInfo
{
	public:
		/**** Need Some of these Fields ****/
		std::string peerId;
		std::string name; /* if has alternative name? */
		double tfRate; /* kbytes */
		int  status; /* FT_STATE_... */
		uint64_t transfered ; // used when no chunkmap data is available
};

enum QueueMove { 	QUEUE_TOP 	 = 0x00, 
						QUEUE_UP  	 = 0x01, 
						QUEUE_DOWN	 = 0x02, 
						QUEUE_BOTTOM = 0x03
};

enum DwlSpeed 		{ 	SPEED_LOW 		= 0x00, 
							SPEED_NORMAL 	= 0x01, 
							SPEED_HIGH		= 0x02
};



class FileInfo
{
	/* old BaseInfo Entries */
	public:

		FileInfo() :flags(0), mId(0) { return; }
		RsCertId id; /* key for matching everything */
		int flags; /* INFO_TAG above */

		/* allow this to be tweaked by the GUI Model */
		mutable unsigned int mId; /* (GUI) Model Id -> unique number */

		/* Old FileInfo Entries */
	public:

		static const int kRsFiStatusNone = 0;
		static const int kRsFiStatusStall = 1;
		static const int kRsFiStatusProgress = 2;
		static const int kRsFiStatusDone = 2;

		/* FileInfo(); */

		int searchId;      /* 0 if none */
		std::string path;
		std::string fname;
		std::string hash;
		std::string ext;

		uint64_t size;
		uint64_t avail; /* how much we have */
		int status;

		bool inRecommend;

		double rank;
		int age;
		uint32_t queue_position ;

		/* Transfer Stuff */
		uint64_t transfered;
		double   tfRate; /* in kbytes */
		uint32_t  downloadStatus; /* 0 = Err, 1 = Ok, 2 = Done */
		std::list<TransferInfo> peers;

		DwlSpeed priority ;
		time_t lastTS;
};

std::ostream &operator<<(std::ostream &out, const FileInfo &info);


/* matched to the uPnP states */
#define UPNP_STATE_UNINITIALISED  0
#define UPNP_STATE_UNAVAILABILE   1
#define UPNP_STATE_READY          2
#define UPNP_STATE_FAILED_TCP     3
#define UPNP_STATE_FAILED_UDP     4
#define UPNP_STATE_ACTIVE         5

class RsConfig
{
	public:
		RsConfig()
		{
			localPort = extPort = 0 ;
			firewalled = forwardPort = false ;
			maxDownloadDataRate = maxUploadDataRate = maxIndivDataRate = 0 ;
			promptAtBoot = 0 ;
			DHTActive = uPnPActive = netLocalOk = netUpnpOk = netDhtOk = netStunOk = netExtraAddressOk = false ;
			uPnPState = DHTPeers = 0 ;
		}
		std::string		ownId;
		std::string		ownName;

		std::string		localAddr;
		int			localPort;
		std::string		extAddr;
		int			extPort;
		std::string		extName;

		bool			firewalled;
		bool			forwardPort;

		int			maxDownloadDataRate;     /* kb */
		int			maxUploadDataRate;     /* kb */
		int			maxIndivDataRate; /* kb */

		int			promptAtBoot; /* popup the password prompt */

		/* older data types */
		bool			DHTActive;
		bool			uPnPActive;

		int			uPnPState;
		int			DHTPeers;

		/* Flags for Network Status */
		bool 			netLocalOk;     /* That we've talked to someone! */
		bool			netUpnpOk; /* upnp is enabled and active */
		bool			netDhtOk;  /* response from dht */
		bool			netStunOk;  /* recvd stun / udp packets */
		bool			netExtraAddressOk;  /* recvd ip address with external finder*/
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

class SearchRequest
{
	public:
	int searchId;
	RsCertId toId;  /* all zeros for everyone! */
	std::list<Condition> tests;
};


/********************** For FileCache Interface *****************/

#define DIR_TYPE_ROOT		0x01
#define DIR_TYPE_PERSON  	0x02
#define DIR_TYPE_DIR  		0x04
#define DIR_TYPE_FILE 		0x08

/* flags for Directry request -
 * two types;
 * (1) Local / Remote (top byte)
 * (2) Request type: Parent / Child - allows reduction in workload.
 *     (TODO)
 */

#define DIR_FLAGS_LOCAL         0x1000
#define DIR_FLAGS_REMOTE        0x2000

#define DIR_FLAGS_PARENT        0x0001
#define DIR_FLAGS_DETAILS       0x0002
#define DIR_FLAGS_CHILDREN      0x0004
#define DIR_FLAGS_NETWORK_WIDE  0x0008
#define DIR_FLAGS_BROWSABLE     0x0010

class DirStub
{
	public:
	uint8_t type;
	std::string name;
	void *ref;
};

class DirDetails
{
	public:
	void *parent;
	int prow; /* parent row */

	void *ref;
	uint8_t type;
	std::string id;
	std::string name;
	std::string hash;
	std::string path;
	uint64_t count;
	uint32_t age;
	uint32_t flags;
	uint32_t min_age ;	// minimum age of files in this subtree

	std::list<DirStub> children;
};

class FileDetail
{
	public:
	std::string id;
	std::string name;
	std::string hash;
	std::string path;
	uint64_t size;
	uint32_t age;
	uint32_t rank;
};

class CompressedChunkMap ;

class FileChunksInfo
{
	public:
		enum ChunkState { CHUNK_DONE, CHUNK_ACTIVE, CHUNK_OUTSTANDING } ;
		enum ChunkStrategy { CHUNK_STRATEGY_STREAMING, CHUNK_STRATEGY_RANDOM } ;

		uint64_t file_size ;					// real size of the file
		uint32_t chunk_size ;				// size of chunks
		uint32_t flags ;
		uint32_t strategy ;

		// dl state of chunks. Only the last chunk may have size < chunk_size
		std::vector<ChunkState> chunks ;	

		// For each source peer, gives the compressed bit map of have/don't have sate
		std::map<std::string, CompressedChunkMap> compressed_peer_availability_maps ;

		// For each chunk (by chunk number), gives the completion of the chunk.
		//                     
		std::vector<std::pair<uint32_t,uint32_t> > active_chunks ;
};

class CompressedChunkMap
{
	public:
		CompressedChunkMap() {}

		CompressedChunkMap(const std::vector<FileChunksInfo::ChunkState>& uncompressed_data)
		{
			_map.resize( getCompressedSize(uncompressed_data.size()),0 ) ;

			for(uint32_t i=0;i<uncompressed_data.size();++i)
				if(uncompressed_data[i]==FileChunksInfo::CHUNK_DONE) 
					set(i) ;
		}

		CompressedChunkMap(uint32_t nb_chunks,uint32_t value)
		{
			_map.resize(getCompressedSize(nb_chunks),value) ;
		}

		static uint32_t getCompressedSize(uint32_t size) { return (size>>5) + !!(size&31) ; }

		uint32_t filledChunks(uint32_t nbchks)
		{
			uint32_t res = 0 ;
			for(uint32_t i=0;i<std::min(nbchks,(uint32_t)_map.size()*32);++i)
				res += operator[](i) ;
			return res ;
		}
		inline bool operator[](uint32_t i) const { return (_map[i >> 5] & (1 << (i & 31))) > 0 ; }

		inline void   set(uint32_t j) { _map[j >> 5] |=  (1 << (j & 31)) ; }
		inline void reset(uint32_t j) { _map[j >> 5] &= ~(1 << (j & 31)) ; }

		/// compressed map, one bit per chunk
		std::vector<uint32_t> _map ;
};

class CRC32Map
{
	public:
		// Build from a file.
		//
		CRC32Map(uint64_t file_size,uint32_t chunk_size) 
			: _crcs( file_size/chunk_size + ( (file_size%chunk_size)>0)), _map(file_size/chunk_size + ( (file_size%chunk_size)>0),0)
		{
		}
		CRC32Map() {}

		// Compares two maps and returns the valid chunks in a compressed chunk map.
		//
		friend CompressedChunkMap compare(const CRC32Map& crc1,const CRC32Map& crc2) ;

		void set(uint32_t i,uint32_t val) { _crcs[i] = val ; _map.set(i) ; }

		uint32_t operator[](int i) const { return _crcs[i] ; }
		uint32_t size() const { return _crcs.size() ; }
	private:
		std::vector<uint32_t> _crcs;
		CompressedChunkMap _map ;

		friend class RsTurtleFileCrcItem ;
};

/* class which encapsulates download details */
class DwlDetails {
public:
	DwlDetails() { return; }
	DwlDetails(std::string fname, std::string hash, int count, std::string dest,
			uint32_t flags, std::list<std::string> srcIds, uint32_t queue_pos)
	: fname(fname), hash(hash), count(count), dest(dest), flags(flags),
	srcIds(srcIds), queue_position(queue_pos), retries(0) { return; }

	/* download details */
	std::string fname;
	std::string hash;
	int count;
	std::string dest;
	uint32_t flags;
	std::list<std::string> srcIds;

	/* internally used in download queue */
	uint32_t queue_position;

	/* how many times a failed dwl will be requeued */
	unsigned int retries;
};

#endif
