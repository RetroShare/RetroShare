/*
 * libretroshare/src/pqi: pqilistener.h
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

#ifndef MRK_PQI_GENERIC_LISTEN_HEADER
#define MRK_PQI_GENERIC_LISTEN_HEADER

// operating system specific network header.
#include "pqi/pqinetwork.h"

class pqilistener
{
	public:

	pqilistener() 		{ return; }
virtual ~pqilistener() 		{ return; }

virtual int     tick()					{ return 1; }
virtual int     status()				{ return 1; }
virtual int     setListenAddr(struct sockaddr_in addr)  { return 1; }
virtual int	setuplisten() 				{ return 1; }
virtual int     resetlisten() 				{ return 1; }

};


#endif // MRK_PQI_GENERIC_LISTEN_HEADER
