/*******************************************************************************
 * libretroshare/src/pqi: pqipersongrp.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2008  Robert Fernie.                                     *
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
#ifndef MRK_PQI_PERSON_HANDLER_HEADER
#define MRK_PQI_PERSON_HANDLER_HEADER

#include "pqi/pqihandler.h"
#include "pqi/pqiperson.h"
#include "pqi/pqilistener.h"
#include "pqi/pqiservice.h"
#include "pqi/pqimonitor.h"
#include "pqi/p3cfgmgr.h"


// So this is a specific implementation 
//
// it is designed to have one pqilistensocket + a series of pqisockets
//
// as an added bonus, we are going to 
// make this a pqitunnelserver, to which services can be attached.

const unsigned long PQIPERSON_NO_LISTENER = 	0x0001;

const unsigned long PQIPERSON_ALL_BW_LIMITED =  0x0010;
struct RsPeerCryptoParams;

class pqipersongrp: public pqihandler, public pqiMonitor, public p3ServiceServer, public pqiNetListener
{
	public:
    pqipersongrp(p3ServiceControl *ctrl, unsigned long flags);

	/*************************** Setup *************************/
	/* pqilistener */

virtual bool resetListener(const struct sockaddr_storage &local); // overloaded from pqiNetListener 
int     init_listener(); 

	/*************** pqiMonitor callback ***********************/
virtual void    statusChange(const std::list<pqipeer> &plist);

#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections
virtual void    statusChanged();
///////////////////////////////////////////////////////////
#endif

	/******************* Peer Control **************************/
virtual int addPeer(const RsPeerId& id); /* can be overloaded for testing */
int     removePeer(const RsPeerId& id);
int     connectPeer(const RsPeerId& id
#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections
					, bool bConnect = false
///////////////////////////////////////////////////////////
#endif
					);

	// New speedy recv.
virtual bool RecvRsRawItem(RsRawItem *item);


	/* Work-around to dodgy pointer stuff */
int	tagHeartbeatRecvd(const RsPeerId& id);

	/*** callback from children ****/
bool    notifyConnect(const RsPeerId& id, uint32_t type, bool success, bool isIncomingConnection, const struct sockaddr_storage &remote_peer_address);
//bool    notifyConnect(std::string id, uint32_t type, bool success);

	// tick interfaces.
virtual int tick();
virtual int status();

virtual bool getCryptoParams(const RsPeerId&,RsPeerCryptoParams&) ;
	protected:

virtual bool locked_getCryptoParams(const RsPeerId&, RsPeerCryptoParams&) { return false ;}

	/********* FUNCTIONS to OVERLOAD for specialisation ********/
	// THESE NEED TO BE LOCKED UNTIL PQILISTENER IS THREAD-SAFE.
virtual pqilistener *locked_createListener(const struct sockaddr_storage &laddr) = 0;
virtual pqiperson   *locked_createPerson(const RsPeerId& id, pqilistener *listener) = 0;
	/********* FUNCTIONS to OVERLOAD for specialisation ********/

	/* Overloaded RsItem Check
	 * checks item->cid vs Person
	 */
virtual int checkOutgoingRsItem(RsItem *item, int global) 
	{ 
                (void) item;   /* suppress unused parameter warning */
                (void) global; /* suppress unused parameter warning */
		return 1; 
	}

	private:

	// The serviceserver operation.
    //int tickServiceRecv();
	//int tickServiceSend();

	pqilistener *pqil;
	RsMutex pqilMtx; /* MUTEX */
	unsigned long initFlags;
};

class pqipersongrpDummy: public pqipersongrp
{
	public:
    pqipersongrpDummy(p3ServiceControl *ctrl, unsigned long flags)
    :pqipersongrp(ctrl, flags) { return; }

	protected:

	/********* FUNCTIONS to OVERLOAD for specialisation ********/
virtual pqilistener *locked_createListener(const struct sockaddr_storage &laddr);
virtual pqiperson   *locked_createPerson(const RsPeerId& id, pqilistener *listener);
	/********* FUNCTIONS to OVERLOAD for specialisation ********/
};



#endif // MRK_PQI_PERSON_HANDLER_HEADER
