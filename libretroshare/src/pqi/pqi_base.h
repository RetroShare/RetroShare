/*******************************************************************************
 * libretroshare/src/pqi: pqi_base.h                                           *
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
#ifndef PQI_BASE_ITEM_HEADER
#define PQI_BASE_ITEM_HEADER

#include <list>
#include <string>
#include <iostream>
#include <functional>
#include <algorithm>
#include <inttypes.h>

#include "pqi/pqinetwork.h"

struct RSTrafficClue;

/*** Base DataTypes: ****/
#include "serialiser/rsserial.h"
#include "retroshare/rstypes.h"


#define PQI_MIN_PORT 10 // TO ALLOW USERS TO HAVE PORT 80! - was 1024
#define PQI_MIN_PORT_RNG 1024
#define PQI_MAX_PORT 65535
#define PQI_DEFAULT_PORT 7812

int getPQIsearchId();
int fixme(char *str, int n);

struct RsPeerCryptoParams;

//! controlling data rates
/*!
 * For controlling data rates.
 * #define DEBUG_RATECAP	1
 */

class RsBwRates
{
	public:
	RsBwRates()
	:mRateIn(0), mRateOut(0), mMaxRateIn(0), mMaxRateOut(0), mQueueIn(0), mQueueOut(0) {return;}
	float mRateIn;
	float mRateOut;
	float mMaxRateIn;
	float mMaxRateOut;
	int   mQueueIn;
	int   mQueueOut;
};


class RateInterface
{

public:

	RateInterface()
	        :bw_in(0), bw_out(0), bwMax_in(0), bwMax_out(0),
	          bwCapEnabled(false), bwCap_in(0), bwCap_out(0) { return; }

	virtual	~RateInterface() { return; }

	virtual void    getRates(RsBwRates &rates)
	{
		rates.mRateIn = bw_in;
		rates.mRateOut = bw_out;
		rates.mMaxRateIn = bwMax_in;
		rates.mMaxRateOut = bwMax_out;
		return;
	}

	virtual int gatherStatistics(std::list<RSTrafficClue>& /* outqueue_lst */,std::list<RSTrafficClue>& /* inqueue_lst */) { return 0;}

	virtual int     getQueueSize(bool /* in */) { return 0;}
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
		{
			bwMax_in = val;
			if (bwCapEnabled)
			{
				if (bwMax_in > bwCap_in)
				{
					bwMax_in = bwCap_in;
				}
			}
		}
		else
		{
			bwMax_out = val;
			if (bwCapEnabled)
			{
				if (bwMax_out > bwCap_out)
				{
					bwMax_out = bwCap_out;
				}
			}
		}

		return;
	}


	virtual void	setRateCap(float val_in, float val_out)
	{
		if ((val_in == 0) && (val_out == 0))
		{
#ifdef DEBUG_RATECAP
			std::cerr << "RateInterface::setRateCap() Now disabled" << std::endl;
#endif
			bwCapEnabled = false;
		}
		else
		{
#ifdef DEBUG_RATECAP
			std::cerr << "RateInterface::setRateCap() Enabled ";
			std::cerr << "in: " << bwCap_in << " out: " << bwCap_out << std::endl;
#endif
			bwCapEnabled = true;
			bwCap_in = val_in;
			bwCap_out = val_out;
		}
		return;
	}

protected:

	virtual void	setRate(bool in, float val)
	{
		if (in)
			bw_in = val;
		else
			bw_out = val;
		return;
	}

private:
	float	bw_in, bw_out, bwMax_in, bwMax_out;
	bool    bwCapEnabled;
	float   bwCap_in, bwCap_out;

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
		explicit PQInterface(const RsPeerId &id) :peerId(id), traf_in(0), traf_out(0) { return; }
		virtual	~PQInterface() { return; }

		/*!
		 * allows user to send RsItems to a particular facility  (file, network)
		 */
		virtual int	SendItem(RsItem *) = 0;

		// this function is overloaded in classes that need the serilized size to be returned.
		virtual int	SendItem(RsItem *item,uint32_t& size) 
		{
			size = 0 ;

			static bool already=false ;
			if(!already)
			{
				std::cerr << "Warning: PQInterface::SendItem(RsItem*,uint32_t&) calledbut not overloaded! Serialized size will not be returned." << std::endl;
				already=true ;
			}
			return SendItem(item) ;
		}

		virtual bool getCryptoParams(RsPeerCryptoParams&) { return false ;}

		/*!
		 * Retrieve RsItem from a facility
		 */
		virtual RsItem *GetItem() = 0;
		virtual bool RecvItem(RsItem * /*item*/ )  { return false; }  /* alternative for for GetItem(), when we want to push */

		/**
		 * also there are  tick + person id  functions.
		 */
		virtual int tick() { return 0; }
		virtual int status() { return 0; }
		virtual const RsPeerId& PeerId() { return peerId; }

		// the callback from NetInterface Connection Events.
		virtual int	notifyEvent(NetInterface * /*ni*/, int /*event*/,
								const sockaddr_storage & /*remote_peer_address*/)
		{ return 0; }

		virtual uint64_t getTraffic(bool in)
		{
			uint64_t ret = 0;
			if (in)
			{
				ret = traf_in;
				traf_in = 0;
				return ret;
			}
			ret = traf_out;
			traf_out = 0;
			return ret;
		}
		uint64_t traf_in;
		uint64_t traf_out;

	private:

		RsPeerId peerId;
};



/**** Consts for pqiperson -> placed here for NetBinDummy usage() */

const uint32_t PQI_CONNECT_TCP = 0x0001;
const uint32_t PQI_CONNECT_UDP = 0x0002;
const uint32_t PQI_CONNECT_HIDDEN_TOR_TCP = 0x0004;
const uint32_t PQI_CONNECT_HIDDEN_I2P_TCP = 0x0008;


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
	BinInterface() {}
	virtual ~BinInterface() {}

	/**
	 * To be called loop, for updating state
	 */
	virtual int tick() = 0;

	/**
	 * Sends data to a prescribed location (implementation dependent)
	 *@param data what will be sent
	 *@param len the size of data pointed to in memory
	 */
	virtual int senddata(void *data, int len) = 0;

	/**
	 * reads data from a prescribed location (implementation dependent)
	 *@param data what will be sent
	 *@param len the size of data pointed to in memory
	 */
	virtual int readdata(void *data, int len) = 0;

	/**
	 * Is more particular the case of the sending data through a socket (internet)
	 * moretoread and candsend, take a microsec timeout argument.
	 *
	 */
	virtual int netstatus() = 0;
	virtual int isactive() = 0;
	virtual bool moretoread(uint32_t usec) = 0;
	virtual bool cansend(uint32_t usec) = 0;

	/**
	 *  method for streamer to shutdown bininterface
	 **/
	virtual int close() = 0;

	/**
	 * If hashing data
	 **/
	virtual RsFileHash gethash() = 0;

	/**
	 * Number of bytes read/sent
	 */
	virtual uint64_t bytecount() { return 0; }

	/**
	 *  used by pqistreamer to limit transfers
	 **/
	virtual bool bandwidthLimited() { return true; }
};



static const int NET_CONNECT_RECEIVED     = 1;
static const int NET_CONNECT_SUCCESS      = 2;
static const int NET_CONNECT_UNREACHABLE  = 3;
static const int NET_CONNECT_FIREWALLED   = 4;
static const int NET_CONNECT_FAILED       = 5;

static const uint32_t NET_PARAM_CONNECT_DELAY   = 1;
static const uint32_t NET_PARAM_CONNECT_PERIOD  = 2;
static const uint32_t NET_PARAM_CONNECT_TIMEOUT = 3;
static const uint32_t NET_PARAM_CONNECT_FLAGS    = 4;
static const uint32_t NET_PARAM_CONNECT_BANDWIDTH = 5;

static const uint32_t NET_PARAM_CONNECT_PROXY = 6;
static const uint32_t NET_PARAM_CONNECT_SOURCE = 7;

static const uint32_t NET_PARAM_CONNECT_DOMAIN_ADDRESS = 8;
static const uint32_t NET_PARAM_CONNECT_REMOTE_PORT = 9;


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
	NetInterface(PQInterface *p_in, const RsPeerId& id) : p(p_in), peerId(id) {}

	virtual ~NetInterface() {}

	/* TODO
	 * The data entrypoint is connect(const struct sockaddr_storage &raddr)
	 * To generalize NetInterface we should have a more general type for raddr
	 * As an example a string containing an url or encoded like a domain name
	 */
	virtual int connect(const struct sockaddr_storage &raddr) = 0;

	virtual int listen() = 0;
	virtual int stoplistening() = 0;
	virtual int disconnect() = 0;
	virtual int reset() = 0;
	virtual const RsPeerId& PeerId() { return peerId; }
	virtual int getConnectAddress(struct sockaddr_storage &raddr) = 0;

	virtual bool connect_parameter(uint32_t type, uint32_t value) = 0;
	virtual bool connect_parameter(uint32_t /* type */ , const std::string & /* value */ ) { return false; } // not generally used.
	virtual bool connect_additional_address(uint32_t /*type*/, const struct sockaddr_storage & /*addr*/) { return false; } // only needed by udp.

protected:
	PQInterface *parent() { return p; }

private:
	PQInterface *p;
	RsPeerId peerId;
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
	NetBinInterface(PQInterface *parent, const RsPeerId& id) :
		NetInterface(parent, id) {}
	virtual ~NetBinInterface() {}
};

#define CHAN_SIGN_SIZE 16
#define CERTSIGNLEN 16       /* actual byte length of signature */
#define PQI_PEERID_LENGTH 32 /* When expanded into a string */



#endif // PQI_BASE_ITEM_HEADER

