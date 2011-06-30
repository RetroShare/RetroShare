
/*
 * libretroshare/src/serialiser: rsphotoitems.cc
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
#include "serialiser/rsphotoitems.h"
#include "serialiser/rstlvbase.h"

#define RSSERIAL_DEBUG 1
#include <iostream>

/*************************************************************************/

void 	RsPhotoItem::clear()
{
	srcId.clear();
	photoId.clear();
	size = 0;

	name.clear();
	comment.clear();

	location.clear();
	date.clear();

	/* not serialised */
	isAvailable = false;
	path.clear();
}

std::ostream &RsPhotoItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsPhotoItem", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "srcId: " << srcId << std::endl;
        printIndent(out, int_Indent);
        out << "photoId: " << photoId << std::endl;
        printIndent(out, int_Indent);
        out << "size:  " << size  << std::endl;

        printIndent(out, int_Indent);
        out << "name:  " << name  << std::endl;

        printIndent(out, int_Indent);
	std::string cnv_comment(comment.begin(), comment.end());
        out << "msg:  " << cnv_comment << std::endl;

        printIndent(out, int_Indent);
        out << "location:  " << location << std::endl;
        printIndent(out, int_Indent);
        out << "date:  " << date << std::endl;

        printIndent(out, int_Indent);
        out << "(NS) isAvailable:  " << isAvailable << std::endl;
        printIndent(out, int_Indent);
        out << "(NS) path:  " << path << std::endl;

        printRsItemEnd(out, "RsPhotoItem", indent);
        return out;
}

/*************************************************************************/
/*************************************************************************/

void 	RsPhotoShowItem::clear()
{
	showId.clear();
	name.clear();
	comment.clear();

	location.clear();
	date.clear();

	photos.clear();
}

std::ostream &RsPhotoShowItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsPhotoShowItem", indent);
	uint16_t int_Indent = indent + 2;
	uint16_t int_Indent2 = int_Indent + 2;

        printIndent(out, int_Indent);
        out << "showId: " << showId << std::endl;
        printIndent(out, int_Indent);
        out << "name:  " << name  << std::endl;

        printIndent(out, int_Indent);
	std::string cnv_comment(comment.begin(), comment.end());
        out << "msg:  " << cnv_comment << std::endl;

        printIndent(out, int_Indent);
        out << "location:  " << location << std::endl;
        printIndent(out, int_Indent);
        out << "date:  " << date << std::endl;

        printIndent(out, int_Indent);
        out << "Photos in Show: " << photos.size() << std::endl;

	std::list<RsPhotoRefItem>::iterator it;
	for(it = photos.begin(); it != photos.end(); it++)
	{
        	printIndent(out, int_Indent2);
        	out << "PhotoId:  " << it->photoId << std::endl;
        	printIndent(out, int_Indent2 + 2);
		std::string cnv_comment2(it->altComment.begin(), it->altComment.end());
        	out << "AltComment:  " << cnv_comment2 << std::endl;
        	printIndent(out, int_Indent2 + 2);
        	out << "Delta T:  " << it->deltaT << std::endl;
	}

        printRsItemEnd(out, "RsPhotoShowItem", indent);
        return out;
}

RsPhotoRefItem::RsPhotoRefItem()
	:deltaT(0)
{
	return;
}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

/* TODO serialiser */

#if 0

uint32_t    RsPhotoSerialiser::sizeLink(RsPhotoLinkMsg *item)
{
	uint32_t s = 8; /* header */
	s += GetTlvStringSize(item->rid);
	s += 4; /* timestamp */
	s += GetTlvWideStringSize(item->title);
	s += GetTlvWideStringSize(item->comment);
	s += 4; /* linktype  */
	s += GetTlvWideStringSize(item->link);

	return s;
}

/* serialise the data to the buffer */
bool     RsPhotoSerialiser::serialiseLink(RsPhotoLinkMsg *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeLink(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	std::cerr << "RsPhotoLinkSerialiser::serialiseLink() Header: " << ok << std::endl;
	std::cerr << "RsPhotoLinkSerialiser::serialiseLink() Size: " << tlvsize << std::endl;

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GENID, item->rid);
	std::cerr << "RsPhotoLinkSerialiser::serialiseLink() rid: " << ok << std::endl;

	ok &= setRawUInt32(data, tlvsize, &offset, item->timestamp);
	std::cerr << "RsPhotoLinkSerialiser::serialiseLink() timestamp: " << ok << std::endl;

	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_TITLE, item->title);
	std::cerr << "RsPhotoLinkSerialiser::serialiseLink() Title: " << ok << std::endl;
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_COMMENT, item->comment);
	std::cerr << "RsPhotoLinkSerialiser::serialiseLink() Comment: " << ok << std::endl;

	ok &= setRawUInt32(data, tlvsize, &offset, item->linktype);
	std::cerr << "RsPhotoLinkSerialiser::serialiseLink() linktype: " << ok << std::endl;

	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_LINK, item->link);
	std::cerr << "RsPhotoLinkSerialiser::serialiseLink() Link: " << ok << std::endl;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsPhotoLinkSerialiser::serialiseLink() Size Error! " << std::endl;
	}

	return ok;
}

RsPhotoLinkMsg *RsPhotoSerialiser::deserialiseLink(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_RANK != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_RANK_LINK != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsPhotoLinkMsg *item = new RsPhotoLinkMsg();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GENID, item->rid);
	ok &= getRawUInt32(data, rssize, &offset, &(item->timestamp));
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_TITLE, item->title);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_COMMENT, item->comment);
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


uint32_t    RsPhotoSerialiser::size(RsItem *item)
{
	return sizeLink((RsPhotoLinkMsg *) item);
}

bool     RsPhotoSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	return serialiseLink((RsPhotoLinkMsg *) item, data, pktsize);
}

RsItem *RsPhotoSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	return deserialiseLink(data, pktsize);
}


#endif


/*************************************************************************/

