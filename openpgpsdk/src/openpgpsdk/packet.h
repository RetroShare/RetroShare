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
 * packet related headers.
 */

#ifndef OPS_PACKET_H
#define OPS_PACKET_H

#include "configure.h"

#include <time.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include "types.h"
#include "errors.h"

/** General-use structure for variable-length data 
 */

typedef struct
    {
    size_t len;
    unsigned char *contents;
    } ops_data_t;

/************************************/
/* Packet Tags - RFC4880, 4.2 */
/************************************/

/** Packet Tag - Bit 7 Mask (this bit is always set).
 * The first byte of a packet is the "Packet Tag".  It always
 * has bit 7 set.  This is the mask for it.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_ALWAYS_SET		0x80

/** Packet Tag - New Format Flag.
 * Bit 6 of the Packet Tag is the packet format indicator.
 * If it is set, the new format is used, if cleared the
 * old format is used.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_NEW_FORMAT		0x40


/** Old Packet Format: Mask for content tag.
 * In the old packet format bits 5 to 2 (including)
 * are the content tag.  This is the mask to apply
 * to the packet tag.  Note that you need to
 * shift by #OPS_PTAG_OF_CONTENT_TAG_SHIFT bits.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_OF_CONTENT_TAG_MASK	0x3c
/** Old Packet Format: Offset for the content tag.
 * As described at #OPS_PTAG_OF_CONTENT_TAG_MASK the
 * content tag needs to be shifted after being masked
 * out from the Packet Tag.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_OF_CONTENT_TAG_SHIFT	2
/** Old Packet Format: Mask for length type.
 * Bits 1 and 0 of the packet tag are the length type
 * in the old packet format.
 *
 * See #ops_ptag_of_lt_t for the meaning of the values.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_OF_LENGTH_TYPE_MASK	0x03


/** Old Packet Format Lengths.
 * Defines the meanings of the 2 bits for length type in the
 * old packet format.
 *
 * \see RFC4880 4.2.1
 */
typedef enum
    {
    OPS_PTAG_OF_LT_ONE_BYTE		=0x00, /*!< Packet has a 1 byte length - header is 2 bytes long. */ 
    OPS_PTAG_OF_LT_TWO_BYTE		=0x01, /*!< Packet has a 2 byte length - header is 3 bytes long. */ 
    OPS_PTAG_OF_LT_FOUR_BYTE		=0x02, /*!< Packet has a 4 byte length - header is 5 bytes long. */ 
    OPS_PTAG_OF_LT_INDETERMINATE	=0x03  /*!< Packet has a indeterminate length. */ 
    } ops_ptag_of_lt_t;


/** New Packet Format: Mask for content tag.
 * In the new packet format the 6 rightmost bits
 * are the content tag.  This is the mask to apply
 * to the packet tag.  Note that you need to
 * shift by #OPS_PTAG_NF_CONTENT_TAG_SHIFT bits.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_NF_CONTENT_TAG_MASK	0x3f
/** New Packet Format: Offset for the content tag.
 * As described at #OPS_PTAG_NF_CONTENT_TAG_MASK the
 * content tag needs to be shifted after being masked
 * out from the Packet Tag.
 *
 * \see RFC4880 4.2
 */
#define OPS_PTAG_NF_CONTENT_TAG_SHIFT	0


/** Structure to hold one parse error string. */
typedef struct
    {
    const char *error; /*!< error message. */
    } ops_parser_error_t;

/** Structure to hold one error code */
typedef struct
    {
    ops_errcode_t errcode;
    } ops_parser_errcode_t;

/** Structure to hold one packet tag.
 * \see RFC4880 4.2
 */
typedef struct
    {
    unsigned		new_format;	/*!< Whether this packet tag is new (true) or old format (false) */
    unsigned		content_tag;	/*!< content_tag value - See #ops_content_tag_t for meanings */
    ops_ptag_of_lt_t	length_type;	/*!< Length type (#ops_ptag_of_lt_t) - only if this packet tag is old format.  Set to 0 if new format. */
    unsigned		length;		/*!< The length of the packet.  This value is set when we read and compute the
					  length information, not at the same moment we create the packet tag structure.
					  Only defined if #length_read is set. */  /* XXX: Ben, is this correct? */
    unsigned		position;	/*!< The position (within the current reader) of the packet */
    } ops_ptag_t;

/** Public Key Algorithm Numbers.
 * OpenPGP assigns a unique Algorithm Number to each algorithm that is part of OpenPGP.
 *
 * This lists algorithm numbers for public key algorithms.
 * 
 * \see RFC4880 9.1
 */
typedef enum
    {
    OPS_PKA_RSA			=1,	/*!< RSA (Encrypt or Sign) */
    OPS_PKA_RSA_ENCRYPT_ONLY	=2,	/*!< RSA Encrypt-Only (deprecated - \see RFC4880 13.5) */
    OPS_PKA_RSA_SIGN_ONLY	=3,	/*!< RSA Sign-Only (deprecated - \see RFC4880 13.5) */
    OPS_PKA_ELGAMAL		=16,	/*!< Elgamal (Encrypt-Only) */
    OPS_PKA_DSA			=17,	/*!< DSA (Digital Signature Algorithm) */
    OPS_PKA_RESERVED_ELLIPTIC_CURVE	=18,	/*!< Reserved for Elliptic Curve */
    OPS_PKA_RESERVED_ECDSA		=19,	/*!< Reserved for ECDSA */
    OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN	=20, 	/*!< Deprecated. */
    OPS_PKA_RESERVED_DH			=21,	/*!< Reserved for Diffie-Hellman (X9.42, as defined for IETF-S/MIME) */
    OPS_PKA_PRIVATE00		=100,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE01		=101,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE02		=102,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE03		=103,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE04		=104,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE05		=105,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE06		=106,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE07		=107,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE08		=108,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE09		=109,	/*!< Private/Experimental Algorithm */
    OPS_PKA_PRIVATE10		=110,	/*!< Private/Experimental Algorithm */
    } ops_public_key_algorithm_t;

/** Structure to hold one DSA public key parameters.
 *
 * \see RFC4880 5.5.2
 */
typedef struct
    {
    BIGNUM *p;	/*!< DSA prime p */
    BIGNUM *q;	/*!< DSA group order q */
    BIGNUM *g;	/*!< DSA group generator g */
    BIGNUM *y;	/*!< DSA public key value y (= g^x mod p with x being the secret) */
    } ops_dsa_public_key_t;

/** Structure to hold on RSA public key.
 *
 * \see RFC4880 5.5.2
 */
typedef struct
    {
    BIGNUM *n;	/*!< RSA public modulus n */
    BIGNUM *e;	/*!< RSA public encryptiong exponent e */
    } ops_rsa_public_key_t;

/** Structure to hold on ElGamal public key parameters.
 *
 * \see RFC4880 5.5.2
 */
typedef struct
    {
    BIGNUM *p;	/*!< ElGamal prime p */
    BIGNUM *g;	/*!< ElGamal group generator g */
    BIGNUM *y;	/*!< ElGamal public key value y (= g^x mod p with x being the secret) */
    } ops_elgamal_public_key_t;

/** Union to hold public key parameters of any algorithm */
typedef union
    {
    ops_dsa_public_key_t	dsa;		/*!< A DSA public key */
    ops_rsa_public_key_t	rsa;		/*!< An RSA public key */
    ops_elgamal_public_key_t	elgamal;	/*!< An ElGamal public key */
    } ops_public_key_union_t;

/** Version.
 * OpenPGP has two different protocol versions: version 3 and version 4.
 *
 * \see RFC4880 5.2
 */
typedef enum
    {
    OPS_V2=2,	/*<! Version 2 (essentially the same as v3) */
    OPS_V3=3,	/*<! Version 3 */
    OPS_V4=4,	/*<! Version 4 */
    } ops_version_t;

/** Structure to hold one pgp public key */
typedef struct
    {
    ops_version_t		version;	/*!< version of the key (v3, v4...) */
    time_t			creation_time;  /*!< when the key was created.  Note that interpretation varies with key
						  version. */
    unsigned			days_valid;	/*!< validity period of the key in days since creation.  A value of 0
						  has a special meaning indicating this key does not expire.  Only
						  used with v3 keys. */
    ops_public_key_algorithm_t	algorithm;	/*!< Public Key Algorithm type */
    ops_public_key_union_t	key;		/*!< Public Key Parameters */
    } ops_public_key_t;

/** Structure to hold data for one RSA secret key
 */
typedef struct
    {
    BIGNUM *d;
    BIGNUM *p;
    BIGNUM *q;
    BIGNUM *u;
    } ops_rsa_secret_key_t;

/** ops_dsa_secret_key_t */
typedef struct
    {
    BIGNUM *x;
    } ops_dsa_secret_key_t;

/** ops_secret_key_union_t */ 
typedef struct
    {
    ops_rsa_secret_key_t rsa;
    ops_dsa_secret_key_t dsa;
    } ops_secret_key_union_t;

/** s2k_usage_t
 */
typedef enum
    {
    OPS_S2KU_NONE=0,
    OPS_S2KU_ENCRYPTED_AND_HASHED=254,
    OPS_S2KU_ENCRYPTED=255,
    } ops_s2k_usage_t;

/** s2k_specifier_t
 */
typedef enum
    {
    OPS_S2KS_SIMPLE=0,
    OPS_S2KS_SALTED=1,
    OPS_S2KS_ITERATED_AND_SALTED=3
    } ops_s2k_specifier_t;

/** Symmetric Key Algorithm Numbers.
 * OpenPGP assigns a unique Algorithm Number to each algorithm that is part of OpenPGP.
 *
 * This lists algorithm numbers for symmetric key algorithms.
 * 
 * \see RFC4880 9.2
 */
typedef enum
    {
    OPS_SA_PLAINTEXT	=0, /*!< Plaintext or unencrypted data */
    OPS_SA_IDEA		=1, /*!< IDEA */
    OPS_SA_TRIPLEDES	=2, /*!< TripleDES */
    OPS_SA_CAST5	=3, /*!< CAST5 */
    OPS_SA_BLOWFISH	=4, /*!< Blowfish */
    OPS_SA_AES_128	=7, /*!< AES with 128-bit key (AES) */
    OPS_SA_AES_192	=8, /*!< AES with 192-bit key */
    OPS_SA_AES_256	=9, /*!< AES with 256-bit key */
    OPS_SA_TWOFISH	=10, /*!< Twofish with 256-bit key (TWOFISH) */
    OPS_SA_CAMELLIA_128 =11, /*!< Camellia with 128-bit key */
    OPS_SA_CAMELLIA_192 =12, /*!< Camellia with 192-bit key */
    OPS_SA_CAMELLIA_256 =13, /*!< Camellia with 256-bit key */
    } ops_symmetric_algorithm_t;

/** Hashing Algorithm Numbers.
 * OpenPGP assigns a unique Algorithm Number to each algorithm that is part of OpenPGP.
 * 
 * This lists algorithm numbers for hash algorithms.
 *
 * \see RFC4880 9.4
 */
typedef enum
    {
    OPS_HASH_UNKNOWN	=-1,	/*!< used to indicate errors */
    OPS_HASH_MD5	= 1,	/*!< MD5 */
    OPS_HASH_SHA1	= 2,	/*!< SHA-1 */
    OPS_HASH_RIPEMD	= 3,	/*!< RIPEMD160 */

    OPS_HASH_SHA256	= 8,	/*!< SHA256 */
    OPS_HASH_SHA384	= 9,	/*!< SHA384 */
    OPS_HASH_SHA512	=10,	/*!< SHA512 */
    OPS_HASH_SHA224 = 11,   /*!< SHA224 */
    } ops_hash_algorithm_t;

// Maximum block size for symmetric crypto
#define OPS_MAX_BLOCK_SIZE	16

// Maximum key size for symmetric crypto
#define OPS_MAX_KEY_SIZE	32

// Salt size for hashing
#define OPS_SALT_SIZE		8

// Hash size for secret key check
#define OPS_CHECKHASH_SIZE	20

// SHA1 Hash Size \todo is this the same as OPS_CHECKHASH_SIZE??
#define OPS_SHA1_HASH_SIZE 		SHA_DIGEST_LENGTH
#define OPS_SHA224_HASH_SIZE	SHA224_DIGEST_LENGTH
#define OPS_SHA256_HASH_SIZE	SHA256_DIGEST_LENGTH
#define OPS_SHA384_HASH_SIZE	SHA384_DIGEST_LENGTH
#define OPS_SHA512_HASH_SIZE	SHA512_DIGEST_LENGTH

// Max hash size
#define OPS_MAX_HASH_SIZE	64

/** ops_secret_key_t
 */
typedef struct
    {
    ops_public_key_t		public_key;
    ops_s2k_usage_t		s2k_usage;
    ops_s2k_specifier_t		s2k_specifier;
    ops_symmetric_algorithm_t	algorithm;  // the algorithm used to encrypt
    					    // the key
    ops_hash_algorithm_t	hash_algorithm;
    unsigned char		salt[OPS_SALT_SIZE];
    unsigned			octet_count;
    unsigned char		iv[OPS_MAX_BLOCK_SIZE];
    ops_secret_key_union_t	key;
    unsigned			checksum;
    unsigned char		checkhash[OPS_CHECKHASH_SIZE];
    } ops_secret_key_t;

/** Structure to hold one trust packet's data */

typedef struct
    {
    ops_data_t data; /*<! Trust Packet */
    } ops_trust_t;
	
/** Structure to hold one user id */
typedef struct
    {
    unsigned char *user_id;	/*!< User ID - UTF-8 string */
    } ops_user_id_t;

/** Structure to hold one user attribute */
typedef struct
    {
    ops_data_t data; /*!< User Attribute */
    } ops_user_attribute_t;

/** Signature Type.
 * OpenPGP defines different signature types that allow giving different meanings to signatures.  Signature types
 * include 0x10 for generitc User ID certifications (used when Ben signs Weasel's key), Subkey binding signatures,
 * document signatures, key revocations, etc.
 *
 * Different types are used in different places, and most make only sense in their intended location (for instance a
 * subkey binding has no place on a UserID).
 *
 * \see RFC4880 5.2.1
 */
typedef enum
    {
    OPS_SIG_BINARY	=0x00,	/*<! Signature of a binary document */
    OPS_SIG_TEXT	=0x01,	/*<! Signature of a canonical text document */
    OPS_SIG_STANDALONE	=0x02,	/*<! Standalone signature */

    OPS_CERT_GENERIC	=0x10,	/*<! Generic certification of a User ID and Public Key packet */
    OPS_CERT_PERSONA	=0x11,	/*<! Persona certification of a User ID and Public Key packet */
    OPS_CERT_CASUAL	=0x12,	/*<! Casual certification of a User ID and Public Key packet */
    OPS_CERT_POSITIVE	=0x13,	/*<! Positive certification of a User ID and Public Key packet */

    OPS_SIG_SUBKEY	=0x18,	/*<! Subkey Binding Signature */
    OPS_SIG_PRIMARY	=0x19,	/*<! Primary Key Binding Signature */
    OPS_SIG_DIRECT	=0x1f,	/*<! Signature directly on a key */

    OPS_SIG_REV_KEY	=0x20,	/*<! Key revocation signature */
    OPS_SIG_REV_SUBKEY	=0x28,	/*<! Subkey revocation signature */
    OPS_SIG_REV_CERT	=0x30,	/*<! Certification revocation signature */

    OPS_SIG_TIMESTAMP	=0x40,	/*<! Timestamp signature */

    OPS_SIG_3RD_PARTY	=0x50,	/*<! Third-Party Confirmation signature */
    } ops_sig_type_t;

/** Struct to hold parameters of an RSA signature */
typedef struct
    {
    BIGNUM			*sig;	/*!< the signature value (m^d % n) */
    } ops_rsa_signature_t;

/** Struct to hold parameters of a DSA signature */
typedef struct
    {
    BIGNUM			*r;	/*!< DSA value r */
    BIGNUM			*s;	/*!< DSA value s */
    } ops_dsa_signature_t;

/** ops_elgamal_signature_t */
typedef struct
    {
    BIGNUM			*r;
    BIGNUM			*s;
    } ops_elgamal_signature_t;

/** Struct to hold data for a private/experimental signature */
typedef struct
    {
    ops_data_t	data;
    } ops_unknown_signature_t;

/** Union to hold signature parameters of any algorithm */
typedef union
    {
    ops_rsa_signature_t		rsa;	/*!< An RSA Signature */
    ops_dsa_signature_t		dsa;	/*!< A DSA Signature */
    ops_elgamal_signature_t	elgamal; /* deprecated */
    ops_unknown_signature_t 	unknown; /* private or experimental */
    } ops_signature_union_t;

#define OPS_KEY_ID_SIZE		8

/** Struct to hold a signature packet.
 *
 * \see RFC4880 5.2.2
 * \see RFC4880 5.2.3
 */
typedef struct
    {
    ops_version_t		version;	/*!< signature version number */
    ops_sig_type_t		type;		/*!< signature type value */
    time_t			creation_time;	/*!< creation time of the signature */
    unsigned char		signer_id[OPS_KEY_ID_SIZE];	/*!< Eight-octet key ID of signer*/
    ops_public_key_algorithm_t	key_algorithm;	/*!< public key algorithm number */
    ops_hash_algorithm_t	hash_algorithm;	/*!< hashing algorithm number */
    ops_signature_union_t	signature;	/*!< signature parameters */
    size_t			v4_hashed_data_length;
    unsigned char* 		v4_hashed_data;
    ops_boolean_t		creation_time_set:1;
    ops_boolean_t		signer_id_set:1;
    } ops_signature_info_t;

/** Struct used when parsing a signature */
typedef struct
    {
    ops_signature_info_t	info; /*!< The signature information */
    /* The following fields are only used while parsing the signature */
    unsigned char		hash2[2];	/*!< high 2 bytes of hashed value - for quick test */
    size_t			v4_hashed_data_start; /* only valid if accumulate is set */
    ops_hash_t			*hash;		/*!< if set, the hash filled in for the data so far */
    } ops_signature_t;

/** The raw bytes of a signature subpacket */

typedef struct
    {
    ops_content_tag_t		tag;
    size_t			length;
    unsigned char		*raw;
    } ops_ss_raw_t;

/** Signature Subpacket : Trust Level */

typedef struct
    {
    unsigned char		level;	/*<! Trust Level */
    unsigned char		amount; /*<! Amount */
    } ops_ss_trust_t;

/** Signature Subpacket : Revocable */
typedef struct
	{
	ops_boolean_t	revocable;
	} ops_ss_revocable_t;
	
/** Signature Subpacket : Time */
typedef struct
    {
    time_t			time;
    } ops_ss_time_t;

/** Signature Subpacket : Key ID */
typedef struct
    {
    unsigned char		key_id[OPS_KEY_ID_SIZE];
    } ops_ss_key_id_t;

/** Signature Subpacket : Notation Data */
typedef struct
    {
    ops_data_t flags;
    ops_data_t name;
    ops_data_t value;
    } ops_ss_notation_data_t;

/** Signature Subpacket : User Defined */
typedef struct
    {
    ops_data_t data;
    } ops_ss_userdefined_t;

/** Signature Subpacket : Unknown */
typedef struct
    {
    ops_data_t data;
    } ops_ss_unknown_t;

/** Signature Subpacket : Preferred Symmetric Key Algorithm */
typedef struct
    {
    ops_data_t data;
    /* Note that value 0 may represent the plaintext algorithm
       so we cannot expect data->contents to be a null-terminated list */
    } ops_ss_preferred_ska_t;

/** Signature Subpacket : Preferrred Hash Algorithm */
typedef struct
    {
    ops_data_t data;
    } ops_ss_preferred_hash_t;

/** Signature Subpacket : Preferred Compression */
typedef struct
    {
    ops_data_t data;
    } ops_ss_preferred_compression_t;

/** Signature Subpacket : Key Flags */
typedef struct
    {
    ops_data_t data;
    } ops_ss_key_flags_t;

/** Signature Subpacket : Key Server Preferences */
typedef struct
    {
    ops_data_t data;
    } ops_ss_key_server_prefs_t;

/** Signature Subpacket : Features */
typedef struct
    {
    ops_data_t data;
    } ops_ss_features_t;

/** Signature Subpacket : Signature Target */
typedef struct
    {
    ops_public_key_algorithm_t pka_alg;
    ops_hash_algorithm_t hash_alg;
    ops_data_t hash;
    } ops_ss_signature_target_t;
    
/** Signature Subpacket : Embedded Signature */
typedef struct
    {
    ops_data_t sig;
    } ops_ss_embedded_signature_t;

/** ops_packet_t */

typedef struct
    {
    size_t			length;
    unsigned char		*raw;
    } ops_packet_t;

/** Types of Compression */
typedef enum
    {
    OPS_C_NONE=0,
    OPS_C_ZIP=1,
    OPS_C_ZLIB=2,
    OPS_C_BZIP2=3,
    } ops_compression_type_t;

/* unlike most structures, this will feed its data as a stream
 * to the application instead of directly including it */
/** ops_compressed_t */
typedef struct
    {
    ops_compression_type_t	type;
    } ops_compressed_t;

/** ops_one_pass_signature_t */
typedef struct
    {
    unsigned char		version;
    ops_sig_type_t		sig_type;
    ops_hash_algorithm_t	hash_algorithm;
    ops_public_key_algorithm_t	key_algorithm;
    unsigned char		keyid[OPS_KEY_ID_SIZE];
    ops_boolean_t		nested;
    } ops_one_pass_signature_t;

/** Signature Subpacket : Primary User ID */
typedef struct
    {
    ops_boolean_t	primary_user_id;
    } ops_ss_primary_user_id_t;

/** Signature Subpacket : Regexp */
typedef struct
    {
    char *text;
    } ops_ss_regexp_t;

/** Signature Subpacket : Policy URL */
typedef struct
    {
    char *text;
    } ops_ss_policy_url_t;

/** Signature Subpacket : Preferred Key Server */
typedef struct
    {
    char *text;
    } ops_ss_preferred_key_server_t;

/** Signature Subpacket : Revocation Key */
typedef struct
    {
    unsigned char clss;  /* class - name changed for C++ */
    unsigned char algid;
    unsigned char fingerprint[20];
    } ops_ss_revocation_key_t;

/** Signature Subpacket : Revocation Reason */
typedef struct
    {
    unsigned char code;
    char *text;
    } ops_ss_revocation_reason_t;

/** literal_data_type_t */
typedef enum
    {
    OPS_LDT_BINARY='b',
    OPS_LDT_TEXT='t',
    OPS_LDT_UTF8='u',
    OPS_LDT_LOCAL='l',
    OPS_LDT_LOCAL2='1'
    } ops_literal_data_type_t;

/** ops_literal_data_header_t */
typedef struct
    {
    ops_literal_data_type_t		format;
    char			filename[256];
    time_t			modification_time;
    } ops_literal_data_header_t;

/** ops_literal_data_body_t */
typedef struct
    {
    unsigned			length;
    unsigned char		*data;//[8192];
    } ops_literal_data_body_t;

/** ops_mdc_t */
typedef struct
    {
    unsigned char		data[20]; // size of SHA1 hash
    } ops_mdc_t;

/** ops_armoured_header_value_t */
typedef struct
    {
    char *key;
    char *value;
    } ops_armoured_header_value_t;

/** ops_headers_t */
typedef struct
    {
    ops_armoured_header_value_t *headers;
    unsigned nheaders;
    } ops_headers_t;

/** ops_armour_header_t */
typedef struct
    {
    const char *type;
    ops_headers_t headers;
    } ops_armour_header_t;

/** ops_armour_trailer_t */
typedef struct
    {
    const char *type;
    } ops_armour_trailer_t;

/** ops_signed_cleartext_header_t */
typedef struct
    {
    ops_headers_t headers;
    } ops_signed_cleartext_header_t;

/** ops_signed_cleartext_body_t */
typedef struct
    {
    unsigned			length;
    unsigned char		*data; // \todo fix hard-coded value?
    } ops_signed_cleartext_body_t;

/** ops_signed_cleartext_trailer_t */
typedef struct
    {
    struct _ops_hash_t		*hash;	/*!< This will not have been finalised, but will have seen all the cleartext data in canonical form */
    } ops_signed_cleartext_trailer_t;

/** ops_unarmoured_text_t */
typedef struct
    {
    unsigned			length;
    unsigned char		*data;
    } ops_unarmoured_text_t;

typedef enum
    {
    SE_IP_DATA_VERSION=1
    } ops_se_ip_data_version_t;

typedef enum
    {
    OPS_PKSK_V3=3
    } ops_pk_session_key_version_t;

/** ops_pk_session_key_parameters_rsa_t */
typedef struct
    {
    BIGNUM			*encrypted_m;
    BIGNUM			*m;
    } ops_pk_session_key_parameters_rsa_t;

/** ops_pk_session_key_parameters_elgamal_t */
typedef struct
    {
    BIGNUM			*g_to_k;
    BIGNUM		        *encrypted_m;
    } ops_pk_session_key_parameters_elgamal_t;

/** ops_pk_session_key_parameters_t */
typedef union
    {
    ops_pk_session_key_parameters_rsa_t		rsa;
    ops_pk_session_key_parameters_elgamal_t	elgamal;
    } ops_pk_session_key_parameters_t;

/** ops_pk_session_key_t */
typedef struct
    {
    ops_pk_session_key_version_t version;
    unsigned char		key_id[OPS_KEY_ID_SIZE];
    ops_public_key_algorithm_t	algorithm;
    ops_pk_session_key_parameters_t parameters;
    ops_symmetric_algorithm_t	symmetric_algorithm;
    unsigned char		key[OPS_MAX_KEY_SIZE];
    unsigned short		checksum;
    } ops_pk_session_key_t;

/** ops_secret_key_passphrase_t */
typedef struct
    {
    const ops_secret_key_t     *secret_key;
    char		       **passphrase; /* point somewhere that gets filled in to work around constness of content */
    } ops_secret_key_passphrase_t;

typedef enum
    {
    OPS_SE_IP_V1=1
    } ops_se_ip_version_t;

/** ops_se_ip_data_header_t */
typedef struct
    {
    ops_se_ip_version_t		version;
    } ops_se_ip_data_header_t;

/** ops_se_ip_data_body_t */
typedef struct
    {
    unsigned			length;
    unsigned char*		data; // \todo remember to free this
    } ops_se_ip_data_body_t;

/** ops_se_data_body_t */
typedef struct
    {
    unsigned			length;
    unsigned char		data[8192]; // \todo parameterise this!
    } ops_se_data_body_t;

/** ops_get_secret_key_t */
typedef struct
    {
    const ops_secret_key_t      **secret_key;
    const ops_pk_session_key_t *pk_session_key;
    } ops_get_secret_key_t;

/** ops_parser_union_content_t */
typedef union
    {
    ops_parser_error_t		error;
    ops_parser_errcode_t	errcode;
    ops_ptag_t			ptag;
    ops_public_key_t		public_key;
    ops_trust_t			trust;
    ops_user_id_t		user_id;
    ops_user_attribute_t	user_attribute;
    ops_signature_t		signature;
    ops_ss_raw_t		ss_raw;
    ops_ss_trust_t		ss_trust;
    ops_ss_revocable_t		ss_revocable;
    ops_ss_time_t		ss_time;
    ops_ss_key_id_t		ss_issuer_key_id;
    ops_ss_notation_data_t	ss_notation_data;
    ops_packet_t		packet;
    ops_compressed_t		compressed;
    ops_one_pass_signature_t	one_pass_signature;
    ops_ss_preferred_ska_t	ss_preferred_ska;
    ops_ss_preferred_hash_t     ss_preferred_hash;
    ops_ss_preferred_compression_t     ss_preferred_compression;
    ops_ss_key_flags_t 		ss_key_flags;
    ops_ss_key_server_prefs_t	ss_key_server_prefs;
    ops_ss_primary_user_id_t	ss_primary_user_id;
    ops_ss_regexp_t		ss_regexp;
    ops_ss_policy_url_t 	ss_policy_url;
    ops_ss_preferred_key_server_t	ss_preferred_key_server;
    ops_ss_revocation_key_t	ss_revocation_key;
    ops_ss_userdefined_t 	ss_userdefined;
    ops_ss_unknown_t	 	ss_unknown;
    ops_literal_data_header_t	literal_data_header;
    ops_literal_data_body_t	literal_data_body;
	ops_mdc_t				mdc;
    ops_ss_features_t		ss_features;
    ops_ss_signature_target_t ss_signature_target;
    ops_ss_embedded_signature_t ss_embedded_signature;
    ops_ss_revocation_reason_t	ss_revocation_reason;
    ops_secret_key_t		secret_key;
    ops_user_id_t		ss_signers_user_id;
    ops_armour_header_t		armour_header;
    ops_armour_trailer_t	armour_trailer;
    ops_signed_cleartext_header_t signed_cleartext_header;
    ops_signed_cleartext_body_t	signed_cleartext_body;
    ops_signed_cleartext_trailer_t signed_cleartext_trailer;
    ops_unarmoured_text_t	unarmoured_text;
    ops_pk_session_key_t	pk_session_key;
    ops_secret_key_passphrase_t	secret_key_passphrase;
    ops_se_ip_data_header_t	se_ip_data_header;
    ops_se_ip_data_body_t	se_ip_data_body;
    ops_se_data_body_t		se_data_body;
    ops_get_secret_key_t	get_secret_key;
    } ops_parser_content_union_t;

/** ops_parser_content_t */
struct ops_parser_content_t
    {
    ops_content_tag_t		tag;
    unsigned char		critical; /* for signature subpackets */
    ops_parser_content_union_t 	content;
    };

/** ops_fingerprint_t */
typedef struct
    {
    unsigned char 		fingerprint[20];
    unsigned			length;
    } ops_fingerprint_t;

void ops_init(void);
void ops_finish(void);
void ops_keyid(unsigned char keyid[OPS_KEY_ID_SIZE],
	       const ops_public_key_t *key);
void ops_fingerprint(ops_fingerprint_t *fp,const ops_public_key_t *key);
void ops_public_key_free(ops_public_key_t *key);
void ops_public_key_copy(ops_public_key_t *dst,const ops_public_key_t *src);
void ops_user_id_free(ops_user_id_t *id);
void ops_user_attribute_free(ops_user_attribute_t *att);
void ops_signature_free(ops_signature_t *sig);
void ops_trust_free(ops_trust_t *trust);
void ops_ss_preferred_ska_free(ops_ss_preferred_ska_t *ss_preferred_ska);
void ops_ss_preferred_hash_free(ops_ss_preferred_hash_t *ss_preferred_hash);
void ops_ss_preferred_compression_free(ops_ss_preferred_compression_t *ss_preferred_compression);
void ops_ss_key_flags_free(ops_ss_key_flags_t *ss_key_flags);
void ops_ss_key_server_prefs_free(ops_ss_key_server_prefs_t *ss_key_server_prefs);
void ops_ss_features_free(ops_ss_features_t *ss_features);
void ops_ss_notation_data_free(ops_ss_notation_data_t *ss_notation_data);
void ops_ss_policy_url_free(ops_ss_policy_url_t *ss_policy_url);
void ops_ss_preferred_key_server_free(ops_ss_preferred_key_server_t *ss_preferred_key_server);
void ops_ss_regexp_free(ops_ss_regexp_t *ss_regexp);
void ops_ss_userdefined_free(ops_ss_userdefined_t *ss_userdefined);
void ops_ss_reserved_free(ops_ss_unknown_t *ss_unknown);
void ops_ss_revocation_reason_free(ops_ss_revocation_reason_t *ss_revocation_reason);
void ops_ss_signature_target_free(ops_ss_signature_target_t *ss_signature_target);
void ops_ss_embedded_signature_free(ops_ss_embedded_signature_t *ss_embedded_signature);

void ops_packet_free(ops_packet_t *packet);
void ops_parser_content_free(ops_parser_content_t *c);
void ops_secret_key_free(ops_secret_key_t *key);
void ops_secret_key_copy(ops_secret_key_t *dst,const ops_secret_key_t *src);
void ops_pk_session_key_free(ops_pk_session_key_t *sk);

/* vim:set textwidth=120: */
/* vim:set ts=8: */

#endif
