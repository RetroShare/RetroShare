#pragma once

#include <string>
#include <util/rsid.h>

static const int SSL_ID_SIZE = 16 ;

typedef t_RsGenericIdType<SSL_ID_SIZE> SSLIdType ;

class RsCertificate
{
	public:
		// Constructs from text
		RsCertificate(const std::string& input_string) ;

		virtual ~RsCertificate();

		// Outut to text
		std::string toStdString_oldFormat() const ;
		std::string toStdString() const ;

	private:
		bool initFromString(const std::string& str,std::string& err_string) ;
		bool initFromString_oldFormat(const std::string& str,std::string& err_string) ;

		static void addPacket(uint8_t ptag, const unsigned char *mem, size_t size, unsigned char *& buf, size_t& offset, size_t& buf_size) ;

		RsCertificate(const RsCertificate&) {}	// non copy-able
		const RsCertificate& operator=(const RsCertificate&) { return *this ;}	// non copy-able

		unsigned char ipv4_external_ip_and_port[6] ;
		unsigned char ipv4_internal_ip_and_port[6] ;

		unsigned char *binary_pgp_key ;
		size_t         binary_pgp_key_size ;

		std::string location_name ;
		SSLIdType location_id ;
		std::string pgp_version ;
		std::string dns_name ;
};

