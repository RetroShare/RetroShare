#ifndef RSQBLOGITEM_H_
#define RSQBLOGITEM_H_

/*
 * libretroshare/src/serialiser: rsqblogitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Chris Parker.
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

#include <map>
#include <list>
#include <string>

#include "../serialiser/rsserviceids.h"
#include "../serialiser/rsserial.h"
#include "../serialiser/rsmsgitems.h"
#include "../serialiser/rstlvkvwide.h"


/*!
 *  retroshare qblog msg item for storing received and sent blog message
 */
class RsQblogMsg: public RsMsgItem
{
	public:
	RsQblogMsg()
	:RsMsgItem(RS_SERVICE_TYPE_QBLOG)

	{ return; }
virtual ~RsQblogMsg();

/// inherited method from RsItem
std::ostream &print(std::ostream &out, uint16_t indent = 0);

};


/*!
 *  to serialise rsQblogItems: method names are self explanatory
 */
class RsQblogMsgSerialiser : public RsMsgSerialiser
{

		public:
	RsQblogMsgSerialiser()
	:RsMsgSerialiser(RS_SERVICE_TYPE_QBLOG)
	{ return; }
virtual     ~RsQblogMsgSerialiser()
	{ return; }

};


#endif /*RSQBLOGITEM_H_*/
