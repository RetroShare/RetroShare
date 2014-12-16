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


const int pqipersonzone = 82371;
#include "util/rsdebug.h"
#include "util/rsstring.h"
#include "retroshare/rspeers.h"

/****
 * #define PERSON_DEBUG 1
 ****/

pqiperson::pqiperson(const RsPeerId& id, pqipersongrp *pg)
	:PQInterface(id), mNotifyMtx("pqiperson-notify"), mPersonMtx("pqiperson"), 
	active(false), activepqi(NULL), 
	inConnectAttempt(false), waittimes(0), 
	pqipg(pg)
{

	/* must check id! */

	return;
}

pqiperson::~pqiperson()
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	// clean up the children.
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		pqiconnect *pc = (it->second);
		delete pc;
	}
	kids.clear();
}


	// The PQInterface interface.
int     pqiperson::SendItem(RsItem *i,uint32_t& serialized_size)
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	std::string out = "pqiperson::SendItem()";
	if (active)
	{
		out += " Active: Sending On\n";
		i->print_string(out, 5);
#ifdef PERSON_DEBUG
		std::cerr << out << std::endl;
#endif
		return activepqi -> SendItem(i,serialized_size);
	}
	else
	{
		out += " Not Active: Used to put in ToGo Store\n";
		out += " Now deleting...";
		delete i;
	}
	pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out);
	return 0; // queued.	
}

RsItem *pqiperson::GetItem()
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	if (active)
		return activepqi -> GetItem();
	// else not possible.
	return NULL;
}

bool pqiperson::RecvItem(RsItem *item)
{
#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::RecvItem()";
	std::cerr << std::endl;
#endif

	return pqipg->recvItem((RsRawItem *) item);
}


int 	pqiperson::status()
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	if (active)
		return activepqi -> status();
	return -1;
}

int 	pqiperson::receiveHeartbeat()
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

        pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::receiveHeartbeat() from peer : " + PeerId().toStdString());
        lastHeartbeatReceived = time(NULL);

		return true ;
}

	// tick......
int	pqiperson::tick()
{
	int activeTick = 0;
	{
		RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/
	
	        //if lastHeartbeatReceived is 0, it might be not activated so don't do a net reset.
		if (active && (lastHeartbeatReceived != 0) &&
	            (time(NULL) - lastHeartbeatReceived) > HEARTBEAT_REPEAT_TIME * 5) 
		{
			int ageLastIncoming = time(NULL) - activepqi->getLastIncomingTS();
			std::string out = "pqiperson::tick() WARNING No heartbeat from: " + PeerId().toStdString();
			//out << " assume dead. calling pqissl::reset(), LastHeartbeat was: ";
			rs_sprintf_append(out, " LastHeartbeat was: %ld secs ago", time(NULL) - lastHeartbeatReceived);
			rs_sprintf_append(out, " LastIncoming was: %d secs ago", ageLastIncoming);
			pqioutput(PQL_WARNING, pqipersonzone, out);
	
	#define NO_PACKET_TIMEOUT 60
			
			if (ageLastIncoming > NO_PACKET_TIMEOUT)
			{
				out = "pqiperson::tick() " + PeerId().toStdString();
				out += " No Heartbeat & No Packets -> assume dead. calling pqissl::reset()";
				pqioutput(PQL_WARNING, pqipersonzone, out);
	            	
				this->reset_locked();
			}
	
		}
	
	
		{
			std::string out = "pqiperson::tick() Id: " + PeerId().toStdString() + " ";
			if (active)
				out += "***Active***";
			else
				out += ">>InActive<<";
	
			out += "\n";
			rs_sprintf_append(out, "Activepqi: %p inConnectAttempt: ", activepqi);
	
			if (inConnectAttempt)
				out += "In Connection Attempt";
			else
				out += "   Not Connecting    ";
			out += "\n";
	
			// tick the children.
			std::map<uint32_t, pqiconnect *>::iterator it;
			for(it = kids.begin(); it != kids.end(); ++it)
			{
				if (0 < (it->second) -> tick())
				{
					activeTick = 1;
				}
				rs_sprintf_append(out, "\tTicking Child: %d\n", it->first);
			}
	
			pqioutput(PQL_DEBUG_ALL, pqipersonzone, out);
		} // end of pqioutput.
	}

	// handle Notify Events that were generated.
	processNotifyEvents();

	return activeTick;
}

// callback function for the child - notify of a change.
// This is only used for out-of-band info....
// otherwise could get dangerous loops.
// - Actually, now we have - must store and process later.
int 	pqiperson::notifyEvent(NetInterface *ni, int newState, const struct sockaddr_storage &remote_peer_address)
{
	if (mPersonMtx.trylock())
	{
		handleNotifyEvent_locked(ni, newState, remote_peer_address);

		mPersonMtx.unlock();
		
		return 1;
	}


	RsStackMutex stack(mNotifyMtx); /**** LOCK MUTEX ****/

	mNotifyQueue.push_back(NotifyData(ni, newState, remote_peer_address));

	return 1;
}


void 	pqiperson::processNotifyEvents()
{
	NetInterface *ni;
	int state;
	struct sockaddr_storage addr;

	while(1)
	{
		{
			RsStackMutex stack(mNotifyMtx); /**** LOCK MUTEX ****/

			if (mNotifyQueue.empty())
			{
				return;
			}
			NotifyData &data = mNotifyQueue.front();
			ni = data.mNi;
			state = data.mState;
			addr = data.mAddr;

			mNotifyQueue.pop_front();
		}

		RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/
		handleNotifyEvent_locked(ni, state, addr);
	}
	return;
}


int 	pqiperson::handleNotifyEvent_locked(NetInterface *ni, int newState, const struct sockaddr_storage &remote_peer_address)
{

	{
		std::string out = "pqiperson::notifyEvent() Id: " + PeerId().toStdString() + "\n";
		rs_sprintf_append(out, "Message: %d from: %p\n", newState, ni);
		rs_sprintf_append(out, "Active pqi : %p", activepqi);

		pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out);
	}

	/* find the pqi, */
	pqiconnect *pqi = NULL;
	uint32_t    type = 0;
	std::map<uint32_t, pqiconnect *>::iterator it;
		
	/* start again */
	int i = 0;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		std::string out;
		rs_sprintf(out, "pqiperson::connectattempt() Kid# %d of %u\n", i, kids.size());
		rs_sprintf_append(out, " type: %u in_ni: %p", it->first, ni);
		pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out);
		i++;

		if ((it->second)->thisNetInterface(ni))
		{
			pqi = (it->second);
			type = (it->first);
		}
	}

	if (!pqi)
	{
	  pqioutput(PQL_WARNING, pqipersonzone, "Unknown notfyEvent Source!");
	  return -1;
	}
		

	switch(newState)
	{
	case CONNECT_RECEIVED:
	case CONNECT_SUCCESS:

		/* notify */
                if (pqipg) {
                        pqipg->notifyConnect(PeerId(), type, true, remote_peer_address);
                }

		if ((active) && (activepqi != pqi)) // already connected - trouble
		{
			pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::notifyEvent() Id: " + PeerId().toStdString() + " CONNECT_SUCCESS+active-> activing new connection, shutting others");

			// This is the RESET that's killing the connections.....
                        //activepqi -> reset();
			// this causes a recursive call back into this fn.
			// which cleans up state.
			// we only do this if its not going to mess with new conn.
		}

		/* now install a new one. */
        if(!active)
        {

			pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::notifyEvent() Id: " + PeerId().toStdString() + " CONNECT_SUCCESS->marking so! (resetting others)");

			// mark as active.
			active = true;
                        lastHeartbeatReceived = 0;
			activepqi = pqi;
	  		inConnectAttempt = false;

			activepqi->start();  // STARTUP THREAD.

			/* reset all other children? (clear up long UDP attempt) */
			for(it = kids.begin(); it != kids.end(); ++it)
			{
				if (!(it->second)->thisNetInterface(ni))
				{
					std::string out;
					rs_sprintf(out, "Resetting pqi ref : %p", &(it->second));
					pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out);
					it->second->reset();
				} else {
					//std::cerr << "Active pqi : not resetting." << std::endl;
				}
			}
			return 1;
		}
		break;
	case CONNECT_UNREACHABLE:
	case CONNECT_FIREWALLED:
	case CONNECT_FAILED:


		if (active)
		{
			if (activepqi == pqi)
			{
				pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::notifyEvent() Id: " + PeerId().toStdString() + " CONNECT_FAILED->marking so!");

				activepqi->stop(); // STOP THREAD.
				active = false;
				activepqi = NULL;
			}
			else 
			{
				pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::notifyEvent() Id: " + PeerId().toStdString() + " CONNECT_FAILED-> from an unactive connection, don't flag the peer as not connected, just try next attempt !");
			}
		}
		else
		{
			pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::notifyEvent() Id: " + PeerId().toStdString() + " CONNECT_FAILED+NOT active -> try connect again");
		}

		/* notify up */
		if (pqipg)
		{
			pqipg->notifyConnect(PeerId(), type, false, remote_peer_address);
		}

		return 1;

		break;
	default:
		break;
	}
	return -1;
}

/***************** Not PQInterface Fns ***********************/

int 	pqiperson::reset()
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	return reset_locked();
}

int 	pqiperson::reset_locked()
{
	pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::reset() resetting all pqiconnect for Id: " + PeerId().toStdString());

	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		(it->second) -> stop(); // STOP THREAD.
		(it->second) -> reset();
	}		

	activepqi = NULL;
	active = false;
	lastHeartbeatReceived = 0;

	return 1;
}

int 	pqiperson::fullstopthreads()
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::fullstopthreads() for Id: " + PeerId().toStdString());

	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		(it->second) -> fullstop(); // WAIT FOR THREAD TO STOP.
	}		

	activepqi = NULL;
	active = false;
	lastHeartbeatReceived = 0;

	return 1;
}

int	pqiperson::addChildInterface(uint32_t type, pqiconnect *pqi)
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	{
		std::string out;
		rs_sprintf(out, "pqiperson::addChildInterface() : Id %s %u", PeerId().toStdString().c_str(), type);
		pqioutput(PQL_DEBUG_BASIC, pqipersonzone, out);
	}

	kids[type] = pqi;
	return 1;
}

/***************** PRIVATE FUNCTIONS ***********************/
// functions to iterate over the connects and change state.


int 	pqiperson::listen()
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	pqioutput(PQL_DEBUG_BASIC, pqipersonzone, "pqiperson::listen() Id: " + PeerId().toStdString());

	if (!active)
	{
		std::map<uint32_t, pqiconnect *>::iterator it;
		for(it = kids.begin(); it != kids.end(); ++it)
		{
			// set them all listening.
			(it->second) -> listen();
		}
	}
	return 1;
}


int 	pqiperson::stoplistening()
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	pqioutput(PQL_DEBUG_BASIC, pqipersonzone, "pqiperson::stoplistening() Id: " + PeerId().toStdString());

	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		// set them all listening.
		(it->second) -> stoplistening();
	}
	return 1;
}

int	pqiperson::connect(uint32_t type, const struct sockaddr_storage &raddr, 
				const struct sockaddr_storage &proxyaddr, const struct sockaddr_storage &srcaddr,
				uint32_t delay, uint32_t period, uint32_t timeout, uint32_t flags, uint32_t bandwidth, 
				const std::string &domain_addr, uint16_t domain_port)
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

#ifdef PERSON_DEBUG
#endif
	{
		std::string out = "pqiperson::connect() Id: " + PeerId().toStdString();
		rs_sprintf_append(out, " type: %u", type);
		out += " addr: ";
		out += sockaddr_storage_tostring(raddr);
		out += " proxyaddr: ";
		out += sockaddr_storage_tostring(proxyaddr);
		out += " srcaddr: ";
		out += sockaddr_storage_tostring(srcaddr);
		rs_sprintf_append(out, " delay: %u", delay);
		rs_sprintf_append(out, " period: %u", period);
		rs_sprintf_append(out, " timeout: %u", timeout);
		rs_sprintf_append(out, " flags: %u", flags);
		rs_sprintf_append(out, " bandwidth: %u", bandwidth);
		//std::cerr << out.str();
		pqioutput(PQL_WARNING, pqipersonzone, out);
	}

	std::map<uint32_t, pqiconnect *>::iterator it;
	
	it = kids.find(type);
	if (it == kids.end())
	{
#ifdef PERSON_DEBUG
		//pqioutput(PQL_DEBUG_BASIC, pqipersonzone, "pqiperson::connect() missing pqiconnect");
#endif
		/* notify of fail! */

		pqipg->notifyConnect(PeerId(), type, false, raddr);

		return 0;
	}

#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::connect() WARNING, resetting for new connection attempt" << std::endl;
#endif
	/* set the parameters */
	pqioutput(PQL_WARNING, pqipersonzone, "pqiperson::connect reset() before connection attempt");
	(it->second)->reset();

#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::connect() WARNING, clearing rate cap" << std::endl;
#endif
	setRateCap_locked(0,0);

#ifdef PERSON_DEBUG
	std::cerr << "pqiperson::connect() setting connect_parameters" << std::endl;
#endif
	
	// These two are universal.
	(it->second)->connect_parameter(NET_PARAM_CONNECT_DELAY, delay);
	(it->second)->connect_parameter(NET_PARAM_CONNECT_TIMEOUT, timeout);

	// these 5 are only used by UDP connections.
	(it->second)->connect_parameter(NET_PARAM_CONNECT_PERIOD, period);
	(it->second)->connect_parameter(NET_PARAM_CONNECT_FLAGS, flags);
	(it->second)->connect_parameter(NET_PARAM_CONNECT_BANDWIDTH, bandwidth);

	(it->second)->connect_additional_address(NET_PARAM_CONNECT_PROXY, proxyaddr);
	(it->second)->connect_additional_address(NET_PARAM_CONNECT_SOURCE, srcaddr);

	// These are used by Proxy/Hidden 
	(it->second)->connect_parameter(NET_PARAM_CONNECT_DOMAIN_ADDRESS, domain_addr);
	(it->second)->connect_parameter(NET_PARAM_CONNECT_REMOTE_PORT, domain_port);

	(it->second)->connect(raddr);
		
	// flag if we started a new connectionAttempt.
	inConnectAttempt = true;

	return 1;
}


void    pqiperson::getRates(RsBwRates &rates)
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	// get the rate from the active one.
	if ((!active) || (activepqi == NULL))
		return;
	activepqi -> getRates(rates);
}
int     pqiperson::gatherOutQueueStatistics(std::vector<uint32_t>& per_service,std::vector<uint32_t>& per_priority)
{
    RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

    // get the rate from the active one.
    if ((!active) || (activepqi == NULL))
        return 0;
    return activepqi -> gatherOutQueueStatistics(per_service,per_priority);
}
int     pqiperson::getQueueSize(bool in)
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	// get the rate from the active one.
	if ((!active) || (activepqi == NULL))
		return 0;
	return activepqi -> getQueueSize(in);
}

bool pqiperson::getCryptoParams(RsPeerCryptoParams& params)
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	if(active && activepqi != NULL)
		return activepqi->getCryptoParams(params) ;
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

bool pqiconnect::getCryptoParams(RsPeerCryptoParams& params)
{
	pqissl *ssl = dynamic_cast<pqissl*>(ni) ;

	if(ssl != NULL)
	{
		ssl->getCryptoParams(params) ;
		return true ;
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

float   pqiperson::getRate(bool in)
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	// get the rate from the active one.
	if ((!active) || (activepqi == NULL))
		return 0;
	return activepqi -> getRate(in);
}

void    pqiperson::setMaxRate(bool in, float val)
{
	RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/

	// set to all of them. (and us)
	PQInterface::setMaxRate(in, val);
	// clean up the children.
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		(it->second) -> setMaxRate(in, val);
	}
}

void    pqiperson::setRateCap(float val_in, float val_out)
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
	// We could actually put that code in RsMutex::lock()
	
	if(pthread_equal(mPersonMtx.owner(),pthread_self()))			// 1 - unlocked, or already locked by same thread
		return setRateCap_locked(val_in, val_out);					//     -> do nothing
	else
	{																				// 2 - lock was free or locked by different thread => wait.
		RsStackMutex stack(mPersonMtx); /**** LOCK MUTEX ****/
		setRateCap_locked(val_in, val_out);
	}
}

void    pqiperson::setRateCap_locked(float val_in, float val_out)
{
	// set to all of them. (and us)
	PQInterface::setRateCap(val_in, val_out);
	// clean up the children.
	std::map<uint32_t, pqiconnect *>::iterator it;
	for(it = kids.begin(); it != kids.end(); ++it)
	{
		(it->second) -> setRateCap(val_in, val_out);
	}
}



