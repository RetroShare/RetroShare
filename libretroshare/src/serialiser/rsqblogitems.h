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
 
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rsmsgitems.h"
#include "serialiser/rstlvkvwide.h"


const uint8_t RS_PKT_SUBTYPE_QBLOG_PROFILE = 0x01;


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
 *  retroshare qblog profile item for storing received and sent profile info
 * designed in an open ended way to accomodate multiple fields
 */
class RsQblogProfile: public RsItem
{
	public:
	RsQblogProfile() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_QBLOG, // add profile id type
		RS_PKT_SUBTYPE_QBLOG_PROFILE)
	{ return; }
virtual ~RsQblogProfile();
virtual void clear();

/// inherited method from RsItem
std::ostream &print(std::ostream &out, uint16_t indent = 0);

uint32_t timeStamp;

/// contains various profile information set by user, this and below use an open ended format 
RsTlvKeyValueWideSet openProfile;

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

/*!
 *  to serialise rsQblogProfile items, method names are self explanatory 
 */
class RsQblogProfileSerialiser : public RsSerialType
{

		public:
	RsQblogProfileSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_QBLOG)
	{ return; }
virtual     ~RsQblogProfileSerialiser()
	{ return; }
					/**
					 * check size of RsItem to be serialised
					 * @param RsItem RsItem which is going to be serilised
					 * @return size of the RsItem
					 */	
virtual	uint32_t    size(RsItem *);

					/**
					 * serialise contents of item to data
					 * @param item RsItem which is going to be serilised
					 * @param data where contents will be serialised into
					 * @return size of the RsItem in bytes
					 */	
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);

					/**
					 * serialise contents of item to data
					 * @param data where contents will be deserialisedout of
					 * @return size of the RsItem in bytes
					 */	
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

				
virtual	uint32_t    sizeItem(RsQblogProfile *);
virtual	bool        serialiseItem  (RsQblogProfile *item, void *data, uint32_t *size);
virtual	RsQblogProfile *deserialiseItem(void *data, uint32_t *size);
	
};

#endif /*RSQBLOGITEM_H_*/
