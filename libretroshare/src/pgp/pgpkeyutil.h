/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 Cyril Soler <csoler@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
 
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

		static std::string makeArmouredKey(const unsigned char *keydata,size_t key_size,const std::string& version_string) ;
	private:
		// Computes the 24 bits CRC checksum necessary to all PGP data.
		// 
		static uint32_t compute24bitsCRC(unsigned char *data,size_t len) ;
};

// This class handles the parsing of PGP packet headers under various (old and new) formats.
//
class PGPKeyParser
{
	public:
		static const uint8_t PGP_PACKET_TAG_PUBLIC_KEY  =  6 ;
		static const uint8_t PGP_PACKET_TAG_USER_ID     = 13 ;
		static const uint8_t PGP_PACKET_TAG_SIGNATURE   =  2 ;

		static uint64_t read_KeyID(unsigned char *& data) ;
		static uint32_t read_125Size(unsigned char *& data) ;
		static uint32_t read_partialBodyLength(unsigned char *& data) ;
		static void     read_packetHeader(unsigned char *& data,uint8_t& packet_tag,uint32_t& packet_length) ;
};


