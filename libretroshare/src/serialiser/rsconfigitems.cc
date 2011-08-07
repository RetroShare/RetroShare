
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
#include "retroshare/rspeers.h" // Needed for RsGroupInfo.

/***
 * #define RSSERIAL_DEBUG 		1
 * #define RSSERIAL_ERROR_DEBUG 	1
 ***/

#define RSSERIAL_ERROR_DEBUG 		1

#include <iostream>

// For transition.
RsPeerNetItem *convertToNetItem(RsPeerOldNetItem *old);

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
	s += 4; // chunk_strategy
	s += 4; // flags
	s += 4; // chunk map size
	s += 4*item->compressed_chunk_map._map.size(); // compressed_chunk_map

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

	ok &= setRawUInt32(data, tlvsize, &offset, item->flags);
	ok &= setRawUInt32(data, tlvsize, &offset, item->chunk_strategy);
	ok &= setRawUInt32(data, tlvsize, &offset, item->compressed_chunk_map._map.size());

	for(uint32_t i=0;i<item->compressed_chunk_map._map.size();++i)
		ok &= setRawUInt32(data, tlvsize, &offset, item->compressed_chunk_map._map[i]);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_ERROR_DEBUG
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

	ok &= getRawUInt32(data, rssize, &offset, &(item->flags));
	ok &= getRawUInt32(data, rssize, &offset, &(item->chunk_strategy));
	uint32_t map_size = 0 ;
	ok &= getRawUInt32(data, rssize, &offset, &map_size);

	item->compressed_chunk_map._map.resize(map_size) ;
	for(uint32_t i=0;i<map_size;++i)
		ok &= getRawUInt32(data, rssize, &offset, &(item->compressed_chunk_map._map[i]));

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
#ifdef RSSERIAL_ERROR_DEBUG
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
#ifdef RSSERIAL_ERROR_DEBUG
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
#ifdef RSSERIAL_ERROR_DEBUG
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
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_ERROR_DEBUG
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
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsGeneralConfigSerialiser::deserialiseKeyValueSet() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef RSSERIAL_ERROR_DEBUG
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
	RsPeerOldNetItem *oldpni;
	RsPeerStunItem *psi;
	RsPeerNetItem *pni;
	RsPeerGroupItem *pgi;

	if (NULL != (oldpni = dynamic_cast<RsPeerOldNetItem *>(i)))
	{
		return sizeOldNet(oldpni);
	}
	if (NULL != (pni = dynamic_cast<RsPeerNetItem *>(i)))
	{
		return sizeNet(pni);
	}
	else if (NULL != (psi = dynamic_cast<RsPeerStunItem *>(i)))
	{
		return sizeStun(psi);
	}
	else if (NULL != (pgi = dynamic_cast<RsPeerGroupItem *>(i)))
	{
		return sizeGroup(pgi);
	}

	return 0;
}

/* serialise the data to the buffer */
bool    RsPeerConfigSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsPeerOldNetItem *oldpni;
	RsPeerNetItem *pni;
	RsPeerStunItem *psi;
	RsPeerGroupItem *pgi;

	if (NULL != (oldpni = dynamic_cast<RsPeerOldNetItem *>(i)))
	{
		return serialiseOldNet(oldpni, data, pktsize);
	}
	if (NULL != (pni = dynamic_cast<RsPeerNetItem *>(i)))
	{
		return serialiseNet(pni, data, pktsize);
	}
	else if (NULL != (psi = dynamic_cast<RsPeerStunItem *>(i)))
	{
		return serialiseStun(psi, data, pktsize);
	}
	else if (NULL != (pgi = dynamic_cast<RsPeerGroupItem *>(i)))
	{
		return serialiseGroup(pgi, data, pktsize);
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
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::deserialise() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	RsPeerOldNetItem *old = NULL;
	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_PEER_OLD_NET:
			old = deserialiseOldNet(data, pktsize);
			/* upgrade mechanism */
			return convertToNetItem(old);
		case RS_PKT_SUBTYPE_PEER_NET:
			return deserialiseNet(data, pktsize);
		case RS_PKT_SUBTYPE_PEER_STUN:
			return deserialiseStun(data, pktsize);
		case RS_PKT_SUBTYPE_PEER_GROUP:
			return deserialiseGroup(data, pktsize);
		default:
			return NULL;
	}
	return NULL;
}


/*************************************************************************/

RsPeerOldNetItem::~RsPeerOldNetItem()
{
	return;
}

void RsPeerOldNetItem::clear()
{
	pid.clear();
        gpg_id.clear();
        location.clear();
	netMode = 0;
	visState = 0;
	lastContact = 0;

	sockaddr_clear(&currentlocaladdr);
	sockaddr_clear(&currentremoteaddr);
        dyndns.clear();
}

std::ostream &RsPeerOldNetItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPeerOldNetItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
    	out << "PeerId: " << pid << std::endl; 

        printIndent(out, int_Indent);
        out << "GPGid: " << gpg_id << std::endl;

        printIndent(out, int_Indent);
        out << "location: " << location << std::endl;

    	printIndent(out, int_Indent);
	out << "netMode: " << netMode << std::endl;

	printIndent(out, int_Indent);
	out << "visState: " << visState << std::endl;
	
	printIndent(out, int_Indent);
	out << "lastContact: " << lastContact << std::endl;

	printIndent(out, int_Indent);
	out << "currentlocaladdr: " << rs_inet_ntoa(currentlocaladdr.sin_addr);
	out << ":" << htons(currentlocaladdr.sin_port) << std::endl;

	printIndent(out, int_Indent);
	out << "currentremoteaddr: " << rs_inet_ntoa(currentremoteaddr.sin_addr);
	out << ":" << htons(currentremoteaddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "DynDNS: " << dyndns << std::endl;

        printIndent(out, int_Indent);
        out << "ipAdressList: size : " << ipAddressList.size() << ", adresses : " << std::endl;
        for (std::list<IpAddressTimed>::iterator ipListIt = ipAddressList.begin(); ipListIt!=(ipAddressList.end()); ipListIt++) {
                printIndent(out, int_Indent);
                out << rs_inet_ntoa(ipListIt->ipAddr.sin_addr) << ":" << ntohs(ipListIt->ipAddr.sin_port) << " seenTime : " << ipListIt->seenTime << std::endl;
        }

        printRsItemEnd(out, "RsPeerOldNetItem", indent);
	return out;
}

/*************************************************************************/

uint32_t RsPeerConfigSerialiser::sizeOldNet(RsPeerOldNetItem *i)
{	
	uint32_t s = 8; /* header */
	s += GetTlvStringSize(i->pid); /* peerid */ 
        s += GetTlvStringSize(i->gpg_id);
        s += GetTlvStringSize(i->location);
        s += 4; /* netMode */
	s += 4; /* visState */
	s += 4; /* lastContact */
	s += GetTlvIpAddrPortV4Size(); /* localaddr */ 
	s += GetTlvIpAddrPortV4Size(); /* remoteaddr */ 
        s += GetTlvStringSize(i->dyndns);

	//add the size of the ip list
        int ipListSize = i->ipAddressList.size();
	s += ipListSize * GetTlvIpAddrPortV4Size();
	s += ipListSize * 8; //size of an uint64

	return s;

}

bool RsPeerConfigSerialiser::serialiseOldNet(RsPeerOldNetItem *item, void *data, uint32_t *size)
{
	uint32_t tlvsize = RsPeerConfigSerialiser::sizeOldNet(item);
	uint32_t offset = 0;

#ifdef RSSERIAL_ERROR_DEBUG
	std::cerr << "RsPeerConfigSerialiser::serialiseOldNet() ERROR should never use this function" << std::endl;
#endif

	if(*size < tlvsize)
		return false; /* not enough space */

	*size = tlvsize;

	bool ok = true;

	// serialise header

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsPeerConfigSerialiser::serialiseOldNet() Header: " << ok << std::endl;
	std::cerr << "RsPeerConfigSerialiser::serialiseOldNet() Header test: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->pid); /* Mandatory */
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GPGID, item->gpg_id); /* Mandatory */
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_LOCATION, item->location); /* Mandatory */
        ok &= setRawUInt32(data, tlvsize, &offset, item->netMode); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->visState); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->lastContact); /* Mandatory */
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->currentlocaladdr));
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->currentremoteaddr));
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns);

	//store the ip list
	std::list<IpAddressTimed>::iterator ipListIt;
        for (ipListIt = item->ipAddressList.begin(); ipListIt!=(item->ipAddressList.end()); ipListIt++) {
	    ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(ipListIt->ipAddr));
	    ok &= setRawUInt64(data, tlvsize, &offset, ipListIt->seenTime);
	}

	if(offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialise() Size Error! " << std::endl;
#endif
	}

	return ok;

}

RsPeerOldNetItem *RsPeerConfigSerialiser::deserialiseOldNet(void *data, uint32_t *size)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

#ifdef RSSERIAL_ERROR_DEBUG
	std::cerr << "RsPeerConfigSerialiser::serialiseOldNet() ERROR should never use this function" << std::endl;
#endif

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_PEER_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_PEER_OLD_NET != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	RsPeerOldNetItem *item = new RsPeerOldNetItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->pid); /* Mandatory */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GPGID, item->gpg_id); /* Mandatory */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_LOCATION, item->location); /* Mandatory */
        ok &= getRawUInt32(data, rssize, &offset, &(item->netMode)); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->visState)); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->lastContact)); /* Mandatory */
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->currentlocaladdr));
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->currentremoteaddr));
        //ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns);
        GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns); //use this line for backward compatibility

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

        //if (offset != rssize)
        if (false) //use this line for backward compatibility
	{

		/* error */
		delete item;
		return NULL;
	}

	return item;
}

/****************************************************************************/
RsPeerNetItem *convertToNetItem(RsPeerOldNetItem *old)
{
	RsPeerNetItem *item = new RsPeerNetItem();

	/* copy over data */
	item->pid = old->pid;
        item->gpg_id = old->gpg_id;
        item->location = old->location;
	item->netMode = old->netMode;
	item->visState = old->visState;
	item->lastContact = old->lastContact;

	item->currentlocaladdr = old->currentlocaladdr;
	item->currentremoteaddr = old->currentremoteaddr;
        item->dyndns = old->dyndns;

	std::list<IpAddressTimed>::iterator it;
	for(it = old->ipAddressList.begin(); it != old->ipAddressList.end(); it++)
	{
		RsTlvIpAddressInfo info;
		info.addr = it->ipAddr;
		info.seenTime = it->seenTime;
		info.source = 0;

		item->extAddrList.addrs.push_back(info);
	}

	/* delete old data */
	delete old;

	return item;
}

/****************************************************************************/

RsPeerNetItem::~RsPeerNetItem()
{
	return;
}

void RsPeerNetItem::clear()
{
	pid.clear();
        gpg_id.clear();
        location.clear();
	netMode = 0;
	visState = 0;
	lastContact = 0;

	sockaddr_clear(&currentlocaladdr);
	sockaddr_clear(&currentremoteaddr);
        dyndns.clear();

	localAddrList.TlvClear();
	extAddrList.TlvClear();
}

std::ostream &RsPeerNetItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPeerNetItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
    	out << "PeerId: " << pid << std::endl; 

        printIndent(out, int_Indent);
        out << "GPGid: " << gpg_id << std::endl;

        printIndent(out, int_Indent);
        out << "location: " << location << std::endl;

    	printIndent(out, int_Indent);
	out << "netMode: " << netMode << std::endl;

	printIndent(out, int_Indent);
	out << "visState: " << visState << std::endl;
	
	printIndent(out, int_Indent);
	out << "lastContact: " << lastContact << std::endl;

	printIndent(out, int_Indent);
	out << "currentlocaladdr: " << rs_inet_ntoa(currentlocaladdr.sin_addr);
	out << ":" << htons(currentlocaladdr.sin_port) << std::endl;

	printIndent(out, int_Indent);
	out << "currentremoteaddr: " << rs_inet_ntoa(currentremoteaddr.sin_addr);
	out << ":" << htons(currentremoteaddr.sin_port) << std::endl;

        printIndent(out, int_Indent);
        out << "DynDNS: " << dyndns << std::endl;

	localAddrList.print(out, int_Indent);
	extAddrList.print(out, int_Indent);

        printRsItemEnd(out, "RsPeerNetItem", indent);
	return out;
}

/*************************************************************************/

uint32_t RsPeerConfigSerialiser::sizeNet(RsPeerNetItem *i)
{	
	uint32_t s = 8; /* header */
	s += GetTlvStringSize(i->pid); /* peerid */ 
        s += GetTlvStringSize(i->gpg_id);
        s += GetTlvStringSize(i->location);
        s += 4; /* netMode */
	s += 4; /* visState */
	s += 4; /* lastContact */
	s += GetTlvIpAddrPortV4Size(); /* localaddr */ 
	s += GetTlvIpAddrPortV4Size(); /* remoteaddr */ 
        s += GetTlvStringSize(i->dyndns);

	//add the size of the ip list
	s += i->localAddrList.TlvSize();
	s += i->extAddrList.TlvSize();

	return s;

}

bool RsPeerConfigSerialiser::serialiseNet(RsPeerNetItem *item, void *data, uint32_t *size)
{
	uint32_t tlvsize = RsPeerConfigSerialiser::sizeNet(item);
	uint32_t offset = 0;

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsPeerConfigSerialiser::serialiseNet() tlvsize: " << tlvsize << std::endl;
#endif

	if(*size < tlvsize)
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialiseNet() ERROR not enough space" << std::endl;
#endif
		return false; /* not enough space */
	}

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
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GPGID, item->gpg_id); /* Mandatory */
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_LOCATION, item->location); /* Mandatory */
        ok &= setRawUInt32(data, tlvsize, &offset, item->netMode); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->visState); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->lastContact); /* Mandatory */
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->currentlocaladdr));
	ok &= SetTlvIpAddrPortV4(data, tlvsize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->currentremoteaddr));
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns);

	ok &= item->localAddrList.SetTlv(data, tlvsize, &offset);
	ok &= item->extAddrList.SetTlv(data, tlvsize, &offset);

	if(offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialiseNet() Size Error! " << std::endl;
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


#ifdef RSSERIAL_DEBUG
	std::cerr << "RsPeerConfigSerialiser::deserialiseNet() rssize: " << rssize << std::endl;
#endif

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_PEER_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_PEER_NET != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::deserialiseNet() ERROR Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::deserialiseNet() ERROR not enough data" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	RsPeerNetItem *item = new RsPeerNetItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->pid); /* Mandatory */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GPGID, item->gpg_id); /* Mandatory */
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_LOCATION, item->location); /* Mandatory */
        ok &= getRawUInt32(data, rssize, &offset, &(item->netMode)); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->visState)); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->lastContact)); /* Mandatory */
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_LOCAL, &(item->currentlocaladdr));
	ok &= GetTlvIpAddrPortV4(data, rssize, &offset, TLV_TYPE_IPV4_REMOTE, &(item->currentremoteaddr));
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns); 
	ok &= item->localAddrList.GetTlv(data, rssize, &offset);
	ok &= item->extAddrList.GetTlv(data, rssize, &offset);


        if (offset != rssize)
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::deserialiseNet() ERROR size mismatch" << std::endl;
#endif
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
#ifdef RSSERIAL_ERROR_DEBUG
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

/*************************************************************************/

RsPeerGroupItem::RsPeerGroupItem() : RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_PEER_CONFIG, RS_PKT_SUBTYPE_PEER_GROUP)
{
}

RsPeerGroupItem::~RsPeerGroupItem()
{
}

void RsPeerGroupItem::clear()
{
	id.clear();
	name.clear();
	flag = 0;
	peerIds.clear();
}

std::ostream &RsPeerGroupItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPeerGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "groupId: " << id << std::endl; 

	printIndent(out, int_Indent);
	out << "groupName: " << name << std::endl;

	printIndent(out, int_Indent);
	out << "groupFlag: " << flag << std::endl;

	std::list<std::string>::iterator it;
	for (it = peerIds.begin(); it != peerIds.end(); it++) {
		printIndent(out, int_Indent);
		out << "peerId: " << *it << std::endl;
	}

	printRsItemEnd(out, "RsPeerGroupItem", indent);
	return out;
}

/* set data from RsGroupInfo to RsPeerGroupItem */
void RsPeerGroupItem::set(RsGroupInfo &groupInfo)
{
	id = groupInfo.id;
	name = groupInfo.name;
	flag = groupInfo.flag;
	peerIds = groupInfo.peerIds;
}

/* get data from RsGroupInfo to RsPeerGroupItem */
void RsPeerGroupItem::get(RsGroupInfo &groupInfo)
{
	groupInfo.id = id;
	groupInfo.name = name;
	groupInfo.flag = flag;
	groupInfo.peerIds = peerIds;
}

/*************************************************************************/

uint32_t RsPeerConfigSerialiser::sizeGroup(RsPeerGroupItem *i)
{	
	uint32_t s = 8; /* header */
	s += 4; /* version */
	s += GetTlvStringSize(i->id);
	s += GetTlvStringSize(i->name);
	s += 4; /* flag */

	std::list<std::string>::iterator it;
	for (it = i->peerIds.begin(); it != i->peerIds.end(); it++) {
		s += GetTlvStringSize(*it);
	}

	return s;
}

bool RsPeerConfigSerialiser::serialiseGroup(RsPeerGroupItem *item, void *data, uint32_t *size)
{
	uint32_t tlvsize = RsPeerConfigSerialiser::sizeGroup(item);
	uint32_t offset = 0;

	if(*size < tlvsize)
		return false; /* not enough space */

	*size = tlvsize;

	bool ok = true;

	// serialise header

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsPeerConfigSerialiser::serialiseGroup() Header: " << ok << std::endl;
	std::cerr << "RsPeerConfigSerialiser::serialiseGroup() Header: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, 0);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_KEY, item->id);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->name);
	ok &= setRawUInt32(data, tlvsize, &offset, item->flag);

	std::list<std::string>::iterator it;
	for (it = item->peerIds.begin(); it != item->peerIds.end(); it++) {
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, *it);
	}

	if(offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialiseGroup() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsPeerGroupItem *RsPeerConfigSerialiser::deserialiseGroup(void *data, uint32_t *size)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_PEER_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_PEER_GROUP != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	RsPeerGroupItem *item = new RsPeerGroupItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	uint32_t version;
	ok &= getRawUInt32(data, rssize, &offset, &version);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_KEY, item->id);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, item->name);
	ok &= getRawUInt32(data, rssize, &offset, &(item->flag));

	std::string peerId;
	while (offset != rssize) {
		peerId.erase();

		ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, peerId);

		item->peerIds.push_back(peerId);
	}

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
#ifdef RSSERIAL_ERROR_DEBUG
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





