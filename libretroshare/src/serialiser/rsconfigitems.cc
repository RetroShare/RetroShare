
/*
 * libretroshare/src/serialiser: rsconfigitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Chris Parker.
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
#include "serialiser/rsconfigitems.h"

#define RSSERIAL_DEBUG 1
#include <iostream>

/*************************************************************************/

uint32_t    RsFileTransferSerialiser::size(RsItem *i)
{
	RsFileTransfer *rft;

	if (NULL != (rft = dynamic_cast<RsFileTransfer *>(i)))
	{
		return sizeTransfer(rft);
	}
	return 0;
}

/* serialise the data to the buffer */
bool    RsFileTransferSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsFileTransfer *rft;

	if (NULL != (rft = dynamic_cast<RsFileTransfer *>(i)))
	{
		return serialiseTransfer(rft, data, pktsize);
	}
	return false;
}

RsItem *RsFileTransferSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE_CONFIG != getRsItemType(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DEFAULT:
			return deserialiseTransfer(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
	return NULL;
}

/*************************************************************************/

RsFileTransfer::~RsFileTransfer()
{
	return;
}

void 	RsFileTransfer::clear()
{

	file.TlvClear();
	allPeerIds.TlvClear();
	cPeerId = "";
	state = 0;
	in = false;
	transferred = 0;
	crate = 0;
	trate = 0;
	lrate = 0;
	ltransfer = 0;

}

std::ostream &RsFileTransfer::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileTransfer", indent);
	uint16_t int_Indent = indent + 2;
	file.print(out, int_Indent);
	allPeerIds.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "cPeerId: " << cPeerId << std::endl;

        printIndent(out, int_Indent);
        out << "State: " << state << std::endl;
        printIndent(out, int_Indent);
        out << "In/Out: " << in << std::endl;

        printIndent(out, int_Indent);
        out << "Transferred: " << transferred << std::endl;

        printIndent(out, int_Indent);
        out << "crate: " << crate << std::endl;
        printIndent(out, int_Indent);
        out << "trate: " << trate << std::endl;
        printIndent(out, int_Indent);
        out << "lrate: " << lrate << std::endl;
        printIndent(out, int_Indent);
        out << "ltransfer: " << ltransfer << std::endl;

        printRsItemEnd(out, "RsFileTransfer", indent);
        return out;

}


uint32_t    RsFileTransferSerialiser::sizeTransfer(RsFileTransfer *item)
{
	uint32_t s = 8; /* header */
	s += item->file.TlvSize();
	s += item->allPeerIds.TlvSize();
	s += GetTlvStringSize(item->cPeerId);
	s += 2; /* state */
	s += 2; /* in/out */
	s += 8; /* transferred */
	s += 4; /* crate */
	s += 4; /* trate */
	s += 4; /* lrate */
	s += 4; /* ltransfer */

	return s;
}

bool     RsFileTransferSerialiser::serialiseTransfer(RsFileTransfer *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeTransfer(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() Header: " << ok << std::endl;
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->file.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() FileItem: " << ok << std::endl;
	ok &= item->allPeerIds.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() allPeerIds: " << ok << std::endl;

        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->cPeerId);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() cId: " << ok << std::endl;

	ok &= setRawUInt16(data, tlvsize, &offset, item->state);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() State: " << ok << std::endl;
	ok &= setRawUInt16(data, tlvsize, &offset, item->in);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() In/Out: " << ok << std::endl;

	ok &= setRawUInt64(data, tlvsize, &offset, item->transferred);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() Transferred: " << ok << std::endl;

	ok &= setRawUInt32(data, tlvsize, &offset, item->crate);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() crate: " << ok << std::endl;
	ok &= setRawUInt32(data, tlvsize, &offset, item->trate);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() trate: " << ok << std::endl;
	ok &= setRawUInt32(data, tlvsize, &offset, item->lrate);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() lrate: " << ok << std::endl;
	ok &= setRawUInt32(data, tlvsize, &offset, item->ltransfer);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() ltransfer: " << ok << std::endl;


	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileTransferSerialiser::serialiseTransfer() Size Error! " << std::endl;
	}

	return ok;
}

RsFileTransfer *RsFileTransferSerialiser::deserialiseTransfer(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_DEFAULT != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileTransfer *item = new RsFileTransfer();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= item->file.GetTlv(data, rssize, &offset);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() FileItem: " << ok << std::endl;
	ok &= item->allPeerIds.GetTlv(data, rssize, &offset);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() allPeerIds: " << ok << std::endl;

	/* string */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->cPeerId);
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() cId: " << ok << std::endl;

	/* data */
	ok &= getRawUInt16(data, rssize, &offset, &(item->state));
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() State: " << ok << std::endl;
	ok &= getRawUInt16(data, rssize, &offset, &(item->in));
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() In/Out: " << ok << std::endl;
	ok &= getRawUInt64(data, rssize, &offset, &(item->transferred));
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() Transferred: " << ok << std::endl;
	ok &= getRawUInt32(data, rssize, &offset, &(item->crate));
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() crate: " << ok << std::endl;
	ok &= getRawUInt32(data, rssize, &offset, &(item->trate));
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() trate: " << ok << std::endl;
	ok &= getRawUInt32(data, rssize, &offset, &(item->lrate));
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() lrate: " << ok << std::endl;
	ok &= getRawUInt32(data, rssize, &offset, &(item->ltransfer));
	std::cerr << "RsFileTransferSerialiser::serialiseTransfer() ltransfer: " << ok << std::endl;

	if (offset != rssize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}


/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

RsGeneralConfigSerialiser::~RsGeneralConfigSerialiser()
{
	return;
}

uint32_t    RsGeneralConfigSerialiser::size(RsItem *i)
{
	RsConfigKeyValueSet  *kvs;

	if (NULL != (kvs = dynamic_cast<RsConfigKeyValueSet *>(i)))
	{
		return sizeKeyValueSet(kvs);
	}
	else if (NULL != (kvs = dynamic_cast<RsConfigKeyValueSet *>(i)))
	{
		return sizeKeyValueSet(kvs);
	}

	return 0;
}

/* serialise the data to the buffer */
bool    RsGeneralConfigSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsConfigKeyValueSet  *kvs;

	/* do reply first - as it is derived from Item */
	if (NULL != (kvs = dynamic_cast<RsConfigKeyValueSet *>(i)))
	{
		return serialiseKeyValueSet(kvs, data, pktsize);
	}
	else if (NULL != (kvs = dynamic_cast<RsConfigKeyValueSet *>(i)))
	{
		return serialiseKeyValueSet(kvs, data, pktsize);
	}

	return false;
}

RsItem *RsGeneralConfigSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_GENERAL_CONFIG != getRsItemType(rstype)))
	{
		std::cerr << "RsGeneralConfigSerialiser::deserialise() Wrong Type" << std::endl;
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_KEY_VALUE:
			return deserialiseKeyValueSet(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
	return NULL;
}

/*************************************************************************/

RsConfigKeyValueSet::~RsConfigKeyValueSet()
{
	return;
}

void 	RsConfigKeyValueSet::clear()
{
	tlvkvs.pairs.clear();
}

std::ostream &RsConfigKeyValueSet::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsConfigKeyValueSet", indent);
	uint16_t int_Indent = indent + 2;

        tlvkvs.print(out, int_Indent);

        printRsItemEnd(out, "RsConfigKeyValueSet", indent);
        return out;
}


uint32_t    RsGeneralConfigSerialiser::sizeKeyValueSet(RsConfigKeyValueSet *item)
{
	uint32_t s = 8; /* header */
	s += item->tlvkvs.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsGeneralConfigSerialiser::serialiseKeyValueSet(RsConfigKeyValueSet *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeKeyValueSet(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsGeneralConfigSerialiser::serialiseKeyValueSet() Header: " << ok << std::endl;
	std::cerr << "RsGeneralConfigSerialiser::serialiseKeyValueSet() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->tlvkvs.SetTlv(data, tlvsize, &offset);
	std::cerr << "RsGeneralConfigSerialiser::serialiseKeyValueSet() kvs: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGeneralConfigSerialiser::serialiseKeyValueSet() Size Error! " << std::endl;
	}

	return ok;
}

RsConfigKeyValueSet *RsGeneralConfigSerialiser::deserialiseKeyValueSet(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_GENERAL_CONFIG != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_KEY_VALUE != getRsItemSubType(rstype)))
	{
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() Wrong Type" << std::endl;
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() Not Enough Space" << std::endl;
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsConfigKeyValueSet *item = new RsConfigKeyValueSet();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= item->tlvkvs.GetTlv(data, rssize, &offset), 
	std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() kvs: " << ok << std::endl;
	if (offset != rssize)
	{
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() offset != rssize" << std::endl;
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() ok = false" << std::endl;
		delete item;
		return NULL;
	}

	return item;
}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

RsPeerConfigSerialiser::~RsPeerConfigSerialiser()
{
	return;
}

uint32_t    RsPeerConfigSerialiser::size(RsItem *i)
{
	RsPeerNetItem *pni;
	RsPeerStunItem *psi;

	if (NULL != (pni = dynamic_cast<RsPeerNetItem *>(i)))
	{
		return sizeNet(pni);
	}
	else if (NULL != (psi = dynamic_cast<RsPeerStunItem *>(i)))
	{
		return sizeStun(psi);
	}

	return 0;
}

/* serialise the data to the buffer */
bool    RsPeerConfigSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsPeerNetItem *pni;
	RsPeerStunItem *psi;

	if (NULL != (pni = dynamic_cast<RsPeerNetItem *>(i)))
	{
		return serialiseNet(pni, data, pktsize);
	}
	else if (NULL != (psi = dynamic_cast<RsPeerStunItem *>(i)))
	{
		return serialiseStun(psi, data, pktsize);
	}

	return false;
}

RsItem *RsPeerConfigSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_PEER_CONFIG != getRsItemType(rstype)))
	{
		std::cerr << "RsPeerConfigSerialiser::deserialise() Wrong Type" << std::endl;
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_PEER_NET:
			return deserialiseNet(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_PEER_STUN:
			return deserialiseStun(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
	return NULL;
}


/*************************************************************************/

RsPeerNetItem::~RsPeerNetItem()
{
	return;
}

void RsPeerNetItem::clear()
{
	pid.clear();
	netMode = 0;
	visState = 0;
	lastContact = 0;

	sockaddr_clear(&localaddr);
	sockaddr_clear(&remoteaddr);
}

std::ostream &RsPeerNetItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPeerNetItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
    	out << "PeerId: " << pid << std::endl; 

    	printIndent(out, int_Indent);
	out << "netMode: " << netMode << std::endl;

	printIndent(out, int_Indent);
	out << "visState: " << visState << std::endl;
	
	printIndent(out, int_Indent);
	out << "lastContact: " << lastContact << std::endl;

	printIndent(out, int_Indent);
	out << "localaddr: " << inet_ntoa(localaddr.sin_addr);
	out << ":" << htons(localaddr.sin_port) << std::endl;

	printIndent(out, int_Indent);
	out << "remoteaddr: " << inet_ntoa(remoteaddr.sin_addr);
	out << ":" << htons(remoteaddr.sin_port) << std::endl;

        printRsItemEnd(out, "RsPeerNetItem", indent);
	return out;
}

/*************************************************************************/

uint32_t RsPeerConfigSerialiser::sizeNet(RsPeerNetItem *i)
{	
	uint32_t s = 8; /* header */
	s += GetTlvStringSize(i->pid); /* peerid */ 
	s += 4; /* netMode */
	s += 4; /* visState */
	s += 4; /* lastContact */
	s += GetTlvIpAddrPortV4Size(); /* localaddr */ 
	s += GetTlvIpAddrPortV4Size(); /* remoteaddr */ 

	return s;

}

bool RsPeerConfigSerialiser::serialiseNet(RsPeerNetItem *item, void *data, uint32_t *size)
{
	uint32_t tlvsize = RsPeerConfigSerialiser::sizeNet(item);
	uint32_t offset = 0;

	if(*size < tlvsize)
		return false; /* not enough space */

	*size = tlvsize;

	bool ok = true;

	// serialise header

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsPeerConfigSerialiser::serialiseNet() Header: " << ok << std::endl;
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() Header: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->pid); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() pid: " << ok << std::endl;

	ok &= setRawUInt32(data, tlvsize, &offset, item->netMode); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() netMode: " << ok << std::endl; 

	ok &= setRawUInt32(data, tlvsize, &offset, item->visState); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() visState: " << ok << std::endl;

	ok &= setRawUInt32(data, tlvsize, &offset, item->lastContact); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() lastContact: " << ok << std::endl;

        ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->localaddr)); 
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() localaddr: " << ok << std::endl;

        ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->remoteaddr)); 
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() remoteaddr: " << ok << std::endl;

	if(offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsPeerConfigSerialiser::serialise() Size Error! " << std::endl;
	}

	return ok;

}

RsPeerNetItem *RsPeerConfigSerialiser::deserialiseNet(void *data, uint32_t *size)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_PEER_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_PEER_NET != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	RsPeerNetItem *item = new RsPeerNetItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->pid); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::deserialiseNet() pid: " << ok << std::endl;

	ok &= getRawUInt32(data, rssize, &offset, &(item->netMode)); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::deserialiseNet() netMode: " << ok << std::endl; 

	ok &= getRawUInt32(data, rssize, &offset, &(item->visState)); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::deserialiseNet() visState: " << ok << std::endl;

	ok &= getRawUInt32(data, rssize, &offset, &(item->lastContact)); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::deserialiseNet() lastContact: " << ok << std::endl;

        ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->localaddr)); 
	std::cerr << "RsPeerConfigSerialiser::deserialiseNet() localaddr: " << ok << std::endl;

        ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->remoteaddr)); 
	std::cerr << "RsPeerConfigSerialiser::deserialiseNet() remoteaddr: " << ok << std::endl;

	if (offset != rssize)
	{

		/* error */
		delete item;
		return NULL;
	}

	return item;
}

/****************************************************************************/

RsPeerStunItem::~RsPeerStunItem()
{
	return;
}

void RsPeerStunItem::clear()
{
	stunList.TlvClear();
}

std::ostream &RsPeerStunItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPeerStunItem", indent);
	uint16_t int_Indent = indent + 2;

	stunList.printHex(out, int_Indent);

        printRsItemEnd(out, "RsPeerStunItem", indent);
	return out;
}

/*************************************************************************/

uint32_t RsPeerConfigSerialiser::sizeStun(RsPeerStunItem *i)
{	
	uint32_t s = 8; /* header */
	s += i->stunList.TlvSize(); /* stunList */ 

	return s;

}

bool RsPeerConfigSerialiser::serialiseStun(RsPeerStunItem *item, void *data, uint32_t *size)
{
	uint32_t tlvsize = RsPeerConfigSerialiser::sizeStun(item);
	uint32_t offset = 0;

	if(*size < tlvsize)
		return false; /* not enough space */

	*size = tlvsize;

	bool ok = true;

	// serialise header

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsPeerConfigSerialiser::serialiseStun() Header: " << ok << std::endl;
	std::cerr << "RsPeerConfigSerialiser::serialiseStun() Header: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->stunList.SetTlv(data, tlvsize, &offset); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::serialiseStun() stunList: " << ok << std::endl; 

	if(offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsPeerConfigSerialiser::serialiseStun() Size Error! " << std::endl;
	}

	return ok;

}

RsPeerStunItem *RsPeerConfigSerialiser::deserialiseStun(void *data, uint32_t *size)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_PEER_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_PEER_STUN != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	RsPeerStunItem *item = new RsPeerStunItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= item->stunList.GetTlv(data, rssize, &offset); /* Mandatory */
	std::cerr << "RsPeerConfigSerialiser::deserialiseStun() stunList: " << ok << std::endl; 

	if (offset != rssize)
	{

		/* error */
		delete item;
		return NULL;
	}

	return item;
}


/****************************************************************************/


RsCacheConfig::~RsCacheConfig()
{
	return;
}

void RsCacheConfig::clear()
{
	pid.clear();
	cachetypeid = 0;
	cachesubid = 0;
	path = "";
	name = "";
	hash = "";
	size = 0;
	recvd = 0;

}

std::ostream &RsCacheConfig::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsCacheConfig", indent); 
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent); //indent 
	out << "pid: " << pid << std::endl; // display value of peerid

	printIndent(out, int_Indent); //indent 
	out << "cacheid: " << cachetypeid << ":" << cachesubid << std::endl; // display value of cacheid
	
	printIndent(out, int_Indent);
	out << "path: " << path << std::endl; // display value of path

	printIndent(out, int_Indent);
	out << "name: " << name << std::endl; // display value of name

	printIndent(out, int_Indent);
	out << "hash: " << hash << std::endl; // display value of hash

	printIndent(out, int_Indent);
	out << "size: " << size << std::endl; // display value of size

	printIndent(out, int_Indent);
	out << "recvd: " << recvd << std::endl; // display value of recvd

	printRsItemEnd(out, "RsCacheConfig", indent); // end of 'WRITE' check
	return out;
}

/**************************************************************************/


RsCacheConfigSerialiser::~RsCacheConfigSerialiser()
{
	return;
}

uint32_t RsCacheConfigSerialiser::size(RsItem *i)
{
	RsCacheConfig *item = (RsCacheConfig *) i;

	uint32_t s = 8; // to store calculated size, initiailize with size of header


	s += GetTlvStringSize(item->pid);
	s += 2; /* cachetypeid */
	s += 2; /* cachesubid */
	s += GetTlvStringSize(item->path);
	s += GetTlvStringSize(item->name);
	s += GetTlvStringSize(item->hash);
	s += 8; /* size */
	s += 4; /* recvd */

	return s;
}

bool RsCacheConfigSerialiser::serialise(RsItem *i, void *data, uint32_t *size)
{
	RsCacheConfig *item = (RsCacheConfig *) i;
	uint32_t tlvsize = RsCacheConfigSerialiser::size(item);
	uint32_t offset = 0;

	if(*size < tlvsize)
		return false; /* not enough space */

	*size = tlvsize;

	bool ok = true;

	ok &=setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsCacheConfigSerialiser::serialise() Header: " << ok << std::endl;
	std::cerr << "RsCacheConfigSerialiser::serialise() Size: " << size << std::endl;

	/* skip the header */
	offset += 8;
	
	/* add the mandatory parts first */

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->pid);
	std::cerr << "RsCacheConfigSerialiser::serialise() peerid: " << ok << std::endl;

	ok &= setRawUInt16(data, tlvsize, &offset, item->cachetypeid);
	std::cerr << "RsCacheConfigSerialiser::serialise() cacheTypeId: " << ok << std::endl;

	ok &= setRawUInt16(data, tlvsize, &offset, item->cachesubid);
	std::cerr << "RsCacheConfigSerialiser::serialise() cacheSubId: " << ok << std::endl;

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PATH, item->path);
	std::cerr << "RsCacheConfigSerialiser::serialise() path: " << ok << std::endl;

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->name);
	std::cerr << "RsCacheConfigSerialiser::serialise() name: " << ok << std::endl;

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_SHA1, item->hash);
	std::cerr << "RsCacheConfigSerialiser::serialise() hash: " << ok << std::endl;

	ok &= setRawUInt64(data, tlvsize, &offset, item->size);
	std::cerr << "RsCacheConfigSerialiser::serialise() size: " << ok << std::endl;

	ok &= setRawUInt32(data, tlvsize, &offset, item->recvd);
	std::cerr << "RsCacheConfigSerialiser::serialise() recvd: " << ok << std::endl;


	if (offset !=tlvsize)
	{
		ok = false;
		std::cerr << "RsConfigSerialiser::serialisertransfer() Size Error! " << std::endl;
	}

	return ok;
}

RsItem *RsCacheConfigSerialiser::deserialise(void *data, uint32_t *size)
{/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset;
	offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_CACHE_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_DEFAULT != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	/* ready to load */
	RsCacheConfig *item = new RsCacheConfig();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */ 

	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->pid);
	std::cerr << "RsCacheConfigSerialiser::deserialise() peerid: " << ok << std::endl;

	ok &= getRawUInt16(data, rssize, &offset, &(item->cachetypeid));
	std::cerr << "RsCacheConfigSerialiser::serialise() cacheTypeId: " << ok << std::endl;

	ok &= getRawUInt16(data, rssize, &offset, &(item->cachesubid));
	std::cerr << "RsCacheConfigSerialiser::serialise() cacheSubId: " << ok << std::endl;

	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PATH, item->path);
	std::cerr << "RsCacheConfigSerialiser::serialise() path: " << ok << std::endl;

	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, item->name);
	std::cerr << "RsCacheConfigSerialiser::serialise() name: " << ok << std::endl;

	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_HASH_SHA1, item->hash);
	std::cerr << "RsCacheConfigSerialiser::deserialise() hash: " << ok << std::endl;

	ok &= getRawUInt64(data, rssize, &offset, &(item->size));
	std::cerr << "RsCacheConfigSerialiser::deserialise() size: " << ok << std::endl;

	ok &= getRawUInt32(data, rssize, &offset, &(item->recvd));
	std::cerr << "RsCacheConfigSerialiser::deserialise() recvd: " << ok << std::endl;


	if (offset != rssize)
	{

		/* error */
		delete item;
		return NULL;
	}

	return item;
}





