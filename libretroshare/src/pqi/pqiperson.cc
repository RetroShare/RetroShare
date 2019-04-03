/*
 * libretroshare/src/pqi pqiperson.cc
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

#include "pqi/pqi.h"
#include "pqi/pqiperson.h"
#include "pqi/pqipersongrp.h"
#include "pqi/pqissl.h"
#include "util/rsdebug.h"
#include "util/rsstring.h"
#include "retroshare/rspeers.h"

static struct RsLog::logInfo pqipersonzoneInfo = {RsLog::Default, "pqiperson"};
#define pqipersonzone &pqipersonzoneInfo

/****
 * #define PERSON_DEBUG 1
 ****/

pqiperson::pqiperson(const RsPeerId& id, pqipersongrp *pg) :
	PQInterface(id), mNotifyMtx("pqiperson-notify"), mPersonMtx("pqiperson"),
	active(false), activepqi(NULL), inConnectAttempt(false),// waittimes(0),
	pqipg(pg) {} // TODO: must check id!

pqiperson::~pqiperson()
{
	RS_STACK_MUTEX(mPersonMtx);

	// clean up the childrens
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		pqiconnect *pc = (it->second);
		delete pc;
	}
	kids.clear();
}

int pqiperson::SendItem(RsItem *i,uint32_t& serialized_size)
{
	RS_STACK_MUTEX(mPersonMtx);

	if(active)
	{
		// every outgoing item goes through this function, so try to not waste cpu cycles
		// check if debug output is wanted, to avoid unecessary work
		// getZoneLevel() locks a global mutex and does a lookup in a map or returns a default value
		// (not sure if this is a performance problem)
		if (PQL_DEBUG_BASIC <= pqipersonzoneInfo.lvl)
		{
			std::string out = "pqiperson::SendItem() Active: Sending On\n";
			i->print_string(out, 5); // this can be very expensive
#ifdef PERSON_DEBUG
			std::cerr << out << std::endl;
#endif
			pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out);
		}
		return activepqi -> SendItem(i,serialized_size);
	}
	else
	{
		if (PQL_DEBUG_BASIC <= pqipersonzoneInfo.lvl)
		{
			std::string out = "pqiperson::SendItem()";
			out += " Not Active: Used to put in ToGo Store\n";
			out += " Now deleting...";
			pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out);
		}
		delete i;
	}
	return 0; // queued.	
}

RsItem *pqiperson::GetItem()
{
	RS_STACK_MUTEX(mPersonMtx);

	if (active)
		return activepqi->GetItem();
	// else not possible.
	return NULL;
}

bool pqiperson::RecvItem(RsItem *item)
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::RecvItem()" << std::endl;
#endif

	return pqipg->recvItem((RsRawItem *) item);
}


int pqiperson::status()
{
	RS_STACK_MUTEX(mPersonMtx);

	if (active)
		return activepqi -> status();
	return -1;
}

int pqiperson::receiveHeartbeat()
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::receiveHeartbeat() from peer : "
			  << PeerId().toStdString() << std::endl;
#endif

	RS_STACK_MUTEX(mPersonMtx);
	lastHeartbeatReceived = time(NULL);

	return 1;
}

int	pqiperson::tick()
{
	int activeTick = 0;

	{
		RS_STACK_MUTEX(mPersonMtx);

#ifdef PERSON_DEBUG
        if(active)
        {
        std::cerr << "pqiperson: peer=" << (activepqi? (activepqi->PeerId()): (RsPeerId())) <<", active=" << active << ", last HB=" << time(NULL) - lastHeartbeatReceived << " secs ago." ;
        if(lastHeartbeatReceived==0)
            std::cerr << "!!!!!!!" << std::endl;
        else
            std::cerr << std::endl;
        }
#endif
        
		//if lastHeartbeatReceived is 0, it might be not activated so don't do a net reset.
		if ( active  && time(NULL)  > lastHeartbeatReceived + HEARTBEAT_REPEAT_TIME * 20)
		{
			int ageLastIncoming = time(NULL) - activepqi->getLastIncomingTS();

#ifdef PERSON_DEBUG
			std::cerr << "pqiperson::tick() WARNING No heartbeat from: "
					  << PeerId().toStdString() << " LastHeartbeat was: "
					  << time(NULL) - lastHeartbeatReceived
					  << "secs ago LastIncoming was: " << ageLastIncoming
					  << "secs ago" << std::endl;
#endif
	
			if (ageLastIncoming > 60) // Check timeout
			{
				std::cerr << "pqiperson::tick() " << PeerId().toStdString() << " No Heartbeat & No Packets for 60 secs -> assume dead." << std::endl;
				this->reset_locked();
			}
	
		}
	
	
		{
#ifdef PERSON_DEBUG
			std::string statusStr = " inactive ";
			if (active)
				statusStr = " active ";

			std::string connectStr = " Not Connecting ";
			if (inConnectAttempt)
				connectStr = " In Connection Attempt ";

			std::cerr << "pqiperson::tick() Id: " << PeerId().toStdString()
					  << "activepqi: " << activepqi << " inConnectAttempt:"
					  << connectStr << std::endl;
#endif
	
			// tick the children.
			std::map<uint32_t, pqiconnect *>::iterator it;
			for(it = kids.begin(); it != kids.end(); ++it)
			{
				if (0 < (it->second)->tick())
					activeTick = 1;
#ifdef PERSON_DEBUG
				std::cerr << "\tTicking Child: "<< it->first << std::endl;
#endif
			}
		}
	}

	// handle Notify Events that were generated.
	processNotifyEvents();

	return activeTick;
}

// callback function for the child - notify of a change.
// This is only used for out-of-band info....
// otherwise could get dangerous loops.
// - Actually, now we have - must store and process later.
int pqiperson::notifyEvent(NetInterface *ni, int newState,
						   const sockaddr_storage &remote_peer_address)
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::notifyEvent() adding event to Queue. newState="
			  << newState << " from IP = "
			  << sockaddr_storage_tostring(remote_peer_address) << std::endl;
#endif

    if (mPersonMtx.trylock())
	{
		handleNotifyEvent_locked(ni, newState, remote_peer_address);
		mPersonMtx.unlock();
		return 1;
	}

	RS_STACK_MUTEX(mNotifyMtx);
	mNotifyQueue.push_back(NotifyData(ni, newState, remote_peer_address));
	return 1;
}

void pqiperson::processNotifyEvents()
{
	NetInterface *ni;
	int state;
	sockaddr_storage addr;

	while(1) // While there is notification to handle
	{
		{
			RS_STACK_MUTEX(mNotifyMtx);

			if(mNotifyQueue.empty())
				return;

			NotifyData &data = mNotifyQueue.front();
			ni = data.mNi;
			state = data.mState;
			addr = data.mAddr;

			mNotifyQueue.pop_front();
		}

		RS_STACK_MUTEX(mPersonMtx);
		handleNotifyEvent_locked(ni, state, addr);
	}
}


int pqiperson::handleNotifyEvent_locked(NetInterface *ni, int newState,
										const sockaddr_storage &remote_peer_address)
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::handleNotifyEvent_locked() Id: "
			  << PeerId().toStdString() << " Message: " << newState
			  << " from: " << ni << std::endl;
	int i = 0;
#endif

	/* find the pqi, */
	pqiconnect *pqi = NULL;
	uint32_t    type = 0;
	std::map<uint32_t, pqiconnect *>::iterator it;
		
	/* start again */
	for(it = kids.begin(); it != kids.end(); ++it)
	{
#ifdef PERSON_DEBUG
		std::cerr << "pqiperson::handleNotifyEvent_locked() Kid# " << i
				  << " of " << kids.size() << " type: " << it->first
				  << " in_ni: " << ni << std::endl;
		++i;
#endif

		if ((it->second)->thisNetInterface(ni))
		{
			pqi = (it->second);
			type = (it->first);
		}
	}

	if (!pqi)
    {
	  std::cerr << "pqiperson::handleNotifyEvent_locked Unknown Event Source!"
				<< std::endl;
	  return -1;
	}
		

	switch(newState)
	{
	case CONNECT_RECEIVED:
	case CONNECT_SUCCESS:
	{

		/* notify */
		if (pqipg)
		{
			pqissl *ssl = dynamic_cast<pqissl*>(ni);
			if(ssl)
				pqipg->notifyConnect(PeerId(), type, true, ssl->actAsServer(), remote_peer_address);
			else
				pqipg->notifyConnect(PeerId(), type, true, false, remote_peer_address);
		}

		if ((active) && (activepqi != pqi)) // already connected - trouble
		{
			// TODO: 2015/12/19 Is this block dead code?

			std::cerr << "pqiperson::handleNotifyEvent_locked Id: "
					  << PeerId().toStdString() << " CONNECT_SUCCESS+active->"
					  << "activing new connection, shutting others"
					  << std::endl;

			// This is the RESET that's killing the connections.....
			//activepqi -> reset();
			// this causes a recursive call back into this fn.
			// which cleans up state.
			// we only do this if its not going to mess with new conn.
		}

		/* now install a new one. */
		{
#ifdef PERSON_DEBUG
			std::cerr << "pqiperson::handleNotifyEvent_locked Id: "
					  << PeerId().toStdString() << " CONNECT_SUCCESS->marking "
					  << "so! (resetting others)" << std::endl;
#endif

			// mark as active.
			active = true;
			lastHeartbeatReceived = time(NULL) ;
			activepqi = pqi;
			inConnectAttempt = false;

			// STARTUP THREAD
			activepqi->start("pqi " + PeerId().toStdString().substr(0, 11));

			// reset all other children (clear up long UDP attempt)
			for(it = kids.begin(); it != kids.end(); ++it)
				if (!(it->second)->thisNetInterface(ni))
					it->second->reset();
			return 1;
        }
		break;
	}
	case CONNECT_UNREACHABLE:
	case CONNECT_FIREWALLED:
	case CONNECT_FAILED:
	{
		if (active && (activepqi == pqi))
		{
#ifdef PERSON_DEBUG
			std::cerr << "pqiperson::handleNotifyEvent_locked Id: "
					  << PeerId().toStdString()
					  << " CONNECT_FAILED->marking so!" << std::endl;
#endif

			activepqi->shutdown(); // STOP THREAD.
			active = false;
			activepqi = NULL;
		}
#ifdef PERSON_DEBUG
		else
			std::cerr << "pqiperson::handleNotifyEvent_locked Id: "
					  << PeerId().toStdString() + " CONNECT_FAILED-> from "
					  << "an unactive connection, don't flag the peer as "
					  << "not connected, just try next attempt !" << std::endl;
#endif
		/* notify up */
		if (pqipg)
			pqipg->notifyConnect(PeerId(), type, false, false, remote_peer_address);

		return 1;
	}
	default:
		return -1;
	}
}

/***************** Not PQInterface Fns ***********************/

int pqiperson::reset()
{
	RS_STACK_MUTEX(mPersonMtx);
	return reset_locked();
}

int pqiperson::reset_locked()
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::reset_locked() resetting all pqiconnect for Id: "
			  << PeerId().toStdString() << std::endl;
#endif

	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		(it->second) -> shutdown(); // STOP THREAD.
		(it->second) -> reset();
	}

	activepqi = NULL;
	active = false;
	lastHeartbeatReceived = 0;

	return 1;
}

int pqiperson::fullstopthreads()
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::fullstopthreads() for Id: "
			  << PeerId().toStdString() << std::endl;
#endif

	RS_STACK_MUTEX(mPersonMtx);

	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
		(it->second)->fullstop(); // WAIT FOR THREAD TO STOP.

	activepqi = NULL;
	active = false;
	lastHeartbeatReceived = 0;

	return 1;
}

int	pqiperson::addChildInterface(uint32_t type, pqiconnect *pqi)
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::addChildInterface() : Id "
			  << PeerId().toStdString() << " " << type << std::endl;
#endif

	RS_STACK_MUTEX(mPersonMtx);

	kids[type] = pqi;
	return 1;
}

/***************** PRIVATE FUNCTIONS ***********************/
// functions to iterate over the connects and change state.


int pqiperson::listen()
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::listen() Id: " + PeerId().toStdString() << std::endl;
#endif

	RS_STACK_MUTEX(mPersonMtx);

	if (!active)
	{
		std::map<uint32_t, pqiconnect *>::iterator it;
		for(it = kids.begin(); it != kids.end(); ++it)
			(it->second)->listen();
	}
	return 1;
}


int pqiperson::stoplistening()
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::stoplistening() Id: " + PeerId().toStdString()
			  << std::endl;
#endif

	RS_STACK_MUTEX(mPersonMtx);

	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
		(it->second)->stoplistening();

	return 1;
}

int	pqiperson::connect(uint32_t type, const sockaddr_storage &raddr,
					   const sockaddr_storage &proxyaddr,
					   const sockaddr_storage &srcaddr,
					   uint32_t delay, uint32_t period, uint32_t timeout,
					   uint32_t flags, uint32_t bandwidth,
					   const std::string &domain_addr, uint16_t domain_port)
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::connect() Id: " << PeerId().toStdString()
			  << " type: " << type << " addr: "
			  << sockaddr_storage_tostring(raddr) << " proxyaddr: "
			  << sockaddr_storage_tostring(proxyaddr) << " srcaddr: "
			  << sockaddr_storage_tostring(srcaddr) << " delay: " << delay
			  << " period: " << period << " timeout: " << timeout << " flags: "
			  << flags << " bandwidth: " << bandwidth << std::endl;
#endif

	RS_STACK_MUTEX(mPersonMtx);

	std::map<uint32_t, pqiconnect *>::iterator it;
	
	it = kids.find(type);
	if (it == kids.end())
	{
		/* notify of fail! */
		pqipg->notifyConnect(PeerId(), type, false, false, raddr);
		return 0;
	}

	pqiconnect *pqi = it->second;

#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::connect() resetting for new connection attempt" << std::endl;
#endif

	/* set the parameters */
	pqi->reset();

#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::connect() clearing rate cap" << std::endl;
#endif
	setRateCap_locked(0,0);

#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::connect() setting connect_parameters" << std::endl;
#endif
	
	// These two are universal.
	pqi->connect_parameter(NET_PARAM_CONNECT_DELAY, delay);
	pqi->connect_parameter(NET_PARAM_CONNECT_TIMEOUT, timeout);

	// these 5 are only used by UDP connections.
	pqi->connect_parameter(NET_PARAM_CONNECT_PERIOD, period);
	pqi->connect_parameter(NET_PARAM_CONNECT_FLAGS, flags);
	pqi->connect_parameter(NET_PARAM_CONNECT_BANDWIDTH, bandwidth);

	pqi->connect_additional_address(NET_PARAM_CONNECT_PROXY, proxyaddr);
	pqi->connect_additional_address(NET_PARAM_CONNECT_SOURCE, srcaddr);

	// These are used by Proxy/Hidden 
	pqi->connect_parameter(NET_PARAM_CONNECT_DOMAIN_ADDRESS, domain_addr);
	pqi->connect_parameter(NET_PARAM_CONNECT_REMOTE_PORT, domain_port);

	pqi->connect(raddr);
		
	// flag if we started a new connectionAttempt.
	inConnectAttempt = true;

	return 1;
}


void pqiperson::getRates(RsBwRates &rates)
{
	RS_STACK_MUTEX(mPersonMtx);

	// get the rate from the active one.
	if ((!active) || (activepqi == NULL))
		return;

	activepqi->getRates(rates);
}

int pqiperson::gatherStatistics(std::list<RSTrafficClue>& out_lst,
								std::list<RSTrafficClue>& in_lst)
{
	RS_STACK_MUTEX(mPersonMtx);

	// Get the rate from the active one.
	if( (!active) || (activepqi == NULL) )
		return 0 ;

	return activepqi->gatherStatistics(out_lst, in_lst);
}

int pqiperson::getQueueSize(bool in)
{
	RS_STACK_MUTEX(mPersonMtx);

	// get the rate from the active one.
	if ((!active) || (activepqi == NULL))
		return 0;

	return activepqi->getQueueSize(in);
}

bool pqiperson::getCryptoParams(RsPeerCryptoParams & params)
{
	RS_STACK_MUTEX(mPersonMtx);

	if(active && activepqi != NULL)
		return activepqi->getCryptoParams(params);
	else
	{
		params.connexion_state = 0;
		params.cipher_name.clear();
		params.cipher_bits_1 = 0;
		params.cipher_bits_2 = 0;
		params.cipher_version.clear();

		return false ;
	}
}

bool pqiconnect::getCryptoParams(RsPeerCryptoParams & params)
{
	pqissl *ssl = dynamic_cast<pqissl*>(ni);

	if(ssl != NULL)
	{
		ssl->getCryptoParams(params);
		return true;
	}
	else
	{
		params.connexion_state = 0 ;
		params.cipher_name.clear() ;
		params.cipher_bits_1 = 0 ;
		params.cipher_bits_2 = 0 ;
		params.cipher_version.clear() ;
		return false ;
	}
}

float pqiperson::getRate(bool in)
{
	RS_STACK_MUTEX(mPersonMtx);

	// get the rate from the active one.
	if ((!active) || (activepqi == NULL))
		return 0;

	return activepqi -> getRate(in);
}

uint64_t pqiperson::getTraffic(bool in)
{
	if ((!active) || (activepqi == NULL))
		return 0;
	return activepqi -> getTraffic(in);
}

void pqiperson::setMaxRate(bool in, float val)
{
	RS_STACK_MUTEX(mPersonMtx);

	// set to all of them. (and us)
	PQInterface::setMaxRate(in, val);
	// clean up the children.
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
		(it->second) -> setMaxRate(in, val);
}

void pqiperson::setRateCap(float val_in, float val_out)
{
	// This methods might be called all the way down from pqiperson::tick() down
	// to pqissludp while completing a UDP connexion, causing a deadlock.
	//
	// We need to make sure the mutex is not already locked by current thread. If so, we call the
	// locked version directly if not, we lock, call, and unlock, possibly waiting if the
	// lock is already acquired by another thread.
	//
	// The lock cannot be locked by the same thread between the first test and
	// the "else" statement, so there is no possibility for this code to fail.
	//
	// We could actually put that code in RsMutex::lock()?
	// TODO: 2015/12/19 This code is already in RsMutex::lock() but is guarded
	// by RSTHREAD_SELF_LOCKING_GUARD which is specifically unset in the header
	// Why is that code guarded? Do it have an impact on performance?
	// Or we should not get in the situation of trying to relock the mutex on
	// the same thread NEVER?
	
	if(pthread_equal(mPersonMtx.owner(), pthread_self()))
		// Unlocked, or already locked by same thread
		setRateCap_locked(val_in, val_out);
	else
	{
		// Lock was free or locked by different thread => wait.
		RS_STACK_MUTEX(mPersonMtx);
		setRateCap_locked(val_in, val_out);
	}
}

void pqiperson::setRateCap_locked(float val_in, float val_out)
{
	// set to all of them. (and us)
	PQInterface::setRateCap(val_in, val_out);
	// clean up the children.
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
		(it->second)->setRateCap(val_in, val_out);
}
