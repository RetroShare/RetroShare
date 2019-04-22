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

#include "retroshare/rstypes.h"
#include "util/rsurl.h"

#include <set>
#include <string>

struct RsPeerDetails;

class RsCertificate
{
public:
	typedef enum { RS_CERTIFICATE_OLD_FORMAT, RS_CERTIFICATE_RADIX } Format;

	/**
	 * @brief Costruct an empty certificate, use toghether with
	 * if(initializeFromString) for safe certificate radix string parsing
	 */
	RsCertificate() :
	    ipv4_external_ip_and_port{0,0,0,0,0,0},
	    ipv4_internal_ip_and_port{0,0,0,0,0,0},
	    binary_pgp_key(nullptr), binary_pgp_key_size(0),
	    pgp_version("Version: OpenPGP:SDK v0.9"), only_pgp(true),
	    hidden_node(false) {}

	/**
	 * @brief Initialize from certificate string
	 * @param[in] str radix format string
	 * @param[out] errCode storage for eventual error code
	 * @return false on failure, true otherwise
	 */
	bool initializeFromString(const std::string& str, uint32_t& errCode);

	/// Constructs from binary gpg key, and RsPeerDetails.
	RS_DEPRECATED RsCertificate( const RsPeerDetails& details,
	               const unsigned char *gpg_mem_block,
	               size_t gpg_mem_block_size );

	virtual ~RsCertificate();

	/// Convert to certificate radix string
	std::string toStdString() const;

	std::string ext_ip_string() const;
	std::string loc_ip_string() const;
	std::string location_name_string() const { return location_name; }
	std::string dns_string() const { return dns_name ; }
	RsPeerId sslid() const { return location_id ; }
	std::string hidden_node_string() const;

	std::string armouredPGPKey() const;

	unsigned short ext_port_us() const;
	unsigned short loc_port_us() const;

	const unsigned char *pgp_key() const { return binary_pgp_key ; }
	size_t pgp_key_size() const { return binary_pgp_key_size ; }

	static bool cleanCertificate(
	        const std::string& input, std::string& output,
	        RsCertificate::Format& format, int& error_code, bool check_content);

	const std::set<RsUrl>& locators() const { return mLocators; }

	/**
	 * @deprecated using this costructor may raise exception that cause
	 *	crash if not handled, use empty constructor + if(initFromString) for a
	 *	safer behaviour.
	 */
	RS_DEPRECATED explicit RsCertificate(const std::string& input_string);

private:
	// new radix format
	static bool cleanCertificate( const std::string& input,
	                              std::string& output, int&);

	static void scan_ip( const std::string& ip_string, unsigned short port,
	                     unsigned char *destination_memory );

	static void addPacket(uint8_t ptag, const unsigned char *mem, size_t size,
	                      unsigned char*& buf, size_t& offset, size_t& buf_size);

	RsCertificate(const RsCertificate&) {} /// non copy-able
	const RsCertificate& operator=(const RsCertificate&); /// non copy-able

	unsigned char ipv4_external_ip_and_port[6];
	unsigned char ipv4_internal_ip_and_port[6];

	unsigned char *binary_pgp_key;
	size_t         binary_pgp_key_size;

	std::string location_name;
	RsPeerId location_id;
	std::string pgp_version;
	std::string dns_name;
	std::string hidden_node_address;
	std::set<RsUrl> mLocators;

	bool only_pgp ; /// does the cert contain only pgp info?
	bool hidden_node; /// IP or hidden Node Address.
};

