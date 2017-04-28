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

#include "rsitems/rsphotoitems.h"

#include "serialiser/rstlvbinary.h"
#include "serialization/rstypeserializer.h"

#define GXS_PHOTO_SERIAL_DEBUG


RsItem *RsGxsPhotoSerialiser::create_item(uint16_t service, uint8_t item_sub_id) const
{
    if(service != RS_SERVICE_GXS_TYPE_PHOTO)
        return NULL ;

    switch(item_sub_id)
    {
    case RS_PKT_SUBTYPE_PHOTO_COMMENT_ITEM: return new RsGxsPhotoCommentItem() ;
    case RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM: return new RsGxsPhotoAlbumItem() ;
    case RS_PKT_SUBTYPE_PHOTO_ITEM: return new RsGxsPhotoPhotoItem() ;
    default:
        return NULL ;
    }
}

void RsGxsPhotoAlbumItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_CAPTION,  album.mCaption,       "mCaption");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_CATEGORY, album.mCategory,      "mCategory");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR,    album.mDescription,   "mDescription");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_HASH_TAG, album.mHashTags,      "mHashTags");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_MSG,      album.mOther,         "mOther");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_PATH,     album.mPhotoPath,     "mPhotoPath");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_NAME,     album.mPhotographer,  "mPhotographer");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DATE,     album.mWhen,          "mWhen");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_LOCATION, album.mWhere,         "mWhere");
     RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_PIC_TYPE, album.mThumbnail.type,"mThumbnail.type");

     RsTlvBinaryDataRef b(RS_SERVICE_GXS_TYPE_PHOTO, album.mThumbnail.data,album.mThumbnail.size);

     RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,b,"thumbnail binary data") ;
}
void RsGxsPhotoPhotoItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_CAPTION, photo.mCaption);
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_CATEGORY, photo.mCategory);
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_DESCR, photo.mDescription);
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_HASH_TAG, photo.mHashTags);
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_MSG, photo.mOther);
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_PIC_AUTH, photo.mPhotographer);
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_DATE, photo.mWhen);
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_LOCATION, photo.mWhere);
    RsTypeSerializer::serial_process(j,ctx, TLV_TYPE_STR_PIC_TYPE, photo.mThumbnail.type);

    RsTlvBinaryDataRef b(RS_SERVICE_GXS_TYPE_PHOTO,photo.mThumbnail.data, photo.mThumbnail.size);
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx, b, "mThumbnail") ;
}
void RsGxsPhotoCommentItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_COMMENT,comment.mComment,"mComment");
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,comment.mCommentFlag,"mCommentFlag");
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
