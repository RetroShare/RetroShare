/*******************************************************************************
 * libretroshare/src/pgp: rscertificate.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 Cyril Soler <csoler@users.sourceforge.net>                   *
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

#pragma once
/*
 * RetroShare
 *
 * Copyright (C) 2012-2014  Cyril Soler <csoler@users.sourceforge.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "retroshare/rstypes.h"
#include "util/rsurl.h"

#include <set>
#include <string>

struct RsPeerDetails;

class RsCertificate
{
	public:
		typedef enum { RS_CERTIFICATE_OLD_FORMAT, RS_CERTIFICATE_RADIX } Format ;

		// Constructs from text.
		//		- new format: The input string should only contain radix chars and spaces/LF/tabs.
		//
		explicit RsCertificate(const std::string& input_string) ;

		// Constructs from binary gpg key, and RsPeerDetails.
		//
		RsCertificate(const RsPeerDetails& details,const unsigned char *gpg_mem_block,size_t gpg_mem_block_size) ;

		// Constructs

		virtual ~RsCertificate();

		// Outut to text
		std::string toStdString() const ;

		std::string ext_ip_string() const ;
		std::string loc_ip_string() const ;
		std::string location_name_string() const { return location_name; }
		std::string dns_string() const { return dns_name ; }
		RsPeerId sslid() const { return location_id ; }
		std::string hidden_node_string() const;

		std::string armouredPGPKey() const ;

		unsigned short ext_port_us() const ;
		unsigned short loc_port_us() const ;

		const unsigned char *pgp_key() const { return binary_pgp_key ; }
		size_t pgp_key_size() const { return binary_pgp_key_size ; }

		static bool cleanCertificate(const std::string& input, std::string& output, RsCertificate::Format& format, int& error_code, bool check_content) ;
		const std::set<RsUrl>& locators() const { return mLocators; }

	private:
		static bool cleanCertificate(const std::string& input,std::string& output,int&) ;					// new radix format
		static void scan_ip(const std::string& ip_string, unsigned short port,unsigned char *destination_memory) ;

		bool initFromString(const std::string& str,uint32_t& err_code) ;

		static void addPacket(uint8_t ptag, const unsigned char *mem, size_t size, unsigned char *& buf, size_t& offset, size_t& buf_size) ;

		RsCertificate(const RsCertificate&) {}	// non copy-able
		const RsCertificate& operator=(const RsCertificate&)
		{ memset(ipv4_external_ip_and_port,0,6); memset(ipv4_internal_ip_and_port,0,6);
			binary_pgp_key = NULL; binary_pgp_key_size = 0;
			only_pgp = false; hidden_node = false;
			return *this ;}	// non copy-able

		unsigned char ipv4_external_ip_and_port[6] ;
		unsigned char ipv4_internal_ip_and_port[6] ;

		unsigned char *binary_pgp_key ;
		size_t         binary_pgp_key_size ;

		std::string location_name ;
		RsPeerId location_id ;
		std::string pgp_version ;
		std::string dns_name ;
		std::string hidden_node_address;
		std::set<RsUrl> mLocators;

		bool only_pgp ; // does the cert contain only pgp info?
		bool hidden_node; // IP or hidden Node Address.
};

