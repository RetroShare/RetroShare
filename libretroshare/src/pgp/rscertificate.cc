#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>

#include <util/radix64.h>
#include <pgp/pgpkeyutil.h>
#include "rscertificate.h"

static const std::string PGP_CERTIFICATE_START     ( "-----BEGIN PGP PUBLIC KEY BLOCK-----" );
static const std::string PGP_CERTIFICATE_END       ( "-----END PGP PUBLIC KEY BLOCK-----" );
static const std::string EXTERNAL_IP_BEGIN_SECTION ( "--EXT--" );
static const std::string LOCAL_IP_BEGIN_SECTION    ( "--LOCAL--" );
static const std::string SSLID_BEGIN_SECTION       ( "--SSLID--" );
static const std::string LOCATION_BEGIN_SECTION    ( "--LOCATION--" );

static const uint8_t CERTIFICATE_PTAG_PGP_SECTION           = 0x01 ;  
static const uint8_t CERTIFICATE_PTAG_EXTIPANDPORT_SECTION  = 0x02 ;  
static const uint8_t CERTIFICATE_PTAG_LOCIPANDPORT_SECTION  = 0x03 ;  
static const uint8_t CERTIFICATE_PTAG_DNS_SECTION           = 0x04 ;  
static const uint8_t CERTIFICATE_PTAG_SSLID_SECTION         = 0x05 ;  
static const uint8_t CERTIFICATE_PTAG_NAME_SECTION          = 0x06 ;  

std::string RsCertificate::toStdString_oldFormat() const
{
	std::string res ;

	res += PGPKeyManagement::makeArmouredKey(binary_pgp_key,binary_pgp_key_size,pgp_version) ;

	res += SSLID_BEGIN_SECTION ;
	res += location_id.toStdString() ;
	res += ";" ;
	res += LOCATION_BEGIN_SECTION ;
	res += location_name ;
	res += ";\n" ;

	std::ostringstream os ;
	os << LOCAL_IP_BEGIN_SECTION ;
	os << (int)ipv4_internal_ip_and_port[0] << "." << (int)ipv4_internal_ip_and_port[1] << "." << (int)ipv4_internal_ip_and_port[2] << "." << (int)ipv4_internal_ip_and_port[3] ;
	os << ":" ;
	os << ipv4_internal_ip_and_port[4]*256+ipv4_internal_ip_and_port[5] ;
	os << ";" ;

	os << EXTERNAL_IP_BEGIN_SECTION ;
	os << (int)ipv4_external_ip_and_port[0] << "." << (int)ipv4_external_ip_and_port[1] << "." << (int)ipv4_external_ip_and_port[2] << "." << (int)ipv4_external_ip_and_port[3] ;
	os << ":" ;
	os << ipv4_external_ip_and_port[4]*256+ipv4_external_ip_and_port[5] ;
	os << ";" ;

	res += os.str() ;
	res += "\n" ;

	return res ;
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

std::string RsCertificate::toStdString() const
{
	std::string res ;
	size_t BS = 1000 ;
	size_t p = 0 ;
	unsigned char *buf = new unsigned char[BS] ;

	addPacket( CERTIFICATE_PTAG_PGP_SECTION         , binary_pgp_key                         , binary_pgp_key_size     , buf, p, BS ) ;
	addPacket( CERTIFICATE_PTAG_EXTIPANDPORT_SECTION, ipv4_external_ip_and_port              ,                     6   , buf, p, BS ) ;
	addPacket( CERTIFICATE_PTAG_LOCIPANDPORT_SECTION, ipv4_internal_ip_and_port              ,                     6   , buf, p, BS ) ;
	addPacket( CERTIFICATE_PTAG_DNS_SECTION         , (unsigned char *)dns_name.c_str()      ,     dns_name.length()   , buf, p, BS ) ;
	addPacket( CERTIFICATE_PTAG_NAME_SECTION        , (unsigned char *)location_name.c_str() ,location_name.length()   , buf, p, BS ) ;
	addPacket( CERTIFICATE_PTAG_SSLID_SECTION       , location_id.toByteArray()              ,location_id.SIZE_IN_BYTES, buf, p, BS ) ;

	std::string out_string ;

	Radix64::encode((char *)buf, p, out_string) ;

	delete[] buf ;
	return out_string ;
}

RsCertificate::RsCertificate(const std::string& str)
	: 
	location_name(""), 
	pgp_version("Version: OpenPGP:SDK v0.9"),
		dns_name("")
{
	std::string err_string ;

	if(!initFromString(str,err_string) && !initFromString_oldFormat(str,err_string))
		throw std::runtime_error(err_string) ;
}

bool RsCertificate::initFromString(const std::string& instr,std::string& err_string)
{
	std::string str ;

	// 0 - clean the string and check that it is pure radix64
	//
	for(uint32_t i=0;i<instr.length();++i)
	{
		if(instr[i] == ' ' || instr[i] == '\t' || instr[i] == '\n')
			continue ;

		if(! ( (instr[i] >= '0' && instr[i] <= '9') || (instr[i] >= 'a' && instr[i] <= 'z') || (instr[i] >= 'A' && instr[i] <= 'Z') || (instr[i] == '+' || instr[i] == '/') || instr[i] == '='))
			return false ;

		str += instr[i] ;
	}

	std::cerr << "Decodign from:" << str << std::endl;
	// 1 - decode the string.
	//
	char *bf = NULL ;
	size_t size ;
	Radix64::decode(str,bf, size) ;

	unsigned char *buf = (unsigned char *)bf ;
	size_t total_s = 0 ;

	while(total_s < size)
	{
		uint8_t ptag = buf[0];
		buf = &buf[1] ;

		unsigned char *buf2 = buf ;
		uint32_t s = PGPKeyParser::read_125Size(buf) ;

		total_s += 1 + ((unsigned long)buf-(unsigned long)buf2) ;

		std::cerr << "Packet parse: read ptag " << (int)ptag << ", size " << size << ", total_s = " << total_s << std::endl;

		switch(ptag)
		{
			case CERTIFICATE_PTAG_PGP_SECTION: binary_pgp_key = new unsigned char[s] ;
														  memcpy(binary_pgp_key,buf,s) ;
														  binary_pgp_key_size = s ;
														  buf = &buf[s] ;
														  break ;

			case CERTIFICATE_PTAG_NAME_SECTION: location_name = std::string((char *)buf,s) ;
															buf = &buf[s] ;
															break ;

			case CERTIFICATE_PTAG_SSLID_SECTION: 
															if(s != location_id.SIZE_IN_BYTES)
															{
																err_string = "Inconsistent size in certificate section 'location ID'" ;
																return false ;
															}

															location_id = SSLIdType(buf) ;
															buf = &buf[s] ;
															break ;

			case CERTIFICATE_PTAG_DNS_SECTION: dns_name = std::string((char *)buf,s) ;
															buf = &buf[s] ;
														  break ;

			case CERTIFICATE_PTAG_LOCIPANDPORT_SECTION: 
														  if(s != 6)
														  {
															  err_string = "Inconsistent size in certificate section 'external IP'" ;
															  return false ;
														  }

														  memcpy(ipv4_internal_ip_and_port,buf,s) ;
														  buf = &buf[s] ;
														  break ;
			case CERTIFICATE_PTAG_EXTIPANDPORT_SECTION: 
														  if(s != 6)
														  {
															  err_string = "Inconsistent size in certificate section 'external IP'" ;
															  return false ;
														  }

														  memcpy(ipv4_external_ip_and_port,buf,s) ;
														  buf = &buf[s] ;
														  break ;
			default:
														  err_string = "Cannot read certificate. Parsing error in binary packets." ;
														  return false ;
		}

		total_s += s ;
	}

	delete[] bf ;
	return true ;
}

bool RsCertificate::initFromString_oldFormat(const std::string& certstr,std::string& err_string)
{
	//parse the text to get ip address
	try 
	{
		const std::string CERT_SSL_ID = "--SSLID--";
		const std::string CERT_LOCATION = "--LOCATION--";
		const std::string CERT_LOCAL_IP = "--LOCAL--";
		const std::string CERT_EXT_IP = "--EXT--";
		const std::string CERT_DYNDNS = "--DYNDNS--";

		std::string cert;
		std::string peerInfo;

		/* search for -----END CERTIFICATE----- */
		std::string pgpend("-----END PGP PUBLIC KEY BLOCK-----");

		size_t pos = certstr.find(pgpend);

		if (pos != std::string::npos) 
		{
			pos += pgpend.length();
			cert = certstr.substr(0, pos);
			if (pos + 1 < certstr.length())
				peerInfo = certstr.substr(pos + 1);
		}

		if(cert.empty())
			return false ;

		// find radix 64 part.

		std::string radix_cert = PGPKeyParser::extractRadixPartFromArmouredKey(certstr,pgp_version) ;

		char *key_bin ;
		Radix64::decode(radix_cert,key_bin,binary_pgp_key_size) ;

		binary_pgp_key = (unsigned char *)key_bin ;

#ifdef P3PEERS_DEBUG
		std::cerr << "Parsing cert for sslid, location, ext and local address details. : " << certstr << std::endl;
#endif

		//let's parse the ssl id
		size_t parsePosition = peerInfo.find(CERT_SSL_ID);
		std::cerr << "sslid position : " << parsePosition << std::endl;
		if (parsePosition != std::string::npos) {
			parsePosition += CERT_SSL_ID.length();
			std::string subCert = peerInfo.substr(parsePosition);
			parsePosition = subCert.find(";");
			if (parsePosition != std::string::npos) {
				std::string ssl_id = subCert.substr(0, parsePosition);
				std::cerr << "SSL id : " << ssl_id << std::endl;

				location_id = SSLIdType(ssl_id) ;
			}
		}

		//let's parse the location
		parsePosition = peerInfo.find(CERT_LOCATION);
		std::cerr << "location position : " << parsePosition << std::endl;
		if (parsePosition != std::string::npos) {
			parsePosition += CERT_LOCATION.length();
			std::string subCert = peerInfo.substr(parsePosition);
			parsePosition = subCert.find(";");
			if (parsePosition != std::string::npos) {
				std::string location = subCert.substr(0, parsePosition);
				std::cerr << "location : " << location << std::endl;

				location_name = location;
			}
		}

		//let's parse ip local address
		parsePosition = peerInfo.find(CERT_LOCAL_IP);
		std::cerr << "local ip position : " << parsePosition << std::endl;
		if (parsePosition != std::string::npos) {
			parsePosition += CERT_LOCAL_IP.length();
			std::string subCert = peerInfo.substr(parsePosition);
			parsePosition = subCert.find(":");
			if (parsePosition != std::string::npos) {
				std::string local_ip = subCert.substr(0, parsePosition);
				std::cerr << "Local Ip : " << local_ip << std::endl;

				unsigned short localPort ;

				//let's parse local port
				subCert = subCert.substr(parsePosition + 1);
				parsePosition = subCert.find(";");
				if (parsePosition != std::string::npos) {
					std::string local_port = subCert.substr(0, parsePosition);
					std::cerr << "Local port : " << local_port << std::endl;
					sscanf(local_port.c_str(), "%hu", &localPort);
				}

				int d0,d1,d2,d3 ;

				if(4 != sscanf(local_ip.c_str(),"%d.%d.%d.%d",&d0,&d1,&d2,&d3))
				{
					err_string = "Cannot parse ip from given string." ;
					return false ;
				}
				ipv4_internal_ip_and_port[0] = d0 ;
				ipv4_internal_ip_and_port[1] = d1 ;
				ipv4_internal_ip_and_port[2] = d2 ;
				ipv4_internal_ip_and_port[3] = d3 ;

				ipv4_internal_ip_and_port[4] = (localPort >> 8 ) & 0xff ;
				ipv4_internal_ip_and_port[5] = localPort & 0xff ;
			}
		}

		//let's parse ip ext address
		parsePosition = peerInfo.find(CERT_EXT_IP);
		std::cerr << "Ext ip position : " << parsePosition << std::endl;
		if (parsePosition != std::string::npos) {
			parsePosition = parsePosition + CERT_EXT_IP.length();
			std::string subCert = peerInfo.substr(parsePosition);
			parsePosition = subCert.find(":");
			if (parsePosition != std::string::npos) {
				std::string ext_ip = subCert.substr(0, parsePosition);
				std::cerr << "Ext Ip : " << ext_ip << std::endl;

				unsigned short extPort ;
				//let's parse ext port
				subCert = subCert.substr(parsePosition + 1);
				parsePosition = subCert.find(";");
				if (parsePosition != std::string::npos) {
					std::string ext_port = subCert.substr(0, parsePosition);
					std::cerr << "Ext port : " << ext_port << std::endl;
					sscanf(ext_port.c_str(), "%hu", &extPort);
				}
				
				int d0,d1,d2,d3 ;

				if(4 != sscanf(ext_ip.c_str(),"%d.%d.%d.%d",&d0,&d1,&d2,&d3))
				{
					err_string = "Cannot parse ip from given string." ;
					return false ;
				}
				ipv4_external_ip_and_port[0] = d0 ;
				ipv4_external_ip_and_port[1] = d1 ;
				ipv4_external_ip_and_port[2] = d2 ;
				ipv4_external_ip_and_port[3] = d3 ;

				ipv4_external_ip_and_port[4] = (extPort >> 8 ) & 0xff ;
				ipv4_external_ip_and_port[5] = extPort & 0xff ;
			}
		}

		//let's parse DynDNS
		parsePosition = peerInfo.find(CERT_DYNDNS);
		std::cerr << "location DynDNS : " << parsePosition << std::endl;
		if (parsePosition != std::string::npos) {
			parsePosition += CERT_DYNDNS.length();
			std::string subCert = peerInfo.substr(parsePosition);
			parsePosition = subCert.find(";");
			if (parsePosition != std::string::npos) {
				std::string DynDNS = subCert.substr(0, parsePosition);
				std::cerr << "DynDNS : " << DynDNS << std::endl;

				dns_name = DynDNS;
			}
		}

	} 
	catch (...) 
	{
		std::cerr << "ConnectFriendWizard : Parse ip address error." << std::endl;
		return false ;
	}

	return true;
}





