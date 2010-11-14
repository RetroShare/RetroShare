#ifndef RETROSHARE_P3_DISC_INTERFACE_H
#define RETROSHARE_P3_DISC_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3discovery.h
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

#include "retroshare/rsdisc.h"
#include "services/p3disc.h"

class p3Discovery: public RsDisc
{
	public:

	p3Discovery(p3disc *disc)
	:mDisc(disc) { return; }
virtual ~p3Discovery() { return; }

virtual bool	getDiscFriends(std::string id, std::list<std::string> &friends);
virtual bool	getDiscGPGFriends(std::string id, std::list<std::string> &gpg_friends);
virtual bool 	getDiscVersions(std::map<std::string, std::string> &versions);

	private:

	p3disc *mDisc;
};

#endif

