
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
	cPeerId.clear() ;
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
	parent_groups.clear() ;
}

std::ostream &RsFileConfigItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsFileConfigItem", indent);
	uint16_t int_Indent = indent + 2;
	file.print(out, int_Indent);

	printIndent(out, int_Indent); out << "flags: " << flags << std::endl;
	printIndent(out, int_Indent); out << "groups:" ;

	for(std::list<std::string>::const_iterator it(parent_groups.begin());it!=parent_groups.end();++it)
		out << (*it) << " " ;
	out << std::endl;

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
	s += RsPeerId::SIZE_IN_BYTES;
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

	ok &= item->cPeerId.serialise(data, tlvsize, offset) ;

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

	ok &= item->cPeerId.deserialise(data, rssize, offset) ;

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
	s += 4;	// flags

	for(std::list<std::string>::const_iterator it(item->parent_groups.begin());it!=item->parent_groups.end();++it)	// parent groups
		s += GetTlvStringSize(*it); 	

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

	for(std::list<std::string>::const_iterator it(item->parent_groups.begin());ok && it!=item->parent_groups.end();++it)	// parent groups
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GROUPID, *it);

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

	while(offset < rssize)
	{
		std::string tmp ;
		if(GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GROUPID, tmp))
			item->parent_groups.push_back(tmp) ;
		else
			break ;
	}

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
	RsPeerStunItem *psi;
	RsPeerNetItem *pni;
	RsPeerGroupItem *pgi;
	RsPeerServicePermissionItem *pri;

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
	else if (NULL != (pri = dynamic_cast<RsPeerServicePermissionItem *>(i)))
	{
		return sizePermissions(pri);
	}

	return 0;
}

/* serialise the data to the buffer */
bool    RsPeerConfigSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsPeerNetItem *pni;
	RsPeerStunItem *psi;
	RsPeerGroupItem *pgi;
	RsPeerServicePermissionItem *pri;

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
	else if (NULL != (pri = dynamic_cast<RsPeerServicePermissionItem *>(i)))
	{
		return serialisePermissions(pri, data, pktsize);
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

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_PEER_NET:
			return deserialiseNet(data, pktsize);
		case RS_PKT_SUBTYPE_PEER_STUN:
			return deserialiseStun(data, pktsize);
		case RS_PKT_SUBTYPE_PEER_GROUP:
			return deserialiseGroup(data, pktsize);
		case RS_PKT_SUBTYPE_PEER_PERMISSIONS:
			return deserialisePermissions(data, pktsize);
		default:
			return NULL;
	}
	return NULL;
}



/****************************************************************************/

RsPeerNetItem::~RsPeerNetItem()
{
	return;
}

void RsPeerNetItem::clear()
{
	peerId.clear();
        pgpId.clear();
        location.clear();
	netMode = 0;
	vs_disc = 0;
	vs_dht = 0;
	lastContact = 0;

	localAddrV4.TlvClear();
	extAddrV4.TlvClear();
	localAddrV6.TlvClear();
	extAddrV6.TlvClear();

	dyndns.clear();

	localAddrList.TlvClear();
	extAddrList.TlvClear();

	domain_addr.clear();
	domain_port = 0;
}

std::ostream &RsPeerNetItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPeerNetItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
    	out << "PeerId: " << peerId.toStdString() << std::endl; 

        printIndent(out, int_Indent);
        out << "PgpId: " << pgpId.toStdString() << std::endl;

        printIndent(out, int_Indent);
        out << "location: " << location << std::endl;

    	printIndent(out, int_Indent);
	out << "netMode: " << netMode << std::endl;

	printIndent(out, int_Indent);
	out << "vs_disc: " << vs_disc << std::endl;
	
	printIndent(out, int_Indent);
	out << "vs_dht: " << vs_dht << std::endl;
	
	printIndent(out, int_Indent);
	out << "lastContact: " << lastContact << std::endl;

	printIndent(out, int_Indent);
	out << "localAddrV4: " << std::endl;
	localAddrV4.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "extAddrV4: " << std::endl;
	extAddrV4.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "localAddrV6: " << std::endl;
	localAddrV6.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "extAddrV6: " << std::endl;
	extAddrV6.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "DynDNS: " << dyndns << std::endl;

	localAddrList.print(out, int_Indent);
	extAddrList.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "DomainAddr: " << domain_addr;
	out << ":" << domain_port << std::endl;
        printRsItemEnd(out, "RsPeerNetItem", indent);
	return out;
}

/*************************************************************************/

uint32_t RsPeerConfigSerialiser::sizeNet(RsPeerNetItem *i)
{	
	uint32_t s = 8; /* header */
	s += RsPeerId::SIZE_IN_BYTES;
	s += RsPgpId::SIZE_IN_BYTES;
        s += GetTlvStringSize(i->location);
        s += 4; /* netMode */
	s += 2; /* vs_disc */
	s += 2; /* vs_dht */
	s += 4; /* lastContact */

	s += i->localAddrV4.TlvSize(); /* localaddr */ 
	s += i->extAddrV4.TlvSize(); /* remoteaddr */ 
	s += i->localAddrV6.TlvSize(); /* localaddr */ 
	s += i->extAddrV6.TlvSize(); /* remoteaddr */ 

	s += GetTlvStringSize(i->dyndns);

	//add the size of the ip list
	s += i->localAddrList.TlvSize();
	s += i->extAddrList.TlvSize();

	s += GetTlvStringSize(i->domain_addr);
	s += 2; /* domain_port */

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
	ok &= item->peerId.serialise(data, tlvsize, offset);
	ok &= item->pgpId.serialise(data, tlvsize, offset);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_LOCATION, item->location); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->netMode); /* Mandatory */
	ok &= setRawUInt16(data, tlvsize, &offset, item->vs_disc); /* Mandatory */
	ok &= setRawUInt16(data, tlvsize, &offset, item->vs_dht); /* Mandatory */
	ok &= setRawUInt32(data, tlvsize, &offset, item->lastContact); /* Mandatory */
	ok &= item->localAddrV4.SetTlv(data, tlvsize, &offset); 
	ok &= item->extAddrV4.SetTlv(data, tlvsize, &offset);
	ok &= item->localAddrV6.SetTlv(data, tlvsize, &offset); 
	ok &= item->extAddrV6.SetTlv(data, tlvsize, &offset);

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns);

	ok &= item->localAddrList.SetTlv(data, tlvsize, &offset);
	ok &= item->extAddrList.SetTlv(data, tlvsize, &offset);

	// New for V0.6.
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DOMADDR, item->domain_addr);
	ok &= setRawUInt16(data, tlvsize, &offset, item->domain_port); /* Mandatory */

	if(offset != tlvsize)
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialiseNet() Size Error! " << std::endl;
#endif
		ok = false;
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
	ok &= item->peerId.deserialise(data, rssize, offset);
	ok &= item->pgpId.deserialise(data, rssize, offset);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_LOCATION, item->location); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->netMode)); /* Mandatory */
	ok &= getRawUInt16(data, rssize, &offset, &(item->vs_disc)); /* Mandatory */
	ok &= getRawUInt16(data, rssize, &offset, &(item->vs_dht)); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->lastContact)); /* Mandatory */
	
	ok &= item->localAddrV4.GetTlv(data, rssize, &offset); 
	ok &= item->extAddrV4.GetTlv(data, rssize, &offset);
	ok &= item->localAddrV6.GetTlv(data, rssize, &offset); 
	ok &= item->extAddrV6.GetTlv(data, rssize, &offset);

	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns); 
	ok &= item->localAddrList.GetTlv(data, rssize, &offset);
	ok &= item->extAddrList.GetTlv(data, rssize, &offset);

	// New for V0.6.
        ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DOMADDR, item->domain_addr);
	ok &= getRawUInt16(data, rssize, &offset, &(item->domain_port)); /* Mandatory */

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

uint32_t RsPeerConfigSerialiser::sizePeerBandwidthLimits(RsPeerBandwidthLimitsItem *i)
{	
	uint32_t s = 8; /* header */
        s += 4; // number of elements
        s += i->peers.size() * (4 + 4 + RsPgpId::SIZE_IN_BYTES) ;

	return s;
}

bool RsPeerConfigSerialiser::serialisePeerBandwidthLimits(RsPeerBandwidthLimitsItem *item, void *data, uint32_t *size)
{
	uint32_t tlvsize = RsPeerConfigSerialiser::sizePeerBandwidthLimits(item);
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

	ok &= setRawUInt32(data, tlvsize, &offset, item->peers.size()); /* Mandatory */
    
    	for(std::map<RsPgpId,PeerBandwidthLimits>::const_iterator it(item->peers.begin());it!=item->peers.end();++it)
        {
            ok &= it->first.serialise(data,tlvsize,offset);
                    
	    ok &= setRawUInt32(data, tlvsize, &offset, it->second.max_up_rate_kbs); /* Mandatory */
	    ok &= setRawUInt32(data, tlvsize, &offset, it->second.max_dl_rate_kbs); /* Mandatory */
        }

	if(offset != tlvsize)
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialiseNet() Size Error! " << std::endl;
#endif
		ok = false;
	}

	return ok;

}

RsPeerBandwidthLimitsItem *RsPeerConfigSerialiser::deserialisePeerBandwidthLimits(void *data, uint32_t *size)
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
		(RS_PKT_SUBTYPE_PEER_BANDLIMITS != getRsItemSubType(rstype)))
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

	RsPeerBandwidthLimitsItem *item = new RsPeerBandwidthLimitsItem();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
    	uint32_t n ;
	ok &= getRawUInt32(data, rssize, &offset, &n) ;
    
    	for(uint32_t i=0;i<n;++i)
	{
		RsPgpId pgpid ;
		ok &= pgpid.deserialise(data,rssize,offset) ;

		PeerBandwidthLimits p ;
		ok &= getRawUInt32(data, rssize, &offset, &(p.max_up_rate_kbs)); /* Mandatory */
		ok &= getRawUInt32(data, rssize, &offset, &(p.max_dl_rate_kbs)); /* Mandatory */

		item->peers[pgpid] = p ;
	}
        
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

std::ostream &RsPeerBandwidthLimitsItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPeerBandwidthLimitsItem", indent);
	uint16_t int_Indent = indent + 2;

    	for(std::map<RsPgpId,PeerBandwidthLimits>::const_iterator it(peers.begin());it!=peers.end();++it)
        {
		printIndent(out, int_Indent);
        	out << it->first << " : " << it->second.max_up_rate_kbs << " (up) " << it->second.max_dl_rate_kbs << " (dn)" << std::endl;
        }

        printRsItemEnd(out, "RsPeerStunItem", indent);
	return out;
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
	pgpList.ids.clear();
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

    std::set<RsPgpId>::iterator it;
	for (it = pgpList.ids.begin(); it != pgpList.ids.end(); ++it) {
		printIndent(out, int_Indent);
		out << "peerId: " << it->toStdString() << std::endl;
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
	pgpList.ids = groupInfo.peerIds;
}

/* get data from RsGroupInfo to RsPeerGroupItem */
void RsPeerGroupItem::get(RsGroupInfo &groupInfo)
{
	groupInfo.id = id;
	groupInfo.name = name;
	groupInfo.flag = flag;
	groupInfo.peerIds = pgpList.ids;
}

/*************************************************************************/

uint32_t RsPeerConfigSerialiser::sizeGroup(RsPeerGroupItem *i)
{	
	uint32_t s = 8; /* header */
	s += 4; /* version */
	s += GetTlvStringSize(i->id);
	s += GetTlvStringSize(i->name);
	s += 4; /* flag */
	s += i->pgpList.TlvSize();
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
	ok &= item->pgpList.SetTlv(data, tlvsize, &offset);

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
	ok &= item->pgpList.GetTlv(data, rssize, &offset);

	if (offset != rssize)
	{
		/* error */
		delete item;
		return NULL;
	}

	return item;
}

/**************************************************************/

std::ostream& RsPeerServicePermissionItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPeerServicePermissionItem", indent);
	uint16_t int_Indent = indent + 2;

	for(uint32_t i=0;i<pgp_ids.size();++i)
	{
		printIndent(out, int_Indent);
		out << "pgp id: " << pgp_ids[i].toStdString() << ": " << service_flags[i].toUInt32() << std::endl; 
	}
	printRsItemEnd(out, "RsPeerServicePermissionItem", indent);
	return out;
}

uint32_t RsPeerConfigSerialiser::sizePermissions(RsPeerServicePermissionItem *i)
{	
	uint32_t s = 8; /* header */
	s += 4 ; // number of pgp ids in he item.

	for(uint32_t j=0;j<i->pgp_ids.size();++j)
	{
		s += RsPgpId::SIZE_IN_BYTES ;//GetTlvStringSize(i->pgp_ids[j]) ;
		s += 4; /* flag */
	}

	return s;
}

bool RsPeerConfigSerialiser::serialisePermissions(RsPeerServicePermissionItem *item, void *data, uint32_t *size)
{
	uint32_t tlvsize = RsPeerConfigSerialiser::sizePermissions(item);
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
	ok &= setRawUInt32(data, tlvsize, &offset, item->pgp_ids.size());

	for(uint32_t i=0;i<item->pgp_ids.size();++i)
	{
		ok &= item->pgp_ids[i].serialise(data, tlvsize, offset) ;
		ok &= setRawUInt32(data, tlvsize, &offset, item->service_flags[i].toUInt32());
	}

	if(offset != tlvsize)
	{
		ok = false;
		std::cerr << "(EE) Item size ERROR in RsPeerServicePermissionItem!" << std::endl;
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsPeerConfigSerialiser::serialisePermissions() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsPeerServicePermissionItem *RsPeerConfigSerialiser::deserialisePermissions(void *data, uint32_t *size)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION1 != getRsItemVersion(rstype)) ||
		(RS_PKT_CLASS_CONFIG != getRsItemClass(rstype)) ||
		(RS_PKT_TYPE_PEER_CONFIG  != getRsItemType(rstype)) ||
		(RS_PKT_SUBTYPE_PEER_PERMISSIONS != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*size < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	RsPeerServicePermissionItem *item = new RsPeerServicePermissionItem ;
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	uint32_t s;
	ok &= getRawUInt32(data, rssize, &offset, &s);
	item->pgp_ids.resize(s) ;
	item->service_flags.resize(s) ;

	for(uint32_t i=0;i<s;++i)
	{
		uint32_t flags ;
		ok &= item->pgp_ids[i].deserialise(data, rssize, offset) ;
		ok &= getRawUInt32(data, rssize, &offset, &flags);
		
		item->service_flags[i] = ServicePermissionFlags(flags) ;
	}

	if (offset != rssize)
	{
		/* error */
		std::cerr << "(EE) Item size ERROR in RsPeerServicePermissionItem!" << std::endl;
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
	hash.clear() ;
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


	s += item->pid.serial_size();
	s += 2; /* cachetypeid */
	s += 2; /* cachesubid */
	s += GetTlvStringSize(item->path);
	s += GetTlvStringSize(item->name);
	s += item->hash.serial_size();
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

	ok &= item->pid.serialise(data, tlvsize, offset) ;
	ok &= setRawUInt16(data, tlvsize, &offset, item->cachetypeid);
	ok &= setRawUInt16(data, tlvsize, &offset, item->cachesubid);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PATH, item->path);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->name);
	ok &= item->hash.serialise(data, tlvsize, offset) ;
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

	ok &= item->pid.deserialise(data, rssize, offset) ;
	ok &= getRawUInt16(data, rssize, &offset, &(item->cachetypeid));
	ok &= getRawUInt16(data, rssize, &offset, &(item->cachesubid));
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PATH, item->path);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, item->name);
	ok &= item->hash.deserialise(data, rssize, offset) ;
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





