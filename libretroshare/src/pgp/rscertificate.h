#pragma once

#include <string>
#include <util/rsid.h>

class RsPeerDetails ;

static const int SSL_ID_SIZE = 16 ;

typedef t_RsGenericIdType<SSL_ID_SIZE> SSLIdType ;

class RsCertificate
{
	public:
		typedef enum { RS_CERTIFICATE_OLD_FORMAT, RS_CERTIFICATE_RADIX } Format ;

		// Constructs from text.
		// 	- old format: The input string must comply with the GPG format (See RFC4880) 
		//		- new format: The input string should only contain radix chars and spaces/LF/tabs.
		//
		RsCertificate(const std::string& input_string) ;

		// Constructs from binary gpg key, and RsPeerDetails.
		//
		RsCertificate(const RsPeerDetails& details,const unsigned char *gpg_mem_block,size_t gpg_mem_block_size) ;

		// Constructs

		virtual ~RsCertificate();

		// Outut to text
		std::string toStdString_oldFormat() const ;
		std::string toStdString() const ;

		std::string ext_ip_string() const ;
		std::string loc_ip_string() const ;
		std::string dns_string() const { return dns_name ; }
		std::string sslid_string() const { return location_id.toStdString(false) ; }
		std::string armouredPGPKey() const ;

		unsigned short ext_port_us() const ;
		unsigned short loc_port_us() const ;

		const unsigned char *pgp_key() const { return binary_pgp_key ; }
		size_t pgp_key_size() const { return binary_pgp_key_size ; }

		static bool cleanCertificate(const std::string& input,std::string& output,RsCertificate::Format& format,int& error_code) ;

	private:
		static bool cleanCertificate(const std::string& input,std::string& output,int&) ;					// new radix format
		static bool cleanCertificate_oldFormat(const std::string& input,std::string& output,int&) ;		// old text format
		static void scan_ip(const std::string& ip_string, unsigned short port,unsigned char *destination_memory) ;

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

		bool only_pgp ; // does the cert contain only pgp info?
};

