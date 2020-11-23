/*******************************************************************************
 * libretroshare/src/pgp: pgpkeyutil.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 Cyril Soler <csoler@users.sourceforge.net>                   *
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
#include <stdint.h>
#include <util/radix64.h>
#include "pgpkeyutil.h"

#include <iostream>
#include <stdexcept>

/****************************/
/*  #define DEBUG_PGPUTIL 1 */
/****************************/

#define PGP_CRC24_INIT 0xB704CEL
#define PGP_CRC24_POLY 0x1864CFBL

#define PGP_CERTIFICATE_START_STRING "-----BEGIN PGP PUBLIC KEY BLOCK-----"
#define PGP_CERTIFICATE_END_STRING   "-----END PGP PUBLIC KEY BLOCK-----"
//
// All size are big endian
// MPI: 2 bytes size (length in bits) + string of octets
//
bool PGPKeyManagement::createMinimalKey(const std::string& pgp_certificate,std::string& cleaned_certificate) 
{
	try
	{
		// 0 - Extract Radix64 portion of the certificate
		//
		std::string version_string ;
		std::string radix_cert = PGPKeyParser::extractRadixPartFromArmouredKey(pgp_certificate,version_string) ;

		// 1 - Convert armored key into binary key
		//
        std::vector<uint8_t> keydata = Radix64::decode(radix_cert) ;

		size_t new_len ;
        findLengthOfMinimalKey(keydata.data(), keydata.size(), new_len) ;

        cleaned_certificate = makeArmouredKey(keydata.data(), new_len, version_string) ;
		return true ;
	}
	catch(std::exception& e)
	{
		cleaned_certificate = "" ;
		std::cerr << "Certificate cleaning failed: " << e.what() << std::endl;
		return false ;
	}
}

void PGPKeyManagement::findLengthOfMinimalKey(const unsigned char *keydata,size_t len,size_t& new_len)
{
	unsigned char *data = (unsigned char *)keydata ;

#ifdef DEBUG_PGPUTIL
	std::cerr << "Total size: " << len << std::endl;
#endif

	uint8_t packet_tag;
	uint32_t packet_length ;

	// 2 - parse key data, only keep public key data, user id and self-signature.

	bool public_key=false ;
	bool own_signature=false ;
	bool user_id=false ;

	while(true) 
	{
		PGPKeyParser::read_packetHeader(data,packet_tag,packet_length) ;
#ifdef DEBUG_PGPUTIL
		std::cerr << "Header:" << std::endl;
		std::cerr << "  Packet tag: " << (int)packet_tag << std::endl;
		std::cerr << "  Packet length: " << packet_length << std::endl;
#endif

		data += packet_length ;

		if(packet_tag == PGPKeyParser::PGP_PACKET_TAG_PUBLIC_KEY)
			public_key = true ;
		if(packet_tag == PGPKeyParser::PGP_PACKET_TAG_USER_ID)
			user_id = true ;
		if(packet_tag == PGPKeyParser::PGP_PACKET_TAG_SIGNATURE)
			own_signature = true ;

		if(public_key && own_signature && user_id) 
			break ;

		if( (uint64_t)data - (uint64_t)keydata >= len )
			break ;
	}

	new_len = (uint64_t)data - (uint64_t)keydata ;
}
	
std::string PGPKeyParser::extractRadixPartFromArmouredKey(const std::string& pgp_certificate,std::string& version_string)
{
	int n = pgp_certificate.length() ;
	int i=0 ;
	version_string = "" ;

	while(i < n && pgp_certificate[i] != '\n') ++i ;	// remove first part -----BEGIN PGP CERTIFICATE-----
	++i ;
	while(i < n && pgp_certificate[i] != '\n') version_string += pgp_certificate[i++] ;	// remove first part Version: [fdfdfdf]
	++i ;
	while(i < n && pgp_certificate[i] != '\n') ++i ;	// remove blank line

	++i ;

	int j=n-1 ;

	while(j>0 && pgp_certificate[j] != '=' && j>=i) --j ;

	std::string radix_cert = pgp_certificate.substr(i,j-i) ;

#ifdef DEBUG_PGPUTIL
	std::cerr << "extracted radix cert: " << std::endl;
	std::cerr << radix_cert ;
#endif
	return radix_cert ;
}


std::string PGPKeyManagement::makeArmouredKey(const unsigned char *keydata,size_t key_size,const std::string& version_string)
{
	std::string outstring ;
	Radix64::encode(keydata,key_size,outstring) ;

	uint32_t crc = compute24bitsCRC((unsigned char *)keydata,key_size) ;

	unsigned char tmp[3] = { uint8_t((crc >> 16) & 0xff), uint8_t((crc >> 8) & 0xff), uint8_t(crc & 0xff) } ;
	std::string crc_string ;
	Radix64::encode(tmp,3,crc_string) ;

#ifdef DEBUG_PGPUTIL
	std::cerr << "After signature pruning: " << std::endl;
	std::cerr << outstring << std::endl;
#endif

	std::string certificate = std::string(PGP_CERTIFICATE_START_STRING) + "\n" + version_string + "\n\n" ;

	for(uint32_t i=0;i<outstring.length();i+=64)
		certificate += outstring.substr(i,64) + "\n" ;

	certificate += "=" + crc_string + "\n" ;
	certificate += std::string(PGP_CERTIFICATE_END_STRING) + "\n" ;

	return certificate ;
}

uint32_t PGPKeyManagement::compute24bitsCRC(unsigned char *octets, size_t len)
{
	long crc = PGP_CRC24_INIT;
	int i;
	while (len--) {
		crc ^= (*octets++) << 16;
		for (i = 0; i < 8; i++) {
			crc <<= 1;
			if (crc & 0x1000000)
				crc ^= PGP_CRC24_POLY;
		}
	}
    return crc & 0xFFFFFFL;
}

bool PGPKeyManagement::parseSignature(const unsigned char *signature, size_t sign_len, PGPSignatureInfo& info)
{
    unsigned char *data = (unsigned char *)signature ;

#ifdef DEBUG_PGPUTIL
    std::cerr << "Total size: " << len << std::endl;
#endif

    uint8_t packet_tag;
    uint32_t packet_length ;

    PGPKeyParser::read_packetHeader(data,packet_tag,packet_length) ;
    
#ifdef DEBUG_PGPUTIL
    std::cerr << "Packet tag : " << (int)packet_tag << ", length=" << packet_length << std::endl;
#endif
    
    // 2 - parse key data, only keep public key data, user id and self-signature.

    bool issuer_found=false ;
    
    if(sign_len < 12)	// conservative check to allow the explicit reads below, until header of first sub-packet
        return false ;
    
    unsigned char signature_type = data[0] ;
    
    if(signature_type != 4)
        return false ;
    
	info.signature_version    = data[0] ; data += 1 ;	// skip version number
    info.signature_type       = data[0] ; data += 1 ;	// skip signature type
    info.public_key_algorithm = data[0] ; data += 1 ;	// skip public key algorithm
    info.hash_algorithm       = data[0] ; data += 1 ;	// skip hash algorithm

    uint32_t hashed_size = 256u*data[0] + data[1] ;
    data += 2 ;
    
    // now read hashed sub-packets
    
    uint8_t *start_hashed_data = data ;
   
    while(true) 
    {
	    int subpacket_size = PGPKeyParser::read_125Size(data) ; // following RFC4880
	    uint8_t subpacket_type = data[0] ; data+=1 ;

#ifdef DEBUG_PGPUTIL
	    std::cerr << "  SubPacket tag: " << (int)subpacket_type << std::endl;
	    std::cerr << "  SubPacket length: " << subpacket_size << std::endl;
#endif

	    if(subpacket_type == PGPKeyParser::PGP_PACKET_TAG_ISSUER && subpacket_size == 9)
	    {
		    issuer_found = true ;
		    info.issuer = PGPKeyParser::read_KeyID(data) ;
	    }
	    else
		    data += subpacket_size-1 ;	// we remove the size of subpacket type

	    if(issuer_found)
		    break ;

	    if( (uint64_t)data - (uint64_t)start_hashed_data >= hashed_size )
		    break ;
    }
    // non hashed sub-packets are ignored for now. 
    
    return issuer_found ;
}

uint64_t PGPKeyParser::read_KeyID(unsigned char *& data)
{
	uint64_t val = 0 ;

	val |= uint64_t( *data ) << 56 ; ++data ;
	val |= uint64_t( *data ) << 48 ; ++data ;
	val |= uint64_t( *data ) << 40 ; ++data ;
	val |= uint64_t( *data ) << 32 ; ++data ;
	val |= uint64_t( *data ) << 24 ; ++data ;
	val |= uint64_t( *data ) << 16 ; ++data ;
	val |= uint64_t( *data ) <<  8 ; ++data ;
	val |= uint64_t( *data ) <<  0 ; ++data ;

	return val ;
}

uint32_t PGPKeyParser::write_125Size(unsigned char *data,uint32_t size)
{
    if(size < 192)//192 To know if size is coded with One Char < 0xC0
	{
		data[0] = size ;
		return 1;
	}

    if(size < 8384)//8384 To know if size is coded with Two Chars < 0xE0. See RFC4880
	{
        data[1] =  (size - 192) & 0xFF ;//Warning data[1] could be "negative", recode it using 8bits type
        data[0] = ((size - 192 - data[1]) >> 8) + 192 ;

		return 2 ;
	}

	data[0] = 0xFF ; //Else size is coded with 4 Chars + 1 at 0xFF
	data[1] = (size >> 24) & 0xFF ;
	data[2] = (size >> 16) & 0xFF ;
	data[3] = (size >>  8) & 0xFF ;
	data[4] = (size      ) & 0xFF ;

	return 5 ;
}

uint32_t PGPKeyParser::read_125Size(unsigned char *& data)
{
	uint8_t b1 = *data ;
	++data ;

	if (b1 < 192) //192 Size is coded with One Char. See RFC4880 https://tools.ietf.org/html/rfc4880#section-4.2.2.1
		return b1 ;

	uint8_t b2 = *data ;
	++data ;

	if (b1 < 224)//224 = 0xC0+0x20 Size is coded with Two Chars
		return ( (b1-192) << 8 ) + b2 + 192 ; // see RFC4880

	if(b1 != 0xFF)// Else Coded with 4 Chars but first == 0xFF
		throw std::runtime_error("GPG parsing error") ;

	uint8_t b3 = *data ; ++data ;
	uint8_t b4 = *data ; ++data ;
	uint8_t b5 = *data ; ++data ;

	return (b2 << 24) | (b3 << 16) | (b4 << 8) | b5 ;
}

uint32_t PGPKeyParser::read_partialBodyLength(unsigned char *& data)
{
	uint8_t b1 =*data ;
	++data ;

	return 1 << (b1 & 0x1F) ;
}


void PGPKeyParser::read_packetHeader(unsigned char *& data,uint8_t& packet_tag,uint32_t& packet_length)
{
	uint8_t b1 = *data ;
	++data ;

	bool new_format = b1 & 0x40 ;

	if(new_format)
	{
#ifdef DEBUG_PGPUTIL
		std::cerr << "Packet is in new format" << std::endl;
#endif
		packet_tag = b1 & 0x3f ;
		packet_length = read_125Size(data) ;
	}
	else
	{
#ifdef DEBUG_PGPUTIL
		std::cerr << "Packet is in old format" << std::endl;
#endif
		uint8_t length_type = b1 & 0x03 ;
		packet_tag  = (b1 & 0x3c) >> 2 ;

		int length_size ;
		switch(length_type)
		{
			case 0: length_size = 1 ;
					  break ;
			case 1: length_size = 2 ;
					  break ;
			case 2: length_size = 4 ;
					  break ;
			default:
					  throw std::runtime_error("Unhandled length type!") ;
		}

		packet_length = 0 ;
		for(int k=0;k<length_size;++k)
		{
			packet_length <<= 8 ;
			packet_length |= *data ;
			++data ;
		}
	}
}

