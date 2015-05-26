/*
 * libretroshare/src/services/p3banlist.h
 *
 * Exchange list of Peers for Banning / Whitelisting.
 *
 * Copyright 2011 by Robert Fernie.
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

#pragma once

#include <netinet/in.h>

class RsBanList;
extern RsBanList *rsBanList ;

class BanListPeer
{
public:

    struct sockaddr_storage addr;
    uint8_t masked_bytes ;
    uint32_t reason; 		// Dup Self, Dup Friend
    int level; 			// LOCAL, FRIEND, FoF.
    time_t mTs;
    bool state ; 			// true=>active, false=>just stored but inactive
};

class RsBanList
{
	public:
		virtual bool isAddressAccepted(const struct sockaddr_storage& addr) =0;
        virtual void getListOfBannedIps(std::list<BanListPeer>& list) =0;
};



