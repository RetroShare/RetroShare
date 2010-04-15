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
#include <inttypes.h>

#include "pqi/pqinetwork.h"

/*** Base DataTypes: ****/
#include "serialiser/rsserial.h"


#define PQI_MIN_PORT 1024
#define PQI_MAX_PORT 50000
#define PQI_DEFAULT_PORT 7812

int getPQIsearchId();
int fixme(char *str, int n);

//! controlling data rates
/*!
 * For controlling data rates.
 */
class RateInterface
{

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


class NetInterface;

//! The basic exchange interface.
/*!
 *
 * This inherits the RateInterface, as Bandwidth control 
 * is critical to a networked application.
 **/
class PQInterface: public RateInterface
{
public:
	PQInterface(std::string id) :peerId(id) { return; }
virtual	~PQInterface() { return; }

virtual int	SendItem(RsItem *) = 0;
virtual RsItem *GetItem() = 0;

/**
 * also there are  tick + person id  functions.
 */
virtual int     tick() { return 0; }
virtual int     status() { return 0; }
virtual std::string PeerId() { return peerId; }

	// the callback from NetInterface Connection Events.
virtual int	notifyEvent(NetInterface *ni, int event) { return 0; }

	private:

	std::string peerId;
};



/**** Consts for pqiperson -> placed here for NetBinDummy usage() */

const uint32_t PQI_CONNECT_TCP = 0x0001;
const uint32_t PQI_CONNECT_UDP = 0x0002;
const uint32_t PQI_CONNECT_TUNNEL = 0x0003;
const uint32_t PQI_CONNECT_DO_NEXT_ATTEMPT = 0x0004;


#define BIN_FLAGS_NO_CLOSE  0x0001
#define BIN_FLAGS_READABLE  0x0002
#define BIN_FLAGS_WRITEABLE 0x0004
#define BIN_FLAGS_NO_DELETE 0x0008
#define BIN_FLAGS_HASH_DATA 0x0010

/*!
 * This defines the binary interface used by Network/loopback/file
 * interfaces
 * e.g. sending binary data to file or to memory or even through a socket
 */
class BinInterface
{
public:
	BinInterface() { return; }
virtual ~BinInterface() { return; }

/**
 * To be called loop, for updating state
 */
virtual int     tick() = 0;

/**
 * Sends data to a prescribed location (implementation dependent)
 *@param data what will be sent
 *@param len the size of data pointed to in memory
 */
virtual int	senddata(void *data, int len) = 0;

/**
 * reads data from a prescribed location (implementation dependent)
 *@param data what will be sent
 *@param len the size of data pointed to in memory
 */
virtual int	readdata(void *data, int len) = 0;

/**
 * Is more particular the case of the sending data through a socket (internet)
 */
virtual int	netstatus() = 0;
virtual int	isactive() = 0;
virtual bool	moretoread() = 0;
virtual bool 	cansend() = 0;

/**
 *  method for streamer to shutdown bininterface
 **/
virtual int	close() = 0;

/**
 * If hashing data
 **/
virtual std::string gethash() = 0;

/**
 * Number of bytes read/sent
 */
virtual uint64_t bytecount() { return 0; }

/**
 *  used by pqistreamer to limit transfers
 **/
virtual bool 	bandwidthLimited() { return true; }
};



static const int NET_CONNECT_RECEIVED     = 1;
static const int NET_CONNECT_SUCCESS      = 2;
static const int NET_CONNECT_UNREACHABLE  = 3;
static const int NET_CONNECT_FIREWALLED   = 4;
static const int NET_CONNECT_FAILED       = 5;

static const uint32_t NET_PARAM_CONNECT_DELAY   = 1;
static const uint32_t NET_PARAM_CONNECT_PERIOD  = 2;
static const uint32_t NET_PARAM_CONNECT_TIMEOUT = 3;
//static const uint32_t NET_PARAM_CONNECT_TUNNEL_SOURCE_PEER_ID   = 4;
//static const uint32_t NET_PARAM_CONNECT_TUNNEL_RELAY_PEER_ID   = 5;
//static const uint32_t NET_PARAM_CONNECT_TUNNEL_DEST_PEER_ID   = 6;


/*!
 * ******************** Network INTERFACE ***************************
 * This defines the Network interface used by sockets/SSL/XPGP
 * interfaces
 *
 * NetInterface: very pure interface, so no tick....
 *
 * It is passed a pointer to a PQInterface *parent,
 * this is used to notify the system of Connect/Disconnect Events.
 *
 * Below are the Events for callback.
 **/
class NetInterface
{
public:

	/**
	 * @param p_in used to notify system of connect/disconnect events
	 */
	NetInterface(PQInterface *p_in, std::string id)
	:p(p_in), peerId(id) { return; }

virtual ~NetInterface() 
	{ return; }

virtual int connect(struct sockaddr_in raddr) = 0; 
virtual int listen() = 0; 
virtual int stoplistening() = 0; 
virtual int disconnect() = 0;
virtual int reset() = 0;
virtual std::string PeerId() { return peerId; }
virtual int getConnectAddress(struct sockaddr_in &raddr) = 0;

virtual bool connect_parameter(uint32_t type, uint32_t value) = 0;

protected:
PQInterface *parent() { return p; }

private:
	PQInterface *p;
	std::string peerId;
};

//! network binary interface (abstract)
/**
 * Should be used for networking and connecting to addresses
 *
 * @see pqissl
 * @see NetInterface
 * @see BinInterface
 **/
class NetBinInterface: public NetInterface, public BinInterface
{
public:
	NetBinInterface(PQInterface *parent, std::string id)
	:NetInterface(parent, id)
	{ return; }
virtual ~NetBinInterface() { return; }
};

#define CHAN_SIGN_SIZE 16
#define CERTSIGNLEN 16       /* actual byte length of signature */
#define PQI_PEERID_LENGTH 32 /* When expanded into a string */



#endif // PQI_BASE_ITEM_HEADER

