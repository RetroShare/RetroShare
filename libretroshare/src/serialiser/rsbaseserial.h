#ifndef RS_BASE_PACKING_H
#define RS_BASE_PACKING_H

/*
 * libretroshare/src/serialiser: rsbaseserial.h
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

#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <retroshare/rsids.h>

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

bool getRawUInt8(void *data, uint32_t size, uint32_t *offset, uint8_t *out);
bool setRawUInt8(void *data, uint32_t size, uint32_t *offset, uint8_t in);

bool getRawUInt16( const void* data, uint32_t size, uint32_t *offset,
                   uint16_t *out );
bool setRawUInt16(void *data, uint32_t size, uint32_t *offset, uint16_t in);

bool getRawUInt32(void *data, uint32_t size, uint32_t *offset, uint32_t *out);
bool setRawUInt32(void *data, uint32_t size, uint32_t *offset, uint32_t in);

bool getRawUInt64(void *data, uint32_t size, uint32_t *offset, uint64_t *out);
bool setRawUInt64(void *data, uint32_t size, uint32_t *offset, uint64_t in);

bool getRawUFloat32(void *data, uint32_t size, uint32_t *offset, float& out);
bool setRawUFloat32(void *data, uint32_t size, uint32_t *offset, float in);

uint32_t getRawStringSize(const std::string &outStr);
bool getRawString(void *data, uint32_t size, uint32_t *offset, std::string &outStr);
bool setRawString(void *data, uint32_t size, uint32_t *offset, const std::string &inStr);

bool setRawTimeT(void *data, uint32_t size, uint32_t *offset, const time_t& inStr);
bool getRawTimeT(void *data, uint32_t size, uint32_t *offset, time_t& outStr);

#endif

