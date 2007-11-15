/*
 * "$Id: pqi_base.h,v 1.18 2007-05-05 16:10:05 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
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



#ifndef PQI_BASE_ITEM_HEADER
#define PQI_BASE_ITEM_HEADER

#include <list>
#include <string>
#include <iostream>
#include <functional>
#include <algorithm>

#include "pqi/pqinetwork.h"


#define PQI_MIN_PORT 1024
#define PQI_MAX_PORT 16000
#define PQI_DEFAULT_PORT 7812

int getPQIsearchId();
int fixme(char *str, int n);

struct chan_id
{
	int route[10];
};

typedef unsigned long SearchId;  /* unsigned 32 bits */
typedef struct chan_id ChanId;

/* basic worker fns on cids */
int     pqicid_clear(ChanId *cid);
int     pqicid_copy(const ChanId *cid, ChanId *newcid);
int     pqicid_cmp(const ChanId *cid1, ChanId *cid2);


// So the basic pqi interface defines
//
// 1) Person -> reference object 
// 2) PQItem 
// 3) PQInterface
// 	which defines:
// 		- send/recieve objs
// 		- send/recieve files
//
// This is then extended to
// 4) SInterface
// 	which seperates the data into providing a basic search interface.
//		chat/messages/files/search objs.
//

// TODO IN THIS CLASS.
// 1) Add serialisation.... (later)
// 2) reduce base interface to the minimum.
// 3) Convert cid -> std::stack.

// Base for all Security/Id/Certificate schemes.
// The list of these must be maintained seperately
// from the PQItems....


// Person - includes
// 1) name + signature (unique id)
// 2) address + timestamps of connections.
// 3) status 
// 4) groups.
//
// only requires certificate. to complete...

#define PERSON_STATUS_VALID     	0x0001
#define PERSON_STATUS_ACCEPTED  	0x0002
#define PERSON_STATUS_INUSE     	0x0004
#define PERSON_STATUS_LISTENING 	0x0008
#define PERSON_STATUS_CONNECTED 	0x0010
#define PERSON_STATUS_WILL_LISTEN 	0x0020
#define PERSON_STATUS_WILL_CONNECT 	0x0040

#define PERSON_STATUS_MANUAL 		0x0080
#define PERSON_STATUS_FIREWALLED 	0x0100
#define PERSON_STATUS_FORWARDED 	0x0200
#define PERSON_STATUS_LOCAL 		0x0400
#define PERSON_STATUS_TRUSTED		0x0800

/* some extra flags for advising the gui....
 * have no affect on the pqi networking.
 */

#define PERSON_STATUS_INCHAT 		0x1000
#define PERSON_STATUS_INMSG 		0x2000

class Person
{
public:

	Person();
virtual	~Person();

std::string Name();
void	Name(std::string);

	// Signature needs to be defined.
virtual std::string Signature() { return Name(); };

		// These operate on the status.
bool	Valid();
void	Valid(bool);
bool	Accepted();
void	Accepted(bool);
bool	InUse();
void	InUse(bool);
bool	Listening();
void	Listening(bool);
bool	Connected();
void	Connected(bool);
bool	WillListen();
void	WillListen(bool);
bool	WillConnect();
void	WillConnect(bool);
bool	Manual();
void	Manual(bool);
bool	Firewalled();
void	Firewalled(bool);
bool	Forwarded();
void	Forwarded(bool);
bool	Local();
void	Local(bool);
bool	Trusted();
void	Trusted(bool);

/* GUI Flags */
bool	InChat();
void	InChat(bool);
bool	InMessage();
void	InMessage(bool);

unsigned int	Status();
void	Status(unsigned int s);

int	addGroup(std::string);
int	removeGroup(std::string);
bool	Group(std::string);

int	cidpop();
void	cidpush(int id);

	/* Dynamic Address Foundation */
bool    hasDHT();
std::string getDHTId();
void    setDHT(struct sockaddr_in addr, unsigned int flags);

	// public for the moment.
	struct sockaddr_in lastaddr, localaddr, serveraddr;
	std::string dynDNSaddr;

	bool dhtFound;
	unsigned int dhtFlags;
	struct sockaddr_in dhtaddr;

	time_t lc_timestamp; // last connect timestamp
	time_t lr_timestamp; // last receive timestamp

	time_t nc_timestamp; // next connect timestamp.
	time_t nc_timeintvl; // next connect time interval.

	ChanId cid; // to get to the correct pqissl.

	int trustLvl; /* new field */

private:
	std::string name;

	unsigned int status;
	std::list<std::string> groups;
};


class PQItem
{
	// include serialisation (Boost) functions...

	// private copy constructor... prevent copying...
	PQItem(const PQItem &) {return; }
public:
	PQItem();
	PQItem(int t, int st); // initialise with subtypes.
virtual	~PQItem();

	// other type of serialise....
virtual std::ostream &print(std::ostream &out);

	// copy functions.
virtual PQItem *clone();
void copy(const PQItem *pqi);
void copyIDs(const PQItem *pqi);

	// stack operations -> could be converted to STL stack.
int	cidpop();
void	cidpush(int id);

	// data
	ChanId cid;
	SearchId sid;

	int type;
	int subtype;

	int flags;

	Person *p; // This is shallow copied inside to itself, 
			// Over serialisation - becomes a name....
};


// Some Functional Objects for operations on PQItems

class PQItem_MatchSid
//: public unary_function <PQItem *, bool>
{
	unsigned long sid;

	public:
	explicit PQItem_MatchSid(const unsigned long s) : sid(s) {}

	bool operator()(const PQItem *pqi) const 
	{
		if (pqi == NULL)
			return false;
		return (pqi -> sid == sid);
	}
};


class PQItem_MatchType
//: public unary_function <PQItem *, bool>
{
	int type, subtype;

	public:
	explicit PQItem_MatchType(const int t, const int s = 0) 
	: type(t), subtype(s) {}

	bool operator()(const PQItem *pqi) const 
	{
		if (pqi == NULL)
			return false;
		if (type == pqi -> type)
		{
			if (subtype == 0)
				return true;
			return (pqi -> subtype == subtype);
		}
		return false;
	}
};


#define PQIFILE_STD				0x0001
#define PQIFILE_DIRLIST_FILE			0x0002
#define PQIFILE_DIRLIST_DIR			0x0004


class PQFileItem: public PQItem
{
	// private copy constructor... prevent copying...
	PQFileItem(const PQFileItem &) {return; }
public:
	PQFileItem();
	PQFileItem(int st); // initialise with subtypes.
virtual	~PQFileItem();

	// copy functions.
virtual PQFileItem *clone();
void	copy(const PQFileItem *src);

std::ostream &print(std::ostream &out);
virtual int match(std::string term);

	std::string hash;
	std::string name;
	std::string path;
	std::string ext;


	int     size; 	// total size of file.
	long    fileoffset; // used for file requests.
	long    chunksize;  // used for file requests.

	int     reqtype;
	int     exttype;

};


class PQFileData: public PQFileItem
{
public:
	PQFileData();
virtual ~PQFileData();
virtual PQFileData *clone();
void 	copy(const PQFileData *fd);

virtual std::ostream &print(std::ostream &out);

int 	writedata(std::ostream &out);
int	readdata(std::istream &in, int maxbytes);
int 	endstream();

void 	isend();

	// data
	int datalen;
	void *data;

	int fileflags;
};

// And finally the Control/Info Interface.


class FileTransferItem: public PQFileItem
{
	// private copy constructor... prevent copying...
	// FileTransferItem(const FileTransferItem &) {return; }
public:
	FileTransferItem();
virtual	~FileTransferItem();

virtual FileTransferItem *clone();
void 	copy(const FileTransferItem *ft);

virtual std::ostream &print(std::ostream &out);

	int transferred;
	int state;
	float crate;
	float trate;
	bool in;

	int ltransfer;
	float lrate;
};

#define FT_STATE_FAILED 	0
#define FT_STATE_OKAY   	1
#define FT_STATE_COMPLETE	2



class RateCntrlItem: public PQFileItem
{
public:
	RateCntrlItem();
virtual	~RateCntrlItem();

virtual RateCntrlItem *clone();
void 	copy(const RateCntrlItem *ft);

virtual std::ostream &print(std::ostream &out);

	float total;
	float individual;
};





class RateInterface
{
// controlling data rates.
public:

	RateInterface()
	:bw_in(0), bw_out(0), bwMax_in(0), bwMax_out(0) { return; }
virtual	~RateInterface() { return; }

virtual float	getRate(bool in)
	{
	if (in)
		return bw_in;
	return bw_out;
	}

virtual float	getMaxRate(bool in)
	{
	if (in)
		return bwMax_in;
	return bwMax_out;
	}

virtual void	setMaxRate(bool in, float val)
	{
	if (in)
		bwMax_in = val;
	else
		bwMax_out = val;
	return;
	}

protected:

void	setRate(bool in, float val)
	{
	if (in)
		bw_in = val;
	else
		bw_out = val;
	return;
	}

	private:
float	bw_in, bw_out, bwMax_in, bwMax_out;
};





/*********************** PQI INTERFACE ******************************\
 * The basic exchange interface.
 * This inherits the RateInterface, as Bandwidth control 
 * is a critical to a networked application.
 *
 */
class NetInterface;

class PQInterface: public RateInterface
{
public:
	PQInterface()  { return; }
virtual	~PQInterface() { return; }

virtual int	SendItem(PQItem *) = 0;
virtual PQItem *GetItem() = 0;

// also there are  tick + person id  functions.
virtual int     tick() { return 0; }
virtual int     status() { return 0; }
virtual Person *getContact() { return NULL; } 

	// the callback from NetInterface Connection Events.
virtual int	notifyEvent(NetInterface *ni, int event) { return 0; }
};

/********************** Binary INTERFACE ****************************
 * This defines the binary interface used by Network/loopback/file
 * interfaces
 */

#define BIN_FLAGS_NO_CLOSE  0x0001
#define BIN_FLAGS_READABLE  0x0002
#define BIN_FLAGS_WRITEABLE 0x0004

class BinInterface
{
public:
	BinInterface() { return; }
virtual ~BinInterface() { return; }

virtual int     tick() = 0;

virtual int	senddata(void *data, int len) = 0;
virtual int	readdata(void *data, int len) = 0;
virtual int	netstatus() = 0;
virtual int	isactive() = 0;
virtual bool	moretoread() = 0;
virtual bool 	cansend() = 0;
		/* used by pqistreamer to limit transfers */
virtual bool 	bandwidthLimited() { return true; }
};


/********************** Network INTERFACE ***************************
 * This defines the Network interface used by sockets/SSL/XPGP
 * interfaces
 *
 * NetInterface: very pure interface, so no tick....
 *
 * It is passed a pointer to a PQInterface *parent, 
 * this is used to notify the system of Connect/Disconnect Events.
 *
 * Below are the Events for callback.
 */

static const int NET_CONNECT_RECEIVED     = 1;
static const int NET_CONNECT_SUCCESS      = 2;
static const int NET_CONNECT_UNREACHABLE  = 3;
static const int NET_CONNECT_FIREWALLED   = 4;
static const int NET_CONNECT_FAILED       = 5;

class NetInterface
{
public:
	NetInterface(PQInterface *p_in, Person *person_in)
	:p(p_in), person(person_in) { return; }

virtual ~NetInterface() 
	{ return; }

// virtual int tick() = 0; // Already defined for BinInterface.

virtual int connectattempt() = 0; 
virtual int listen() = 0; 
virtual int stoplistening() = 0; 
virtual int disconnect() = 0;
virtual int reset() = 0;
virtual Person *getContact() { return person; } 

protected:
PQInterface *parent() { return p; }

private:
	PQInterface *p;
	Person *person;
};


class NetBinInterface: public NetInterface, public BinInterface
{
public:
	NetBinInterface(PQInterface *parent, Person *person)
	:NetInterface(parent, person)
	{ return; }
virtual ~NetBinInterface() { return; }
};

#define CHAN_SIGN_SIZE 16
#define CERTSIGNLEN 16

class certsign
{
        public:
	bool    operator<(const certsign &ref) const;
	bool    operator==(const certsign &ref) const;
	char    data[CERTSIGNLEN];
};


// IDS aren't inportant in the base class.

// EXCEPT FOR MAYBE THE FLAGS.

// These are OR'ed together.
const int PQI_ITEM_FLAG_NEW     = 0x001;
const int PQI_ITEM_FLAG_LOCAL   = 0x002;
const int PQI_ITEM_FLAG_PRIVATE = 0x004;

const int PQI_ITEM_TYPE_ITEM = 0x000;
const int PQI_ITEM_TYPE_FILEITEM = 0x001;
const int PQI_ITEM_TYPE_SEARCHITEM = 2;
const int PQI_ITEM_TYPE_CHATITEM = 3;
const int PQI_ITEM_TYPE_INFOITEM = 5;
const int PQI_ITEM_TYPE_COMMANDITEM = 6;
const int PQI_ITEM_TYPE_AUTODISCITEM = 7;

const int PQI_ITEM_TYPE_TUNNELITEM = 777;
const int PQI_ITEM_TYPE_TUNNELINITITEM = 778;

// Sub Type.
const int PQI_FI_SUBTYPE_GENERAL = 1;
const int PQI_FI_SUBTYPE_DATA    = 2;
const int PQI_FI_SUBTYPE_CANCELRECV  = 3;
const int PQI_FI_SUBTYPE_CANCELSEND  = 4;
const int PQI_FI_SUBTYPE_REQUEST  = 5;
const int PQI_FI_SUBTYPE_TRANSFER  = 6;
const int PQI_FI_SUBTYPE_RATE  = 7;
const int PQI_FI_SUBTYPE_ERROR = 8;

const int PQI_FD_FLAG_ENDSTREAM = 1;

#endif // PQI_BASE_ITEM_HEADER

