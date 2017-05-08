/*
 * libretroshare/src/pqi pqiperson.h
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



#ifndef MRK_PQI_PERSON_HEADER
#define MRK_PQI_PERSON_HEADER


#include <string>
#include "pqi/pqi.h"
#include "util/rsnet.h"

#include <list>

class pqiperson;
class RsPeerCryptoParams ;

static const int CONNECT_RECEIVED     = 1; 
static const int CONNECT_SUCCESS      = 2;
static const int CONNECT_UNREACHABLE  = 3;
static const int CONNECT_FIREWALLED   = 4;
static const int CONNECT_FAILED       = 5;

static const time_t HEARTBEAT_REPEAT_TIME = 5;

#include "pqi/pqiqosstreamer.h"
#include "pqi/pqithreadstreamer.h"

class pqiconnect : public pqiQoSstreamer, public NetInterface
{
public:
	pqiconnect(PQInterface *parent, RsSerialiser *rss, NetBinInterface *ni_in) :
		pqiQoSstreamer(parent, rss, ni_in->PeerId(), ni_in, 0),  // pqistreamer will cleanup NetInterface.
		NetInterface(NULL, ni_in->PeerId()), // No need for callback
		ni(ni_in) {}

	virtual ~pqiconnect() {}
	virtual bool getCryptoParams(RsPeerCryptoParams& params);

	// presents a virtual NetInterface -> passes to ni.
	virtual int	connect(const struct sockaddr_storage &raddr) { return ni->connect(raddr); }
	virtual int	listen() { return ni->listen(); }
	virtual int	stoplistening() { return ni->stoplistening(); }
    	virtual int 	reset() { pqistreamer::reset(); return ni->reset(); }
	virtual int 	disconnect() { return reset() ; }
	virtual bool connect_parameter(uint32_t type, uint32_t value) { return ni->connect_parameter(type, value);}
	virtual bool connect_parameter(uint32_t type, const std::string &value) { return ni->connect_parameter(type, value);}
	virtual bool connect_additional_address(uint32_t type, const struct sockaddr_storage &addr) { return ni->connect_additional_address(type, addr); }
	virtual int getConnectAddress(struct sockaddr_storage &raddr){ return ni->getConnectAddress(raddr); }

	// get the contact from the net side!
	virtual const RsPeerId& PeerId() { return ni->PeerId(); }

	// to check if our interface.
	virtual bool thisNetInterface(NetInterface *ni_in) { return (ni_in == ni); }

protected:
	NetBinInterface *ni;
};


class pqipersongrp;

class NotifyData
{
public:
	NotifyData() : mNi(NULL), mState(0)
	{
		sockaddr_storage_clear(mAddr);
	}

	NotifyData(NetInterface *ni, int state, const sockaddr_storage &addr) :
		mNi(ni), mState(state), mAddr(addr) {}

	NetInterface *mNi;
	int mState;
	sockaddr_storage mAddr;
};


class pqiperson: public PQInterface
{
public:
	pqiperson(const RsPeerId& id, pqipersongrp *ppg);
	virtual ~pqiperson(); // must clean up children.

	// control of the connection.
	int reset();
	int listen();
	int stoplistening();
	
	int	connect(uint32_t type, const sockaddr_storage &raddr,
				const sockaddr_storage &proxyaddr, const sockaddr_storage &srcaddr,
				uint32_t delay, uint32_t period, uint32_t timeout, uint32_t flags,
				uint32_t bandwidth, const std::string &domain_addr, uint16_t domain_port);

	int fullstopthreads();
	int receiveHeartbeat();

	// add in connection method.
	int addChildInterface(uint32_t type, pqiconnect *pqi);

	virtual bool getCryptoParams(RsPeerCryptoParams&);

	// The PQInterface interface.
	virtual int SendItem(RsItem *,uint32_t& serialized_size);
	virtual int SendItem(RsItem *item)
	{
		std::cerr << "Warning pqiperson::sendItem(RsItem*) should not be called."
				  << "Plz call SendItem(RsItem *,uint32_t& serialized_size) instead."
				  << std::endl;
		uint32_t serialized_size;
		return SendItem(item, serialized_size);
	}

	virtual RsItem *GetItem();
	virtual bool RecvItem(RsItem *item);
	
	virtual int status();
	virtual int	tick();

	// overloaded callback function for the child - notify of a change.
	virtual int notifyEvent(NetInterface *ni, int event, const struct sockaddr_storage &addr);

	// PQInterface for rate control overloaded....
	virtual int getQueueSize(bool in);
	virtual void getRates(RsBwRates &rates);
	virtual float getRate(bool in);
	virtual void setMaxRate(bool in, float val);
	virtual void setRateCap(float val_in, float val_out);
	virtual int gatherStatistics(std::list<RSTrafficClue>& outqueue_lst,
								 std::list<RSTrafficClue>& inqueue_lst);

private:
	void processNotifyEvents();
	int handleNotifyEvent_locked(NetInterface *ni, int event,
								 const sockaddr_storage &addr);

	RsMutex mNotifyMtx; // LOCKS Notify Queue
	std::list<NotifyData> mNotifyQueue;

	RsMutex mPersonMtx; // LOCKS below

	int reset_locked();

	void setRateCap_locked(float val_in, float val_out);

	std::map<uint32_t, pqiconnect *> kids;
	bool active;
	pqiconnect *activepqi;
	bool inConnectAttempt;
	//int waittimes;
	time_t lastHeartbeatReceived; // use to track connection failure
	pqipersongrp *pqipg; /* parent for callback */
};

#endif
