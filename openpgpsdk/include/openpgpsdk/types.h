/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. 
 * 
 * You may obtain a copy of the License at 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * 
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 */

#ifndef OPS_TYPES_H
#define OPS_TYPES_H

/** Special type for intermediate function casting, avoids warnings on
    some platforms
*/
typedef void (*ops_void_fptr)(void);
#define ops_fcast(f) ((ops_void_fptr)f)

/** ops_map_t
 */
typedef struct 
    {
    int type;
    char *string;
    } ops_map_t;

/** Boolean type */
typedef unsigned ops_boolean_t;

/** ops_content_tag_t */

/* PTag Content Tags */
/***************************/

/** Package Tags (aka Content Tags) and signature subpacket types.
 * This enumerates all rfc-defined packet tag values and the
 * signature subpacket type values that we understand.
 *
 * \see RFC4880 4.3
 * \see RFC4880 5.2.3.1
 */

enum ops_content_tag_t
    {
    OPS_PTAG_CT_RESERVED		= 0,	/*!< Reserved - a packet tag must not have this value */
    OPS_PTAG_CT_PK_SESSION_KEY		= 1,	/*!< Public-Key Encrypted Session Key Packet */
    OPS_PTAG_CT_SIGNATURE		= 2,	/*!< Signature Packet */
    OPS_PTAG_CT_SK_SESSION_KEY		= 3,	/*!< Symmetric-Key Encrypted Session Key Packet */
    OPS_PTAG_CT_ONE_PASS_SIGNATURE	= 4,	/*!< One-Pass Signature Packet */
    OPS_PTAG_CT_SECRET_KEY		= 5,	/*!< Secret Key Packet */
    OPS_PTAG_CT_PUBLIC_KEY		= 6,	/*!< Public Key Packet */
    OPS_PTAG_CT_SECRET_SUBKEY		= 7,	/*!< Secret Subkey Packet */
    OPS_PTAG_CT_COMPRESSED		= 8,	/*!< Compressed Data Packet */
    OPS_PTAG_CT_SE_DATA			= 9,	/*!< Symmetrically Encrypted Data Packet */
    OPS_PTAG_CT_MARKER			=10,	/*!< Marker Packet */
    OPS_PTAG_CT_LITERAL_DATA		=11,	/*!< Literal Data Packet */
    OPS_PTAG_CT_TRUST			=12,	/*!< Trust Packet */
    OPS_PTAG_CT_USER_ID			=13,	/*!< User ID Packet */
    OPS_PTAG_CT_PUBLIC_SUBKEY		=14,	/*!< Public Subkey Packet */
    OPS_PTAG_CT_RESERVED2		=15,	/*!< reserved */
    OPS_PTAG_CT_RESERVED3		=16,	/*!< reserved */
    OPS_PTAG_CT_USER_ATTRIBUTE		=17,	/*!< User Attribute Packet */
    OPS_PTAG_CT_SE_IP_DATA		=18,	/*!< Sym. Encrypted and Integrity Protected Data Packet */
    OPS_PTAG_CT_MDC			=19,	/*!< Modification Detection Code Packet */

    OPS_PARSER_PTAG			=0x100,	/*!< Internal Use: The packet is the "Packet Tag" itself - used when
						     callback sends back the PTag. */
    OPS_PTAG_RAW_SS			=0x101,	/*!< Internal Use: content is raw sig subtag */
    OPS_PTAG_SS_ALL			=0x102,	/*!< Internal Use: select all subtags */
    OPS_PARSER_PACKET_END		=0x103,

    /* signature subpackets (0x200-2ff) (type+0x200) */
    /* only those we can parse are listed here */
    OPS_PTAG_SIGNATURE_SUBPACKET_BASE	=0x200,		/*!< Base for signature subpacket types - All signature type
							     values are relative to this value. */
    OPS_PTAG_SS_CREATION_TIME		=0x200+2,	/*!< signature creation time */
    OPS_PTAG_SS_EXPIRATION_TIME		=0x200+3,	/*!< signature expiration time */

    OPS_PTAG_SS_EXPORTABLE_CERTIFICATION =0x200+4, /*!< exportable certification */
    OPS_PTAG_SS_TRUST			=0x200+5,	/*!< trust signature */
    OPS_PTAG_SS_REGEXP			=0x200+6,	/*!< regular expression */
    OPS_PTAG_SS_REVOCABLE		=0x200+7,	/*!< revocable */
    OPS_PTAG_SS_KEY_EXPIRATION_TIME	=0x200+9,	/*!< key expiration time */
    OPS_PTAG_SS_RESERVED		=0x200+10,	/*!< reserved */
    OPS_PTAG_SS_PREFERRED_SKA 		=0x200+11,	/*!< preferred symmetric algorithms */
    OPS_PTAG_SS_REVOCATION_KEY 		=0x200+12,	/*!< revocation key */
    OPS_PTAG_SS_ISSUER_KEY_ID		=0x200+16, /*!< issuer key ID */
    OPS_PTAG_SS_NOTATION_DATA		=0x200+20, /*!< notation data */
    OPS_PTAG_SS_PREFERRED_HASH          =0x200+21, /*!< preferred hash algorithms */
    OPS_PTAG_SS_PREFERRED_COMPRESSION	=0x200+22, /*!< preferred compression algorithms */
    OPS_PTAG_SS_KEY_SERVER_PREFS	=0x200+23, /*!< key server preferences */
    OPS_PTAG_SS_PREFERRED_KEY_SERVER	=0x200+24, /*!< Preferred Key Server */
    OPS_PTAG_SS_PRIMARY_USER_ID		=0x200+25, /*!< primary User ID */
    OPS_PTAG_SS_POLICY_URI		=0x200+26, /*!< Policy URI */
    OPS_PTAG_SS_KEY_FLAGS 		=0x200+27, /*!< key flags */
    OPS_PTAG_SS_SIGNERS_USER_ID		=0x200+28, /*!< Signer's User ID */
    OPS_PTAG_SS_REVOCATION_REASON	=0x200+29, /*!< reason for revocation */
    OPS_PTAG_SS_FEATURES		=0x200+30, /*!< features */
    OPS_PTAG_SS_SIGNATURE_TARGET =0x200+31, /*!< signature target */
    OPS_PTAG_SS_EMBEDDED_SIGNATURE=0x200+32, /*!< embedded signature */

    OPS_PTAG_SS_USERDEFINED00	=0x200+100, /*!< internal or user-defined */
    OPS_PTAG_SS_USERDEFINED01	=0x200+101, 
    OPS_PTAG_SS_USERDEFINED02	=0x200+102,
    OPS_PTAG_SS_USERDEFINED03	=0x200+103,
    OPS_PTAG_SS_USERDEFINED04	=0x200+104,
    OPS_PTAG_SS_USERDEFINED05	=0x200+105,
    OPS_PTAG_SS_USERDEFINED06	=0x200+106,
    OPS_PTAG_SS_USERDEFINED07	=0x200+107,
    OPS_PTAG_SS_USERDEFINED08	=0x200+108,
    OPS_PTAG_SS_USERDEFINED09	=0x200+109,
    OPS_PTAG_SS_USERDEFINED10	=0x200+110,

	
    /* pseudo content types */
    OPS_PTAG_CT_LITERAL_DATA_HEADER	=0x300,
    OPS_PTAG_CT_LITERAL_DATA_BODY	=0x300+1,
    OPS_PTAG_CT_SIGNATURE_HEADER	=0x300+2,
    OPS_PTAG_CT_SIGNATURE_FOOTER	=0x300+3,
    OPS_PTAG_CT_ARMOUR_HEADER		=0x300+4,
    OPS_PTAG_CT_ARMOUR_TRAILER		=0x300+5,
    OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER	=0x300+6,
    OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY	=0x300+7,
    OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER=0x300+8,
    OPS_PTAG_CT_UNARMOURED_TEXT		=0x300+9,
    OPS_PTAG_CT_ENCRYPTED_SECRET_KEY	=0x300+10, // In this case the algorithm specific fields will not be initialised
    OPS_PTAG_CT_SE_DATA_HEADER		=0x300+11,
    OPS_PTAG_CT_SE_DATA_BODY		=0x300+12,
    OPS_PTAG_CT_SE_IP_DATA_HEADER	=0x300+13,
    OPS_PTAG_CT_SE_IP_DATA_BODY		=0x300+14,
    OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY=0x300+15,

    /* commands to the callback */
    OPS_PARSER_CMD_GET_SK_PASSPHRASE	=0x400,
    OPS_PARSER_CMD_GET_SECRET_KEY	=0x400+1,


    /* Errors */
    OPS_PARSER_ERROR			=0x500,	/*!< Internal Use: Parser Error */
    OPS_PARSER_ERRCODE			=0x500+1, /*! < Internal Use: Parser Error with errcode returned */
    };

/** Used to specify whether subpackets should be returned raw, parsed or ignored.
 */

enum ops_parse_type_t
    {
    OPS_PARSE_RAW,	/*!< Callback Raw */
    OPS_PARSE_PARSED,	/*!< Callback Parsed */
    OPS_PARSE_IGNORE, 	/*!< Don't callback */
    };

typedef struct _ops_crypt_t ops_crypt_t;

/** ops_hash_t */
typedef struct _ops_hash_t ops_hash_t;

/** 
   keep both ops_content_tag_t and ops_packet_tag_t because we might
   want to introduce some bounds checking i.e. is this really a valid value
   for a packet tag? 
*/
typedef enum ops_content_tag_t ops_packet_tag_t;
/** SS types are a subset of all content types.
*/
typedef enum ops_content_tag_t ops_ss_type_t;
/* typedef enum ops_sig_type_t ops_sig_type_t; */

/** Revocation Reason type */
typedef unsigned char ops_ss_rr_code_t;

/** ops_parse_type_t */
typedef enum ops_parse_type_t ops_parse_type_t;

/** ops_parser_content_t */
typedef struct ops_parser_content_t ops_parser_content_t;

/** Reader Flags */
/*
typedef enum
    {
    OPS_RETURN_LENGTH=1,
    } ops_reader_flags_t;
typedef enum ops_reader_ret_t ops_reader_ret_t;
*/

/** Writer flags */
typedef enum
    {
    OPS_WF_DUMMY,
    } ops_writer_flags_t;
/** ops_writer_ret_t */
/* typedef enum ops_writer_ret_t ops_writer_ret_t; */

/**
 * \ingroup Create
 * Contains the required information about how to write
 */
typedef struct ops_create_info ops_create_info_t;

#endif
