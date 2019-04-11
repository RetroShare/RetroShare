/*******************************************************************************
 * libretroshare/src/serialiser: rsbaseserial.h                                *
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
#ifndef RS_BASE_PACKING_H
#define RS_BASE_PACKING_H

#include <string>
#include <stdlib.h>
#include <stdint.h>

#include "retroshare/rsids.h"
#include "util/rstime.h"

/*******************************************************************
 * This is at the lowlevel packing routines. They are usually 
 * created in pairs - one to pack the data, the other to unpack.
 *
 * getRawXXX(void *data, uint32_t size, uint32_t *offset, XXX *out);
 * setRawXXX(void *data, uint32_t size, uint32_t *offset, XXX *in);
 *
 *
 * data - the base pointer to the serialised data.
 * size - size of the memory pointed to by data.
 * *offset - where we want to (un)pack the data.
 * 		This is incremented by the datasize.
 *
 * *in / *out - the data to (un)pack.
 *
 ******************************************************************/

bool getRawUInt8(const void *data, uint32_t size, uint32_t *offset, uint8_t *out);
bool setRawUInt8(void *data, uint32_t size, uint32_t *offset, uint8_t in);

bool getRawUInt16(const void *data, uint32_t size, uint32_t *offset, uint16_t *out);
bool setRawUInt16(void *data, uint32_t size, uint32_t *offset, uint16_t in);

bool getRawUInt32(const void *data, uint32_t size, uint32_t *offset, uint32_t *out);
bool setRawUInt32(void *data, uint32_t size, uint32_t *offset, uint32_t in);

bool getRawUInt64(const void *data, uint32_t size, uint32_t *offset, uint64_t *out);
bool setRawUInt64(void *data, uint32_t size, uint32_t *offset, uint64_t in);

bool getRawUFloat32(const void *data, uint32_t size, uint32_t *offset, float& out);
bool setRawUFloat32(void *data, uint32_t size, uint32_t *offset, float in);

uint32_t getRawStringSize(const std::string &outStr);
bool getRawString(const void *data, uint32_t size, uint32_t *offset, std::string &outStr);
bool setRawString(void *data, uint32_t size, uint32_t *offset, const std::string &inStr);

bool getRawTimeT(const void *data, uint32_t size, uint32_t *offset, rstime_t& outTime);
bool setRawTimeT(void *data, uint32_t size, uint32_t *offset, const rstime_t& inTime);

#endif

