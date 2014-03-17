#ifndef RETROSHARE_DISC_GUI_INTERFACE_H
#define RETROSHARE_DISC_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsdisc.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2008 by Robert Fernie.
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
