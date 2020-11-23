/*******************************************************************************
 * libretroshare/src/dht: connectstatebox.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie <drbob@lunamutt.com>                   *
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
#ifndef CONNECT_STATUS_BOX_H
#define CONNECT_STATUS_BOX_H

/* a connect state box */

#define CSB_START		1
#define CSB_TCP_WAIT		2
#define CSB_DIRECT_ATTEMPT	3
#define CSB_DIRECT_WAIT 	4
#define CSB_PROXY_ATTEMPT	5
#define CSB_PROXY_WAIT		6
#define CSB_RELAY_ATTEMPT	7
#define CSB_RELAY_WAIT		8
#define CSB_REVERSE_WAIT	9
#define CSB_FAILED_WAIT		10
#define CSB_CONNECTED		11


#define CSB_NETSTATE_UNKNOWN		0
#define CSB_NETSTATE_FORWARD		1
#define CSB_NETSTATE_STABLENAT		2
#define CSB_NETSTATE_EXCLUSIVENAT	3
#define CSB_NETSTATE_FIREWALLED		4

#define CSB_CONNECT_DIRECT		1
#define CSB_CONNECT_UNREACHABLE		2

/* return values */
#define CSB_ACTION_MASK_MODE		0x00ff
#define CSB_ACTION_MASK_PORT		0xff00

#define CSB_ACTION_WAIT			0x0001
#define CSB_ACTION_TCP_CONN		0x0002
#define CSB_ACTION_DIRECT_CONN		0x0004
#define CSB_ACTION_PROXY_CONN		0x0008
#define CSB_ACTION_RELAY_CONN		0x0010

#define CSB_ACTION_DHT_PORT		0x0100
#define CSB_ACTION_PROXY_PORT		0x0200

/* update input */
#define	CSB_UPDATE_NONE			0x0000
#define	CSB_UPDATE_CONNECTED		0x0001
#define	CSB_UPDATE_DISCONNECTED		0x0002
#define	CSB_UPDATE_AUTH_DENIED		0x0003
#define	CSB_UPDATE_RETRY_ATTEMPT	0x0004
#define	CSB_UPDATE_FAILED_ATTEMPT	0x0005
#define	CSB_UPDATE_MODE_UNAVAILABLE	0x0006

#include <iosfwd>
#include <string>

#include <stdlib.h>
#include "util/rstime.h"
#include <inttypes.h>

#include <retroshare/rsconfig.h>

class PeerConnectStateBox
{
	public:
	PeerConnectStateBox();

	uint32_t connectCb(uint32_t cbtype, RsNetworkMode netmode, RsNatHoleMode nathole, RsNatTypeMode nattype);
	uint32_t updateCb(uint32_t updateType);

	bool shouldUseProxyPort(RsNetworkMode netmode, RsNatHoleMode nathole, RsNatTypeMode nattype);

	uint32_t calcNetState(RsNetworkMode netmode, RsNatHoleMode nathole, RsNatTypeMode nattype);
	std::string connectState() const;

	std::string mPeerId;

	bool storeProxyPortChoice(uint32_t flags, bool useProxyPort);
	bool getProxyPortChoice();
	
	private:

	uint32_t connectCb_direct();
	uint32_t connectCb_unreachable();

	void errorMsg(std::ostream &out, std::string msg, uint32_t updateParam);
	void stateMsg(std::ostream &out, std::string msg, uint32_t updateParam);


	uint32_t mState;
	uint32_t mNetState;
	rstime_t mStateTS;
	uint32_t mNoAttempts;
	uint32_t mNoFailedAttempts;
	rstime_t mNextAttemptTS;
	rstime_t mAttemptLength;

	// ProxyPort Storage.
	uint32_t mProxyPortFlags;
	bool     mProxyPortChoice;
	rstime_t   mProxyPortTS;
};


#endif
