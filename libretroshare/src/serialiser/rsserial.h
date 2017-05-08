#ifndef RS_BASE_SERIALISER_H
#define RS_BASE_SERIALISER_H

/*
 * libretroshare/src/serialiser: rsserial.h
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

#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <iosfwd>
#include <stdlib.h>
#include <stdint.h>

/*******************************************************************
 * This is the Top-Level serialiser/deserialise, 
 *
 * Data is Serialised into the following format
 *
 * -----------------------------------------
 * |    TYPE (4 bytes) | Size (4 bytes)    |
 * -----------------------------------------
 * |                                       |
 * |         Data ....                     |
 * |                                       |
 * -----------------------------------------
 *
 * Size is the total size of the packet (including the 8 byte header)
 * Type is composed of:
 *
 * 8 bits: Version (0x01)
 * 8 bits: Class  
 * 8 bits: Type
 * 8 bits: SubType
 ******************************************************************/

const uint8_t RS_PKT_VERSION1        = 0x01;
const uint8_t RS_PKT_VERSION_SERVICE = 0x02;

const uint8_t RS_PKT_CLASS_BASE      = 0x01;
const uint8_t RS_PKT_CLASS_CONFIG    = 0x02;

const uint8_t RS_PKT_SUBTYPE_DEFAULT = 0x01; /* if only one subtype */

class RsItem ;
class RsSerialType ;


class RsSerialiser
{
	public:
	RsSerialiser();
	~RsSerialiser();
	bool        addSerialType(RsSerialType *type);

	uint32_t    size(RsItem *);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);
	
	
	private:
	std::map<uint32_t, RsSerialType *> serialisers;
};

bool     setRsItemHeader(void *data, uint32_t size, uint32_t type, uint32_t pktsize);

/* Extract Header Information from Packet */
uint32_t getRsItemId(void *data);
uint32_t getRsItemSize(void *data);

uint8_t  getRsItemVersion(uint32_t type);
uint8_t  getRsItemClass(uint32_t type);
uint8_t  getRsItemType(uint32_t type);
uint8_t  getRsItemSubType(uint32_t type);

uint16_t  getRsItemService(uint32_t type);

/* size constants */
uint32_t getRsPktBaseSize();
uint32_t getRsPktMaxSize();



/* helper fns for printing */
std::ostream &printRsItemBase(std::ostream &o, std::string n, uint16_t i);
std::ostream &printRsItemEnd(std::ostream &o, std::string n, uint16_t i);

/* defined in rstlvtypes.cc - redeclared here for ease */
std::ostream &printIndent(std::ostream &out, uint16_t indent);
/* Wrapper class for data that is serialised somewhere else */

#endif /* RS_BASE_SERIALISER_H */
