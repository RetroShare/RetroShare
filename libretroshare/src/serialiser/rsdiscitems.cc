
/*
 * libretroshare/src/serialiser: rsdiscitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "serialiser/rsbaseserial.h"
#include "serialiser/rsbaseserial.h"

#include "serialiser/rsserviceids.h"
#include "serialiser/rsdiscitems.h"

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"

/***
#define RSSERIAL_DEBUG 1
***/

#define RSSERIAL_DEBUG 1

#include <iostream>

/*************************************************************************/

uint32_t    RsDiscSerialiser::size(RsItem *i)
{
	RsDiscOwnItem  *rdi;
	RsDiscReply *rdr;
	RsDiscIssuer *rds;
	RsDiscVersion *rdv;
        RsDiscHeartbeat *rdt;

	/* do reply first - as it is derived from Item */
	if (NULL != (rdr = dynamic_cast<RsDiscReply *>(i)))
	{
		return sizeReply(rdr);
	}
	else if (NULL != (rds = dynamic_cast<RsDiscIssuer *>(i)))
	{
		return sizeIssuer(rds);
	}
	else if (NULL != (rdi = dynamic_cast<RsDiscOwnItem *>(i)))
	{
		return sizeItem(rdi);
	}
	else if (NULL != (rdv = dynamic_cast<RsDiscVersion *>(i)))
	{
		return sizeVersion(rdv);
	}
        else if (NULL != (rdt = dynamic_cast<RsDiscHeartbeat *>(i)))
        {
                return sizeHeartbeat(rdt);
        }

	return 0;
}

/* serialise the data to the buffer */
bool    RsDiscSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsDiscOwnItem  *rdi;
	RsDiscReply *rdr;
	RsDiscIssuer *rds;
	RsDiscVersion *rdv;
        RsDiscHeartbeat *rdt;

	/* do reply first - as it is derived from Item */
	if (NULL != (rdr = dynamic_cast<RsDiscReply *>(i)))
	{
		return serialiseReply(rdr, data, pktsize);
	}
	else if (NULL != (rds = dynamic_cast<RsDiscIssuer *>(i)))
	{
		return serialiseIssuer(rds, data, pktsize);
	}
	else if (NULL != (rdi = dynamic_cast<RsDiscOwnItem *>(i)))
	{
		return serialiseItem(rdi, data, pktsize);
	}
	else if (NULL != (rdv = dynamic_cast<RsDiscVersion *>(i)))
	{
		return serialiseVersion(rdv, data, pktsize);
	}
        else if (NULL != (rdt = dynamic_cast<RsDiscHeartbeat *>(i)))
        {
                return serialiseHeartbeat(rdt, data, pktsize);
        }

	return false;
}

RsItem *RsDiscSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)))
	{
		std::cerr << "RsDiscSerialiser::deserialise() Wrong Type" << std::endl;
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DISC_REPLY:
			return deserialiseReply(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_DISC_OWN:
			return deserialiseOwnItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_DISC_ISSUER:
			return deserialiseIssuer(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_DISC_VERSION:
			return deserialiseVersion(data, pktsize);
			break;
                case RS_PKT_SUBTYPE_DISC_HEARTBEAT:
                        return deserialiseHeartbeat(data, pktsize);
                        break;
                default:
			return NULL;
			break;
	}
	return NULL;
}

/*************************************************************************/

RsDiscOwnItem::~RsDiscOwnItem()
{
	return;
}

void 	RsDiscOwnItem::clear()
{
	memset(&laddr, 0, sizeof(laddr));
	memset(&saddr, 0, sizeof(laddr));
	contact_tf = 0;
	discFlags = 0;
        ipAddressList.clear();
}

std::ostream &RsDiscOwnItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsDiscOwnItem", indent);
	uint16_t int_Indent = indent + 2;

        printIndent(out, int_Indent);
        out << "Local Address: " << inet_ntoa(laddr.sin_addr);
	out << " Port: " << ntohs(laddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "Server Address: " << inet_ntoa(saddr.sin_addr);
	out << " Port: " << ntohs(saddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "Contact TimeFrame: " << contact_tf;
        out << std::endl;

        printIndent(out, int_Indent);
        out << "DiscFlags:  " << discFlags  << std::endl;

        printIndent(out, int_Indent);
        out << "IpAddressListSize:  " << ipAddressList.size()  << std::endl;

        printIndent(out, int_Indent);
        out << "RsDiscOwnItem::print() IpAddressList: " << std::endl;
        for (std::list<IpAddressTimed>::iterator ipListIt = ipAddressList.begin(); ipListIt!=(ipAddressList.end()); ipListIt++) {
        printIndent(out, int_Indent);
               out << inet_ntoa(ipListIt->ipAddr.sin_addr) << ":" << ntohs(ipListIt->ipAddr.sin_port) << " seenTime : " << ipListIt->seenTime << std::endl;
        }

        printRsItemEnd(out, "RsDiscOwnItem", indent);
        return out;
}


uint32_t    RsDiscSerialiser::sizeItem(RsDiscOwnItem *item)
{
	uint32_t s = 8; /* header */
	s += GetTlvIpAddrPortV4Size(); /* laddr */
	s += GetTlvIpAddrPortV4Size(); /* saddr */
	s += 2; /* contact_tf */
	s += 4; /* discFlags  */
        s += 4; /* ipaddress list size  */

	//add the size of the ip list
	int ipListSize = item->ipAddressList.size();
	s += ipListSize * GetTlvIpAddrPortV4Size();
	s += ipListSize * 8; //size of an uint64

	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialiseItem(RsDiscOwnItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

        /* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset,
					TLV_TYPE_IPV4_LOCAL, &(item->laddr));
        ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset,
					TLV_TYPE_IPV4_REMOTE, &(item->saddr));
	ok &= setRawUInt16(data, tlvsize, &offset, item->contact_tf);
	ok &= setRawUInt32(data, tlvsize, &offset, item->discFlags);

        ok &= setRawUInt32(data, tlvsize, &offset, item->ipAddressList.size());

        //store the ip list
        for (std::list<IpAddressTimed>::iterator ipListIt = item->ipAddressList.begin(); ipListIt != item->ipAddressList.end(); ipListIt++) {
            ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(ipListIt->ipAddr));
            ok &= setRawUInt64(data, tlvsize, &offset, ipListIt->seenTime);
        }

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDiscOwnItem *RsDiscSerialiser::deserialiseOwnItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_OWN != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseItem() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseItem() Not Enough Space" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscOwnItem *item = new RsDiscOwnItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset,
					TLV_TYPE_IPV4_LOCAL, &(item->laddr));
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset,
					TLV_TYPE_IPV4_REMOTE, &(item->saddr));
	ok &= getRawUInt16(data, rssize, &offset, &(item->contact_tf));
	ok &= getRawUInt32(data, rssize, &offset, &(item->discFlags));

        uint32_t listSize;
        ok &= getRawUInt32(data, rssize, &offset, &listSize);

	//get the ip adress list
        int count = 0;
        std::list<IpAddressTimed> ipTimedList;
        while (offset < rssize && count < (int)listSize) {
            count++;
            IpAddressTimed ipTimed;
            sockaddr_clear(&ipTimed.ipAddr);
            ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_REMOTE, &ipTimed.ipAddr);
            if (!ok) { break; }
            uint64_t time = 0;
            ok &= getRawUInt64(data, rssize, &offset, &time);
            if (!ok) { break; }
            ipTimed.seenTime = time;
            ipTimedList.push_back(ipTimed);
        }
        item->ipAddressList = ipTimedList;

	if (offset != rssize)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseItem() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseItem() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}


/*************************************************************************/


RsDiscReply::~RsDiscReply()
{
	return;
}

void 	RsDiscReply::clear()
{
	memset(&currentladdr, 0, sizeof(currentladdr));
	memset(&currentsaddr, 0, sizeof(currentladdr));
	contact_tf = 0;
	discFlags = 0;
	aboutId.clear();
        certGPG.clear();
        ipAddressList.clear();
}

std::ostream &RsDiscReply::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsDiscReply", indent);
	uint16_t int_Indent = indent + 2;

        printIndent(out, int_Indent);
	out << "Local Address: " << inet_ntoa(currentladdr.sin_addr);
	out << " Port: " << ntohs(currentladdr.sin_port) << std::endl;

        printIndent(out, int_Indent);
	out << "Server Address: " << inet_ntoa(currentsaddr.sin_addr);
	out << " Port: " << ntohs(currentsaddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "Contact TimeFrame: " << contact_tf;
        out << std::endl;

        printIndent(out, int_Indent);
        out << "DiscFlags:  " << discFlags  << std::endl;

        printIndent(out, int_Indent);
        out << "AboutId:  " << aboutId  << std::endl;

        printIndent(out, int_Indent);
        out << "certGPG:  " << certGPG  << std::endl;

        printIndent(out, int_Indent);
        out << "IpAddressListSize:  " << ipAddressList.size()  << std::endl;

        printIndent(out, int_Indent);
        out << "RsDiscOwnItem::print() IpAddressList: " << std::endl;
        for (std::list<IpAddressTimed>::iterator ipListIt = ipAddressList.begin(); ipListIt!=(ipAddressList.end()); ipListIt++) {
        printIndent(out, int_Indent);
               out << inet_ntoa(ipListIt->ipAddr.sin_addr) << ":" << ntohs(ipListIt->ipAddr.sin_port) << " seenTime : " << ipListIt->seenTime << std::endl;
        }

        printRsItemEnd(out, "RsDiscReply", indent);
        return out;
}


uint32_t    RsDiscSerialiser::sizeReply(RsDiscReply *item)
{
	uint32_t s = 8; /* header */
	s += GetTlvIpAddrPortV4Size(); /* laddr */
	s += GetTlvIpAddrPortV4Size(); /* saddr */
	s += 2; /* connect_tr */
	s += 4; /* discFlags  */
	s += GetTlvStringSize(item->aboutId);
        s += GetTlvStringSize(item->certGPG);
        s += 4; /* ipaddress list size  */

	//add the size of the ip list
	int ipListSize = item->ipAddressList.size();
	s += ipListSize * GetTlvIpAddrPortV4Size();
	s += ipListSize * 8; //size of an uint64

	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialiseReply(RsDiscReply *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeReply(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::serialiseReply() Header: " << ok << std::endl;
        std::cerr << "RsDiscSerialiser::serialiseReply() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->currentladdr));
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->currentsaddr));
	ok &= setRawUInt16(data, tlvsize, &offset, item->contact_tf);
	ok &= setRawUInt32(data, tlvsize, &offset, item->discFlags);

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->aboutId);

        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_CERT_GPG, item->certGPG);

        ok &= setRawUInt32(data, tlvsize, &offset, item->ipAddressList.size());

        //store the ip list
        std::list<IpAddressTimed>::iterator ipListIt;
        for (ipListIt = item->ipAddressList.begin(); ipListIt!=(item->ipAddressList.end()); ipListIt++) {
	    ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(ipListIt->ipAddr));
	    ok &= setRawUInt64(data, tlvsize, &offset, ipListIt->seenTime);
	}

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::serialiseReply() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDiscReply *RsDiscSerialiser::deserialiseReply(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_REPLY != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseReply() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseReply() pktsize != rssize" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscReply *item = new RsDiscReply();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset,
					TLV_TYPE_IPV4_LOCAL, &(item->currentladdr));
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset,
					TLV_TYPE_IPV4_REMOTE, &(item->currentsaddr));
	ok &= getRawUInt16(data, rssize, &offset, &(item->contact_tf));
	ok &= getRawUInt32(data, rssize, &offset, &(item->discFlags));

	ok &= GetTlvString(data, rssize, &offset,
					TLV_TYPE_STR_PEERID, item->aboutId);

        ok &= GetTlvString(data, rssize, &offset,
                                        TLV_TYPE_STR_CERT_GPG, item->certGPG);

        uint32_t listSize;
        ok &= getRawUInt32(data, rssize, &offset, &listSize);

        //get the ip adress list
        int count = 0;
        std::list<IpAddressTimed> ipTimedList;
        while (offset < rssize && count < (int)listSize) {
            count++;
	    IpAddressTimed ipTimed;
            sockaddr_clear(&ipTimed.ipAddr);
	    ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_REMOTE, &ipTimed.ipAddr);
            if (!ok) { break; }
            uint64_t time = 0;
	    ok &= getRawUInt64(data, rssize, &offset, &time);
            if (!ok) { break; }
            ipTimed.seenTime = time;
	    ipTimedList.push_back(ipTimed);
	}
	item->ipAddressList = ipTimedList;

	if (offset != rssize)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseReply() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseReply() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}



/*************************************************************************/


RsDiscIssuer::~RsDiscIssuer()
{
	return;
}

void 	RsDiscIssuer::clear()
{
	issuerCert = "";
}

std::ostream &RsDiscIssuer::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsDiscIssuer", indent);
	uint16_t int_Indent = indent + 2;

        printIndent(out, int_Indent);
        out << "Cert String:  " << issuerCert  << std::endl;

        printRsItemEnd(out, "RsDiscIssuer", indent);
        return out;
}


uint32_t    RsDiscSerialiser::sizeIssuer(RsDiscIssuer *item)
{
	uint32_t s = 8; /* header */
	s += 4; /* size in RawString() */
	s += item->issuerCert.length();

	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialiseIssuer(RsDiscIssuer *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeIssuer(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::serialiseIssuer() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialiseIssuer() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawString(data, tlvsize, &offset, item->issuerCert);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::serialiseIssuer() Size Error! " << std::endl;
		std::cerr << "Offset: " << offset << " tlvsize: " << tlvsize << std::endl;
#endif
	}

	return ok;
}

RsDiscIssuer *RsDiscSerialiser::deserialiseIssuer(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_ISSUER != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseIssuer() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseIssuer() pktsize != rssize" << std::endl;
		std::cerr << "Pktsize: " << *pktsize << " Rssize: " << rssize << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscIssuer *item = new RsDiscIssuer();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawString(data, rssize, &offset, item->issuerCert);

	if (offset != rssize)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseIssuer() offset != rssize" << std::endl;
		std::cerr << "Offset: " << offset << " Rssize: " << rssize << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseIssuer() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}



/*************************************************************************/


RsDiscVersion::~RsDiscVersion()
{
    return;
}
void RsDiscVersion::clear()
{
	version = "";
}

std::ostream &RsDiscVersion::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsDiscVersion", indent);
	uint16_t int_Indent = indent + 2;

    printIndent(out, int_Indent);
    out << "Version String:  " << version  << std::endl;

    printRsItemEnd(out, "RsDiscVersion", indent);
    return out;
}

uint32_t RsDiscSerialiser::sizeVersion(RsDiscVersion *item)
{
    uint32_t s = 8; /* header */
    s += GetTlvStringSize(item->version);

	return s;
}

/* serialise the data to the buffer */
bool RsDiscSerialiser::serialiseVersion(RsDiscVersion *item, void *data, uint32_t *pktsize)
{
    uint32_t tlvsize = sizeVersion(item);
    uint32_t offset = 0;

    if (*pktsize < tlvsize)
        return false;   /* not enough space */

    *pktsize = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, *pktsize, item->PacketId(), *pktsize);

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsDiscSerialiser::serialiseVersion() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialiseVersion() Size: " << tlvsize << std::endl;
#endif

    /* skip the header */
    offset += 8;

    ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->version);

    if (offset != tlvsize)
    {
        ok = false;
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsDiscSerialiser::serialiseVersion() Size Error! " << std::endl;
		std::cerr << "Offset: " << offset << " tlvsize: " << tlvsize << std::endl;
#endif
    }

    return ok;
}

RsDiscVersion *RsDiscSerialiser::deserialiseVersion(void *data, uint32_t *pktsize)
{
    /* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_VERSION != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseVersion() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseVersion() pktsize != rssize" << std::endl;
		std::cerr << "Pktsize: " << *pktsize << " Rssize: " << rssize << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscVersion *item = new RsDiscVersion();
	item->clear();

	/* skip the header */
	offset += 8;

	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->version);

	if (offset != rssize)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseVersion() offset != rssize" << std::endl;
		std::cerr << "Offset: " << offset << " Rssize: " << rssize << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseVersion() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}


/*************************************************************************/


RsDiscHeartbeat::~RsDiscHeartbeat()
{
    return;
}
void RsDiscHeartbeat::clear()
{
}

std::ostream &RsDiscHeartbeat::print(std::ostream &out, uint16_t indent)
{
    printRsItemBase(out, "RsDiscHeartbeat", indent);
        uint16_t int_Indent = indent + 2;

    printRsItemEnd(out, "RsDiscHeartbeat", indent);
    return out;
}

uint32_t RsDiscSerialiser::sizeHeartbeat(RsDiscHeartbeat *item)
{
    uint32_t s = 8; /* header */

        return s;
}

/* serialise the data to the buffer */
bool RsDiscSerialiser::serialiseHeartbeat(RsDiscHeartbeat *item, void *data, uint32_t *pktsize)
{
    uint32_t tlvsize = sizeHeartbeat(item);
    uint32_t offset = 0;

    if (*pktsize < tlvsize)
        return false;   /* not enough space */

    *pktsize = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, *pktsize, item->PacketId(), *pktsize);

#ifdef RSSERIAL_DEBUG
    std::cerr << "RsDiscSerialiser::serialiseHeartbeat() Header: " << ok << std::endl;
        std::cerr << "RsDiscSerialiser::serialiseHeartbeat() Size: " << tlvsize << std::endl;
#endif

    /* skip the header */
    offset += 8;

    if (offset != tlvsize)
    {
        ok = false;
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsDiscSerialiser::serialiseHeartbeat() Size Error! " << std::endl;
                std::cerr << "Offset: " << offset << " tlvsize: " << tlvsize << std::endl;
#endif
    }

    return ok;
}

RsDiscHeartbeat *RsDiscSerialiser::deserialiseHeartbeat(void *data, uint32_t *pktsize)
{
    /* get the type and size */
        uint32_t rstype = getRsItemId(data);
        uint32_t rssize = getRsItemSize(data);

        uint32_t offset = 0;

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
                (RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
                (RS_PKT_SUBTYPE_DISC_HEARTBEAT != getRsItemSubType(rstype)))
        {
#ifdef RSSERIAL_DEBUG
                std::cerr << "RsDiscSerialiser::deserialiseHeartbeat() Wrong Type" << std::endl;
#endif
                return NULL; /* wrong type */
        }

        if (*pktsize < rssize)    /* check size */
        {
#ifdef RSSERIAL_DEBUG
                std::cerr << "RsDiscSerialiser::deserialiseHeartbeat() pktsize != rssize" << std::endl;
                std::cerr << "Pktsize: " << *pktsize << " Rssize: " << rssize << std::endl;
#endif
                return NULL; /* not enough data */
        }

        /* set the packet length */
        *pktsize = rssize;

        bool ok = true;

        /* ready to load */
        RsDiscHeartbeat *item = new RsDiscHeartbeat();
        item->clear();

        /* skip the header */
        offset += 8;

        if (offset != rssize)
        {
#ifdef RSSERIAL_DEBUG
                std::cerr << "RsDiscSerialiser::deserialiseHeartbeat() offset != rssize" << std::endl;
                std::cerr << "Offset: " << offset << " Rssize: " << rssize << std::endl;
#endif
                /* error */
                delete item;
                return NULL;
        }

        if (!ok)
        {
#ifdef RSSERIAL_DEBUG
                std::cerr << "RsDiscSerialiser::deserialiseHeartbeat() ok = false" << std::endl;
#endif
                delete item;
                return NULL;
        }

        return item;
}


/*************************************************************************/
