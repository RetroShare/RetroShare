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
	RsTypeSerializer::serial_process<uint32_t>(j,ctx,TLV_TYPE_UINT32_PARAM,album.mShareMode,"mShareMode");
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_CAPTION,  album.mCaption,       "mCaption");
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR,    album.mDescription,   "mDescription");
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_NAME,     album.mPhotographer,  "mPhotographer");
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_LOCATION, album.mWhere,         "mWhere");
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DATE,     album.mWhen,          "mWhen");

	album.mThumbnail.serial_process(j, ctx);
}
void RsGxsPhotoPhotoItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DESCR,    photo.mDescription,   "mDescription");
	RsTypeSerializer::serial_process<uint32_t>(j,ctx,TLV_TYPE_UINT32_PARAM,photo.mOrder,"mOrder");
	photo.mLowResImage.serial_process(j, ctx);
	photo.mPhotoFile.serial_process(j, ctx);
}

void RsGxsPhotoAlbumItem::clear()
{
	album.mShareMode = RSPHOTO_SHAREMODE_LOWRESONLY;
	album.mCaption.clear();
	album.mDescription.clear();
	album.mPhotographer.clear();
	album.mWhere.clear();
	album.mWhen.clear();
	album.mThumbnail.clear();

	// not saved
	album.mAutoDownload = false;
}

void RsGxsPhotoPhotoItem::clear()
{
	photo.mDescription.clear();
	photo.mOrder = 0;
	photo.mLowResImage.clear();
	photo.mPhotoFile.clear();

	// not saved
	photo.mPath.clear();
}
