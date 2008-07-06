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
#include <iostream>
#include <string>


typedef std::string   RsCertId;
typedef std::string   RsChanId; 
typedef std::string   RsMsgId; 
typedef std::string   RsAuthId; 


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

	/* Old FileTransferInfo Entries */
	public:
	std::string source;
	std::list<std::string> peerIds;
	int transfered;
	double tfRate; /* kbytes */
	bool download;
	int  downloadStatus; /* 0 = Err, 1 = Ok, 2 = Done */

	/* ENTRIES USED BY SFI ***
	 *
	 * path,
	 * fname, 
	 * hash, 
	 * size, 
	 * avail, 
	 *
	 * source?
	 *
	 */
	

};

class FileTransferInfo: public FileInfo
{
	public:
	FileTransferInfo() { return; }
};

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
	std::string		ownId;
	std::string		ownName;

	std::string		localAddr;
	int			localPort;
	std::string		extAddr;
	int			extPort;
	std::string		extName;

	bool			firewalled;
	bool			forwardPort;

	int			maxDataRate;     /* kb */
	int			maxIndivDataRate; /* kb */

	int			promptAtBoot; /* popup the password prompt */

	bool			DHTActive;
	bool			uPnPActive;

	int			uPnPState;
	int			DHTPeers;
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
	uint32_t prow; /* parent row */

	void *ref;
	uint8_t type;
	std::string id;
	std::string name;
	std::string hash;
	std::string path;
	uint64_t count;
	uint32_t age;
	uint32_t rank;

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


#endif


