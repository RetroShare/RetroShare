/*******************************************************************************
 * libretroshare/src/retroshare: rsdht.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RETROSHARE_DISC_GUI_INTERFACE_H
#define RETROSHARE_DISC_GUI_INTERFACE_H

#include <inttypes.h>
#include <string>
#include <list>
#include <map>
#include <retroshare/rstypes.h>

/* The Main Interface Class - for information about your Peers */
class RsDisc;
extern RsDisc   *rsDisc;

class RsDisc
{
	public:

	RsDisc()  { return; }
virtual ~RsDisc() { return; }

virtual bool	getDiscFriends(const RsPeerId &id, std::list<RsPeerId>& friends) = 0;
virtual bool	getDiscPgpFriends(const RsPgpId &pgpid, std::list<RsPgpId>& gpg_friends) = 0;
virtual bool 	getPeerVersion(const RsPeerId &id, std::string &versions) = 0;
virtual bool 	getWaitingDiscCount(unsigned int *sendCount, unsigned int *recvCount) = 0;

};

#endif
