/*
 * libretroshare/src/pqi pqi.h
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


#ifndef PQI_TOP_HEADER
#define PQI_TOP_HEADER

#include "serialiser/rsserial.h"


class P3Interface
{
public:
	P3Interface() {return; }
virtual ~P3Interface() {return; }

virtual int	tick() { return 1; }
virtual int	status() { return 1; }

virtual int	SendRsRawItem(RsRawItem *) = 0;
};


/**
 * @brief Interface to allow outgoing messages to be sent directly through to
 * the pqiperson, rather than being queued
 */
class pqiPublisher
{
        public:
virtual ~pqiPublisher() { return; }
virtual bool sendItem(RsRawItem *item) = 0;

};


#endif // PQI_TOP_HEADER
