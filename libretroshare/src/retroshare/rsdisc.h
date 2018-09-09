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

/**
 * Pointer to global instance of RsDisc service implementation
 * @jsonapi{development}
 */
extern RsDisc   *rsDisc;

class RsDisc
{
	public:

	RsDisc() {}
	virtual ~RsDisc() {}

	/**
	 * @brief getDiscFriends get a list with all friends (ssl id) to a given friend (ssl id)
	 * @jsonapi{development}
	 * @param[in] id peer to get the friends of
	 * @param[out] friends list of friends (ssl id)
	 * @return true on success false otherwise
	 */
	virtual bool	getDiscFriends(const RsPeerId &id, std::list<RsPeerId>& friends) = 0;

	/**
	 * @brief getDiscPgpFriends get a list with all friends (pgp id) to a given friend (pgp id)
	 * @jsonapi{development}
	 * @param[in] pgpid peer to get the friends of
	 * @param[out] gpg_friends list of friends (gpg id)
	 * @return true on success false otherwise
	 */
	virtual bool	getDiscPgpFriends(const RsPgpId &pgpid, std::list<RsPgpId>& gpg_friends) = 0;

	/**
	 * @brief getPeerVersion get the version string of a peer.
	 * @jsonapi{development}
	 * @param[in] id peer to get the version string of
	 * @param[out] versions version string sent by the peer
	 * @return true on success false otherwise
	 */
	virtual bool 	getPeerVersion(const RsPeerId &id, std::string &versions) = 0;

	/**
	 * @brief getWaitingDiscCount get the number of queued discovery packets.
	 * @jsonapi{development}
	 * @param[out] sendCount number of queued outgoing packets
	 * @param[out] recvCount number of queued incoming packets
	 * @return true on success false otherwise
	 */
	virtual bool 	getWaitingDiscCount(size_t &sendCount, size_t &recvCount) = 0;
};

#endif
