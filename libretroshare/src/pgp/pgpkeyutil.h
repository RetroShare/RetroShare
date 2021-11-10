/*******************************************************************************
 * libretroshare/src/pgp: pgpkeyutil.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Cyril Soler <csoler@users.sourceforge.net>                   *
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

// refer to RFC4880 specif document for loading GPG public keys:
//
// 11.1: transferable public keys
// 	Global structure of transferable public keys
//
// 	- one public key packet (see 12.2)
// 	- zero or more revocation signatures (See signature type 5.2.1 for key signature types)
//
// 	- user certification signatures (0x10 or 0x13)
//
// 	- 5.2.2: Signature format packet
// 	- 5.2.3.1: signature subpacket specification
//
// 	- 4.3: packet tags (1 byte)

#pragma once

#include <stdint.h>
#include <string>

static const uint8_t PGP_PACKET_TAG_HASH_ALGORITHM_UNKNOWN       = 0  ;
static const uint8_t PGP_PACKET_TAG_HASH_ALGORITHM_MD5           = 1  ;
static const uint8_t PGP_PACKET_TAG_HASH_ALGORITHM_SHA1          = 2  ;
static const uint8_t PGP_PACKET_TAG_HASH_ALGORITHM_SHA256        = 8  ;
static const uint8_t PGP_PACKET_TAG_HASH_ALGORITHM_SHA512        = 10 ;

static const uint8_t PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_UNKNOWN = 0  ;
static const uint8_t PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_RSA_ES  = 1  ;
static const uint8_t PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_RSA_E   = 2  ;
static const uint8_t PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_RSA_S   = 3  ;
static const uint8_t PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_DSA     = 17 ;

static const uint8_t PGP_PACKET_TAG_SIGNATURE_VERSION_UNKNOWN    = 0 ;
static const uint8_t PGP_PACKET_TAG_SIGNATURE_VERSION_V3         = 3 ;
static const uint8_t PGP_PACKET_TAG_SIGNATURE_VERSION_V4         = 4 ;

static const uint8_t PGP_PACKET_TAG_SIGNATURE_TYPE_UNKNOWN          = 0xff ;
static const uint8_t PGP_PACKET_TAG_SIGNATURE_TYPE_BINARY_DOCUMENT  = 0x00 ;
static const uint8_t PGP_PACKET_TAG_SIGNATURE_TYPE_CANONICAL_TEXT   = 0x01 ;
static const uint8_t PGP_PACKET_TAG_SIGNATURE_TYPE_STANDALONE_SIG   = 0x02 ;
// All other consts for signature types not used, so not defines.

class PGPSignatureInfo
{
public:
	PGPSignatureInfo() :
	    signature_version   (PGP_PACKET_TAG_SIGNATURE_VERSION_UNKNOWN),
	    signature_type      (PGP_PACKET_TAG_SIGNATURE_TYPE_UNKNOWN),
	    issuer              (0),
	    public_key_algorithm(PGP_PACKET_TAG_PUBLIC_KEY_ALGORITHM_UNKNOWN),
	    hash_algorithm      (PGP_PACKET_TAG_HASH_ALGORITHM_UNKNOWN)
	{}

	uint8_t  signature_version ;
	uint8_t  signature_type ;
	uint64_t issuer ;
	uint8_t  public_key_algorithm ;
	uint8_t  hash_algorithm ;
};

class PGPKeyInfo
{
public:
    PGPKeyInfo() {}

    std::string   user_id;
    unsigned char fingerprint[20];
};


// This class handles GPG keys. For now we only clean them from signatures, but
// in the future, we might cache them to avoid unnecessary calls to gpgme.
//
class PGPKeyManagement
{
	public:
		// Create a minimal key, removing all signatures and third party info.
		//    Input: a clean PGP certificate (starts with "----BEGIN", 
		//           ends with "-----END PGP PUBLIC KEY BLOCK-----"
		//    Output: the same certificate without signatures.
		// 
		// Returns:
		//
		//		true if the certificate cleaning succeeded
		//		false otherwise.
		//
		static bool createMinimalKey(const std::string& pgp_certificate,std::string& cleaned_certificate) ;

		static void findLengthOfMinimalKey(const unsigned char *keydata,size_t key_len,size_t& minimal_key_len) ;
		static std::string makeArmouredKey(const unsigned char *keydata,size_t key_size,const std::string& version_string) ;

		// Computes the 24 bits CRC checksum necessary to all PGP data.
		// 
		static uint32_t compute24bitsCRC(unsigned char *data,size_t len) ;
        
		static bool parseSignature(const unsigned char *signature, size_t sign_len, PGPSignatureInfo& info) ;

        static bool parsePGPPublicKey(const unsigned char *keydata, size_t keylen, PGPKeyInfo& info);
};

// This class handles the parsing of PGP packet headers under various (old and new) formats.
//
class PGPKeyParser
{
	public:
		// These constants correspond to packet tags from RFC4880

		static const uint8_t PGP_PACKET_TAG_PUBLIC_KEY  =  6 ;
		static const uint8_t PGP_PACKET_TAG_USER_ID     = 13 ;
		static const uint8_t PGP_PACKET_TAG_SIGNATURE   =  2 ;
		static const uint8_t PGP_PACKET_TAG_ISSUER      = 16 ;

		// These functions read and move the data pointer to the next byte after the read section.
		//
		static uint64_t read_KeyID(unsigned char *& data) ;
		static uint32_t read_125Size(unsigned char *& data) ;
		static uint32_t read_partialBodyLength(unsigned char *& data) ;
        static void     read_packetHeader(unsigned char *&data, uint8_t& packet_tag, uint32_t& packet_length) ;

		// These functions write, and indicate how many bytes where written.
		//
		static uint32_t write_125Size(unsigned char *data,uint32_t size) ;

		// Helper functions
		//
		static std::string extractRadixPartFromArmouredKey(const std::string& pgp_cert,std::string& version_string);
};


