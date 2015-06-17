/*
 * libretroshare/src/pqi: pqisslpersongrp.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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



#ifndef MRK_PQI_SSL_PERSON_HANDLER_HEADER
#define MRK_PQI_SSL_PERSON_HANDLER_HEADER

#include "pqi/pqipersongrp.h"

class p3PeerMgr;
class RsPeerCryptoParams;
class pqissl ;

class pqisslpersongrp: public pqipersongrp
{
	public:
    pqisslpersongrp(p3ServiceControl *ctrl, unsigned long flags, p3PeerMgr *pm)
    :pqipersongrp(ctrl, flags), mPeerMgr(pm) { return; }

	protected:

	/********* FUNCTIONS to OVERLOAD for specialisation ********/
virtual pqilistener *locked_createListener(const struct sockaddr_storage &laddr);
virtual pqiperson   *locked_createPerson(const RsPeerId& id, pqilistener *listener);
    /********* FUNCTIONS to OVERLOAD for specialisation ********/

    virtual void disconnectPeer(const RsPeerId& peer) ;

	private:

	p3PeerMgr *mPeerMgr;
	std::map<RsPeerId,pqissl*> ssl_tunnels ;
};


#endif // MRK_PQI_SSL_PERSON_HANDLER_HEADER
