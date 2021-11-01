/*******************************************************************************
 * libretroshare/src/pgp: rscertificate.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016  Cyril Soler <csoler@users.sourceforge.net>              *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#include "util/rsmemory.h"
#include "util/rsdebug.h"

#include <set>
#include <string>
#include <memory>

struct RsPeerDetails;

class RsCertificate
{
public:
    enum class RsShortInviteFieldType : uint8_t
    {
        SSL_ID          = 0x00,
        PEER_NAME       = 0x01,
        LOCATOR         = 0x02,
        PGP_FINGERPRINT = 0x03,
        CHECKSUM        = 0x04,

        /* The following will be deprecated, and ported to LOCATOR when generic transport layer will be implemented */
        HIDDEN_LOCATOR  = 0x90,
        DNS_LOCATOR     = 0x91,
        EXT4_LOCATOR    = 0x92,		// external IPv4 address
        LOC4_LOCATOR    = 0x93		// local IPv4 address
    };

	typedef enum { RS_CERTIFICATE_OLD_FORMAT, RS_CERTIFICATE_RADIX, RS_CERTIFICATE_SHORT_RADIX } Format;

	/**
	 * @brief Create certificate object from certificate string
	 * @param[in] str radix format certificate string
	 * @param[out] errorCode Optional storage for eventual error code,
	 *	meaningful only on failure
	 * @return nullptr on failure, pointer to the generated certificate
	 *	otherwise
	 */
	static std::unique_ptr<RsCertificate> fromString(
	        const std::string& str,
	        uint32_t& errorCode = RS_DEFAULT_STORAGE_PARAM(uint32_t) );

	/**
	 * @brief Create certificate object from peer details and PGP memory block
	 * @param[in] details peer details
	 * @param[in] binary_pgp_block pointer to PGP memory block
	 * @param[in] binary_pgp_block_size size of PGP memory block
	 * @return nullptr on failure, pointer to the generated certificate
	 *	otherwise
	 */
	static std::unique_ptr<RsCertificate> fromMemoryBlock(
	        const RsPeerDetails& details, const uint8_t* binary_pgp_block,
	        size_t binary_pgp_block_size );

	~RsCertificate();

    static bool decodeRadix64ShortInvite(const std::string& short_invite_b64,RsPeerDetails& det,uint32_t& error_code);

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
	        RsCertificate::Format& format, uint32_t& error_code, bool check_content);

	const std::set<RsUrl>& locators() const { return mLocators; }

	/**
	 * @deprecated using this costructor may raise exception that cause
	 *	crash if not handled.
	 */
	RS_DEPRECATED_FOR("RsCertificate::fromMemoryBlock(...)")
	RsCertificate( const RsPeerDetails& details,
	               const unsigned char *gpg_mem_block,
	               size_t gpg_mem_block_size );

private:
	// new radix format
	static bool cleanRadix64(const std::string& input, std::string& output, uint32_t &);

	static void scan_ip( const std::string& ip_string, unsigned short port,
	                     unsigned char *destination_memory );

	static void addPacket(uint8_t ptag, const unsigned char *mem, size_t size,
	                      unsigned char*& buf, size_t& offset, size_t& buf_size);

	RsCertificate(const RsCertificate&) {} /// non copy-able
	const RsCertificate& operator=(const RsCertificate&); /// non copy-able

	/// @brief Costruct an empty certificate
	RsCertificate() :
	    ipv4_external_ip_and_port{0,0,0,0,0,0},
	    ipv4_internal_ip_and_port{0,0,0,0,0,0},
	    binary_pgp_key(nullptr), binary_pgp_key_size(0),
	    pgp_version("Version: OpenPGP:SDK v0.9"), only_pgp(true),
	    hidden_node(false) {}

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

	RS_SET_CONTEXT_DEBUG_LEVEL(1)
};

