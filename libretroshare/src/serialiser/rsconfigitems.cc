
/*
 * libretroshare/src/serialiser: rsconfigitems.cc
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

