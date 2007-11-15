/*
 * "$Id: discItem.cc,v 1.8 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */




#include "pqi/discItem.h"

static const int pqidizone = 44863;

#include "pqi/pqidebug.h"
#include <sstream>

PQTunnel *createDiscItems(void *d, int n)
{
	pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  "createDiscItems()");

        // check the discType of DiscItem;
	if ( ((int *) d)[0] == 0)
	{
		return new DiscItem();
	}
	return new DiscReplyItem();
}

DiscItem::DiscItem()
	:PQTunnel(PQI_TUNNEL_DISC_ITEM_TYPE), discType(0), discFlags(0) 
{ 
	pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  "DiscItem::DiscItem()");

	return; 
}

DiscItem *DiscItem::clone()
{
	pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  "DiscItem::clone()");

	DiscItem *di = new DiscItem();
	di -> copy(this);
	return di;
}

void 	DiscItem::copy(const DiscItem *di)
{
	pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  "DiscItem::copy()");

	PQTunnel::copy(di);

	discType = di -> discType;
	saddr = di -> saddr;
	laddr = di -> laddr;
	receive_tr = di -> receive_tr;
	connect_tr = di -> connect_tr;
	discFlags  = di -> discFlags;
}
        // Overloaded from PQTunnel.
const int DiscItem::getSize() const
{
	return 3 * 4 + 2 * sizeof(struct sockaddr_in);
}

int DiscItem::out(void *data, const int size) const
{
	{
	  std::ostringstream out;
	  out << "DiscItem::out() Data: " << data;
	  out << " Size: " << size << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidizone, out.str());
	}

	if (size < DiscItem::getSize())
	{
		pqioutput(PQL_DEBUG_BASIC, pqidizone, 
		  "DiscItem::out() Packet Too Small!");
		return -1;
	}

	char *dta = (char *) data;
	((int *) dta)[0] = discType;
	dta += 4;
	((struct sockaddr_in *) dta)[0] = saddr;
	dta += sizeof(sockaddr_in);
	((struct sockaddr_in *) dta)[0] = laddr;
	dta += sizeof(sockaddr_in);

	// version 1.... 
	// : 4 bytes for receive_tr
	// : 4 bytes for connect_tr
	//((int *) dta)[0] = receive_tr;
	//dta += 4;
	//((int *) dta)[0] = connect_tr;
	//dta += 4;

	// version 2....
	// : 2 bytes for receive_tf
	// : 2 bytes for connect_tf
	// : 4 bytes for cert->flags.
	((unsigned short *) dta)[0] = htons(receive_tr);
	dta += 2;
	((unsigned short *) dta)[0] = htons(connect_tr);
	dta += 2;
	((unsigned long *)  dta)[0] = htonl(discFlags);
	dta += 4;

	return DiscItem::getSize();
}

int DiscItem::in(const void *data, const int size)
{
	{
	  std::ostringstream out;
	  out << "DiscItem::in() Data: " << data;
	  out << " Size: " << size << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidizone, out.str());
	}

	if (size < DiscItem::getSize())
	{
	  std::ostringstream out;
	  out << "DiscItem::in() Pkt to small";
	  pqioutput(PQL_DEBUG_BASIC, pqidizone, out.str());
	  	return -1;
	}

	char *dta = (char *) data;
	discType = ((int *) dta)[0];
	dta += 4;
	saddr = ((struct sockaddr_in *) dta)[0];
	dta += sizeof(sockaddr_in);
	laddr = ((struct sockaddr_in *) dta)[0];
	dta += sizeof(sockaddr_in);

	// version 1.... 
	// : 4 bytes for receive_tr
	// : 4 bytes for connect_tr
	//receive_tf = ((int *) dta)[0];
	//dta += 4;
	//connect_tf = ((int *) dta)[0];
	//dta += 4;

	// version 2....
	// : 2 bytes for receive_tr
	// : 2 bytes for connect_tr
	// : 4 bytes for cert->flags.
	receive_tr = ntohs(((unsigned short *) dta)[0]);
	dta += 2;
	connect_tr = ntohs(((unsigned short *) dta)[0]);
	dta += 2;
	discFlags  = ntohl(((unsigned long *)  dta)[0]);
	dta += 4;

	return getSize();
}

std::ostream &DiscItem::print(std::ostream &out)
{
	out << "-------- DiscItem" << std::endl;
	PQItem::print(out);

	out << "Peer Details" << std::endl;
	out << "Local Address: " << inet_ntoa(laddr.sin_addr);
	out << " Port: " << ntohs(laddr.sin_port) << std::endl;

	out << "Server Address: " << inet_ntoa(saddr.sin_addr);
	out << " Port: " << ntohs(saddr.sin_port) << std::endl;
	out << "[R/C]_TF: " << receive_tr;
	out << "/" << connect_tr << std::endl;
	out << "discFlags: " << discFlags << std::endl;

	return out << "--------" << std::endl;
}


DiscReplyItem::DiscReplyItem()
	:DiscItem(), certDER(NULL), certLen(0)
{ 
	pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  "DiscReplyItem::DiscReplyItem()");

	discType = 1;
	return; 
}

DiscReplyItem::~DiscReplyItem()
{
	pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  "DiscReplyItem::~DiscReplyItem()");

	if (certDER != NULL)
	{
		free(certDER);
		certDER = NULL;
		certLen = 0;
	}
}

DiscReplyItem *DiscReplyItem::clone()
{
	pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  "DiscReplyItem::clone()");

	DiscReplyItem *di = new DiscReplyItem();
	di -> copy(this);
	return di;
}

void 	DiscReplyItem::copy(const DiscReplyItem *di)
{
	pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  "DiscReplyItem::copy()");

	DiscItem::copy(di);


	if (di -> certDER != NULL)
	{
		certLen = di -> certLen;
		certDER = (unsigned char *) malloc(certLen);
		memcpy(certDER, di -> certDER, certLen);
	}
	else
	{
		certLen = 0;
		certDER = NULL;
	}

	return;
}

        // Overloaded from PQTunnel.
const int DiscReplyItem::getSize() const
{
	return DiscItem::getSize() + 4 + certLen;
}

int DiscReplyItem::out(void *data, const int maxsize) const
{
	{
	  std::ostringstream out;
	  out << "DiscReplyItem::out() Data: " << data;
	  out << " Size: " << maxsize << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidizone, out.str());
	}

	if (maxsize < getSize())
	{
	  	std::ostringstream out;
	  	out << "DiscReplyItem::out() Not Enough space";
	  	pqioutput(PQL_DEBUG_BASIC, pqidizone, out.str());

		return -1;
	}

	int basesize = DiscItem::getSize();
	// put out the DiscItem Part.
	DiscItem::out(data, basesize);
	char *loc = ((char *) data) + basesize;
	((int *) loc)[0] = certLen;
	loc += 4;
	memcpy(loc, certDER, certLen);
	return getSize();
}

int DiscReplyItem::in(const void *data, const int size)
{
	{
	  std::ostringstream out;
	  out << "DiscReplyItem::in() Data: " << data;
	  out << " Size: " << size << std::endl;
	  pqioutput(PQL_DEBUG_BASIC, pqidizone, out.str());
	}

	// harder.
	int basesize = DiscItem::getSize();
	if (basesize + 4 > size)
	{
		pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  	"DiscReplyItem::in() Not enough space for Base Class!");
		return -1; // cant be us.
	}

	// read in the DiscItem part.
	DiscItem::in(data, DiscItem::getSize());
	char *loc = ((char *) data) + basesize;
	int certsize = ((int *) loc)[0];
	// check the size again.
	if (basesize + 4 + certsize != size)
	{
		pqioutput(PQL_DEBUG_BASIC, pqidizone, 
	  	"DiscReplyItem::in() Not enough space for Certificate");
		return -1;
	}
	loc += 4;
	// allocate memory for certDER....
	if ((certLen != certsize) && (!certLen))
	{
		if (certDER)
			free(certDER);
	}
	certLen = certsize;
	if (!certDER)
	{
		certDER = (unsigned char *) malloc(certsize);
	}

	memcpy(certDER, loc, certLen);
	return getSize();
}

std::ostream &DiscReplyItem::print(std::ostream &out)
{
	out << "---------------- DiscReplyItem" << std::endl;
	out << "A Friend of a Friend:" << std::endl;
	DiscItem::print(out);

	if (certDER != NULL)
	{
		out << "CertLen: " << certLen << std::endl;
	}
	else
	{
		out << "No Certificate" << std::endl;
	}

	return out << "----------------" << std::endl;
}

