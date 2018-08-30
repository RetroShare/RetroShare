/*******************************************************************************
 * libretroshare/src/pqi: pqilistener.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef MRK_PQI_GENERIC_LISTEN_HEADER
#define MRK_PQI_GENERIC_LISTEN_HEADER

// operating system specific network header.
#include "pqi/pqinetwork.h"

class pqilistener
{
public:
	pqilistener() {}
	virtual ~pqilistener() {}
	virtual int tick() { return 1; }
	virtual int status() { return 1; }
	virtual int setListenAddr(const sockaddr_storage & /*addr*/) { return 1; }
	virtual int setuplisten() { return 1; }
	virtual int resetlisten() { return 1; }
};


#endif // MRK_PQI_GENERIC_LISTEN_HEADER
