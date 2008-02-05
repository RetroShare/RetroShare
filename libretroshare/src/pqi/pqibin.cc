/*
 * "$Id: pqibin.cc,v 1.4 2007-02-18 21:46:49 rmf24 Exp $"
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




#include "pqi/pqibin.h"

BinFileInterface::BinFileInterface(const char *fname, int flags)
	:bin_flags(flags), buf(NULL), hash(NULL), bcount(0)
{
	/* if read + write - open r+ */
	if ((bin_flags & BIN_FLAGS_READABLE) &&
		(bin_flags & BIN_FLAGS_WRITEABLE))
	{
		buf = fopen(fname, "r+");
		/* if the file don't exist */
		if (!buf)
		{
			buf = fopen(fname, "w+");
		}

	}
	else if (bin_flags & BIN_FLAGS_READABLE)
	{
		buf = fopen(fname, "r");
	}
	else if (bin_flags & BIN_FLAGS_WRITEABLE)
	{
		// This is enough to remove old file in Linux...
		// but not in windows.... (what to do)
		buf = fopen(fname, "w");
		fflush(buf); /* this might help windows! */
	}
	else
	{	
		/* not read or write! */
	}

	if (buf)
	{
		/* no problem */

		/* go to the end */
		fseek(buf, 0L, SEEK_END);
		/* get size */
		size = ftell(buf);
		/* back to the start */
		fseek(buf, 0L, SEEK_SET);
	}
	else
	{
		size = 0;
	}

	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{
		hash = new pqihash();
	}
}


BinFileInterface::~BinFileInterface()
{
	if (buf)
	{
		fclose(buf);
	}
}
		
int	BinFileInterface::senddata(void *data, int len)
{
	if (!buf)
		return -1;

	if (1 != fwrite(data, len, 1, buf))
	{
		return -1;
	}
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		hash->addData(data, len);
		bcount += len;
	}
	return len;
}

int	BinFileInterface::readdata(void *data, int len)
{
	if (!buf)
		return -1;

	if (1 != fread(data, len, 1, buf))
	{
		return -1;
	}
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		hash->addData(data, len);
		bcount += len;
	}
	return len;
}

std::string  BinFileInterface::gethash()
{
	std::string hashstr;
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		hash->Complete(hashstr);
	}
	return hashstr;
}

uint64_t BinFileInterface::bytecount()
{
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		return bcount;
	}
	return 0;
}
	

BinMemInterface::BinMemInterface(int defsize, int flags)
	:bin_flags(flags), buf(NULL), size(defsize), 
	recvsize(0), readloc(0), hash(NULL), bcount(0)
	{
		buf = malloc(defsize);
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{
			hash = new pqihash();
		}
	}

BinMemInterface::~BinMemInterface() 
	{ 
		if (buf)
			free(buf);
		return; 
	}

	/* some fns to mess with the memory */
int	BinMemInterface::fseek(int loc)
	{
		if (loc <= recvsize)
		{
			readloc = loc;
			return 1;
		}
		return 0;
	}


int	BinMemInterface::senddata(void *data, int len)
	{
		if(recvsize + len > size)
		{
			/* resize? */
			return -1;
		}
		memcpy((char *) buf + recvsize, data, len);
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{  
			hash->addData(data, len);
			bcount += len;
		}
		recvsize += len;
		return len;
	}

		
int	BinMemInterface::readdata(void *data, int len)
	{
		if(readloc + len > recvsize)
		{
			/* no more stuff? */
			return -1;
		}
		memcpy(data, (char *) buf + readloc, len);
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{  
			hash->addData(data, len);
			bcount += len;
		}
		readloc += len;
		return len;
	}



std::string  BinMemInterface::gethash()
	{
		std::string hashstr;
		if (bin_flags & BIN_FLAGS_HASH_DATA)
		{  
			hash->Complete(hashstr);
		}
		return hashstr;
	}


uint64_t BinMemInterface::bytecount()
{
	if (bin_flags & BIN_FLAGS_HASH_DATA)
	{  
		return bcount;
	}
	return 0;
}
	


/**************************************************************************/


void printNetBinID(std::ostream &out, std::string id, uint32_t t)
{
	out << "NetBinId(" << id << ",";
	if (t == PQI_CONNECT_TCP)
	{
		out << "TCP)";
	}
	else
	{
		out << "UDP)";
	}
}


/************************** NetBinDummy ******************************
 * A test framework, 
 *
 */

#include "pqi/pqiperson.h"

const uint32_t DEFAULT_DUMMY_DELTA 	= 5;

NetBinDummy::NetBinDummy(PQInterface *parent, std::string id, uint32_t t)
       :NetBinInterface(parent, id), type(t), dummyConnected(false), 
        toConnect(false), connectDelta(DEFAULT_DUMMY_DELTA)
{ 
	return; 
}

	// Net Interface
int NetBinDummy::connect(struct sockaddr_in raddr)
{
	std::cerr << "NetBinDummy::connect(";
	std::cerr << inet_ntoa(raddr.sin_addr) << ":";
	std::cerr << htons(raddr.sin_port);
	std::cerr << ") ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl; 

	std::cerr << "NetBinDummy::dummyConnect = true!";
	std::cerr << std::endl; 

	if (type == PQI_CONNECT_TCP)
	{
		std::cerr << "NetBinDummy:: Not connecting TCP";
		std::cerr << std::endl; 
		if (parent())
		{
			parent()->notifyEvent(this, CONNECT_FAILED);
		}
	}
	else if (!dummyConnected)
	{
		toConnect = true;
		connectTS = time(NULL) + connectDelta;
		std::cerr << "NetBinDummy::toConnect = true, connect in: ";
		std::cerr << connectDelta << " secs";
		std::cerr << std::endl; 
	}
	else
	{
		std::cerr << "NetBinDummy:: Already Connected!";
		std::cerr << std::endl; 
	}

	return 1;
}

int NetBinDummy::listen()
{
	std::cerr << "NetBinDummy::connect() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 1;
}

int NetBinDummy::stoplistening()
{
	std::cerr << "NetBinDummy::stoplistening() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 1;
}

int NetBinDummy::disconnect()
{
	std::cerr << "NetBinDummy::disconnect() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	std::cerr << "NetBinDummy::dummyConnect = false!";
	std::cerr << std::endl; 
	dummyConnected = false;

	if (parent())
	{
		parent()->notifyEvent(this, CONNECT_FAILED);
	}

	return 1;
}

int NetBinDummy::reset()
{
	std::cerr << "NetBinDummy::reset() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	disconnect();

	return 1;
}


         // Bin Interface.
int     NetBinDummy::tick()
{

	if (toConnect)
	{
		if (connectTS < time(NULL))
		{
			std::cerr << "NetBinDummy::tick() dummyConnected = true ";
			printNetBinID(std::cerr, PeerId(), type);
			std::cerr << std::endl;
			dummyConnected = true;
			toConnect = false;
			if (parent())
				parent()->notifyEvent(this, CONNECT_SUCCESS);
		}
		else
		{
			std::cerr << "NetBinDummy::tick() toConnect ";
			printNetBinID(std::cerr, PeerId(), type);
			std::cerr << std::endl;
		}
	}
	return 0;
}

int     NetBinDummy::senddata(void *data, int len)
{
	std::cerr << "NetBinDummy::senddata() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	if (dummyConnected)
		return len;
	return 0;
}

int     NetBinDummy::readdata(void *data, int len)
{
	std::cerr << "NetBinDummy::readdata() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 0;
}

int     NetBinDummy::netstatus()
{
	std::cerr << "NetBinDummy::netstatus() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return 1;
}

int     NetBinDummy::isactive()
{
	std::cerr << "NetBinDummy::isactive() ";
	printNetBinID(std::cerr, PeerId(), type);
	if (dummyConnected)
		std::cerr << " true ";
	else
		std::cerr << " false ";
	std::cerr << std::endl;

	return dummyConnected;
}

bool    NetBinDummy::moretoread()
{
	std::cerr << "NetBinDummy::moretoread() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return false;
}

bool    NetBinDummy::cansend()
{
	std::cerr << "NetBinDummy::cansend() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return dummyConnected;
}

std::string NetBinDummy::gethash()
{
	std::cerr << "NetBinDummy::gethash() ";
	printNetBinID(std::cerr, PeerId(), type);
	std::cerr << std::endl;

	return std::string("");
}


