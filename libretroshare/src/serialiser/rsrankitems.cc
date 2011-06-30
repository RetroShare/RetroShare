
/*
 * libretroshare/src/serialiser: rsbaseitems.cc
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
#include "serialiser/rsrankitems.h"
#include "serialiser/rstlvbase.h"

#define RSSERIAL_DEBUG 1
#include <iostream>

/*************************************************************************/

void 	RsRankMsg::clear()
{
	rid.clear();
	timestamp = 0;
	title.clear();
	comment.clear();
}

std::ostream &RsRankMsg::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsRankMsg", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "rid: " << rid << std::endl;

        printIndent(out, int_Indent);
        out << "timestamp:  " << timestamp  << std::endl;


        printIndent(out, int_Indent);

	std::string cnv_title(title.begin(), title.end());
        out << "msg:  " << cnv_title  << std::endl;

        printIndent(out, int_Indent);
	std::string cnv_comment(comment.begin(), comment.end());
        out << "comment:  " << cnv_comment  << std::endl;

        printIndent(out, int_Indent);
        out << "score:  " << score  << std::endl;

        printRsItemEnd(out, "RsRankMsg", indent);
        return out;
}

/*************************************************************************/

void 	RsRankLinkMsg::clear()
{
	rid.clear();
	pid.clear();
	timestamp = 0;
	title.clear();
	comment.clear();
	score = 0;
	linktype = 0;
	link.clear();
}

std::ostream &RsRankLinkMsg::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsRankLinkMsg", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "rid: " << rid << std::endl;
        printIndent(out, int_Indent);
        out << "pid: " << pid << std::endl;

        printIndent(out, int_Indent);
        out << "timestamp:  " << timestamp  << std::endl;

        printIndent(out, int_Indent);

	std::string cnv_title(title.begin(), title.end());
        out << "msg:  " << cnv_title  << std::endl;

        printIndent(out, int_Indent);
	std::string cnv_comment(comment.begin(), comment.end());
        out << "comment:  " << cnv_comment  << std::endl;

        printIndent(out, int_Indent);
        out << "score:  " << score  << std::endl;

        printIndent(out, int_Indent);
        out << "linktype:  " << linktype << std::endl;
        printIndent(out, int_Indent);
	std::string cnv_link(link.begin(), link.end());
        out << "link:  " << cnv_link  << std::endl;

        printRsItemEnd(out, "RsRankLinkMsg", indent);
        return out;
}


uint32_t    RsRankSerialiser::sizeLink(RsRankLinkMsg *item)
{
	uint32_t s = 8; /* header */
	s += GetTlvStringSize(item->rid);
	s += GetTlvStringSize(item->pid);
	s += 4; /* timestamp */
	s += GetTlvWideStringSize(item->title);
	s += GetTlvWideStringSize(item->comment);
	s += 4; /* score  */
	s += 4; /* linktype  */
	s += GetTlvWideStringSize(item->link);

	return s;
}

/* serialise the data to the buffer */
bool     RsRankSerialiser::serialiseLink(RsRankLinkMsg *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeLink(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GENID, item->rid);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, item->pid);

	ok &= setRawUInt32(data, tlvsize, &offset, item->timestamp);

	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_TITLE, item->title);
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_COMMENT, item->comment);

	ok &= setRawUInt32(data, tlvsize, &offset, *((uint32_t *) &(item->score)));

	ok &= setRawUInt32(data, tlvsize, &offset, item->linktype);

	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_LINK, item->link);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsRankLinkSerialiser::serialiseLink() Size Error! " << std::endl;
	}

	return ok;
}

RsRankLinkMsg *RsRankSerialiser::deserialiseLink(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_RANK != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_RANK_LINK3 != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsRankLinkMsg *item = new RsRankLinkMsg();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GENID, item->rid);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->pid);
	ok &= getRawUInt32(data, rssize, &offset, &(item->timestamp));
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_TITLE, item->title);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_COMMENT, item->comment);
	ok &= getRawUInt32(data, rssize, &offset, (uint32_t *) &(item->score));
	ok &= getRawUInt32(data, rssize, &offset, &(item->linktype));
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_LINK, item->link);

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


uint32_t    RsRankSerialiser::size(RsItem *item)
{
	return sizeLink((RsRankLinkMsg *) item);
}

bool     RsRankSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseLink((RsRankLinkMsg *) item, data, pktsize);
}

RsItem *RsRankSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseLink(data, pktsize);
}



/*************************************************************************/

