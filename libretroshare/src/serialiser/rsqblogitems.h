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
#includ <list>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

/*! retroshare data sturcture to be serialised */
class RsQblogItem: RsItem
{
	public:
	RsQblogItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_QBLOG, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
virtual ~RsQblogItem();
virtual void clear();

/**
 * Used for unit test and checking serialisation occured fine on both ends
 * @param out output stream for printing
 * @param indent allows user to choose indentation of output stream
 * @return pointer to stream object, to store output to a variety of formats
 */
std::ostream &print(std::ostream &out, uint16_t indent = 0);


std::multimap<uint32_t, std::string> blogMsgs; /// contain blog mesgs and their blogged times (client time)
std::string status; /// to be serialised: status of a requested user
};

/*! to serialise rsQblogItems: method names are self explanatory */
class RsQblogSerialiser : RsSerialType
{

		public:
	RsQblogSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_QBLOG)
	{ return; }
virtual     ~RsQblogSerialiser()
	{ return; }
					/**
					 * check size of RsItem to be serialised
					 * @param RsItem RsItem which is going to be serilised
					 * @return size of the RsItem
					 */	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

				
virtual	uint32_t    sizeItem(RsQblogItem *);
virtual	bool        serialiseItem  (RsQblogItem *item, void *data, uint32_t *size);
virtual	RsChatItem *deserialiseItem(void *data, uint32_t *size);
	
};


#endif /*RSQBLOGITEM_H_*/
