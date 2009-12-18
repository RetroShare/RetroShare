
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

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

uint32_t    RsFileConfigSerialiser::size(RsItem *i)
{
	RsFileTransfer *rft;
	RsFileConfigItem *rfi;

	if (NULL != (rft = dynamic_cast<RsFileTransfer *>(i)))
	{
		return sizeTransfer(rft);
	}
	if (NULL != (rfi = dynamic_cast<RsFileConfigItem *>(i)))
	{
		return sizeFileItem(rfi);
	}
	return 0;
}

/* serialise the data to the buffer */
bool    RsFileConfigSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsFileTransfer *rft;
	RsFileConfigItem *rfi;

	if (NULL != (rft = dynamic_cast<RsFileTransfer *>(i)))
	{
		return serialiseTransfer(rft, data, pktsize);
	}
	if (NULL != (rfi = dynamic_cast<RsFileConfigItem *>(i)))
	{
		return serialiseFileItem(rfi, data, pktsize);
	}
	return false;
}

RsItem *RsFileConfigSerialiser::deserialise(void *data, uint32_t *pktsize)
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
		case RS_PKT_SUBTYPE_FILE_TRANSFER:
			return deserialiseTransfer(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_FILE_ITEM:
			return deserialiseFileItem(data, pktsize);
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

/*************************************************************************/
/*************************************************************************/

RsFileConfigItem::~RsFileConfigItem()
{
	return;
}

void 	RsFileConfigItem::clear()
{

	file.TlvClear();
	flags = 0;
}

std::ostream &RsFileConfigItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsFileConfigItem", indent);
	uint16_t int_Indent = indent + 2;
	file.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "flags: " << flags << std::endl;

        printRsItemEnd(out, "RsFileConfigItem", indent);
        return out;

}

/*************************************************************************/
/*************************************************************************/


uint32_t    RsFileConfigSerialiser::sizeTransfer(RsFileTransfer *item)
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
	s += 4; // chunk_size
	s += 4; // chunk_number
	s += 4; // chunk_strategy
	s += 4; // chunk map size
	s += 4*item->chunk_map.size(); // chunk_map

	return s;
}

bool     RsFileConfigSerialiser::serialiseTransfer(RsFileTransfer *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeTransfer(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Header: " << ok << std::endl;
	std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->file.SetTlv(data, tlvsize, &offset);
	ok &= item->allPeerIds.SetTlv(data, tlvsize, &offset);

        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->cPeerId);

	ok &= setRawUInt16(data, tlvsize, &offset, item->state);
	ok &= setRawUInt16(data, tlvsize, &offset, item->in);

	ok &= setRawUInt64(data, tlvsize, &offset, item->transferred);

	ok &= setRawUInt32(data, tlvsize, &offset, item->crate);
	ok &= setRawUInt32(data, tlvsize, &offset, item->trate);
	ok &= setRawUInt32(data, tlvsize, &offset, item->lrate);
	ok &= setRawUInt32(data, tlvsize, &offset, item->ltransfer);

	ok &= setRawUInt32(data, tlvsize, &offset, item->chunk_size);
	ok &= setRawUInt32(data, tlvsize, &offset, item->chunk_number);
	ok &= setRawUInt32(data, tlvsize, &offset, item->chunk_strategy);
	ok &= setRawUInt32(data, tlvsize, &offset, item->chunk_map.size());

	for(uint32_t i=0;i<item->chunk_map.size();++i)
		ok &= setRawUInt32(data, tlvsize, &offset, item->chunk_map[i]);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsFileTransfer *RsFileConfigSerialiser::deserialiseTransfer(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FILE_TRANSFER != getRsItemSubType(rstype)))
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
	ok &= item->allPeerIds.GetTlv(data, rssize, &offset);

	/* string */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->cPeerId);

	/* data */
	ok &= getRawUInt16(data, rssize, &offset, &(item->state));
	ok &= getRawUInt16(data, rssize, &offset, &(item->in));
	ok &= getRawUInt64(data, rssize, &offset, &(item->transferred));
	ok &= getRawUInt32(data, rssize, &offset, &(item->crate));
	ok &= getRawUInt32(data, rssize, &offset, &(item->trate));
	ok &= getRawUInt32(data, rssize, &offset, &(item->lrate));
	ok &= getRawUInt32(data, rssize, &offset, &(item->ltransfer));

	ok &= getRawUInt32(data, rssize, &offset, &(item->chunk_size));
	ok &= getRawUInt32(data, rssize, &offset, &(item->chunk_number));
	ok &= getRawUInt32(data, rssize, &offset, &(item->chunk_strategy));
	uint32_t map_size = 0 ;
	ok &= getRawUInt32(data, rssize, &offset, &map_size);

	item->chunk_map.resize(map_size) ;
	for(uint32_t i=0;i<map_size;++i)
		ok &= getRawUInt32(data, rssize, &offset, &(item->chunk_map[i]));

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


uint32_t    RsFileConfigSerialiser::sizeFileItem(RsFileConfigItem *item)
{
	uint32_t s = 8; /* header */
	s += item->file.TlvSize();
	s += 4;

	return s;
}

bool     RsFileConfigSerialiser::serialiseFileItem(RsFileConfigItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeFileItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileConfigSerialiser::serialiseFileItem() Header: " << ok << std::endl;
	std::cerr << "RsFileConfigSerialiser::serialiseFileItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->file.SetTlv(data, tlvsize, &offset);

	ok &= setRawUInt32(data, tlvsize, &offset, item->flags);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileConfigSerialiser::serialiseFileItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsFileConfigItem *RsFileConfigSerialiser::deserialiseFileItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_FILE_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_FILE_ITEM != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFileConfigItem *item = new RsFileConfigItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= item->file.GetTlv(data, rssize, &offset);
	ok &= getRawUInt32(data, rssize, &offset, &(item->flags));

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
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGeneralConfigSerialiser::deserialise() Wrong Type" << std::endl;
#endif
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

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsGeneralConfigSerialiser::serialiseKeyValueSet() Header: " << ok << std::endl;
	std::cerr << "RsGeneralConfigSerialiser::serialiseKeyValueSet() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->tlvkvs.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGeneralConfigSerialiser::serialiseKeyValueSet() Size Error! " << std::endl;
#endif
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
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() Not Enough Space" << std::endl;
#endif
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
	ok &= item->tlvkvs.GetTlv(data, rssize, &offset);

	if (offset != rssize)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() ok = false" << std::endl;
#endif
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
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsPeerConfigSerialiser::deserialise() Wrong Type" << std::endl;
#endif
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

	sockaddr_clear(&currentlocaladdr);
	sockaddr_clear(&currentremoteaddr);
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
	out << "currentlocaladdr: " << inet_ntoa(currentlocaladdr.sin_addr);
	out << ":" << htons(currentlocaladdr.sin_port) << std::endl;

	printIndent(out, int_Indent);
	out << "currentremoteaddr: " << inet_ntoa(currentremoteaddr.sin_addr);
	out << ":" << htons(currentremoteaddr.sin_port) << std::endl;

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

	//add the size of the ip list
        int ipListSize = i->ipAddressList.size();
	s += ipListSize * GetTlvIpAddrPortV4Size();
	s += ipListSize * 8; //size of an uint64

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

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() Header: " << ok << std::endl;
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() Header test: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->pid); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->netMode); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->visState); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->lastContact); /* Mandatory */
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->currentlocaladdr));
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->currentremoteaddr));

	//store the ip list
	std::list<IpAddressTimed>::iterator ipListIt;
        for (ipListIt = item->ipAddressList.begin(); ipListIt!=(item->ipAddressList.end()); ipListIt++) {
	    ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(ipListIt->ipAddr));
	    ok &= setRawUInt64(data, tlvsize, &offset, ipListIt->seenTime);
	}

	if(offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialise() Size Error! " << std::endl;
#endif
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
	ok &= getRawUInt32(data, rssize, &offset, &(item->netMode)); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->visState)); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->lastContact)); /* Mandatory */
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->currentlocaladdr));
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->currentremoteaddr));

	//get the ip adress list
	std::list<IpAddressTimed> ipTimedList;
	while (offset < rssize) {
	    IpAddressTimed ipTimed;
            sockaddr_clear(&ipTimed.ipAddr);
            ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_REMOTE, &ipTimed.ipAddr);
            if (!ok) { break;}
            uint64_t time = 0;
            ok &= getRawUInt64(data, rssize, &offset, &time);
            if (!ok) { break;}
	    ipTimed.seenTime = time;
	    ipTimedList.push_back(ipTimed);
	}
        item->ipAddressList = ipTimedList;

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

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsPeerConfigSerialiser::serialiseStun() Header: " << ok << std::endl;
	std::cerr << "RsPeerConfigSerialiser::serialiseStun() Header: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->stunList.SetTlv(data, tlvsize, &offset); /* Mandatory */

	if(offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialiseStun() Size Error! " << std::endl;
#endif
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

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsCacheConfigSerialiser::serialise() Header: " << ok << std::endl;
	std::cerr << "RsCacheConfigSerialiser::serialise() Size: " << size << std::endl;
#endif

	/* skip the header */
	offset += 8;
	
	/* add the mandatory parts first */

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->pid);
	ok &= setRawUInt16(data, tlvsize, &offset, item->cachetypeid);
	ok &= setRawUInt16(data, tlvsize, &offset, item->cachesubid);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PATH, item->path);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->name);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_SHA1, item->hash);
	ok &= setRawUInt64(data, tlvsize, &offset, item->size);
	ok &= setRawUInt32(data, tlvsize, &offset, item->recvd);

	if (offset !=tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsConfigSerialiser::serialisertransfer() Size Error! " << std::endl;
#endif
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
	ok &= getRawUInt16(data, rssize, &offset, &(item->cachetypeid));
	ok &= getRawUInt16(data, rssize, &offset, &(item->cachesubid));
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PATH, item->path);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, item->name);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_HASH_SHA1, item->hash);
	ok &= getRawUInt64(data, rssize, &offset, &(item->size));
	ok &= getRawUInt32(data, rssize, &offset, &(item->recvd));


	if (offset != rssize)
	{

		/* error */
		delete item;
		return NULL;
	}

	return item;
}





