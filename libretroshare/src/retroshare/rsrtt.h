#ifndef RETROSHARE_RTT_INTERFACE_H
#define RETROSHARE_RTT_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsrtt.h
 *
 * RetroShare C++ Interface.
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

#include <inttypes.h>
#include <string>
#include <list>
#include <retroshare/rstypes.h>

/* The Main Interface Class - for information about your Peers */
class RsRtt;
extern RsRtt *rsRtt;


class RsRttPongResult
{
	public:
	RsRttPongResult()
	:mTS(0), mRTT(0), mOffset(0) { return; }

	RsRttPongResult(double ts, double rtt, double offset)
	:mTS(ts), mRTT(rtt), mOffset(offset) { return; }

	double mTS;
	double mRTT;
	double mOffset;
};

class RsRtt
{
	public:

	RsRtt()  { return; }
virtual ~RsRtt() { return; }

virtual uint32_t getPongResults(const RsPeerId& id, int n, std::list<RsRttPongResult> &results) = 0;
virtual double getMeanOffset(const RsPeerId& id) = 0;

};

#endif
