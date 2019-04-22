/*******************************************************************************
 * libretroshare/src/serialiser: rstlvbase.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie, Horatio, Chris Parker                 *
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

/*******************************************************************
 * These are the general TLV (un)packing routines.
 *
 * Data is Serialised into the following format
 *
 * -----------------------------------------
 * | TLV TYPE (2 bytes) | TLV LEN (4 bytes)|
 * -----------------------------------------
 * |                                       |
 * |         Data ....                     |
 * |                                       |
 * -----------------------------------------
 *
 * Originally TLV TYPE = 2 bytes, and TLV LEN = 2 bytes.
 * However with HTML and WSTRINGS the 64K Limit becomes limiting.
 * The TLV LEN = 4 bytes now!
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

#include <inttypes.h>
#include <string>
#include "util/rsnet.h"


/* 0b 0000 0000 0001 XXXX UInt8       */
/* 0b 0000 0000 0010 XXXX UInt16      */
/* 0b 0000 0000 0011 XXXX UInt32      */
/* 0b 0000 0000 0100 XXXX UInt64      */
/* 0b 0000 0000 0101 XXXX String      */
/* 0b 0000 0000 0110 XXXX Wide String */
/* 0b 0000 0000 0111 XXXX Hashes      */
/* 0b 0000 0000 1000 XXXX IP:Port V4  */

/******* BINARY TYPES *****************/
/* 0b 0000 0001 0000 XXXX CERT        */
/* 0b 0000 0001 0001 XXXX Priv Key    */
/* 0b 0000 0001 0010 XXXX Pub  Key    */
/* 0b 0000 0001 0011 XXXX Signature   */

/******* COMPOUND TYPES ***************/
/* 0b 0001 XXXX XXXX XXXX Compound    */
/* 0b 0001 0000 0000 XXXX FILE        */
/* 0b 0001 0000 0001 XXXX KEY VALUE   */
/* 0b 0001 0000 0010 XXXX PEERS       */
/* 0b 0001 0000 0011 XXXX SERVICES    */

/******* BIG LEN TYPES ***************/

/* 0b 1000 XXXX XXXX XXXX BIG LEN     */
/* 0b 1000 0000 0001 XXXX STRINGS     */
/* 0b 1000 0000 0010 XXXX WSTRINGS    */
/* 0b 1000 0000 0011 XXXX BINARY      */

/* 0b 1001 XXXX XXXX XXXX Compound    */


/* TLV HEADER SIZE (Reference) *******************************/
const uint32_t TLV_HEADER_TYPE_SIZE   = 2;
const uint32_t TLV_HEADER_LEN_SIZE    = 4;
const uint32_t TLV_HEADER_SIZE        = TLV_HEADER_TYPE_SIZE + TLV_HEADER_LEN_SIZE;
/* TLV HEADER SIZE (Reference) *******************************/

const uint16_t TLV_TYPE_UINT32_SIZE   = 0x0030;
const uint16_t TLV_TYPE_UINT32_POP    = 0x0031;
const uint16_t TLV_TYPE_UINT32_AGE    = 0x0032;
const uint16_t TLV_TYPE_UINT32_OFFSET = 0x0033;
const uint16_t TLV_TYPE_UINT32_SERID  = 0x0034;
const uint16_t TLV_TYPE_UINT32_BW     = 0x0035;
const uint16_t TLV_TYPE_UINT32_PARAM  = 0x0030;

const uint16_t TLV_TYPE_UINT64_SIZE   = 0x0040;
const uint16_t TLV_TYPE_UINT64_OFFSET = 0x0041;

const uint16_t TLV_TYPE_STR_PEERID    = 0x0050;
const uint16_t TLV_TYPE_STR_NAME      = 0x0051;
const uint16_t TLV_TYPE_STR_PATH      = 0x0052;
const uint16_t TLV_TYPE_STR_KEY       = 0x0053;
const uint16_t TLV_TYPE_STR_VALUE     = 0x0054;
const uint16_t TLV_TYPE_STR_COMMENT   = 0x0055;
const uint16_t TLV_TYPE_STR_TITLE     = 0x0056;
const uint16_t TLV_TYPE_STR_MSG       = 0x0057;
const uint16_t TLV_TYPE_STR_SUBJECT   = 0x0058;
const uint16_t TLV_TYPE_STR_LINK      = 0x0059;
const uint16_t TLV_TYPE_STR_GENID     = 0x005a;
const uint16_t TLV_TYPE_STR_GPGID     = 0x005b; /* depreciated */
const uint16_t TLV_TYPE_STR_PGPID     = 0x005b; /* same as GPG */
const uint16_t TLV_TYPE_STR_LOCATION  = 0x005c;
const uint16_t TLV_TYPE_STR_CERT_GPG  = 0x005d; 
const uint16_t TLV_TYPE_STR_PGPCERT   = 0x005d; /* same as CERT_GPG */
const uint16_t TLV_TYPE_STR_CERT_SSL  = 0x005e;
const uint16_t TLV_TYPE_STR_VERSION   = 0x005f;
const uint16_t TLV_TYPE_STR_PARAM     = 0x0054; /* same as VALUE ---- TO FIX */

/* Hashs are always strings */
const uint16_t TLV_TYPE_STR_HASH_SHA1 = 0x0070;
const uint16_t TLV_TYPE_STR_HASH_ED2K = 0x0071;

const uint16_t TLV_TYPE_IPV4_LOCAL    = 0x0080;
const uint16_t TLV_TYPE_IPV4_REMOTE   = 0x0081;
const uint16_t TLV_TYPE_IPV4_LAST     = 0x0082;
const uint16_t TLV_TYPE_STR_DYNDNS    = 0x0083;
const uint16_t TLV_TYPE_STR_DOMADDR   = 0x0084;

// rearrange these in the future.
const uint16_t TLV_TYPE_IPV4          = 0x0085;
const uint16_t TLV_TYPE_IPV6          = 0x0086;

/*** MORE STRING IDS ****/
const uint16_t TLV_TYPE_STR_GROUPID   = 0x00a0;
const uint16_t TLV_TYPE_STR_MSGID     = 0x00a1;
const uint16_t TLV_TYPE_STR_PARENTID  = 0x00a2;
const uint16_t TLV_TYPE_STR_THREADID  = 0x00a3;
const uint16_t TLV_TYPE_STR_KEYID     = 0x00a4;

/* even MORE string Ids for GXS services */

const uint16_t TLV_TYPE_STR_CAPTION   = 0x00b1;
const uint16_t TLV_TYPE_STR_CATEGORY  = 0x00b2;
const uint16_t TLV_TYPE_STR_DESCR     = 0x00b3;
const uint16_t TLV_TYPE_STR_SIGN      = 0x00b4;
const uint16_t TLV_TYPE_STR_HASH_TAG  = 0x00b5;
const uint16_t TLV_TYPE_STR_WIKI_PAGE = 0x00b6;
const uint16_t TLV_TYPE_STR_DATE      = 0x00b7;
const uint16_t TLV_TYPE_STR_PIC_TYPE  = 0x00b8;
const uint16_t TLV_TYPE_STR_PIC_AUTH  = 0x00b9;
const uint16_t TLV_TYPE_STR_GXS_ID    = 0x00ba;
const uint16_t TLV_TYPE_STR_COLOR     = 0x00bb;


	/**** Binary Types ****/
const uint16_t TLV_TYPE_CERT_XPGP_DER = 0x0100;
const uint16_t TLV_TYPE_CERT_X509     = 0x0101;
const uint16_t TLV_TYPE_CERT_OPENPGP  = 0x0102;

const uint16_t TLV_TYPE_KEY_EVP_PKEY  = 0x0110; /* Used (Generic - Distrib) */
const uint16_t TLV_TYPE_KEY_PRIV_RSA  = 0x0111; /* not used yet             */
const uint16_t TLV_TYPE_KEY_PUB_RSA   = 0x0112; /* not used yet             */

const uint16_t TLV_TYPE_SIGN_RSA_SHA1 = 0x0120; /* Used (Distrib/Forums)    */

const uint16_t TLV_TYPE_BIN_IMAGE     = 0x0130; /* Used (Generic - Forums)  */
const uint16_t TLV_TYPE_BIN_FILEDATA  = 0x0140; /* Used - ACTIVE!           */
const uint16_t TLV_TYPE_BIN_SERIALISE = 0x0150; /* Used (Generic - Distrib) */
const uint16_t TLV_TYPE_BIN_GENERIC   = 0x0160; /* Used (DSDV Data)         */
const uint16_t TLV_TYPE_BIN_ENCRYPTED = 0x0170; /* Encrypted data           */


	/**** Compound Types ****/
const uint16_t TLV_TYPE_FILEITEM      = 0x1000;
const uint16_t TLV_TYPE_FILESET       = 0x1001;
const uint16_t TLV_TYPE_FILEDATA      = 0x1002;

const uint16_t TLV_TYPE_KEYVALUE      = 0x1010;
const uint16_t TLV_TYPE_KEYVALUESET   = 0x1011;

const uint16_t TLV_TYPE_STRINGSET     = 0x1020; /* dummy non-existant */
const uint16_t TLV_TYPE_PEERSET       = 0x1021;
const uint16_t TLV_TYPE_HASHSET       = 0x1022;

const uint16_t TLV_TYPE_PGPIDSET      = 0x1023;
const uint16_t TLV_TYPE_RECOGNSET     = 0x1024;
const uint16_t TLV_TYPE_GXSIDSET      = 0x1025;
const uint16_t TLV_TYPE_GXSCIRCLEIDSET= 0x1026;
const uint16_t TLV_TYPE_NODEGROUPIDSET= 0x1027;
const uint16_t TLV_TYPE_GXSMSGIDSET   = 0x1028;

const uint16_t TLV_TYPE_SERVICESET    = 0x1030; 

// *_deprectate should not be used anymore!! 
// We use 1040 for both public and private keys, so that transmitting them still works (backward compatibility), and so that
// signatures are kept. But the two different classes will check that the flags are correct when deserialising.

const uint16_t TLV_TYPE_SECURITY_KEY   = 0x1040;
const uint16_t TLV_TYPE_SECURITYKEYSET = 0x1041;

const uint16_t TLV_TYPE_KEYSIGNATURE     = 0x1050;
const uint16_t TLV_TYPE_KEYSIGNATURESET  = 0x1051;
const uint16_t TLV_TYPE_KEYSIGNATURETYPE = 0x1052;

const uint16_t TLV_TYPE_IMAGE         = 0x1060;

const uint16_t TLV_TYPE_ADDRESS_INFO  = 0x1070;
const uint16_t TLV_TYPE_ADDRESS_SET   = 0x1071;
const uint16_t TLV_TYPE_ADDRESS       = 0x1072;

const uint16_t TLV_TYPE_DSDV_ENDPOINT = 0x1080;
const uint16_t TLV_TYPE_DSDV_ENTRY    = 0x1081;
const uint16_t TLV_TYPE_DSDV_ENTRY_SET= 0x1082;

const uint16_t TLV_TYPE_BAN_ENTRY_dep = 0x1090;
const uint16_t TLV_TYPE_BAN_ENTRY     = 0x1092;
const uint16_t TLV_TYPE_BAN_LIST      = 0x1091;

const uint16_t TLV_TYPE_MSG_ADDRESS   = 0x10A0;
const uint16_t TLV_TYPE_MSG_ID        = 0x10A1;


const uint32_t RSTLV_IMAGE_TYPE_PNG = 0x0001;
const uint32_t RSTLV_IMAGE_TYPE_JPG = 0x0002;

/**** Basic TLV Functions ****/
uint32_t GetTlvSize(void *data);
uint16_t GetTlvType(void *data);
bool     SetTlvBase(void *data, uint32_t size, uint32_t *offset, uint16_t type, uint32_t len);
bool     SetTlvSize(void *data, uint32_t size, uint32_t len);
bool 	 SetTlvType(void *data, uint32_t size, uint16_t type);

/* skip past the unknown tlv elements */
bool SkipUnknownTlv(void *data, uint32_t size, uint32_t *offset);

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
bool     GetTlvString(const void *data, uint32_t size, uint32_t *offset, uint16_t type, std::string &in);
uint32_t GetTlvStringSize(const std::string &in);

#ifdef REMOVED_CODE
bool     SetTlvWideString(void *data, uint32_t size, uint32_t *offset, uint16_t type, std::wstring out);
bool     GetTlvWideString(void *data, uint32_t size, uint32_t *offset, uint16_t type, std::wstring &in);
uint32_t GetTlvWideStringSize(std::wstring &in);
#endif

bool     SetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset, uint16_t type, struct sockaddr_in *out);
bool     GetTlvIpAddrPortV4(void *data, uint32_t size, uint32_t *offset, uint16_t type, struct sockaddr_in *in);
uint32_t GetTlvIpAddrPortV4Size();

bool     SetTlvIpAddrPortV6(void *data, uint32_t size, uint32_t *offset, uint16_t type, struct sockaddr_in6 *out);
bool     GetTlvIpAddrPortV6(void *data, uint32_t size, uint32_t *offset, uint16_t type, struct sockaddr_in6 *in);
uint32_t GetTlvIpAddrPortV6Size();

/* additional function to be added

bool SetTlvBinData(void* data, uint32_t size, uint32_t* offset, uint16_t type, void* data_bin, uint32_t len_tlv)

above(SetTlvbinData) is partially implemented

bool GetTlvBinData(void* data, uint32_t size, uint32_t* offset, uint16_t type, void* data_bin, uint32_t len_tlv)

*************************************/
