/*
 * libretroshare/src/dht: stunaddrassist.h
 *
 * BitDht interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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


#ifndef STUN_ADDR_ASSIST_H
#define STUN_ADDR_ASSIST_H

#include "pqi/pqiassist.h"
#include "tcponudp/udpstunner.h"

class  stunAddrAssist: public pqiAddrAssist
{
        public:

        stunAddrAssist(UdpStunner *stunner)
        {
                mStunner = stunner;
        }

virtual bool    getExternalAddr(struct sockaddr_in &remote, uint8_t &stable) 
        {
                return mStunner->externalAddr(remote, stable);
        }

	private:

	UdpStunner *mStunner;
};

#endif

