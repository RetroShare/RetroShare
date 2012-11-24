/*
 * libretroshare/src/retroshare: rsphoto.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Christopher Evi-Parker, Robert Fernie
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

#include <iostream>

#include "rsphotoitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

#define GXS_PHOTO_SERIAL_DEBUG


uint32_t RsGxsPhotoSerialiser::size(RsItem* item)
{
	RsGxsPhotoPhotoItem* ppItem = NULL;
	RsGxsPhotoAlbumItem* paItem = NULL;
        RsGxsPhotoCommentItem* cItem = NULL;

	if((ppItem = dynamic_cast<RsGxsPhotoPhotoItem*>(item)) != NULL)
	{
		return sizeGxsPhotoPhotoItem(ppItem);
	}
	else if((paItem = dynamic_cast<RsGxsPhotoAlbumItem*>(item)) != NULL)
	{
		return sizeGxsPhotoAlbumItem(paItem);
	}
        else if((cItem = dynamic_cast<RsGxsPhotoCommentItem*>(item)) != NULL)
        {
                return sizeGxsPhotoCommentItem(cItem);
        }
	else
	{
#ifdef GXS_PHOTO_SERIAL_DEBUG

#endif
		return NULL;
	}

}

bool RsGxsPhotoSerialiser::serialise(RsItem* item, void* data, uint32_t* size)
{

    RsGxsPhotoPhotoItem* ppItem = NULL;
    RsGxsPhotoAlbumItem* paItem = NULL;
    RsGxsPhotoCommentItem* cItem = NULL;

    if((ppItem = dynamic_cast<RsGxsPhotoPhotoItem*>(item)) != NULL)
    {
        return serialiseGxsPhotoPhotoItem(ppItem, data, size);
    }
    else if((paItem = dynamic_cast<RsGxsPhotoAlbumItem*>(item)) != NULL)
    {
        return serialiseGxsPhotoAlbumItem(paItem, data, size);
    }else if((cItem = dynamic_cast<RsGxsPhotoCommentItem*>(item)) != NULL)
    {
        return serialiseGxsPhotoCommentItem(cItem, data, size);
    }
    else
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG

#endif
            return false;
    }

}

RsItem* RsGxsPhotoSerialiser::deserialise(void* data, uint32_t* size)
{

#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsPhotoSerialiser::deserialise()" << std::endl;
#endif
        /* get the type and size */
        uint32_t rstype = getRsItemId(data);

        if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
                (RS_SERVICE_GXSV1_TYPE_PHOTO != getRsItemService(rstype)))
        {
                return NULL; /* wrong type */
        }

        switch(getRsItemSubType(rstype))
        {

                case RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM:
			return deserialiseGxsPhotoPhotoItem(data, size);
                case RS_PKT_SUBTYPE_PHOTO_ITEM:
			return deserialiseGxsPhotoAlbumItem(data, size);
                case RS_PKT_SUBTYPE_PHOTO_COMMENT_ITEM:
                        return deserialiseGxsPhotoCommentItem(data, size);
		default:
			{
#ifdef GXS_PHOTO_SERIAL_DEBUG
				std::cerr << "RsGxsPhotoSerialiser::deserialise(): subtype could not be dealt with"
                                    << std::endl;
#endif
				break;
			}
        }
        return NULL;
}

uint32_t RsGxsPhotoSerialiser::sizeGxsPhotoAlbumItem(RsGxsPhotoAlbumItem* item)
{

	const RsPhotoAlbum& album = item->album;
	uint32_t s = 8; // header

	s += GetTlvStringSize(album.mCaption);
	s += GetTlvStringSize(album.mCategory);
	s += GetTlvStringSize(album.mDescription);
	s += GetTlvStringSize(album.mHashTags);
	s += GetTlvStringSize(album.mOther);
	s += GetTlvStringSize(album.mPhotoPath);
	s += GetTlvStringSize(album.mPhotographer);
	s += GetTlvStringSize(album.mWhen);
	s += GetTlvStringSize(album.mWhere);

	RsTlvBinaryData b(item->PacketService()); // TODO, need something more persisitent
	b.setBinData(album.mThumbnail.data, album.mThumbnail.size);
	s += GetTlvStringSize(album.mThumbnail.type);
	s += b.TlvSize();

	return s;
}

uint32_t    RsGxsPhotoSerialiser::sizeGxsPhotoCommentItem(RsGxsPhotoCommentItem *item)
{

    const RsPhotoComment& comment = item->comment;
    uint32_t s = 8; // header

    s += GetTlvStringSize(comment.mComment);
    s += 4; // mflags

    return s;

}

bool RsGxsPhotoSerialiser::serialiseGxsPhotoAlbumItem(RsGxsPhotoAlbumItem* item, void* data,
		uint32_t* size)
{

#ifdef GXS_PHOTO_SERIAL_DEBUG
    std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoAlbumItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsPhotoAlbumItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef GXS_PHOTO_SERIAL_DEBUG
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoAlbumItem()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* GxsPhotoAlbumItem */

    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mCaption);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mCategory);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mDescription);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mHashTags);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mOther);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mPhotoPath);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mPhotographer);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mWhen);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mWhere);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->album.mThumbnail.type);
    RsTlvBinaryData b(RS_SERVICE_GXSV1_TYPE_PHOTO); // TODO, need something more persisitent
    b.setBinData(item->album.mThumbnail.data, item->album.mThumbnail.size);
    ok &= b.SetTlv(data, tlvsize, &offset);

    if(offset != tlvsize)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoAlbumItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXS_PHOTO_SERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoAlbumItem() NOK" << std::endl;
    }
#endif

    return ok;
}

RsGxsPhotoAlbumItem* RsGxsPhotoSerialiser::deserialiseGxsPhotoAlbumItem(void* data,
		uint32_t* size)
{


#ifdef GXS_PHOTO_SERIAL_DEBUG
    std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoAlbumItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_GXSV1_TYPE_PHOTO != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_PHOTO_ITEM != getRsItemSubType(rstype)))
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoAlbumItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoAlbumItem() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsPhotoAlbumItem* item = new RsGxsPhotoAlbumItem();
    /* skip the header */
    offset += 8;


    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mCaption);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mCategory);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mDescription);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mHashTags);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mOther);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mPhotoPath);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mPhotographer);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mWhen);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mWhere);
    ok &= GetTlvString(data, rssize, &offset, 1, item->album.mThumbnail.type);

        RsTlvBinaryData b(RS_SERVICE_GXSV1_TYPE_PHOTO); // TODO, need something more persisitent
	ok &= b.GetTlv(data, rssize, &offset);
	item->album.mThumbnail.data = (uint8_t*)b.bin_data;
	item->album.mThumbnail.size = b.bin_len;
	b.TlvShallowClear();

    if (offset != rssize)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoAlbumItem() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoAlbumItem() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}

uint32_t RsGxsPhotoSerialiser::sizeGxsPhotoPhotoItem(RsGxsPhotoPhotoItem* item)
{

	const RsPhotoPhoto& photo = item->photo;

	uint32_t s = 8; // header size
	s += GetTlvStringSize(photo.mCaption);
	s += GetTlvStringSize(photo.mCategory);
	s += GetTlvStringSize(photo.mDescription);
	s += GetTlvStringSize(photo.mHashTags);
	s += GetTlvStringSize(photo.mOther);
	s += GetTlvStringSize(photo.mPhotographer);
	s += GetTlvStringSize(photo.mWhen);
	s += GetTlvStringSize(photo.mWhere);

	RsTlvBinaryData b(item->PacketService()); // TODO, need something more persisitent
	b.setBinData(photo.mThumbnail.data, photo.mThumbnail.size);
	s += GetTlvStringSize(photo.mThumbnail.type);
	s += b.TlvSize();

	return s;
}

bool RsGxsPhotoSerialiser::serialiseGxsPhotoPhotoItem(RsGxsPhotoPhotoItem* item, void* data,
		uint32_t* size)
{


#ifdef GXS_PHOTO_SERIAL_DEBUG
    std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoPhotoItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsPhotoPhotoItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef GXS_PHOTO_SERIAL_DEBUG
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoPhotoItem()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* GxsPhotoAlbumItem */

    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mCaption);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mCategory);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mDescription);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mHashTags);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mOther);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mPhotographer);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mWhen);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mWhere);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->photo.mThumbnail.type);
    RsTlvBinaryData b(RS_SERVICE_GXSV1_TYPE_PHOTO); // TODO, need something more persisitent
    b.setBinData(item->photo.mThumbnail.data, item->photo.mThumbnail.size);
    ok &= b.SetTlv(data, tlvsize, &offset);

    if(offset != tlvsize)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoPhotoItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXS_PHOTO_SERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoPhotoItem() NOK" << std::endl;
    }
#endif

    return ok;
}

RsGxsPhotoPhotoItem* RsGxsPhotoSerialiser::deserialiseGxsPhotoPhotoItem(void* data,
		uint32_t* size)
{


#ifdef GXS_PHOTO_SERIAL_DEBUG
    std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoPhotoItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_GXSV1_TYPE_PHOTO != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM != getRsItemSubType(rstype)))
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoPhotoItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoPhotoItem() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsPhotoPhotoItem* item = new RsGxsPhotoPhotoItem();
    /* skip the header */
    offset += 8;

    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mCaption);
    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mCategory);
    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mDescription);
    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mHashTags);
    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mOther);
    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mPhotographer);
    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mWhen);
    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mWhere);
    ok &= GetTlvString(data, rssize, &offset, 1, item->photo.mThumbnail.type);

        RsTlvBinaryData b(RS_SERVICE_GXSV1_TYPE_PHOTO); // TODO, need something more persisitent
	ok &= b.GetTlv(data, rssize, &offset);
	item->photo.mThumbnail.data = (uint8_t*)(b.bin_data);
	item->photo.mThumbnail.size = b.bin_len;
	b.TlvShallowClear();

    if (offset != rssize)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoPhotoItem() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoPhotoItem() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}



bool        RsGxsPhotoSerialiser::serialiseGxsPhotoCommentItem  (RsGxsPhotoCommentItem *item, void *data, uint32_t *size)
{


#ifdef GXS_PHOTO_SERIAL_DEBUG
    std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoCommentItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsPhotoCommentItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef GXS_PHOTO_SERIAL_DEBUG
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoCommentItem()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* GxsPhotoAlbumItem */

    ok &= SetTlvString(data, tlvsize, &offset, 0, item->comment.mComment);
    ok &= setRawUInt32(data, tlvsize, &offset, item->comment.mCommentFlag);

    if(offset != tlvsize)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoCommentItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXS_PHOTO_SERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsPhotoSerialiser::serialiseGxsPhotoCommentItem() NOK" << std::endl;
    }
#endif

    return ok;
}

RsGxsPhotoCommentItem *    RsGxsPhotoSerialiser::deserialiseGxsPhotoCommentItem(void *data, uint32_t *size)
{


#ifdef GXS_PHOTO_SERIAL_DEBUG
    std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoPhotoItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_GXSV1_TYPE_PHOTO != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_PHOTO_COMMENT_ITEM != getRsItemSubType(rstype)))
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoCommentItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoCommentItem() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsPhotoCommentItem* item = new RsGxsPhotoCommentItem();

    /* skip the header */
    offset += 8;

    ok &= GetTlvString(data, rssize, &offset, 0, item->comment.mComment);
    ok &= getRawUInt32(data, rssize, &offset, &(item->comment.mCommentFlag));

    if (offset != rssize)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoCommentItem() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef GXS_PHOTO_SERIAL_DEBUG
            std::cerr << "RsGxsPhotoSerialiser::deserialiseGxsPhotoCommentItem() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}

void RsGxsPhotoAlbumItem::clear()
{
	album.mCaption.clear();
	album.mCategory.clear();
	album.mDescription.clear();
	album.mHashTags.clear();
	album.mOther.clear();
	album.mPhotoPath.clear();
	album.mPhotographer.clear();
	album.mWhen.clear();
	album.mWhere.clear();
	album.mThumbnail.deleteImage();
}

void RsGxsPhotoCommentItem::clear()
{
    comment.mComment.clear();
    comment.mCommentFlag = 0;
}

std::ostream& RsGxsPhotoCommentItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsPhotoCommentItem", indent);
    uint16_t int_Indent = indent + 2;


    printRsItemEnd(out ,"RsGxsPhotoCommentItem", indent);
    return out;
}

std::ostream& RsGxsPhotoAlbumItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsPhotoAlbumItem", indent);
    uint16_t int_Indent = indent + 2;

    out << album << std::endl;

    printRsItemEnd(out ,"RsGxsPhotoAlbumItem", indent);
    return out;
}

void RsGxsPhotoPhotoItem::clear()
{
	photo.mCaption.clear();
	photo.mCategory.clear();
	photo.mDescription.clear();
	photo.mHashTags.clear();
	photo.mOther.clear();
	photo.mPhotographer.clear();
	photo.mWhen.clear();
	photo.mWhere.clear();
	photo.mThumbnail.deleteImage();
}

std::ostream& RsGxsPhotoPhotoItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsPhotoPhotoItem", indent);
    uint16_t int_Indent = indent + 2;


    printRsItemEnd(out ,"RsGxsPhotoPhotoItem", indent);
    return out;
}

