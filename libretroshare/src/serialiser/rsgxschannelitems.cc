/*
 * libretroshare/src/serialiser: rsgxschannelitems.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie
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

#include <iostream>

#include "rsgxschannelitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

#define GXSCHANNEL_DEBUG	1


uint32_t RsGxsChannelSerialiser::size(RsItem *item)
{
#ifdef GXSCHANNEL_DEBUG
        std::cerr << "RsGxsChannelSerialiser::size()" << std::endl;
#endif

	RsGxsChannelGroupItem* grp_item = NULL;
	RsGxsChannelPostItem* op_item = NULL;

	if((grp_item = dynamic_cast<RsGxsChannelGroupItem*>(item)) != NULL)
	{
		return sizeGxsChannelGroupItem(grp_item);
	}
	else if((op_item = dynamic_cast<RsGxsChannelPostItem*>(item)) != NULL)
	{
		return sizeGxsChannelPostItem(op_item);
	}
	else
	{
		return RsGxsCommentSerialiser::size(item);
	}
	return 0;
}

bool RsGxsChannelSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
#ifdef GXSCHANNEL_DEBUG
        std::cerr << "RsGxsChannelSerialiser::serialise()" << std::endl;
#endif

	RsGxsChannelGroupItem* grp_item = NULL;
	RsGxsChannelPostItem* op_item = NULL;

	if((grp_item = dynamic_cast<RsGxsChannelGroupItem*>(item)) != NULL)
	{
		return serialiseGxsChannelGroupItem(grp_item, data, size);
	}
	else if((op_item = dynamic_cast<RsGxsChannelPostItem*>(item)) != NULL)
	{
		return serialiseGxsChannelPostItem(op_item, data, size);
	}
	else
	{
		return RsGxsCommentSerialiser::serialise(item, data, size);
	}
}

RsItem* RsGxsChannelSerialiser::deserialise(void* data, uint32_t* size)
{
		
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV2_TYPE_CHANNELS != getRsItemService(rstype)))
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialise() ERROR Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM:
			return deserialiseGxsChannelGroupItem(data, size);
			break;
		case RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM:
			return deserialiseGxsChannelPostItem(data, size);
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


void RsGxsChannelGroupItem::clear()
{
	mDescription.clear();
	mImage.TlvClear();
}

std::ostream& RsGxsChannelGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsChannelGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Description: " << mDescription << std::endl;

	out << "Image: " << std::endl;
	mImage.print(out, int_Indent);
  
	printRsItemEnd(out ,"RsGxsChannelGroupItem", indent);
	return out;
}



bool RsGxsChannelGroupItem::fromChannelGroup(RsGxsChannelGroup &group, bool moveImage)
{
	clear();
	meta = group.mMeta;
	mDescription = group.mDescription;

	if (moveImage)
	{
		mImage.binData.bin_data = group.mImage.mData;
		mImage.binData.bin_len = group.mImage.mSize;
		group.mImage.shallowClear();
	}
	else
	{
		mImage.binData.setBinData(group.mImage.mData, group.mImage.mSize);
	}
	return true;
}



bool RsGxsChannelGroupItem::toChannelGroup(RsGxsChannelGroup &group, bool moveImage)
{
	group.mMeta = meta;
	group.mDescription = mDescription;
	if (moveImage)
	{
		group.mImage.take((uint8_t *) mImage.binData.bin_data, mImage.binData.bin_len);
		// mImage doesn't have a ShallowClear at the moment!
		mImage.binData.TlvShallowClear();
	}
	else
	{
		group.mImage.copy((uint8_t *) mImage.binData.bin_data, mImage.binData.bin_len);
	}
	return true;
}


uint32_t RsGxsChannelSerialiser::sizeGxsChannelGroupItem(RsGxsChannelGroupItem *item)
{
	uint32_t s = 8; // header

	s += GetTlvStringSize(item->mDescription);
	s += item->mImage.TlvSize();

	return s;
}

bool RsGxsChannelSerialiser::serialiseGxsChannelGroupItem(RsGxsChannelGroupItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsChannelGroupItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() Size too small" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsChannelGroupItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DESCR, item->mDescription);
	item->mImage.SetTlv(data, tlvsize, &offset);
	
	if(offset != tlvsize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSCHANNEL_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsChannelGroupItem* RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem(void *data, uint32_t *size)
{
	
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV2_TYPE_CHANNELS != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCHANNEL_GROUP_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsChannelGroupItem* item = new RsGxsChannelGroupItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DESCR, item->mDescription);
	item->mImage.GetTlv(data, rssize, &offset);
	
	if (offset != rssize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsChannelPostItem::clear()
{
	mMsg.clear();
	mAttachment.TlvClear();
	mThumbnail.TlvClear();
}

std::ostream& RsGxsChannelPostItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsChannelPostItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Msg: " << mMsg << std::endl;
  
	out << "Attachment: " << std::endl;
	mAttachment.print(out, int_Indent);
  
	out << "Thumbnail: " << std::endl;
	mThumbnail.print(out, int_Indent);
  
	printRsItemEnd(out ,"RsGxsChannelPostItem", indent);
	return out;
}


bool RsGxsChannelPostItem::fromChannelPost(RsGxsChannelPost &post, bool moveImage)
{
	clear();
	meta = post.mMeta;
	mMsg = post.mMsg;

	if (moveImage)
	{
		mThumbnail.binData.bin_data = post.mThumbnail.mData;
		mThumbnail.binData.bin_len = post.mThumbnail.mSize;
		post.mThumbnail.shallowClear();
	}
	else
	{
		mThumbnail.binData.setBinData(post.mThumbnail.mData, post.mThumbnail.mSize);
	}

	std::list<RsGxsFile>::iterator fit;
	for(fit = post.mFiles.begin(); fit != post.mFiles.end(); fit++)
	{
		RsTlvFileItem fi;
		fi.name = fit->mName;
		fi.filesize = fit->mSize;
		fi.hash = fit->mHash;
		mAttachment.items.push_back(fi);
	}
	return true;
}



bool RsGxsChannelPostItem::toChannelPost(RsGxsChannelPost &post, bool moveImage)
{
	post.mMeta = meta;
	post.mMsg = mMsg;
	if (moveImage)
	{
		post.mThumbnail.take((uint8_t *) mThumbnail.binData.bin_data, mThumbnail.binData.bin_len);
		// mThumbnail doesn't have a ShallowClear at the moment!
		mThumbnail.binData.TlvShallowClear();
	}
	else
	{
		post.mThumbnail.copy((uint8_t *) mThumbnail.binData.bin_data, mThumbnail.binData.bin_len);
	}

	post.mCount = 0;
	post.mSize = 0;
	std::list<RsTlvFileItem>::iterator fit;
	for(fit = mAttachment.items.begin(); fit != mAttachment.items.end(); fit++)
	{
		RsGxsFile fi;
		fi.mName = RsDirUtil::getTopDir(fit->name);
		fi.mSize  = fit->filesize;
		fi.mHash  = fit->hash;

		post.mFiles.push_back(fi);
		post.mCount++;
		post.mSize += fi.mSize;
	}
	return true;
}


uint32_t RsGxsChannelSerialiser::sizeGxsChannelPostItem(RsGxsChannelPostItem *item)
{
	uint32_t s = 8; // header

	s += GetTlvStringSize(item->mMsg); // mMsg.
	s += item->mAttachment.TlvSize();
	s += item->mThumbnail.TlvSize();

	return s;
}

bool RsGxsChannelSerialiser::serialiseGxsChannelPostItem(RsGxsChannelPostItem *item, void *data, uint32_t *size)
{
	
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelPostItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsChannelPostItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelPostItem() ERROR space too small" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsChannelPostItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->mMsg);
	item->mAttachment.SetTlv(data, tlvsize, &offset);
	item->mThumbnail.SetTlv(data, tlvsize, &offset);
	
	if(offset != tlvsize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelPostItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSCHANNEL_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsChannelSerialiser::serialiseGxsChannelGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsChannelPostItem* RsGxsChannelSerialiser::deserialiseGxsChannelPostItem(void *data, uint32_t *size)
{
	
#ifdef GXSCHANNEL_DEBUG
	std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXSV2_TYPE_CHANNELS != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSCHANNEL_POST_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsChannelPostItem* item = new RsGxsChannelPostItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->mMsg);
	item->mAttachment.GetTlv(data, rssize, &offset);
	item->mThumbnail.GetTlv(data, rssize, &offset);
	
	if (offset != rssize)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSCHANNEL_DEBUG
		std::cerr << "RsGxsChannelSerialiser::deserialiseGxsChannelPostItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

