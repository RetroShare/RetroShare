/*******************************************************************************
 * libretroshare/src/rsitems: rsphotoitems.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Christopher Evi-Parker,Robert Fernie<retroshare@lunamutt.com>*
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include <iostream>

#include "rsitems/rsphotoitems.h"

#include "serialiser/rstlvbinary.h"
#include "serialiser/rstypeserializer.h"

RsItem *RsGxsPhotoSerialiser::create_item(uint16_t service, uint8_t item_sub_id) const
{
	if(service != RS_SERVICE_GXS_TYPE_PHOTO)
		return NULL ;

	switch(item_sub_id)
	{
	case RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM: return new RsGxsPhotoPhotoItem() ;
	case RS_PKT_SUBTYPE_PHOTO_ITEM: return new RsGxsPhotoAlbumItem() ;
	default:
		return RsGxsCommentSerialiser::create_item(service,item_sub_id) ;
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

	album.mThumbnail.serial_process(j, ctx);
}
void RsGxsPhotoPhotoItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
	RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_CAPTION,  photo.mCaption,        "mCaption");
	RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_CATEGORY, photo.mCategory,       "mCategory");
	RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_DESCR,    photo.mDescription,    "mDescription");
	RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_HASH_TAG, photo.mHashTags,       "mHashTags");
	RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_MSG,      photo.mOther,          "mOther");
	RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_PIC_AUTH, photo.mPhotographer,   "mPhotographer");
	RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_DATE,     photo.mWhen,           "mWhen");
	RsTypeSerializer::serial_process(j, ctx, TLV_TYPE_STR_LOCATION, photo.mWhere,          "mWhere");

	photo.mThumbnail.serial_process(j, ctx);
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
	album.mThumbnail.clear();
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
	photo.mThumbnail.clear();
}
