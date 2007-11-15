/*
 * "$Id: pqi_base.cc,v 1.17 2007-03-31 09:41:32 rmf24 Exp $"
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




#include "pqi/pqi_base.h"

#include <ctype.h>

// local functions.
int pqiroute_setshift(ChanId *item, int chan);
int pqiroute_getshift(ChanId *item);

// these ones are also exported!
int	pqicid_clear(ChanId *cid);
int	pqicid_copy(const ChanId *cid, ChanId *newcid);
int	pqicid_cmp(const ChanId *cid1, ChanId *cid2);

// Helper functions for the PQInterface.

static int next_search_id = 1;

int getPQIsearchId()
{
	return next_search_id++;
}

// SI Operations.
PQItem::PQItem()
	:sid(0), type(0), subtype(0), p(0)
{
	//std::cerr << "Creating PQItem(0,0)@" << std::endl;
	//std::cerr << (void *) this << std::endl;
	pqicid_clear(&cid);
	return;
}

PQItem::~PQItem()
{
	//std::cerr << "Deleting PQItem(" << type << ",";
	//std::cerr << subtype << ")@" << (void *) this << std::endl;
	return;
}

PQItem::PQItem(int t, int st)
	:sid(0), type(t), subtype(st), flags(0), p(0)
{
	//std::cerr << "Creating PQItem(" << t << "," << st << ")@";
	//std::cerr << (void *) this << std::endl;
	pqicid_clear(&cid);
	return;
}

PQItem *PQItem::clone()
{
	PQItem *npqi = new PQItem();
	npqi -> copy(this);

	return npqi;
}

void PQItem::copy(const PQItem *pqi)
{
	sid = pqi -> sid;
	pqicid_copy(&(pqi -> cid), &(cid));
	type = pqi -> type;
	subtype = pqi -> subtype;
	flags = pqi -> flags;
	p =  pqi -> p;
	return;
}

void PQItem::copyIDs(const PQItem *pqi)
{
	sid = pqi -> sid;
	pqicid_copy(&(pqi -> cid), &(cid));
	// Person ref should also be part of the id.
	p =  pqi -> p;
	return;
}


std::ostream &PQItem::print(std::ostream &out)
{
	out << "-- PQItem" << std::endl;
	out << "CID [" << cid.route[0];
	for(int i = 0; i < 10; i++)
		out << ":" << cid.route[i];
	out << "] SID: " << sid << std::endl;
	out << "Type: " << type << "/" << subtype;
	out << " Flags: " << flags << std::endl;
	out << " Source: ";
	if (p == NULL)
	{
		out << "*Unknown*" << std::endl;
	}
	else
	{
		out << p -> Name() << std::endl;
	}
	return out << "--" << std::endl;
}


int	PQItem::cidpop()
{
	return pqiroute_getshift(&cid);
}

void	PQItem::cidpush(int id)
{
	pqiroute_setshift(&cid, id);
	return;
}

PQFileItem::PQFileItem()
	:PQItem(PQI_ITEM_TYPE_FILEITEM, PQI_FI_SUBTYPE_GENERAL), 
	reqtype(PQIFILE_STD) 
{
	//std::cerr << "Creating PQFileItem()" << std::endl;
	return;
}

PQFileItem::PQFileItem(int subtype)
	:PQItem(PQI_ITEM_TYPE_FILEITEM, subtype),
	reqtype(PQIFILE_STD) 
{
	//std::cerr << "Creating PQFileItem()" << std::endl;
	return;
}

PQFileItem::~PQFileItem()
{
	//std::cerr << "Deleting PQFileItem()" << std::endl;
	return;
}


PQFileItem *PQFileItem::clone()
{
	PQFileItem *nfi = new PQFileItem();
	nfi -> copy(this);
	return nfi;
}


void PQFileItem::copy(const PQFileItem *src)
{
	PQItem::copy(src);

	hash = src -> hash;
	name = src -> name;
	path = src -> path;
	ext = src -> ext;

	size = src -> size;
	fileoffset = src -> fileoffset;
	chunksize = src -> chunksize;

	exttype = src -> exttype;
	reqtype = src -> reqtype;
	return;
}

std::ostream &PQFileItem::print(std::ostream &out)
{
	out << "-------- PQFileItem" << std::endl;
	PQItem::print(out);

	out << "Name: " << name << std::endl;
	out << "Hash: " << hash << std::endl;
	out << "Path: " << path << std::endl;
	out << "Ext: " << ext << std::endl;
	out << "Size: " << size << std::endl;
	out << "FileOffset: " << fileoffset << std::endl;
	out << "ChunkSize: " << chunksize << std::endl;
	out << "Ext/Req Types: " << exttype << "/" << reqtype << std::endl;
	return out << "--------" << std::endl;
}


PQFileData::PQFileData()
	:PQFileItem(PQI_FI_SUBTYPE_DATA), 
	datalen(0), data(NULL), fileflags(0)
{
	//std::cerr << "Creating PQFileData()" << std::endl;
	flags = 0;
	return;
}


PQFileData::~PQFileData()
{
	if (data != NULL)
	{
		free(data);
	}
	//std::cerr << "Deleting PQFileData()" << std::endl;
	return;
}

PQFileData *PQFileData::clone()
{
	PQFileData *nfd = new PQFileData();
	nfd -> copy(this);

	return nfd;
}

void 	PQFileData::copy(const PQFileData *fd)
{
	// copy below.
	PQItem::copy(fd);

	if (data != NULL)
	{
		free(data);
	}
	data = malloc(fd -> datalen);
	memcpy(data, fd -> data, fd -> datalen);
	datalen = fd -> datalen;
	fileflags = fd -> fileflags;
	return;
}

std::ostream &PQFileData::print(std::ostream &out)
{
	out << "-------- PQFileData" << std::endl;
	PQItem::print(out);

	out << "DataLen: " << datalen << std::endl;
	out << "FileFlags: " << fileflags << std::endl;
	return out << "--------" << std::endl;
}


int 	PQFileData::writedata(std::ostream &out)
{
	//std::cerr << "PQFileData::writedata() len: " << datalen << std::endl;
	if (data != NULL)
	{
		out.write((char *) data, datalen);
		if (out.bad())
		{
			//std::cerr << "PQFileData::writedata() Bad ostream!" << std::endl;
			return -1;
		}
		return 1;
	}
	return 1;
}


int	PQFileData::readdata(std::istream &in, int maxbytes)
{
	if (data != NULL)
	{
		free(data);
	}
	data = malloc(maxbytes);
	in.read((char *) data, maxbytes);

	int read = in.gcount();

	// if read < maxread then finished?
	if (read < maxbytes)
	{
		// check if finished.
		if (in.eof())
		{
			isend();
		}
		else
		{
			//std::cerr << "pqi_makeFilePkt() Only Read: ";
			//std::cerr << read << "/" << maxbytes << "  Why?";
			//std::cerr << std::endl;
		}
		// adjust the space.
		void *tmpdata = data;
		data = malloc(read);
		memcpy(data, tmpdata, read);
		free(tmpdata);
	}
	datalen = read;
	return 1;
}

int 	PQFileData::endstream()
{
	if (fileflags & PQI_FD_FLAG_ENDSTREAM)
		return 1;
	return 0;
}


void 	PQFileData::isend()
{
	fileflags |= PQI_FD_FLAG_ENDSTREAM;
	return;
}

FileTransferItem::FileTransferItem()
	:PQFileItem(PQI_FI_SUBTYPE_TRANSFER), 
	transferred(0), state(FT_STATE_OKAY), 
	crate(0), trate(0), in(false), ltransfer(0), lrate(0)
{
	//std::cerr << "Creating FileTransferItem()" << std::endl;
	return;
}


FileTransferItem::~FileTransferItem()
{
	//std::cerr << "Deleting FileTransferItem()" << std::endl;
	return;
}

FileTransferItem *FileTransferItem::clone()
{
	FileTransferItem *nfd = new FileTransferItem();
	nfd -> copy(this);

	return nfd;
}

void 	FileTransferItem::copy(const FileTransferItem *fd)
{
	// copy below.
	PQFileItem::copy(fd);

	transferred = fd -> transferred;
	state = fd -> state;
	crate = fd -> crate;
	trate = fd -> trate;
	in = fd -> in;
	ltransfer = fd -> ltransfer;
	lrate = fd -> lrate;
	return;
}

std::ostream &FileTransferItem::print(std::ostream &out)
{
	out << "-------- FileTransferItem" << std::endl;
	PQFileItem::print(out);

	if (in == true)
	{
		out << "Incoming File.....";
	}
	else
	{
		out << "Outgoing File.....";
	}
	out << "Transferred: " << transferred << std::endl;
	out << "Transfer State: " << state << std::endl;
	out << "Current/Total Rate: " << crate << "/" << trate << std::endl;
	out << "LTransfer/LRate: " << ltransfer << "/" << lrate << std::endl;
	return out << "--------" << std::endl;
}


RateCntrlItem::RateCntrlItem()
	:PQFileItem(PQI_FI_SUBTYPE_RATE), 
	total(0), individual(0)
{
	//std::cerr << "Creating RateCntrlItem()" << std::endl;
	return;
}


RateCntrlItem::~RateCntrlItem()
{
	//std::cerr << "Deleting RateCntrlItem()" << std::endl;
	return;
}

RateCntrlItem *RateCntrlItem::clone()
{
	RateCntrlItem *nfd = new RateCntrlItem();
	nfd -> copy(this);

	return nfd;
}

void 	RateCntrlItem::copy(const RateCntrlItem *fd)
{
	// copy below.
	PQFileItem::copy(fd);

	total = fd -> total;
	individual = fd -> individual;
	return;
}

std::ostream &RateCntrlItem::print(std::ostream &out)
{
	out << "-------- RateCntrlItem" << std::endl;
	PQFileItem::print(out);

	out << "Total Rate: " << total << std::endl;
	out << "Individual Rate: " << individual << std::endl;
	return out << "--------" << std::endl;
}


// CHANID Operations.
int	pqicid_clear(ChanId *cid)
{
	for(int i = 0; i < 10; i++)
	{
		cid -> route[i] = 0;
	}
	return 1;
}

int	pqicid_copy(const ChanId *cid, ChanId *newcid)
{
	for(int i = 0; i < 10; i++)
	{
		(newcid -> route)[i] = (cid -> route)[i];
	}
	return 1;
}

int	pqicid_cmp(const ChanId *cid1, ChanId *cid2)
{
	int ret = 0;
	for(int i = 0; i < 10; i++)
	{
		ret = cid1->route[i] - cid2->route[i];
		if (ret != 0)
		{
			return ret;
		}
	}
	return 0;
}




int pqiroute_getshift(ChanId *id)
{
	int *array = id -> route;
	int next = array[0];

	// shift.
	for(int i = 0; i < 10 - 1; i++)
	{
		array[i] = array[i+1];
	}
	array[10 - 1] = 0;

	return next;
}

int pqiroute_setshift(ChanId *id, int chan)
{
	int *array = id -> route;

	// shift.
	for(int i = 10 - 1; i > 0; i--)
	{
		array[i] = array[i-1];
	}
	array[0] = chan;

	return 1;
}

/****************** PERSON DETAILS ***********************/

Person::Person()
	:dhtFound(false), dhtFlags(0),
	lc_timestamp(0), lr_timestamp(0),
	nc_timestamp(0), nc_timeintvl(5),
	name("Unknown"), status(PERSON_STATUS_MANUAL)


	{
		for(int i = 0; i < (signed) sizeof(lastaddr); i++)
		{
			((unsigned char *) (&lastaddr))[i] = 0;
			((unsigned char *) (&localaddr))[i] = 0;
			((unsigned char *) (&serveraddr))[i] = 0;
			((unsigned char *) (&dhtaddr))[i] = 0;
		}
		pqicid_clear(&cid);


		return; 
	}

Person::~Person()
	{
	}
	

int	Person::cidpop()
{
	return pqiroute_getshift(&cid);
}

void	Person::cidpush(int id)
{
	pqiroute_setshift(&cid, id);
	return;
}

bool	Person::Group(std::string in)
	{
		std::list<std::string>::iterator it;
		for(it = groups.begin(); it != groups.end(); it++)
		{
			if (in == (*it))
			{
				return true;
			}
		}
		return false;
	}

	
int	Person::addGroup(std::string in)
	{
		groups.push_back(in);
		return 1;
	}

int	Person::removeGroup(std::string in)
	{
		std::list<std::string>::iterator it;
		for(it = groups.begin(); it != groups.end(); it++)
		{
			if (in == (*it))
			{
				groups.erase(it);
				return 1;
			}
		}
		return 0;
	}



bool	Person::Valid()
	{
		return (status & PERSON_STATUS_VALID);
	}

void	Person::Valid(bool b)
	{
		if (b)
			status |= PERSON_STATUS_VALID;
		else
			status &= ~PERSON_STATUS_VALID;
	}

bool	Person::Accepted()
	{
		return (status & PERSON_STATUS_ACCEPTED);
	}

void	Person::Accepted(bool b)
	{
		if (b)
			status |= PERSON_STATUS_ACCEPTED;
		else
			status &= ~PERSON_STATUS_ACCEPTED;
	}

bool	Person::InUse()
	{
		return (status & PERSON_STATUS_INUSE);
	}

void	Person::InUse(bool b)
	{
		if (b)
			status |= PERSON_STATUS_INUSE;
		else
			status &= ~(PERSON_STATUS_INUSE);
	}


bool	Person::Listening()
	{
		return (status & PERSON_STATUS_LISTENING);
	}

void	Person::Listening(bool b)
	{
		if (b)
			status |= PERSON_STATUS_LISTENING;
		else
			status &= ~PERSON_STATUS_LISTENING;
	}

bool	Person::Connected()
	{
		return (status & PERSON_STATUS_CONNECTED);
	}

void	Person::Connected(bool b)
	{
		if (b)
			status |= PERSON_STATUS_CONNECTED;
		else
			status &= ~PERSON_STATUS_CONNECTED;
	}

bool	Person::WillListen()
	{
		return (status & PERSON_STATUS_WILL_LISTEN);
	}

void	Person::WillListen(bool b)
	{
		if (b)
			status |= PERSON_STATUS_WILL_LISTEN;
		else
			status &= ~PERSON_STATUS_WILL_LISTEN;
	}

bool	Person::WillConnect()
	{
		return (status & PERSON_STATUS_WILL_CONNECT);
	}

void	Person::WillConnect(bool b)
	{
		if (b)
			status |= PERSON_STATUS_WILL_CONNECT;
		else
			status &= ~PERSON_STATUS_WILL_CONNECT;
	}

bool	Person::Manual()
	{
		return (status & PERSON_STATUS_MANUAL);
	}

void	Person::Manual(bool b)
	{
		if (b)
			status |= PERSON_STATUS_MANUAL;
		else
			status &= ~PERSON_STATUS_MANUAL;
	}

bool	Person::Firewalled()
	{
		return (status & PERSON_STATUS_FIREWALLED);
	}

void	Person::Firewalled(bool b)
	{
		if (b)
			status |= PERSON_STATUS_FIREWALLED;
		else
			status &= ~PERSON_STATUS_FIREWALLED;
	}

bool	Person::Forwarded()
	{
		return (status & PERSON_STATUS_FORWARDED);
	}

void	Person::Forwarded(bool b)
	{
		if (b)
			status |= PERSON_STATUS_FORWARDED;
		else
			status &= ~PERSON_STATUS_FORWARDED;
	}

bool	Person::Local()
	{
		return (status & PERSON_STATUS_LOCAL);
	}

void	Person::Local(bool b)
	{
		if (b)
			status |= PERSON_STATUS_LOCAL;
		else
			status &= ~PERSON_STATUS_LOCAL;
	}


bool	Person::Trusted()
	{
		return (status & PERSON_STATUS_TRUSTED);
	}

void	Person::Trusted(bool b)
	{
		if (b)
			status |= PERSON_STATUS_TRUSTED;
		else
			status &= ~PERSON_STATUS_TRUSTED;
	}


unsigned int	Person::Status()
	{
		return status;
	}


void	Person::Status(unsigned int s)
	{
		status = s;
	}

std::string Person::Name()
	{
		return name;
	}


void	Person::Name(std::string n)
	{
		name = n;
	}

        /* Dynamic Address Foundation */
bool    Person::hasDHT()
{
	return dhtFound;
}

void    Person::setDHT(struct sockaddr_in addr, unsigned int flags)
{
	dhtFound = true;
	dhtFlags = flags;
	dhtaddr = addr;
}

/* GUI Flags */
bool	Person::InChat()
	{
		return (status & PERSON_STATUS_INCHAT);
	}

void	Person::InChat(bool b)
	{
		if (b)
			status |= PERSON_STATUS_INCHAT;
		else
			status &= ~PERSON_STATUS_INCHAT;
	}

bool	Person::InMessage()
	{
		return (status & PERSON_STATUS_INMSG);
	}

void	Person::InMessage(bool b)
	{
		if (b)
			status |= PERSON_STATUS_INMSG;
		else
			status &= ~PERSON_STATUS_INMSG;
	}


