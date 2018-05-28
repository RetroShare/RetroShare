/*******************************************************************************
 * libretroshare/src/dht: stunaddrassist.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie. <drbob@lunamutt.com>                  *
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
 ******************************************************************************/

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

virtual bool    getExternalAddr(struct sockaddr_storage &remote, uint8_t &stable) 
        {
		// IPV4 ONLY.
		struct sockaddr_in remotev4;
		if (mStunner->externalAddr(remotev4, stable))
		{
			sockaddr_storage_clear(remote);
			struct sockaddr_in *addr = (struct sockaddr_in *) &remote;
			addr->sin_family = AF_INET;
			addr->sin_addr = remotev4.sin_addr;
			addr->sin_port = remotev4.sin_port;

			return true;
		}
		return false;
        }

virtual int     tick()
	{
                return mStunner->tick();
        }

virtual void    setRefreshPeriod(int32_t period)
	{
		return mStunner->setTargetStunPeriod(period);
	}

	private:

	UdpStunner *mStunner;
};

#endif

