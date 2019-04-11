/*******************************************************************************
 * libretroshare/src/pgp: rscertificate.cc                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 Cyril Soler <csoler@users.sourceforge.net>                   *
 * Copyright 2018 Gioacchino Mazzurco <gio@eigenlab.org>                       *
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
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>

#include <retroshare/rspeers.h>
#include <util/radix64.h>
#include <pgp/pgpkeyutil.h>
#include "rscertificate.h"
#include "util/rsstring.h"
#include "util/stacktrace.h"

//#define DEBUG_RSCERTIFICATE

static const uint8_t CERTIFICATE_VERSION_06 = 0x06;

enum CertificatePtag : uint8_t
{
	CERTIFICATE_PTAG_PGP_SECTION           = 0x01,
	CERTIFICATE_PTAG_EXTIPANDPORT_SECTION  = 0x02,
	CERTIFICATE_PTAG_LOCIPANDPORT_SECTION  = 0x03,
	CERTIFICATE_PTAG_DNS_SECTION           = 0x04,
	CERTIFICATE_PTAG_SSLID_SECTION         = 0x05,
	CERTIFICATE_PTAG_NAME_SECTION          = 0x06,
	CERTIFICATE_PTAG_CHECKSUM_SECTION      = 0x07,
	CERTIFICATE_PTAG_HIDDENNODE_SECTION    = 0x08,
	CERTIFICATE_PTAG_VERSION_SECTION       = 0x09,
	CERTIFICATE_PTAG_EXTRA_LOCATOR         = 10
};

static bool is_acceptable_radix64Char(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' ||  c == '=' ;
}

RsCertificate::~RsCertificate()
{
	delete[] binary_pgp_key ;
}

void RsCertificate::addPacket(uint8_t ptag, const unsigned char *mem, size_t size, unsigned char *& buf, size_t& offset, size_t& buf_size)
{
	// Check that the buffer has sufficient size. If not, increase it.

	while(offset + size + 6 >= buf_size)
	{
		unsigned char *newbuf = new unsigned char[2*buf_size] ;

		memcpy(newbuf, buf, buf_size) ;
		buf_size *= 2 ;

		delete[] buf ;

		buf = newbuf ;
	}

	// Write ptag and size
	
	buf[offset] = ptag ;
	offset += 1 ;

	offset += PGPKeyParser::write_125Size(&buf[offset],size) ;

	// Copy the data

	memcpy(&buf[offset], mem, size) ;
	offset += size ;
}

const RsCertificate&RsCertificate::operator=(const RsCertificate&)
{
	memset(ipv4_external_ip_and_port,0,6);
	memset(ipv4_internal_ip_and_port,0,6);
	binary_pgp_key = nullptr;
	binary_pgp_key_size = 0;
	only_pgp = false;
	hidden_node = false;
	return *this;
}

std::string RsCertificate::toStdString() const
{
	//std::string res ;
	size_t BS = 1000 ;
	size_t p = 0 ;
	unsigned char *buf = new unsigned char[BS] ;

	addPacket( CERTIFICATE_PTAG_VERSION_SECTION, &CERTIFICATE_VERSION_06 , 1              , buf, p, BS ) ;
	addPacket( CERTIFICATE_PTAG_PGP_SECTION    , binary_pgp_key , binary_pgp_key_size     , buf, p, BS ) ;

	if(!only_pgp)
	{
		if (hidden_node)
		{
			addPacket( CERTIFICATE_PTAG_HIDDENNODE_SECTION, (unsigned char *)hidden_node_address.c_str(), hidden_node_address.length() , buf, p, BS ) ;
		}
		else
		{
			addPacket( CERTIFICATE_PTAG_EXTIPANDPORT_SECTION, ipv4_external_ip_and_port              ,                     6   , buf, p, BS ) ;
			addPacket( CERTIFICATE_PTAG_LOCIPANDPORT_SECTION, ipv4_internal_ip_and_port              ,                     6   , buf, p, BS ) ;
			addPacket( CERTIFICATE_PTAG_DNS_SECTION         , (unsigned char *)dns_name.c_str()      ,     dns_name.length()   , buf, p, BS ) ;
		}

		addPacket( CERTIFICATE_PTAG_NAME_SECTION        , (unsigned char *)location_name.c_str() ,location_name.length()   , buf, p, BS ) ;
		addPacket( CERTIFICATE_PTAG_SSLID_SECTION       , location_id.toByteArray()              ,location_id.SIZE_IN_BYTES, buf, p, BS ) ;

		for (const RsUrl& locator : mLocators)
		{
			std::string urlStr(locator.toString());
			addPacket( CERTIFICATE_PTAG_EXTRA_LOCATOR,
			           (unsigned char *) urlStr.c_str(), urlStr.size(),
			           buf, p, BS );
		}
	}

	uint32_t computed_crc = PGPKeyManagement::compute24bitsCRC(buf,p) ;

	// handle endian issues.
	unsigned char mem[3] ;
	mem[0] =  computed_crc        & 0xff ;
	mem[1] = (computed_crc >> 8 ) & 0xff ;
	mem[2] = (computed_crc >> 16) & 0xff ;

	addPacket( CERTIFICATE_PTAG_CHECKSUM_SECTION,mem,3,buf,p,BS) ;

	std::string out_string ;

	Radix64::encode(buf, p, out_string) ;

	// Now slice up to 64 chars.
	//
	std::string out2 ;
	static const int LINE_LENGTH = 64 ;

	for(int i=0;i<(int)out_string.length();++i)
	{
		out2 += out_string[i] ;

		if(i % LINE_LENGTH == LINE_LENGTH-1)
			out2 += '\n' ;
	}

	delete[] buf ;
	return out2 ;
}

RsCertificate::RsCertificate(const std::string& str) :
    location_name(""), pgp_version("Version: OpenPGP:SDK v0.9"),
    dns_name(""), only_pgp(true)
{
	uint32_t err_code;
	binary_pgp_key = nullptr;

	if(!initializeFromString(str, err_code))
	{
		std::cerr << __PRETTY_FUNCTION__ << " is deprecated because it can "
		          << "miserably fail like this! str: " << str
		          << " err_code: " << err_code << std::endl;
		print_stacktrace();
		throw err_code;
	}
}

RsCertificate::RsCertificate(const RsPeerDetails& Detail, const unsigned char *binary_pgp_block,size_t binary_pgp_block_size)
	:pgp_version("Version: OpenPGP:SDK v0.9")
{
	if(binary_pgp_block_size == 0 || binary_pgp_block == NULL)
		throw std::runtime_error("Cannot init a certificate with a void key block.") ;

	binary_pgp_key = new unsigned char[binary_pgp_block_size] ;
	memcpy(binary_pgp_key,binary_pgp_block,binary_pgp_block_size) ;
	binary_pgp_key_size = binary_pgp_block_size ;

	if(!Detail.isOnlyGPGdetail)
	{
		only_pgp = false ;
		location_id = RsPeerId( Detail.id ) ;
		location_name = Detail.location ;

		if (Detail.isHiddenNode)
		{
			hidden_node = true;
	 		hidden_node_address = Detail.hiddenNodeAddress;
			rs_sprintf_append(hidden_node_address, ":%u", Detail.hiddenNodePort);

			memset(ipv4_internal_ip_and_port,0,6) ;
			memset(ipv4_external_ip_and_port,0,6) ;
			dns_name = "" ;
		}
		else
		{
			hidden_node = false;
			hidden_node_address = "";

			try
			{
				scan_ip(Detail.localAddr,Detail.localPort,ipv4_internal_ip_and_port);
			}
			catch(...)
			{
				std::cerr << "RsCertificate::Invalid LocalAddress: "
				          << Detail.localAddr << std::endl;
				memset(ipv4_internal_ip_and_port,0,6);
			}


			try
			{
				scan_ip(Detail.extAddr,Detail.extPort,ipv4_external_ip_and_port);
			}
			catch(...)
			{
				std::cerr << "RsCertificate::Invalid ExternalAddress: "
				          << Detail.extAddr << std::endl;
				memset(ipv4_external_ip_and_port,0,6) ;
			}

			dns_name = Detail.dyndns;

			for(auto&& ipr : Detail.ipAddressList)
				mLocators.insert(RsUrl(ipr.substr(0, ipr.find(' '))));
		}
	}
	else
	{
		only_pgp = true ;
		hidden_node = false;
		hidden_node_address = "";
		location_id = RsPeerId() ;
		location_name = "" ;
		memset(ipv4_internal_ip_and_port,0,6) ;
		memset(ipv4_external_ip_and_port,0,6) ;
		dns_name = "" ;
	}
}

void RsCertificate::scan_ip(const std::string& ip_string, unsigned short port,unsigned char *ip_and_port)
{
	int d0,d1,d2,d3 ;

	if(4 != sscanf(ip_string.c_str(),"%d.%d.%d.%d",&d0,&d1,&d2,&d3))
		throw std::runtime_error( "Cannot parse ip from given string." );

	ip_and_port[0] = d0 ;
	ip_and_port[1] = d1 ;
	ip_and_port[2] = d2 ;
	ip_and_port[3] = d3 ;

	ip_and_port[4] = (port >> 8 ) & 0xff ;
	ip_and_port[5] =  port        & 0xff ;
}

bool RsCertificate::initializeFromString(const std::string& instr,uint32_t& err_code)
{
	try
	{
		std::string str ;
		err_code = CERTIFICATE_PARSING_ERROR_NO_ERROR ;

		// 0 - clean the string and check that it is pure radix64
		//
		for(uint32_t i=0;i<instr.length();++i)
		{
			if(instr[i] == ' ' || instr[i] == '\t' || instr[i] == '\n')
				continue ;

			if(! is_acceptable_radix64Char(instr[i]))
				return false ;

			str += instr[i] ;
		}
#ifdef DEBUG_RSCERTIFICATE
		std::cerr << "Decoding from:" << str << std::endl;
#endif
		// 1 - decode the string.
		//
        std::vector<uint8_t> bf = Radix64::decode(str) ;
        size_t size = bf.size();

		bool checksum_check_passed = false;
		unsigned char *buf = bf.data();
		size_t total_s = 0;
		only_pgp = true;
		uint8_t certificate_version = 0x00;

		while(total_s < size)
		{
			uint8_t ptag = buf[0];
			buf = &buf[1];

			unsigned char *buf2 = buf;
			uint32_t s = PGPKeyParser::read_125Size(buf);

			total_s += 1 + ((size_t)buf-(size_t)buf2) ;

			if(total_s > size)
			{
				err_code = CERTIFICATE_PARSING_ERROR_SIZE_ERROR ;
				return false ;
			}

#ifdef DEBUG_RSCERTIFICATE
			std::cerr << "Packet parse: read ptag " << (int)ptag << ", size " << s << ", total_s = " << total_s << ", expected total = " << size << std::endl;
#endif
			switch(ptag)
			{
			case CERTIFICATE_PTAG_VERSION_SECTION:
				certificate_version = buf[0];
				break;
			case CERTIFICATE_PTAG_PGP_SECTION:
				binary_pgp_key = new unsigned char[s];
				memcpy(binary_pgp_key,buf,s);
				binary_pgp_key_size = s;
				break;
			case CERTIFICATE_PTAG_NAME_SECTION:
				location_name = std::string((char *)buf,s);
				break;
			case CERTIFICATE_PTAG_SSLID_SECTION:
				if(s != location_id.SIZE_IN_BYTES)
				{
					err_code = CERTIFICATE_PARSING_ERROR_INVALID_LOCATION_ID;
					return false;
				}
				location_id = RsPeerId(buf);
				only_pgp = false;
				break;
			case CERTIFICATE_PTAG_DNS_SECTION:
				dns_name = std::string((char *)buf,s);
				break;
			case CERTIFICATE_PTAG_HIDDENNODE_SECTION:
				hidden_node_address = std::string((char *)buf,s);
				hidden_node = true;
				break;
			case CERTIFICATE_PTAG_LOCIPANDPORT_SECTION:
				if(s != 6)
				{
					err_code = CERTIFICATE_PARSING_ERROR_INVALID_LOCAL_IP;
					return false;
				}
				memcpy(ipv4_internal_ip_and_port,buf,s);
				break;
			case CERTIFICATE_PTAG_EXTIPANDPORT_SECTION:
				if(s != 6)
				{
					err_code = CERTIFICATE_PARSING_ERROR_INVALID_EXTERNAL_IP;
					return false;
				}
				memcpy(ipv4_external_ip_and_port,buf,s);
				break;
			case CERTIFICATE_PTAG_CHECKSUM_SECTION:
			{
				if(s != 3 || total_s+3 != size)
				{
					err_code =
					        CERTIFICATE_PARSING_ERROR_INVALID_CHECKSUM_SECTION;
					return false;
				}
				uint32_t computed_crc =
				        PGPKeyManagement::compute24bitsCRC(bf.data(),size-5);
				uint32_t certificate_crc =
				        buf[0] + (buf[1] << 8) + (buf[2] << 16);
				if(computed_crc != certificate_crc)
				{
					err_code = CERTIFICATE_PARSING_ERROR_CHECKSUM_ERROR;
					return false;
				}
				else checksum_check_passed = true;
				break;
			}
			case CERTIFICATE_PTAG_EXTRA_LOCATOR:
				mLocators.insert(RsUrl(std::string((char *)buf, s)));
				break;
			default:
				std::cerr << "(WW) unknwown PTAG 0x" << std::hex << ptag
				          << std::dec << " in certificate! Ignoring it."
				          << std::endl;
				break;
			}

			buf = &buf[s];
			total_s += s ;
		}

		if(!checksum_check_passed)
		{
			err_code = CERTIFICATE_PARSING_ERROR_MISSING_CHECKSUM ;
			return false ;
		}

		if(certificate_version != CERTIFICATE_VERSION_06)
		{
			err_code = CERTIFICATE_PARSING_ERROR_WRONG_VERSION ;
			return false ;
		}
#ifdef DEBUG_RSCERTIFICATE
		std::cerr << "Certificate is version " << (int)certificate_version << std::endl;
#endif

		if(total_s != size)	
			std::cerr << "(EE) Certificate contains trailing characters. Weird." << std::endl;

		return true ;
	}
	catch(std::exception& e)
	{
		if(binary_pgp_key != NULL)
			delete[] binary_pgp_key ;

		err_code = CERTIFICATE_PARSING_ERROR_SIZE_ERROR ;
		return false ;
	}
}

std::string RsCertificate::hidden_node_string() const 
{
	if ((!only_pgp) && (hidden_node))
	{
		return hidden_node_address;
	}

	std::string empty;
	return empty;
}

std::string RsCertificate::ext_ip_string() const
{
	std::ostringstream os ;
	os << (int)ipv4_external_ip_and_port[0] << "." << (int)ipv4_external_ip_and_port[1] << "." << (int)ipv4_external_ip_and_port[2] << "." << (int)ipv4_external_ip_and_port[3] ;
	return os.str() ;
}
std::string RsCertificate::loc_ip_string() const
{
	std::ostringstream os ;
	os << (int)ipv4_internal_ip_and_port[0] << "." << (int)ipv4_internal_ip_and_port[1] << "." << (int)ipv4_internal_ip_and_port[2] << "." << (int)ipv4_internal_ip_and_port[3] ;
	return os.str() ;
}

unsigned short RsCertificate::ext_port_us() const
{
	return (int)ipv4_external_ip_and_port[4]*256 + (int)ipv4_external_ip_and_port[5] ;
}

unsigned short RsCertificate::loc_port_us() const
{
	return (int)ipv4_internal_ip_and_port[4]*256 + (int)ipv4_internal_ip_and_port[5] ;
}

bool RsCertificate::cleanCertificate(const std::string& input,std::string& output,Format& format,int& error_code,bool check_content)
{
	if(cleanCertificate(input,output,error_code))
	{
		format = RS_CERTIFICATE_RADIX ;

        if(!check_content)
            return true ;

        try
        {
			RsCertificate c(input) ;
            return true ;
        }
        catch(uint32_t err_code)
        {
            error_code = err_code ;
            return false;
        }
	}

	return false ;
}

std::string RsCertificate::armouredPGPKey() const
{
	return PGPKeyManagement::makeArmouredKey(binary_pgp_key,binary_pgp_key_size,pgp_version) ;
}

// Yeah, this is simple, and that is what's good about the radix format. Can't be broken ;-)
//
bool RsCertificate::cleanCertificate(const std::string& instr,std::string& str,int& error_code)
{
	error_code = RS_PEER_CERT_CLEANING_CODE_NO_ERROR ;

	// 0 - clean the string and check that it is pure radix64
	//
	for(uint32_t i=0;i<instr.length();++i)
	{
		if(instr[i] == ' ' || instr[i] == '\t' || instr[i] == '\n')
			continue ;

		if(! is_acceptable_radix64Char(instr[i]))
		{
			error_code = RS_PEER_CERT_CLEANING_CODE_WRONG_RADIX_CHAR ;
			return false ;
		}

		str += instr[i] ;
	}

	// Now slice up to 64 chars.
	//
	std::string str2 ;
	static const int LINE_LENGTH = 64 ;

	for(int i=0;i<(int)str.length();++i)
	{
		str2 += str[i] ;

		if(i % LINE_LENGTH == LINE_LENGTH-1)
			str2 += '\n' ;
	}
	str = str2 ;

	return true ;
}






