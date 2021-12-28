/*******************************************************************************
 * libretroshare/src/util: rsdnsutils.cc                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2021 Phenom <retrosharephenom@gmail.com>                          *
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

#include "util/rsnet.h"

#include "util/rsdebug.h"
#include "util/rsthreads.h"
#include "util/rsstring.h"

#ifdef WINDOWS_SYS
#else
#include <netdb.h>
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

bool rsGetHostByNameSpecDNS(const std::string& servername, const std::string& hostname, std::string& returned_addr, int timeout_s /*= -1*/)
{
#ifdef DEBUG_SPEC_DNS
	RS_DBG("servername=", servername, " hostname=", hostname);
#endif

	if (strlen(servername.c_str()) > 256)
	{
		RS_ERR("servername is too long > 256 chars: ", servername);
		return false;
	}
	if (strlen(hostname.c_str()) > 256)
	{
		RS_ERR("hostname is too long > 256 chars: ", hostname);
		return false;
	}

	sockaddr_storage serverAddr;
	bool validServer =   sockaddr_storage_inet_pton(serverAddr, servername)
	                  && sockaddr_storage_isValidNet(serverAddr);
	bool isIPV4 = validServer && sockaddr_storage_ipv6_to_ipv4(serverAddr);

	if (!validServer)
	{
		in_addr in ;
		if (!rsGetHostByName(servername, in))
		{
			RS_ERR("servername is on an unknow format: ", servername);
			return false;
		}

		validServer =   sockaddr_storage_inet_pton(serverAddr, rs_inet_ntoa(in))
		             && sockaddr_storage_isValidNet(serverAddr);
		isIPV4 = validServer && sockaddr_storage_ipv6_to_ipv4(serverAddr);

		if (!validServer)
		{
			RS_ERR("rsGetHostByName return bad answer: ", rs_inet_ntoa(in));
			return false;
		}
	}

	sockaddr_storage_setport( serverAddr, 53);

	//Set the DNS structure to standard queries
	unsigned char buf[65536];
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
	size_t qnameSize = strlen(hostname.c_str()) + 1; //+1 for the leading byte
	memcpy(&qname[1],hostname.c_str(), qnameSize);	//Copy the last 0 too
	// Format Hostname like www.google.com to 3www6google3com
	{
		size_t last = qnameSize;
		for(size_t i = qnameSize-1 ; i > 0 ; i--)
			if(qname[i]=='.')
			{
				size_t labelsize = last-i-1;
				if(labelsize > 63){
					RS_ERR("Labelsize is too long: ", labelsize);
					return false;
				}
				qname[i]=labelsize;
				last = i;
			}
		//First byte
		size_t labelsize = last-1;
		if(labelsize > 63){
			RS_ERR("Labelsize is too long: ", labelsize);
			return false;
		}
		//First byte is Label Type: https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-10
		qname[0] = labelsize; //One Label with Normal label lower 6 bits is the length of the label
	}
	curSendSize += qnameSize +1; //With \0 terminator

	//Point to the query constant portion
	struct QUESTION* qinfo =(struct QUESTION*)&buf[curSendSize];
	qinfo->qtype  = htons(isIPV4 ? DNST_A : DNST_AAAA);  //Type: A / AAAA(Host Address)
	qinfo->qclass = htons(DNSC_IN); //Class: IN
	curSendSize += sizeof(struct QUESTION);

#ifdef DEBUG_SPEC_DNS
	RS_DBG("Sending Packet:\n", hexDump(buf, curSendSize));
#endif
	int s = socket(serverAddr.ss_family , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries
	if(s<0)
	{
		RS_ERR("Could not open socket.");
		return false;
	}

	if (timeout_s > -1)
		rs_setSockTimeout(s, true, timeout_s);

	ssize_t send_size = sendto( s, (char*)buf, curSendSize, 0
	                          , (struct sockaddr*)&serverAddr
	                          , isIPV4 ? sizeof(sockaddr_in)
	                                   : sizeof(sockaddr_in6)
	                          );
	if( send_size < 0)
	{
		RS_ERR("Send Failed with size = ", send_size);
		close(s);
		return false;
	}

#ifdef DEBUG_SPEC_DNS
	RS_DBG("Waiting answer...");
#endif
	//****************************************************************************************//
	//---                          Receive the answer                                      ---//
	//****************************************************************************************//
	socklen_t sa_size = static_cast<socklen_t>(isIPV4 ? sizeof(sockaddr_in)
	                                                  : sizeof(sockaddr_in6)
	                                          );
	ssize_t rec_size=recvfrom( s,(char*)buf , 65536 , 0
	                         , (struct sockaddr*)&serverAddr
	                         , &sa_size
	                         );
	close(s); // No more need of this socket, close it.
	if(rec_size <= 0)
	{
		RS_ERR("Receive Failed");
		return false;
	}
#ifdef DEBUG_SPEC_DNS
	RS_DBG("Received:\n", hexDump(buf, rec_size));
#endif


	if (rec_size< static_cast<ssize_t>(sizeof(struct DNS_HEADER)) )
	{
		RS_ERR("Request received too small to get DNSHeader.");
		return false;
	}
#ifdef DEBUG_SPEC_DNS
	//Point to the header portion
	dns = (struct DNS_HEADER*) buf;
	RS_DBG("The response contains :\n"
	       ,ntohs(dns->q_count)    , " Questions.\n"
	       ,ntohs(dns->ans_count)  , " Answers.\n"
	       ,ntohs(dns->auth_count) , " Authoritative Servers.\n"
	       ,ntohs(dns->add_count)  , " Additional records.");
#endif
	size_t curRecSize = sizeof(struct DNS_HEADER);


	if (rec_size< static_cast<ssize_t>(curRecSize + 1 + sizeof(struct QUESTION)) )
	{
		RS_ERR("Request received too small to get Question return.");
		return false;
	}
	//Point to the query portion
	unsigned char* qnameRecv =static_cast<unsigned char*>(&buf[curRecSize]);
	if (memcmp(qname,qnameRecv,qnameSize + 1 + sizeof(struct QUESTION)) )
	{
		RS_ERR("Request received different from that sent.");
		return false;
	}
	curRecSize += qnameSize + 1 + sizeof(struct QUESTION);

	if (rec_size< static_cast<ssize_t>(curRecSize + 2) )
	{
		RS_ERR("Request received too small to get Answer return.");
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
		RS_ERR("Answer received with unmanaged label format.");
		return false;
	}
	curRecSize += 2 + rLabelSize;

	if (rec_size< static_cast<ssize_t>(curRecSize + sizeof(struct RR_DATA)) )
	{
		RS_ERR("Request received too small to get Data return.");
		return false;
	}
	//Point to the query portion
	struct RR_DATA* rec_data = (struct RR_DATA *)&buf[curRecSize];
	if (rec_data->rtype!=qinfo->qtype)
	{
		RS_ERR("Answer's type received different from query sent.");
		return false;
	}
	if (rec_data->rclass!=qinfo->qclass)
	{
		RS_ERR("Answer's class received different from query sent.");
		return false;
	}
	curRecSize += sizeof(struct RR_DATA);

	if (rec_size< static_cast<ssize_t>(curRecSize + ntohs(rec_data->data_len)) )
	{
		RS_ERR("Request received too small to get Full Data return.");
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
			RS_DBG("Retrieve address: ", rs_inet_ntoa(ipv4Add));
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

			struct sockaddr_storage ss;
			sockaddr_storage_clear(ss);
			sockaddr_in6 addr_ipv6;
			addr_ipv6.sin6_addr = ipv6Add;
			sockaddr_storage_setipv6(ss,&addr_ipv6);

#ifdef DEBUG_SPEC_DNS
			RS_DBG("Retrieve address: ", sockaddr_storage_iptostring(ss).c_str());
#endif
			returned_addr = sockaddr_storage_iptostring(ss);
			return true;
		}
	}

	RS_ERR("Retrieve unmanaged data size=", ntohs(rec_data->data_len));
	return false;
}
