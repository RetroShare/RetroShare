
/*
 * libretroshare/src/gxs: rsopsteditems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012-2013 by Robert Fernie, Christopher Evi-Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "serialiser/rsposteditems.h"
#include "rsbaseserial.h"
#include "rstlvbase.h"


uint32_t RsGxsPostedSerialiser::size(RsItem *item)
{
#ifdef POSTED_DEBUG
	std::cerr << "RsGxsPostedSerialiser::size()" << std::endl;
#endif

	RsGxsPostedGroupItem* pgItem = NULL;
	RsGxsPostedPostItem* ppItem = NULL;

	if ((pgItem = dynamic_cast<RsGxsPostedGroupItem*>(item)) != NULL)
	{
		return sizeGxsPostedGroupItem(pgItem);
	}
	else if ((ppItem = dynamic_cast<RsGxsPostedPostItem*>(item)) != NULL)
	{
		return sizeGxsPostedPostItem(ppItem);
	}
	else
	{
		return RsGxsCommentSerialiser::size(item);
	}

	return NULL;
}

bool RsGxsPostedSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
#ifdef POSTED_DEBUG
	std::cerr << "RsGxsPostedSerialiser::serialise()" << std::endl;
#endif

	RsGxsPostedPostItem* ppItem = NULL;
	RsGxsPostedGroupItem* pgItem = NULL;

	if ((pgItem = dynamic_cast<RsGxsPostedGroupItem*>(item)) != NULL)
	{
		return serialiseGxsPostedGroupItem(pgItem, data, size);
	}
	else if ((ppItem = dynamic_cast<RsGxsPostedPostItem*>(item)) != NULL)
	{
		return serialiseGxsPostedPostItem(ppItem, data, size);
	}
	else
	{
		return RsGxsCommentSerialiser::serialise(item, data, size);
	}
	return false;
}

RsItem* RsGxsPostedSerialiser::deserialise(void *data, uint32_t *size)
{
#ifdef POSTED_DEBUG
	std::cerr << "RsGxsPostedSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
			(RS_SERVICE_GXSV2_TYPE_POSTED != getRsItemService(rstype)))
	{
			std::cerr << "RsGxsPostedSerialiser::deserialise() ERROR Wrong Type";
			std::cerr << std::endl;
			return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{

		case RS_PKT_SUBTYPE_POSTED_GRP_ITEM:
			return deserialiseGxsPostedGroupItem(data, size);
			break;
		case RS_PKT_SUBTYPE_POSTED_POST_ITEM:
			return deserialiseGxsPostedPostItem(data, size);
			break;
		default:
			return RsGxsCommentSerialiser::deserialise(data, size);
			break;
	}
	return NULL;
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

void RsGxsPostedPostItem::clear()
{
	mPost.mLink.clear();
	mPost.mNotes.clear();
}

std::ostream & RsGxsPostedPostItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsGxsPostedPostItem", indent);
        uint16_t int_Indent = indent + 2;

        printIndent(out, int_Indent);
        out << "Link: " << mPost.mLink << std::endl;
        printIndent(out, int_Indent);
        out << "Notes: " << mPost.mNotes << std::endl;

        printRsItemEnd(out ,"RsGxsPostedPostItem", indent);
        return out;
}

uint32_t RsGxsPostedSerialiser::sizeGxsPostedPostItem(RsGxsPostedPostItem* item)
{
	RsPostedPost& p = item->mPost;

	uint32_t s = 8;

	s += GetTlvStringSize(p.mLink);
	s += GetTlvStringSize(p.mNotes);

	return s;
}

bool RsGxsPostedSerialiser::serialiseGxsPostedPostItem(RsGxsPostedPostItem* item, void* data, uint32_t *size)
{

#ifdef POSTED_DEBUG
	std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedPostItem()" << std::endl;
#endif

	uint32_t tlvsize = sizeGxsPostedPostItem(item);
	uint32_t offset = 0;

	if(*size < tlvsize){
		std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedPostItem() Size too small" << std::endl;
		return false;
	}

	*size = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* RsPostedPost */

	ok &= SetTlvString(data, tlvsize, &offset, 1, item->mPost.mLink);
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->mPost.mNotes);

	if(offset != tlvsize)
	{
		std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedPostItem() FAIL Size Error! " << std::endl;
		ok = false;
	}

#ifdef POSTED_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedPostItem() NOK" << std::endl;
	}
#endif

	return ok;
}




RsGxsPostedPostItem* RsGxsPostedSerialiser::deserialiseGxsPostedPostItem(void *data, uint32_t *size)
{

#ifdef POSTED_DEBUG
	std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
			(RS_SERVICE_GXSV2_TYPE_POSTED != getRsItemService(rstype)) ||
			(RS_PKT_SUBTYPE_POSTED_POST_ITEM != getRsItemSubType(rstype)))
	{
			std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem() FAIL wrong type" << std::endl;
			return NULL; /* wrong type */
	}

	if (*size < rssize)	/* check size */
	{
			std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem() FAIL wrong size" << std::endl;
			return NULL; /* not enough data */
	}

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	RsGxsPostedPostItem* item = new RsGxsPostedPostItem();
	/* skip the header */
	offset += 8;

	ok &= GetTlvString(data, rssize, &offset, 1, item->mPost.mLink);
	ok &= GetTlvString(data, rssize, &offset, 1, item->mPost.mNotes);

	if (offset != rssize)
	{
		std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem() FAIL size mismatch" << std::endl;
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef POSTED_DEBUG
			std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}


/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

void RsGxsPostedGroupItem::clear()
{
	mGroup.mDescription.clear();
	return;
}

std::ostream & RsGxsPostedGroupItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsGxsPostedGroupItem", indent);
        uint16_t int_Indent = indent + 2;

        printIndent(out, int_Indent);
        out << "Description: " << mGroup.mDescription << std::endl;

        printRsItemEnd(out ,"RsGxsPostedGroupItem", indent);
        return out;
}


uint32_t RsGxsPostedSerialiser::sizeGxsPostedGroupItem(RsGxsPostedGroupItem* item)
{
	RsPostedGroup& g = item->mGroup;
	uint32_t s = 8; // header

	s += GetTlvStringSize(g.mDescription);

	return s;
}

bool RsGxsPostedSerialiser::serialiseGxsPostedGroupItem(RsGxsPostedGroupItem* item, void* data, uint32_t *size)
{

#ifdef POSTED_DEBUG
	std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedGroupItem()" << std::endl;
#endif

	uint32_t tlvsize = sizeGxsPostedGroupItem(item);
	uint32_t offset = 0;

	if(*size < tlvsize){
#ifdef POSTED_DEBUG
		std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedGroupItem()" << std::endl;
#endif
		return false;
	}

	*size = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* PostedGroupItem */
	ok &= SetTlvString(data, tlvsize, &offset, 1, item->mGroup.mDescription);


	if(offset != tlvsize)
	{
#ifdef POSTED_DEBUG
		std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedGroupItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}

#ifdef POSTED_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedGroupItem() NOK" << std::endl;
	}
#endif

	return ok;
}
RsGxsPostedGroupItem* RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem(void *data, uint32_t *size)
{

#ifdef POSTED_DEBUG
	std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
			(RS_SERVICE_GXSV2_TYPE_POSTED != getRsItemService(rstype)) ||
			(RS_PKT_SUBTYPE_POSTED_GRP_ITEM != getRsItemSubType(rstype)))
	{
			std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem() FAIL wrong type" << std::endl;
			return NULL; /* wrong type */
	}

	if (*size < rssize)	/* check size */
	{
			std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem() FAIL wrong size" << std::endl;
			return NULL; /* not enough data */
	}

	/* set the packet length */
	*size = rssize;

	bool ok = true;

	RsGxsPostedGroupItem* item = new RsGxsPostedGroupItem();
	/* skip the header */
	offset += 8;

	ok &= GetTlvString(data, rssize, &offset, 1, item->mGroup.mDescription);

	if (offset != rssize)
	{
		std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem() FAIL size mismatch" << std::endl;
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
#ifdef POSTED_DEBUG
		std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}

