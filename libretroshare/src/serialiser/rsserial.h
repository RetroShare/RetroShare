/*******************************************************************************
 * libretroshare/src/serialiser: rsserial.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#pragma once

#include <cstring>
#include <map>
#include <string>
#include <iosfwd>
#include <cstdlib>
#include <cstdint>

#include "util/rsdeprecate.h"

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

struct RsItem;
class RsSerialType ;


class RsSerialiser
{
public:
	/** Remember that every pqistreamer allocates an input buffer of this size!
	 * So don't make it too big! */
	static constexpr uint32_t MAX_SERIAL_SIZE = 262143; /* 2^18 -1 */

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

RS_DEPRECATED_FOR(RsSerialiser::MAX_SERIAL_SIZE)
uint32_t getRsPktMaxSize();



/* helper fns for printing */
std::ostream &printRsItemBase(std::ostream &o, std::string n, uint16_t i);
std::ostream &printRsItemEnd(std::ostream &o, std::string n, uint16_t i);

/* defined in rstlvtypes.cc - redeclared here for ease */
std::ostream &printIndent(std::ostream &out, uint16_t indent);
/* Wrapper class for data that is serialised somewhere else */
