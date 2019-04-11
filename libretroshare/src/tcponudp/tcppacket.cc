/*******************************************************************************
 * libretroshare/src/tcponudp: tcppacket.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "tcppacket.h"

/*
 * #include <arpa/inet.h>
 */

#include "util/rsnet.h" /* for winsock.h -> htons etc */

#include <stdlib.h>
#include <string.h>
#include <util/rsmemory.h>

#include <iostream>

/* NOTE That these BIT #defines will only 
 * work on little endian machines....
 * due to the ntohs ...
 */

/* flags 16bit is:
 *
 * || 0 1 2 3 | 4  5 6 7 || 8 9 | 10 11 12 13 14 15 ||
 * <-  HLen -> <--- unused  ---> <---- flags ------->
 *                                URG   PSH   SYN
 *                                   ACK   RST   FIN
 *
 *
 * So in little endian world.
 * 0 & 1 -> unused...
 * URG -> bit 2 => 0x0004
 * ACK -> bit 3 => 0x0008
 * PSH -> bit 4 => 0x0010
 * RST -> bit 5 => 0x0020
 * SYN -> bit 6 => 0x0040
 * FIN -> bit 7 => 0x0080
 *
 * and second byte 0-3 -> hlen, 4-7 unused.
 */

#define TCP_URG_BIT  0x0004
#define TCP_ACK_BIT  0x0008
#define TCP_PSH_BIT  0x0010
#define TCP_RST_BIT  0x0020
#define TCP_SYN_BIT  0x0040
#define TCP_FIN_BIT  0x0080


TcpPacket::TcpPacket(uint8 *ptr, int size)
	:data(0), datasize(0), seqno(0), ackno(0), hlen_flags(0), 
	 winsize(0), ts(0), retrans(0)
	{
		if (size > 0)
		{
			datasize = size;
			data = (uint8 *) rs_malloc(datasize);
            
            		if(data != NULL)
				memcpy(data, (void *) ptr, size);
		}
		return;
	}

TcpPacket::TcpPacket() /* likely control packet */
	:data(0), datasize(0), seqno(0), ackno(0), hlen_flags(0), 
	 winsize(0), ts(0), retrans(0)
	{
		return;
	}


TcpPacket::~TcpPacket()
	{
		if (data)
			free(data);
	}


int	TcpPacket::writePacket(void *buf, int &size)
{
	if (size < TCP_PSEUDO_HDR_SIZE + datasize)
	{
		size = 0;
		return -1;
	}

	/* byte:  0 => uint16 srcport = 0 */
	*((uint16 *) &(((uint8 *) buf)[0])) = htons(0); 

	/* byte:  2 => uint16 destport = 0 */
	*((uint16 *) &(((uint8 *) buf)[2])) = htons(0); 

	/* byte:  4 => uint32 seqno */
	*((uint32 *) &(((uint8 *) buf)[4])) = htonl(seqno); 

	/* byte:  8 => uint32 ackno */
	*((uint32 *) &(((uint8 *) buf)[8])) = htonl(ackno); 

	/* byte: 12 => uint16 len + flags */
	*((uint16 *) &(((uint8 *) buf)[12])) = htons(hlen_flags); 

	/* byte: 14 => uint16 winsize */
	*((uint16 *) &(((uint8 *) buf)[14])) = htons(winsize); 

	/* byte: 16 => uint16 chksum */
	*((uint16 *) &(((uint8 *) buf)[16])) = htons(0); 

	/* byte: 18 => uint16 urgptr */
	*((uint16 *) &(((uint8 *) buf)[18])) = htons(0); 

	/* total 20 bytes */

	/* now the data */
	memcpy((void *) &(((uint8 *) buf)[20]), data, datasize);

	return size = TCP_PSEUDO_HDR_SIZE + datasize;
}


int	TcpPacket::readPacket(void *buf, int size)
{
	if (size < TCP_PSEUDO_HDR_SIZE)
	{
		std::cerr << "TcpPacket::readPacket() Failed Too Small!";
		std::cerr << std::endl;
		return -1;
	}

	/* byte:  0 => uint16 srcport = 0 *******************
	*((uint16 *) &(((uint8 *) buf)[0])) = htons(0); 
	***********/

	/* byte:  2 => uint16 destport = 0 ******************
	*((uint16 *) &(((uint8 *) buf)[2])) = htons(0); 
	***********/

	/* byte:  4 => uint32 seqno */
	seqno = ntohl(  *((uint32 *) &(((uint8 *) buf)[4])) );

	/* byte:  8 => uint32 ackno */
	ackno = ntohl(  *((uint32 *) &(((uint8 *) buf)[8])) );

	/* byte: 12 => uint16 len + flags */
	hlen_flags = ntohs(  *((uint16 *) &(((uint8 *) buf)[12])) );

	/* byte: 14 => uint16 winsize */
	winsize = ntohs(  *((uint16 *) &(((uint8 *) buf)[14])) );

	/* byte: 16 => uint16 chksum *************************
	*((uint16 *) &(((uint8 *) buf)[16])) = htons(0); 
	***********/

	/* byte: 18 => uint16 urgptr *************************
	*((uint16 *) &(((uint8 *) buf)[18])) = htons(0); 
	***********/

	/* total 20 bytes */

	if (data)
	{
		free(data);
		data = NULL ;
	}
	datasize = size - TCP_PSEUDO_HDR_SIZE;

	// this happens for control packets (e.g. syn/ack/fin)
	if(datasize == 0)
	{
		// data is already NULL
		// just return packet size
		return size;
	}

	data = (uint8 *) rs_malloc(datasize);

	if(data == NULL)
	{
		// malloc failed!
		// return 0 to drop packet (will be retransmitted eventually)
		return 0 ;
	}

	/* now the data */
	memcpy(data, (void *) &(((uint8 *) buf)[20]), datasize);

	return size;
}

	/* flags */
bool	TcpPacket::hasSyn()
{
	return (hlen_flags & TCP_SYN_BIT);
}

bool	TcpPacket::hasFin()
{
	return (hlen_flags & TCP_FIN_BIT);
}

bool	TcpPacket::hasAck()
{
	return (hlen_flags & TCP_ACK_BIT);
}

bool	TcpPacket::hasRst()
{
	return (hlen_flags & TCP_RST_BIT);
}


void    TcpPacket::setSyn()
{
	hlen_flags |= TCP_SYN_BIT;
}

void    TcpPacket::setFin()
{
	hlen_flags |= TCP_FIN_BIT;
}

void    TcpPacket::setRst()
{
	hlen_flags |= TCP_RST_BIT;
}

void    TcpPacket::setAckFlag()
{
	hlen_flags |= TCP_ACK_BIT;
}

void    TcpPacket::setAck(uint32 val)
{
	setAckFlag();
	ackno = val;
}

uint32  TcpPacket::getAck()
{
	return ackno;
}


