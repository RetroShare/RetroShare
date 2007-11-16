#ifndef RS_BASE_ITEMS_H
#define RS_BASE_ITEMS_H

/*
 * libretroshare/src/serialiser: rsbaseitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

const uint8_t RS_PKT_TYPE_FILE_ITEM = 0x01;
const uint8_t RS_PKT_TYPE_FILE_DATA = 0x02;
const uint8_t RS_PKT_TYPE_CHAT      = 0x03;
const uint8_t RS_PKT_TYPE_MESSAGE   = 0x04;
const uint8_t RS_PKT_TYPE_STATUS    = 0x05;

/**************************************************************************/
class RsFileItem: public RsItem
{
	public:
	RsFileItem() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_FILE_ITEM, 0)
	{ return; }
virtual ~RsFileItem();
virtual void clear();

	uint32_t reqType;     /* a request / result etc */
	RsTlvFileItem file;   /* file information */
};

class RsFileItemSerialiser: public RsSerialType
{
	public:
	RsFileItemSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_FILE_ITEM)
	{ return; }
virtual     ~RsFileItemSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

class RsFileData: public RsItem
{
	public:
	RsFileData() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_FILE_DATA, 0)
	{ return; }
virtual ~RsFileData();
virtual void clear();

	RsTlvFileItem file;
	RsTlvFileData data;
};

class RsFileDataSerialiser: public RsSerialType
{
	public:
	RsFileDataSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_FILE_DATA)
	{ return; }
virtual     ~RsFileDataSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

class RsChatItem: public RsItem
{
	public:
	RsChatItem() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_CHAT, 0)
	{ return; }
virtual ~RsChatItem();
virtual void clear();

	uint32_t chatFlags;
	uint32_t sendTime;

	std::string message;

};

class RsChatItemSerialiser: public RsSerialType
{
	public:
	RsChatItemSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_CHAT)
	{ return; }
virtual     ~RsChatItemSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

class RsMessageItem: public RsItem
{
	public:
	RsMessageItem() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_MESSAGE, 0)
	{ return; }
virtual ~RsMessageItem();
virtual void clear();

	uint32_t msgFlags;
	uint32_t sendTime;
	uint32_t recvTime;

	std::string subject;
	std::string message;

	RsTlvPeerIdSet msgto;
	RsTlvPeerIdSet msgcc;
	RsTlvPeerIdSet msgbcc;

	RsTlvFileSet attachment;
};

class RsMessageItemSerialiser: public RsSerialType
{
	public:
	RsMessageItemSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_MESSAGE)
	{ return; }
virtual     ~RsMessageItemSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/

class RsStatus: public RsItem
{
	public:
	RsStatus() 
	:RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_STATUS, 0)
	{ return; }
virtual ~RsStatus();
virtual void clear();

	/* status */
	uint32_t status;
	RsTlvServiceIdSet services;
};

class RsStatusSerialiser: public RsSerialType
{
	public:
	RsStatusSerialiser()
	:RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_BASE, 
		RS_PKT_TYPE_STATUS)
	{ return; }
virtual     ~RsStatusSerialiser();
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

};

/**************************************************************************/


/**************************************************************************/
  /* BELOW HERE to be fully defined */
/**************************************************************************/

#endif

