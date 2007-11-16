#ifndef RS_TLV_BASE_H
#define RS_TLV_BASE_H

/*
 * libretroshare/src/serialiser: rstlvbase.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Horatio, Chris Parker
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


/*******************************************************************
 * These are the general TLV (un)packing routines.
 *
 * Data is Serialised into the following format
 *
 * -----------------------------------------
 * | TLV TYPE (2 bytes)| TLV LEN (2 bytes) |
 * -----------------------------------------
 * |                                       |
 * |         Data ....                     |
 * |                                       |
 * -----------------------------------------
 *
 * Size is the total size of the TLV Field (including the 4 byte header)
 *
 * Like the lowlevel packing routines. They are usually 
 * created in pairs - one to pack the data, the other to unpack.
 *
 * GetTlvXXX(void *data, uint32_t size, uint32_t *offset, XXX *out);
 * SetTlvXXX(void *data, uint32_t size, uint32_t *offset, XXX *in);
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

#include <string>


/* 0b 0000 0000 0001 XXXX UInt32      */
/* 0b 0000 0000 0010 XXXX String      */
/* 0b 0000 0000 0011 XXXX IP:Port V4  */
/* 0b 0000 0000 0011 XXXX IP:Port V4  */

/******* BINARY TYPES ****************/
/* 0b 0000 0000 1000 XXXX CERT        */
/* 0b 0000 0000 1001 XXXX Priv Key    */
/* 0b 0000 0000 1010 XXXX Pub  Key    */
/* 0b 0000 0000 1011 XXXX Signature   */

/* 0b 0001 XXXX XXXX XXXX Compound    */

const uint16_t TLV_TYPE_UINT32_SIZE   = 0x0010;
const uint16_t TLV_TYPE_UINT32_POP    = 0x0011;
const uint16_t TLV_TYPE_UINT32_AGE    = 0x0012;
const uint16_t TLV_TYPE_UINT32_OFFSET = 0x0013;

const uint16_t TLV_TYPE_STR_HASH      = 0x0020;
const uint16_t TLV_TYPE_STR_NAME      = 0x0021;
const uint16_t TLV_TYPE_STR_PATH      = 0x0022;
const uint16_t TLV_TYPE_STR_PEERID    = 0x0023;
const uint16_t TLV_TYPE_STR_KEY       = 0x0024;
const uint16_t TLV_TYPE_STR_VALUE     = 0x0025;
const uint16_t TLV_TYPE_STR_COMMENT   = 0x0026;
const uint16_t TLV_TYPE_STR_TITLE     = 0x0027;

const uint16_t TLV_TYPE_UINT8_SERID   = 0x0028;

const uint16_t TLV_TYPE_IPV4_LOCAL    = 0x0030;
const uint16_t TLV_TYPE_IPV4_SERVER   = 0x0031;
const uint16_t TLV_TYPE_IPV4_LAST     = 0x0032;

	/**** Binary Types ****/
const uint16_t TLV_TYPE_CERT_XPGP     = 0x0080;
const uint16_t TLV_TYPE_CERT_X509     = 0x0081;
const uint16_t TLV_TYPE_CERT_OPENPGP  = 0x0082;

const uint16_t TLV_TYPE_PRIV_KEY_RSA  = 0x0090;

const uint16_t TLV_TYPE_PUB_KEY_RSA   = 0x00A0;

const uint16_t TLV_TYPE_SIGN_RSA_SHA1 = 0x00B0;

const uint16_t TLV_TYPE_BIN_FILEDATA  = 0x00C0;

	/**** Compound Types ****/
const uint16_t TLV_TYPE_FILEITEM      = 0x1001;
const uint16_t TLV_TYPE_FILESET       = 0x1002;
const uint16_t TLV_TYPE_FILEDATA      = 0x1003;

const uint16_t TLV_TYPE_KEYVALUE      = 0x1005;
const uint16_t TLV_TYPE_KEYVALUESET   = 0x1006;

const uint16_t TLV_TYPE_PEERSET       = 0x1007;
const uint16_t TLV_TYPE_SERVICESET    = 0x1008;


/**** Basic TLV Functions ****/
uint16_t GetTlvSize(void *data);
uint16_t GetTlvType(void *data);
bool     SetTlvBase(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint16_t len);
bool     SetTlvSize(void *data, uint32_t size, uint16_t len);


/**** Generic TLV Functions ****
 * This have the same data (int or string for example), 
 * but they can have different types eg. a string could represent a name or a path, 
 * so we include a type parameter in the arguments
 */

bool     SetTlvUInt32(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint32_t out);
bool     GetTlvUInt32(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint32_t *in);
uint32_t GetTlvUInt32Size();
uint32_t GetTlvUInt16Size();
uint32_t GetTlvUInt8Size();
/* additiona numerical set and Get Routine to be added

bool     SetTlvUInt16(...)
bool     SetTlvUInt8(...)
bool     GetTlvUInt16(...)
bool     GetTlvUInt8(...)
............................could above just be templated? */


bool     SetTlvString(void *data, uint32_t size, uint32_t *offset, uint16_t type, std::string out);
bool     GetTlvString(void *data, uint32_t size, uint32_t *offset, uint16_t type, std::string &in);
uint32_t GetTlvStringSize(std::string &in);

bool     SetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset, uint16_t type, struct sockaddr_in *out);
bool     GetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset, uint16_t type, struct sockaddr_in *in);
bool     GetTlvIpAddrPortV4Size();

/* additional function to be added

bool SetTlvBinData(void* data, uint32_t size, uint32_t* offset, uint16_t type, void* data_bin, uint32_t len_tlv)

above(SetTlvbinData) is partially implemented

bool GetTlvBinData(void* data, uint32_t size, uint32_t* offset, uint16_t type, void* data_bin, uint32_t len_tlv)

*************************************/
#endif
