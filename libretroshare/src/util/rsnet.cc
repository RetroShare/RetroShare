/*******************************************************************************
 * libretroshare/src/util: rsnet.cc                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright 2015-2018 Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

//#define DEBUG_SPEC_DNS 1

#include "util/rsdebug.h"
#include "util/rsnet.h"
#include "util/rsthreads.h"
#include "util/rsstring.h"
#include "util/rsmemory.h"

#ifdef WINDOWS_SYS
#else
#include <netdb.h>
#endif

#include <cstring>

/* enforce LITTLE_ENDIAN on Windows */
#ifdef WINDOWS_SYS
	#define BYTE_ORDER  1234
	#define LITTLE_ENDIAN 1234
	#define BIG_ENDIAN  4321
#endif

//https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-2
constexpr uint16_t DNSC_IN    = 1; //Internet (IN)
//https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-4
constexpr uint16_t DNST_A     = 1; //Ipv4 address
constexpr uint16_t DNST_AAAA  =28; //Ipv6 address

///Need to pack as we use sizeof them. (avoid padding)
#pragma pack(1)
//DNS header structure
struct DNS_HEADER
{
	unsigned short id; // identification number

	//Header flags https://www.bind9.net/dns-header-flags
	unsigned char rd :1;    //bit 07 Recursion Desired, indicates if the client means a recursive query
	unsigned char tc :1;    //bit 06 TrunCation, indicates that this message was truncated due to excessive length
	unsigned char aa :1;    //bit 05 Authoritative Answer, in a response, indicates if the DNS server is authoritative for the queried hostname
	unsigned char opcode :4;//bit 01-04 The type can be QUERY (standard query, 0), IQUERY (inverse query, 1), or STATUS (server status request, 2)
	unsigned char qr :1;    //bit 00 Indicates if the message is a query (0) or a reply (1)

	unsigned char rcode :4; //bit 12-15 Response Code can be NOERROR (0), FORMERR (1, Format error), SERVFAIL (2), NXDOMAIN (3, Nonexistent domain), etc.
	unsigned char cd :1;    //bit 11 Checking Disabled    [RFC 4035][RFC 6840][RFC Errata 4927] used by DNSSEC
	unsigned char ad :1;    //bit 10 Authentic Data       [RFC 4035][RFC 6840][RFC Errata 4924] used by DNSSEC
	unsigned char z :1;     //bit 09 Zero, reserved for future use
	unsigned char ra :1;    //bit 08 Recursion Available  [RFC 1035] in a response, indicates if the replying DNS server supports recursion

	unsigned short q_count; // number of question entries
	unsigned short ans_count; // number of answer entries
	unsigned short auth_count; // number of authority resource records entries
	unsigned short add_count; // number of additional resource record entries
};
//// OpCode text https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-5
//static const char *opcodetext[] = { "QUERY",      "IQUERY",     "STATUS",
//                                    "RESERVED3",  "NOTIFY",     "UPDATE",
//                                    "STATEFUL",   "RESERVED7",  "RESERVED8",
//                                    "RESERVED9",  "RESERVED10", "RESERVED11",
//                                    "RESERVED12", "RESERVED13", "RESERVED14",
//                                    "RESERVED15" };
//// RCode text https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-6
//static const char *rcodetext[] = { "NOERROR",    "FORMERR",    "SERVFAIL",
//                                   "NXDOMAIN",   "NOTIMP",     "REFUSED",
//                                   "YXDOMAIN",   "YXRRSET",    "NXRRSET",
//                                   "NOTAUTH",    "NOTZONE",    "DSOTYPENI",
//                                   "RESERVED12", "RESERVED13", "RESERVED14",
//                                   "RESERVED15", "BADVERS",    "BADKEY",
//                                   "BADTIME",    "BADMODE",    "BADNAME",
//                                   "BADALG",     "BADTRUNC",   "BADCOOKIE"};

//Constant sized fields of query structure
//https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml
struct QUESTION
{
	unsigned short qtype;     //https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-4
	unsigned short qclass;    //https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-2
};

//Constant sized fields of the resource record structure
struct RR_DATA
{
    unsigned short rtype;   //https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-4
    unsigned short rclass;  //https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-2
    unsigned int rttl;      //Time To Live is the number of seconds left before the information expires. (32bits integer, so maximum 140 years)
    unsigned short data_len;//Lenght of following data
};
#pragma pack()

#ifndef ntohll
uint64_t ntohll(uint64_t x)
{
#ifdef BYTE_ORDER
        #if BYTE_ORDER == BIG_ENDIAN
                return x;
        #elif BYTE_ORDER == LITTLE_ENDIAN

                uint32_t top = (uint32_t) (x >> 32);
                uint32_t bot = (uint32_t) (0x00000000ffffffffULL & x);

                uint64_t rev = ((uint64_t) ntohl(top)) | (((uint64_t) ntohl(bot)) << 32);

                return rev;
        #else
                #error "ENDIAN determination Failed"
        #endif
#else
        #error "ENDIAN determination Failed (BYTE_ORDER not defined)"
#endif

}
#endif
#ifndef htonll
uint64_t htonll(uint64_t x)
{
        return ntohll(x);
}
#endif

void sockaddr_clear(struct sockaddr_in *addr)
{
        memset(addr, 0, sizeof(struct sockaddr_in));
        addr->sin_family = AF_INET;
}

bool rsGetHostByName(const std::string& hostname, in_addr& returned_addr)
{
	addrinfo hint; memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	addrinfo* info = nullptr;
	int res = getaddrinfo(hostname.c_str(), nullptr, &hint, &info);

	bool ok = true;
	if(res > 0 || !info || !info->ai_addr)
	{
		std::cerr << __PRETTY_FUNCTION__ << "(EE) getaddrinfo returned error "
		          << res << " on string \"" << hostname << "\"" << std::endl;
		returned_addr.s_addr = 0;
		ok = false;
	}
	else
		returned_addr.s_addr = ((sockaddr_in*)info->ai_addr)->sin_addr.s_addr;

	if(info) freeaddrinfo(info);

	return ok;
}

bool rsGetHostByNameSpecDNS(const std::string& servername, const std::string& hostname, std::string& returned_addr, int timeout_s /*= -1*/)
{
#ifdef DEBUG_SPEC_DNS
	RsDbg()<<__PRETTY_FUNCTION__<<" servername="<< servername << " hostname=" << hostname << std::endl;
#endif

	if (strlen(servername.c_str()) > 256)
	{
		RsErr()<<__PRETTY_FUNCTION__<<": servername is too long > 256 chars:"<<servername<<std::endl;
		return false;
	}
	if (strlen(hostname.c_str()) > 256)
	{
		RsErr()<<__PRETTY_FUNCTION__<<": hostname is too long > 256 chars:"<<hostname<<std::endl;
		return false;
	}

	bool isIPV4 = false;
	in_addr dns_server_4; in6_addr dns_server_6;
	if (inet_pton(AF_INET, servername.c_str(), &dns_server_4))
		isIPV4 = true;
	else if (inet_pton(AF_INET6, servername.c_str(), &dns_server_6))
		isIPV4 = false;
	else if (rsGetHostByName(servername, dns_server_4))
		isIPV4 = true;
	else
	{
		RsErr()<<__PRETTY_FUNCTION__<<": servername is on an unknow format: "<<servername<<std::endl;
		return false;
	}

	unsigned char buf[65536];

	struct sockaddr_in dest4;struct sockaddr_in6 dest6;
	if (isIPV4)
	{
		dest4.sin_family = AF_INET;
		dest4.sin_port = htons(53);
		dest4.sin_addr.s_addr = dns_server_4.s_addr;
	} else {
		dest6.sin6_family = AF_INET6;
		dest6.sin6_port = htons(53);
		dest6.sin6_flowinfo = 0;
		dest6.sin6_addr = dns_server_6;
		dest6.sin6_scope_id = 0;
	}

	//Set the DNS structure to standard queries
	struct DNS_HEADER* dns = (struct DNS_HEADER *)&buf;
	dns->id = static_cast<unsigned short>(htons(getpid())); //Transaction Id
	//dns flags = 0x0100 Standard Query
	dns->qr = 0;     //Query/Response: Message is a query
	dns->opcode = 0; //OpCode: Standard query
	dns->aa = 0;     //Authoritative: Server is not an authority for domain
	dns->tc = 0;     //TrunCated: Message is not truncated
	dns->rd = 1;     //Recursion Desired: Do query recursively
	dns->ra = 0;     //Recursion Available: Server cannot do recursive queries
	dns->z = 0;      //Z: reserved
	dns->ad = 0;     //Authentic Data: Answer/authority portion was not authenticated by the server
	dns->cd = 0;     //Checking Disabled: Unacceptable
	dns->rcode = 0;  //Response Code: No error

	dns->q_count = htons(1); //1 Question
	dns->ans_count = 0;      //0 Answer
	dns->auth_count = 0;     //0 Authority RRs
	dns->add_count = 0;      //0 Additional RRs
	size_t curSendSize = sizeof(struct DNS_HEADER);

	//Point to the query server name portion
	unsigned char* qname =static_cast<unsigned char*>(&buf[curSendSize]);
	//First byte is Label Type: https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-10
	qname[0] = 0x04; //One Label with Normal label lower 6 bits is the length of the label
	memcpy(&qname[1],hostname.c_str(),strlen(hostname.c_str()));
	size_t qnameSize = strlen((const char*)qname);
	// Format Hostname like www.google.com to 3www6google3com
	{
		size_t last = qnameSize;
		for(size_t i = qnameSize-1 ; i > 0 ; i--)
			if(qname[i]=='.')
			{
				qname[i]=last-i-1;
				last = i;
			}
	}
	curSendSize += qnameSize +1; //With \0 terminator

	//Point to the query constant portion
	struct QUESTION* qinfo =(struct QUESTION*)&buf[curSendSize];
	qinfo->qtype  = htons(isIPV4 ? DNST_A : DNST_AAAA);  //Type: A / AAAA(Host Address)
	qinfo->qclass = htons(DNSC_IN); //Class: IN
	curSendSize += sizeof(struct QUESTION);

#ifdef DEBUG_SPEC_DNS
	RsDbg()<<__PRETTY_FUNCTION__<< " Sending Packet: " << std::endl << hexDump(buf, curSendSize) << std::endl;
#endif
	int s = isIPV4 ? socket(AF_INET  , SOCK_DGRAM , IPPROTO_UDP)
	               : socket(AF_INET6 , SOCK_DGRAM , IPPROTO_UDP) ; //UDP packet for DNS queries
	if (timeout_s > -1)
	{
#ifdef WINDOWS_SYS
		DWORD timeout = timeout_s * 1000;
		setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
#else
		struct timeval tv;
		tv.tv_sec = timeout_s;
		tv.tv_usec = 0;
		setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#endif
	}
	ssize_t send_size = sendto(s, (char*)buf, curSendSize, 0
	                          ,isIPV4 ? (struct sockaddr*)&dest4
	                                  : (struct sockaddr*)&dest6
	                          ,isIPV4 ? sizeof(dest4)
	                                  : sizeof(dest6)
	                          );
	if( send_size < 0)
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Send Failed with size = " << send_size << std::endl;
		return false;
	}

#ifdef DEBUG_SPEC_DNS
	RsDbg()<<__PRETTY_FUNCTION__<< " Waiting answer..." << std::endl;
#endif
	//****************************************************************************************//
	//---                          Receive the answer                                      ---//
	//****************************************************************************************//
	socklen_t dest_size = static_cast<socklen_t>(sizeof dest4);
	ssize_t rec_size=recvfrom(s,(char*)buf , 65536 , 0 , (struct sockaddr*)&dest4 , &dest_size );
	if(rec_size <= 0)
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Receive Failed"<<std::endl;
		return false;
	}
#ifdef DEBUG_SPEC_DNS
	RsDbg()<<__PRETTY_FUNCTION__<< " Received: " << hexDump(buf, rec_size) << std::endl;
#endif


	if (rec_size< static_cast<ssize_t>(sizeof(struct DNS_HEADER)) )
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Request received too small to get DNSHeader."<<std::endl;
		return false;
	}
#ifdef DEBUG_SPEC_DNS
	//Point to the header portion
	dns = (struct DNS_HEADER*) buf;
	RsDbg()<<__PRETTY_FUNCTION__<<" The response contains : " << std::endl
	       <<ntohs(dns->q_count)    << " Questions." << std::endl
	       <<ntohs(dns->ans_count)  << " Answers." << std::endl
	       <<ntohs(dns->auth_count) << " Authoritative Servers." << std::endl
	       <<ntohs(dns->add_count)  << " Additional records." << std::endl;
#endif
	size_t curRecSize = sizeof(struct DNS_HEADER);


	if (rec_size< static_cast<ssize_t>(curRecSize + 1 + sizeof(struct QUESTION)) )
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Request received too small to get Question return."<<std::endl;
		return false;
	}
	//Point to the query portion
	unsigned char* qnameRecv =static_cast<unsigned char*>(&buf[curRecSize]);
	if (memcmp(qname,qnameRecv,qnameSize + 1 + sizeof(struct QUESTION)) )
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Request received different from that sent."<<std::endl;
		return false;
	}
	curRecSize += qnameSize + 1 + sizeof(struct QUESTION);

	if (rec_size< static_cast<ssize_t>(curRecSize + 2) )
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Request received too small to get Answer return."<<std::endl;
		return false;
	}
	//Point to the Answer portion
	unsigned char* reader = &buf[curRecSize];

	size_t rLabelSize=0;
	if((reader[0]&0xC0) == 0)
	{
		//Normal label lower 6 bits is the length of the label
		rLabelSize=(reader[0]&~(0xC0)*256) + reader[1];
	}
	else if ((reader[0]&0xC0) == 0xC0)
	{
		//Compressed label the lower 6 bits and the 8 bits from next octet form a pointer to the compression target.
		//Don't need to read it, maybe the same as in Query
		rLabelSize=0;
	}
	else
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Answer received with unmanaged label format."<<std::endl;
		return false;
	}
	curRecSize += 2 + rLabelSize;

	if (rec_size< static_cast<ssize_t>(curRecSize + sizeof(struct RR_DATA)) )
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Request received too small to get Data return."<<std::endl;
		return false;
	}
	//Point to the query portion
	struct RR_DATA* rec_data = (struct RR_DATA *)&buf[curRecSize];
	if (rec_data->rtype!=qinfo->qtype)
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Answer's type received different from query sent."<<std::endl;
		return false;
	}
	if (rec_data->rclass!=qinfo->qclass)
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Answer's class received different from query sent."<<std::endl;
		return false;
	}
	curRecSize += sizeof(struct RR_DATA);

	if (rec_size< static_cast<ssize_t>(curRecSize + ntohs(rec_data->data_len)) )
	{
		RsErr()<<__PRETTY_FUNCTION__<<": Request received too small to get Full Data return."<<std::endl;
		return false;
	}

	//Retrieve Address
	if(ntohs(rec_data->data_len)==4)
	{
		if (isIPV4)
		{
			in_addr ipv4Add;
			ipv4Add.s_addr=*(in_addr_t*)&buf[curRecSize];
#ifdef DEBUG_SPEC_DNS
			RsDbg()<<__PRETTY_FUNCTION__<< " Retrieve address: " << rs_inet_ntoa(ipv4Add) << std::endl;
#endif
			returned_addr = rs_inet_ntoa(ipv4Add);
			return true;
		}
	}
	else if(ntohs(rec_data->data_len)==16)
	{
		if (!isIPV4)
		{
			in6_addr ipv6Add;
			ipv6Add =*(in6_addr*)&buf[curRecSize];
#ifdef DEBUG_SPEC_DNS
			RsDbg()<<__PRETTY_FUNCTION__<< " Retrieve address: " << rs_inet_ntoa(ipv6Add) << std::endl;
#endif
			returned_addr = rs_inet_ntoa(ipv6Add);
			return true;
		}
	}

	RsErr()<<__PRETTY_FUNCTION__<< " Retrieve unmanaged data size=" << ntohs(rec_data->data_len) << std::endl;
	return false;
}

bool    isValidNet(const struct in_addr *addr)
{
        // invalid address.
	if((*addr).s_addr == INADDR_NONE)
		return false;
	if((*addr).s_addr == 0)
		return false;
	// should do more tests.
	return true;
}


bool    isLoopbackNet(const struct in_addr *addr)
{
	in_addr_t taddr = ntohl(addr->s_addr);
	return (taddr == (127 << 24 | 1));
}

bool isPrivateNet(const struct in_addr *addr)
{
	in_addr_t taddr = ntohl(addr->s_addr);

	if ( (taddr>>24 == 10) ||                     // 10.0.0.0/8
	     (taddr>>20 == (172<<4 | 16>>4)) ||       // 172.16.0.0/12
	     (taddr>>16 == (192<<8 | 168)) ||         // 192.168.0.0/16
	     (taddr>>16 == (169<<8 | 254)) )          // 169.254.0.0/16
		return true;

	return false;
}

bool isLinkLocalNet(const struct in_addr *addr)
{
	in_addr_t taddr = ntohl(addr->s_addr);
	if ( taddr>>16 == (169<<8 | 254) ) return true;  // 169.254.0.0/16

	return false;
}

bool    isExternalNet(const struct in_addr *addr)
{
	if (!isValidNet(addr))
	{
		return false;
	}
	if (isLoopbackNet(addr))
	{
		return false;
	}
	if (isPrivateNet(addr))
	{
		return false;
	}
	return true;
}

std::ostream &operator<<(std::ostream &out, const struct sockaddr_in &addr)
{
	out << "[" << rs_inet_ntoa(addr.sin_addr) << ":";
	out << htons(addr.sin_port) << "]";
	return out;
}

std::ostream& operator<<(std::ostream& o, const sockaddr_storage& addr)
{ return o << sockaddr_storage_tostring(addr); }

/* thread-safe version of inet_ntoa */

std::string rs_inet_ntoa(struct in_addr in)
{
	std::string str;
	uint8_t *bytes = (uint8_t *) &(in.s_addr);
	rs_sprintf(str, "%u.%u.%u.%u", (int) bytes[0], (int) bytes[1], (int) bytes[2], (int) bytes[3]);
	return str;
}

std::string rs_inet_ntoa(const in6_addr& in)
{
	std::string str;
	rs_sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
	           (int)in.s6_addr[0] , (int)in.s6_addr[1],
	           (int)in.s6_addr[2] , (int)in.s6_addr[3],
	           (int)in.s6_addr[4] , (int)in.s6_addr[5],
	           (int)in.s6_addr[6] , (int)in.s6_addr[7],
	           (int)in.s6_addr[8] , (int)in.s6_addr[9],
	           (int)in.s6_addr[10], (int)in.s6_addr[11],
	           (int)in.s6_addr[12], (int)in.s6_addr[13],
	           (int)in.s6_addr[14], (int)in.s6_addr[15]);
	return str;
}
