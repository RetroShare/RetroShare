#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>

#include <retroshare/rspeers.h>
#include <util/radix64.h>
#include <pgp/pgpkeyutil.h>
#include "rscertificate.h"
#include "util/rsstring.h"

#define DEBUG_RSCERTIFICATE 

static const std::string PGP_CERTIFICATE_START     ( "-----BEGIN PGP PUBLIC KEY BLOCK-----" );
static const std::string PGP_CERTIFICATE_END       ( "-----END PGP PUBLIC KEY BLOCK-----" );
static const std::string EXTERNAL_IP_BEGIN_SECTION ( "--EXT--" );
static const std::string LOCAL_IP_BEGIN_SECTION    ( "--LOCAL--" );
static const std::string SSLID_BEGIN_SECTION       ( "--SSLID--" );
static const std::string LOCATION_BEGIN_SECTION    ( "--LOCATION--" );
static const std::string HIDDEN_NODE_BEGIN_SECTION    ( "--HIDDEN--" );

static const uint8_t CERTIFICATE_PTAG_PGP_SECTION           = 0x01 ;  
static const uint8_t CERTIFICATE_PTAG_EXTIPANDPORT_SECTION  = 0x02 ;  
static const uint8_t CERTIFICATE_PTAG_LOCIPANDPORT_SECTION  = 0x03 ;  
static const uint8_t CERTIFICATE_PTAG_DNS_SECTION           = 0x04 ;  
static const uint8_t CERTIFICATE_PTAG_SSLID_SECTION         = 0x05 ;  
static const uint8_t CERTIFICATE_PTAG_NAME_SECTION          = 0x06 ;  
static const uint8_t CERTIFICATE_PTAG_CHECKSUM_SECTION      = 0x07 ;  
static const uint8_t CERTIFICATE_PTAG_HIDDENNODE_SECTION    = 0x08 ;  
static const uint8_t CERTIFICATE_PTAG_VERSION_SECTION       = 0x09 ;  

static const uint8_t CERTIFICATE_VERSION_06 = 0x06 ;

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

std::string RsCertificate::toStdString() const
{
	std::string res ;
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
	}

	uint32_t computed_crc = PGPKeyManagement::compute24bitsCRC(buf,p) ;

	// handle endian issues.
	unsigned char mem[3] ;
	mem[0] =  computed_crc        & 0xff ;
	mem[1] = (computed_crc >> 8 ) & 0xff ;
	mem[2] = (computed_crc >> 16) & 0xff ;

	addPacket( CERTIFICATE_PTAG_CHECKSUM_SECTION,mem,3,buf,p,BS) ;

	std::string out_string ;

	Radix64::encode((char *)buf, p, out_string) ;

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

RsCertificate::RsCertificate(const std::string& str)
	: 
	location_name(""), 
	pgp_version("Version: OpenPGP:SDK v0.9"),
		dns_name(""),only_pgp(true)
{
	uint32_t err_code ;

	if(!initFromString(str,err_code)) // && !initFromString_oldFormat(str,err_code))
		throw err_code ;
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
		location_id = SSLIdType( Detail.id ) ;
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
				scan_ip(Detail.localAddr,Detail.localPort,ipv4_internal_ip_and_port) ;
			}
			catch(...)
			{
				std::cerr << "RsCertificate::Invalid LocalAddress";
				std::cerr << std::endl;
				memset(ipv4_internal_ip_and_port,0,6) ;
			}


			try
			{
				scan_ip(Detail.extAddr,Detail.extPort,ipv4_external_ip_and_port) ;
			}
			catch(...)
			{
				std::cerr << "RsCertificate::Invalid ExternalAddress";
				std::cerr << std::endl;
				memset(ipv4_external_ip_and_port,0,6) ;
			}

			dns_name = Detail.dyndns ;
		}
	}
	else
	{
		only_pgp = true ;
		hidden_node = false;
		hidden_node_address = "";
		location_id = SSLIdType() ;
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

bool RsCertificate::initFromString(const std::string& instr,uint32_t& err_code)
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
	char *bf = NULL ;
	size_t size ;
	Radix64::decode(str,bf, size) ;

	bool checksum_check_passed = false ;
	unsigned char *buf = (unsigned char *)bf ;
	size_t total_s = 0 ;
	only_pgp = true ;
	uint8_t certificate_version = 0x00 ;

	while(total_s < size)
	{
		uint8_t ptag = buf[0];
		buf = &buf[1] ;

		unsigned char *buf2 = buf ;
		uint32_t s = PGPKeyParser::read_125Size(buf) ;

		total_s += 1 + ((unsigned long)buf-(unsigned long)buf2) ;

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
			case CERTIFICATE_PTAG_VERSION_SECTION: certificate_version = buf[0] ;
																buf = &buf[s] ;
																break ;
				
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
																err_code = CERTIFICATE_PARSING_ERROR_INVALID_LOCATION_ID ;
																return false ;
															}

															location_id = SSLIdType(buf) ;
															buf = &buf[s] ;
															only_pgp = false ;
															break ;

			case CERTIFICATE_PTAG_DNS_SECTION: dns_name = std::string((char *)buf,s) ;
															buf = &buf[s] ;
														  break ;

			case CERTIFICATE_PTAG_HIDDENNODE_SECTION: 
				hidden_node_address = std::string((char *)buf,s);
				hidden_node = true;
				buf = &buf[s];
				
				break ;

			case CERTIFICATE_PTAG_LOCIPANDPORT_SECTION: 
														  if(s != 6)
														  {
															  err_code = CERTIFICATE_PARSING_ERROR_INVALID_LOCAL_IP;
															  return false ;
														  }

														  memcpy(ipv4_internal_ip_and_port,buf,s) ;
														  buf = &buf[s] ;
														  break ;
			case CERTIFICATE_PTAG_EXTIPANDPORT_SECTION: 
														  if(s != 6)
														  {
															  err_code = CERTIFICATE_PARSING_ERROR_INVALID_EXTERNAL_IP;
															  return false ;
														  }

														  memcpy(ipv4_external_ip_and_port,buf,s) ;
														  buf = &buf[s] ;
														  break ;
			case CERTIFICATE_PTAG_CHECKSUM_SECTION: 
														  {
															  if(s != 3 || total_s+3 != size)
															  {
																  err_code = CERTIFICATE_PARSING_ERROR_INVALID_CHECKSUM_SECTION ;
																  return false ;
															  }
															  uint32_t computed_crc = PGPKeyManagement::compute24bitsCRC((unsigned char *)bf,size-5) ;
															  uint32_t certificate_crc = buf[0] + (buf[1] << 8) + (buf[2] << 16) ;

															  if(computed_crc != certificate_crc)
															  {
																  err_code = CERTIFICATE_PARSING_ERROR_CHECKSUM_ERROR ;
																  return false ;
															  }
															  else
																  checksum_check_passed = true ;
														  }
														  break ;
			default:
														  err_code = CERTIFICATE_PARSING_ERROR_UNKNOWN_SECTION_PTAG ;
														  return false ;
		}

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

	delete[] bf ;
	return true ;
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

std::string RsCertificate::sslid_string() const 
{
	if (only_pgp)
	{
		std::string empty;
		return empty;
	}
	else
	{
		return location_id.toStdString(false); 
	}
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

bool RsCertificate::cleanCertificate(const std::string& input,std::string& output,Format& format,int& error_code)
{
//	if(cleanCertificate_oldFormat(input,output,error_code))
//	{
//		format = RS_CERTIFICATE_OLD_FORMAT ;
//		return true ;
//	}

	if(cleanCertificate(input,output,error_code))
	{
		format = RS_CERTIFICATE_RADIX ;
		return true ;
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

// All the code below should be removed when in 0.6. Certificates will only use the new format.
//
bool RsCertificate::cleanCertificate_oldFormat(const std::string& certstr,std::string& cleanCertificate,int& error_code)
{
	error_code = RS_PEER_CERT_CLEANING_CODE_UNKOWN_ERROR ; // default
	const std::string& badCertificate(certstr) ;

	std::string pgpend("-----END PGP PUBLIC KEY BLOCK-----");

	size_t pos = certstr.find(pgpend);
	std::string peer_info ;
	std::string cert ;

	if (pos != std::string::npos) 
	{
		pos += pgpend.length();
		cert = certstr.substr(0, pos);
		if (pos + 1 < certstr.length())
			peer_info = certstr.substr(pos + 1);
	}
	else
	{
		error_code = RS_PEER_CERT_CLEANING_CODE_NO_END_TAG ;
		return false ;
	}

	if(cert.empty())
		return false ;

	/*
		Buffer for storing the cleaned certificate. In certain cases the 
		cleanCertificate can be larger than the badCertificate
	 */
	cleanCertificate = "";
	//The entire certificate begin tag
	const char * beginCertTag="-----BEGIN";
	//The entire certificate end tag
	const char * endCertTag="-----END";
	//Tag containing dots. The common part of both start and end tags 
	const char * commonTag="-----";
	//Only BEGIN part of the begin tag
	const char * beginTag="BEGIN";
	//Only END part of the end tag
	const char * endTag="END";
	//The start index of the ----- part of the certificate begin tag
	size_t beginCertStartIdx1=0;
	//The start index of the BEGIN part of the certificate begin tag
	size_t beginCertStartIdx2=0;
	//The start index of the end part(-----) of the certificate begin tag. The begin tag ends with -----. Example -----BEGIN XPGP CERTIFICATE-----
	size_t beginCertEndIdx=0;
	//The start index of the ----- part of the certificate end tag
	size_t endCertStartIdx1=0;
	//The start index of the END part of the certificate end tag
	size_t endCertStartIdx2=0;
	//The start index of the end part(-----) of the certificate end tag. The begin tag ends with -----. Example -----BEGIN XPGP CERTIFICATE-----
	size_t endCertEndIdx=0;
	//The length of the bad certificate.
	size_t lengthOfCert=certstr.length();
	//The current index value in the bad certificate
	size_t currBadCertIdx=0;
	//Temporary index value
	size_t tmpIdx=0;
	//Boolean flag showing if the begin tag or the end tag has been found
	bool found=false;
	/*
		Calculating the value of the beginCertStartIdx1 and beginCertStartIdx2. Here
		we first locate the occurance of ----- and then the location of BEGIN. Next
		we check if there are any non space or non new-line characters between their
		occureance. If there are any other characters between the two(----- and
		BEGIN), other than space and new line then it means that it is the
		certificate begin tag.  Here we take care of the fact that we may have
		introduced some spaces and newlines in the begin tag by mistake. This takes
		care of the spaces and newlines between ----- and BEGIN.
	 */

	while(found==false && (beginCertStartIdx1=certstr.find(commonTag,tmpIdx))!=std::string::npos)
	{
		beginCertStartIdx2=certstr.find(beginTag,beginCertStartIdx1+strlen(commonTag));	
		tmpIdx=beginCertStartIdx1+strlen(commonTag);	
		if(beginCertStartIdx2!=std::string::npos)
		{
			found=true;
			for(size_t i=beginCertStartIdx1+strlen(commonTag);i<beginCertStartIdx2;i++)
			{
				if(certstr[i]!=' ' && certstr[i]!='\n' )
				{
					found=false;
					break;
				}
			}
		}
		else
		{
			break;
		}

	}
	/*
		begin tag not found
	 */
	if(!found)
	{
		std::cerr<<"Certificate corrupted beyond repair: No <------BEGIN > tag"<<std::endl;		
		error_code = RS_PEER_CERT_CLEANING_CODE_NO_BEGIN_TAG ;
		return false;	
	}
	beginCertEndIdx=certstr.find(commonTag,beginCertStartIdx2);
	if(beginCertEndIdx==std::string::npos)
	{	
		std::cerr<<"Certificate corrupted beyond repair: No <------BEGIN > tag"<<std::endl;		
		error_code = RS_PEER_CERT_CLEANING_CODE_NO_BEGIN_TAG ;
		return false;	
	}
	tmpIdx=beginCertEndIdx+strlen(commonTag);
	found=false;
	/*
		Calculating the value of the endCertStartIdx1 and endCertStartIdx2. Here we first locate the occurance of ----- and then 
		the location of END. Next we check if there are any non space or non new-line characters between their occureance. If there are any other
		characters between the two(----- and END), other than space and new line then it means that it is the certificate end tag. 
		Here we take care of the fact that we may have introduced some spaces and newlines in the end tag by mistake. This
		takes care of the spaces and newlines between ----- and END.
	 */
	while(found==false && (endCertStartIdx1=certstr.find(commonTag,tmpIdx))!=std::string::npos)
	{
		endCertStartIdx2=certstr.find(endTag,endCertStartIdx1+strlen(commonTag));	
		tmpIdx=endCertStartIdx1+strlen(commonTag);	
		if(endCertStartIdx2!=std::string::npos)
		{
			found=true;
			for(size_t i=endCertStartIdx1+strlen(commonTag);i<endCertStartIdx2;i++)
			{
				if(certstr[i]!=' '&& certstr[i]!='\n')
				{
					found=false;
					break;
				}
			}
		}
		else
		{
			break;
		}

	}
	/*
		end tag not found
	 */
	if(!found)
	{
		std::cerr<<"Certificate corrupted beyond repair: No <------END > tag"<<std::endl;		
		error_code = RS_PEER_CERT_CLEANING_CODE_NO_END_TAG ;
		return false;
	}	
	endCertEndIdx=certstr.find(commonTag,endCertStartIdx2);
	if(endCertEndIdx==std::string::npos || endCertEndIdx>=lengthOfCert)
	{	
		std::cerr<<"Certificate corrupted beyond repair: No <------END > tag"<<std::endl;		
		error_code = RS_PEER_CERT_CLEANING_CODE_NO_END_TAG ;
		return false;
	}
	/*
		Copying the begin tag(-----BEGIN) to the clean certificate
	 */
	cleanCertificate += beginCertTag;
	currBadCertIdx=beginCertStartIdx2+strlen(beginTag);
	/*
		Copying the name of the tag e.g XPGP CERTIFICATE. At the same time remove any white spaces and new line
		characters.
	 */
	while(currBadCertIdx<beginCertEndIdx)
	{
		if(badCertificate[currBadCertIdx]=='\n')
		{
			currBadCertIdx++;
		}
		else if(badCertificate[currBadCertIdx]==' ' && (badCertificate[currBadCertIdx-1]==' '|| badCertificate[currBadCertIdx-1]=='\n') )
		{
			currBadCertIdx++;
		}
		else
		{
			cleanCertificate += badCertificate[currBadCertIdx];
			currBadCertIdx++;
		}
	}	
	/*
		If the last character is a space we need to remove it.
	 */
	if(cleanCertificate.substr(cleanCertificate.length()-1, 1) == " ")
	{
		cleanCertificate.erase(cleanCertificate.length()-1);
	}
	/*
		Copying the end part of the certificate start tag(-----). 
	 */
	cleanCertificate += commonTag;
	cleanCertificate += "\n";
	currBadCertIdx=currBadCertIdx+strlen(commonTag);  
	/*
		Remove the white spaces between the end of the certificate begin tag and the actual
		start of the certificate.
	 */
	while(badCertificate[currBadCertIdx]=='\n'|| badCertificate[currBadCertIdx]==' ')
	{
		currBadCertIdx++;
	}

	//keep the armor header
	std::list<std::string> header;
	header.push_back("Version");
	header.push_back("Comment");
	header.push_back("MessageID");
	header.push_back("Hash");
	header.push_back("Charset");

	for (std::list<std::string>::iterator headerIt = header.begin (); headerIt != header.end(); headerIt++)
	{
		if (badCertificate.substr(currBadCertIdx, (*headerIt).length()) == *headerIt)
		{
			cleanCertificate += badCertificate.substr(currBadCertIdx, (*headerIt).length());
			currBadCertIdx += (*headerIt).length();
			while(currBadCertIdx<endCertStartIdx1 && badCertificate[currBadCertIdx]!='\n')
			{
				cleanCertificate += badCertificate[currBadCertIdx];
				currBadCertIdx++;
			}
			cleanCertificate += "\n";
		}
	}

	//add empty line after armor header
	cleanCertificate += "\n";

	//Start of the actual certificate. Remove spaces in the certificate
	//and make sure there are 64 characters per line in the
	//new cleaned certificate
	int cntPerLine=0;
	while(currBadCertIdx<endCertStartIdx1)
	{
		if(cntPerLine==64)
		{
			cleanCertificate += "\n";
			cntPerLine=0;
		}

		if(badCertificate[currBadCertIdx]=='=') /* checksum */
			break;
		else if(badCertificate[currBadCertIdx]=='\t')
			currBadCertIdx++;
		else if(badCertificate[currBadCertIdx]==' ')
			currBadCertIdx++;
		else if(badCertificate[currBadCertIdx]=='\n')
			currBadCertIdx++;
		else if(is_acceptable_radix64Char(badCertificate[currBadCertIdx]))
		{
			cleanCertificate += badCertificate[currBadCertIdx];
			cntPerLine++;
			currBadCertIdx++;
		}
		else
		{
			std::cerr << "Warning: Invalid character in radix certificate encoding: " << badCertificate[currBadCertIdx] << std::endl;
			currBadCertIdx++;
		}
	}
	if(currBadCertIdx>=endCertStartIdx1)
	{
		std::cerr<<"Certificate corrupted beyond repair: No checksum, or no newline after first tag"<<std::endl;		
		error_code = RS_PEER_CERT_CLEANING_CODE_NO_CHECKSUM ;
		return false;
	}

	while(currBadCertIdx < endCertStartIdx1 && (badCertificate[currBadCertIdx] == '=' || badCertificate[currBadCertIdx] == ' ' || badCertificate[currBadCertIdx] == '\n' ))
		currBadCertIdx++ ;

	switch(cntPerLine % 4)
	{
		case 0: break ;
		case 1: std::cerr<<"Certificate corrupted beyond repair: wrongnumber of chars on last line (n%4=1)"<<std::endl;		
				  error_code = RS_PEER_CERT_CLEANING_CODE_WRONG_NUMBER;
				  return false ;
		case 2: cleanCertificate += "==" ;
				  break ;
		case 3: cleanCertificate += "=" ;
				  break ;
	}
	cleanCertificate += "\n=";

	//	if (badCertificate[currBadCertIdx] == '=')
	//	{
	/* checksum */

	while(currBadCertIdx<endCertStartIdx1)
	{
		if (badCertificate[currBadCertIdx]==' ')
		{
			currBadCertIdx++;
			continue;
		}
		else if(badCertificate[currBadCertIdx]=='\n')
		{
			currBadCertIdx++;
			continue;
		}
		cleanCertificate += badCertificate[currBadCertIdx];
		cntPerLine++;
		currBadCertIdx++;
	}
	//	}

	if(cleanCertificate.substr(cleanCertificate.length()-1,1)!="\n")
	{
		cleanCertificate += "\n";
		//		std::cerr<<"zeeeee"<<std::endl;
	}
	else
	{
		//		std::cerr<<"zooooo"<<std::endl;
	}
	/*
		Copying the begining part of the certificate end tag. Copying
		-----END part of the tag.
	 */
	cleanCertificate += endCertTag;
	currBadCertIdx=endCertStartIdx2+strlen(endTag);
	/*
		Copying the name of the certificate e.g XPGP CERTIFICATE. The end tag also has the
		the name of the tag.
	 */
	while(currBadCertIdx<endCertEndIdx)
	{
		if(badCertificate[currBadCertIdx]=='\n')
		{
			currBadCertIdx++;
		}
		else if( badCertificate[currBadCertIdx]==' ' && (badCertificate[currBadCertIdx-1]==' '|| badCertificate[currBadCertIdx-1]=='\n'))
		{
			currBadCertIdx++;
		}
		else
		{
			cleanCertificate += badCertificate[currBadCertIdx];
			currBadCertIdx++;
		}
	}	

	/*
		If the last character is a space we need to remove it.
	 */
	if(cleanCertificate.substr(cleanCertificate.length()-1,1)==" ")
	{
		cleanCertificate.erase(cleanCertificate.length()-1);
	}
	/*
		Copying the end part(-----) of the end tag in the certificate. 
	 */	
	cleanCertificate += commonTag;
	cleanCertificate += "\n";

	error_code = RS_PEER_CERT_CLEANING_CODE_NO_ERROR ;

	cleanCertificate += peer_info ;

	return true;
}

std::string RsCertificate::toStdString_oldFormat() const
{
	return std::string() ;

	// not supported anymore.
	//
	std::string res ;

	res += PGPKeyManagement::makeArmouredKey(binary_pgp_key,binary_pgp_key_size,pgp_version) ;

	if(only_pgp)
		return res ;

	res += SSLID_BEGIN_SECTION ;
	res += location_id.toStdString(false) ;
	res += ";" ;
	res += LOCATION_BEGIN_SECTION ;
	res += location_name ;
	res += ";\n" ;

	if (hidden_node)
	{
		std::ostringstream os ;
		os << HIDDEN_NODE_BEGIN_SECTION;
		os << hidden_node_address << ";";

		res += os.str() ;
		res += "\n" ;
	}
	else
	{
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
	}


	return res ;
}

bool RsCertificate::initFromString_oldFormat(const std::string& certstr,uint32_t& /*err_code*/)
{
	return false ; // this format is not supported anymore.

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
		only_pgp = true ;

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
				only_pgp = false ;
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

				scan_ip(local_ip,localPort,ipv4_internal_ip_and_port) ;
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
				
				scan_ip(ext_ip,extPort,ipv4_external_ip_and_port) ;
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




