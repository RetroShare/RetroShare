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
#include "util/rsnet.h"

/* 0b 0000 0000 0001 XXXX UInt8       */
/* 0b 0000 0000 0010 XXXX UInt16      */
/* 0b 0000 0000 0011 XXXX UInt32      */
/* 0b 0000 0000 0100 XXXX UInt64      */
/* 0b 0000 0000 0101 XXXX String      */
/* 0b 0000 0000 0110 XXXX IP:Port V4  */
/* 0b 0000 0000 0111 XXXX ??????      */

/******* BINARY TYPES *****************/
/* 0b 0000 0000 1000 XXXX CERT        */
/* 0b 0000 0000 1001 XXXX Priv Key    */
/* 0b 0000 0000 1010 XXXX Pub  Key    */
/* 0b 0000 0000 1011 XXXX Signature   */

/* 0b 0001 XXXX XXXX XXXX Compound    */

const uint16_t TLV_TYPE_UINT8_SERID   = 0x0010;

const uint16_t TLV_TYPE_UINT16_SERID  = 0x0020;

const uint16_t TLV_TYPE_UINT32_SIZE   = 0x0030;
const uint16_t TLV_TYPE_UINT32_POP    = 0x0031;
const uint16_t TLV_TYPE_UINT32_AGE    = 0x0032;
const uint16_t TLV_TYPE_UINT32_OFFSET = 0x0033;
const uint16_t TLV_TYPE_UINT32_SERID  = 0x0034;

const uint16_t TLV_TYPE_UINT64_SIZE   = 0x0040;
const uint16_t TLV_TYPE_UINT64_OFFSET = 0x0041;

const uint16_t TLV_TYPE_STR_HASH      = 0x0050;
const uint16_t TLV_TYPE_STR_NAME      = 0x0051;
const uint16_t TLV_TYPE_STR_PATH      = 0x0052;
const uint16_t TLV_TYPE_STR_PEERID    = 0x0053;
const uint16_t TLV_TYPE_STR_KEY       = 0x0054;
const uint16_t TLV_TYPE_STR_VALUE     = 0x0055;
const uint16_t TLV_TYPE_STR_COMMENT   = 0x0056;
const uint16_t TLV_TYPE_STR_TITLE     = 0x0057;
const uint16_t TLV_TYPE_STR_MSG       = 0x0058;
const uint16_t TLV_TYPE_STR_SUBJECT   = 0x0059;


const uint16_t TLV_TYPE_IPV4_LOCAL    = 0x0060;
const uint16_t TLV_TYPE_IPV4_SERVER   = 0x0061;
const uint16_t TLV_TYPE_IPV4_LAST     = 0x0062;

	/**** Binary Types ****/
const uint16_t TLV_TYPE_CERT_XPGP_DER = 0x0080;
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

bool     SetTlvUInt8(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint8_t out);
bool     GetTlvUInt8(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint8_t *in);

bool     SetTlvUInt16(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint16_t out);
bool     GetTlvUInt16(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint16_t *in);

bool     SetTlvUInt32(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint32_t out);
bool     GetTlvUInt32(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint32_t *in);

bool     SetTlvUInt64(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint64_t out);
bool     GetTlvUInt64(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint64_t *in);

uint32_t GetTlvUInt8Size();
uint32_t GetTlvUInt16Size();
uint32_t GetTlvUInt32Size();
uint32_t GetTlvUInt64Size();


bool     SetTlvString(void *data, uint32_t size, uint32_t *offset, uint16_t type, std::string out);
bool     GetTlvString(void *data, uint32_t size, uint32_t *offset, uint16_t type, std::string &in);
uint32_t GetTlvStringSize(std::string &in);

bool     SetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset, uint16_t type, struct sockaddr_in *out);
bool     GetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset, uint16_t type, struct sockaddr_in *in);
uint32_t GetTlvIpAddrPortV4Size();

/* additional function to be added

bool SetTlvBinData(void* data, uint32_t size, uint32_t* offset, uint16_t type, void* data_bin, uint32_t len_tlv)

above(SetTlvbinData) is partially implemented

bool GetTlvBinData(void* data, uint32_t size, uint32_t* offset, uint16_t type, void* data_bin, uint32_t len_tlv)

*************************************/
#endif
