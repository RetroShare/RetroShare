/*******************************************************************************
 * libretroshare/src/rsitems: rsphotoitems.h                                   *
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
#ifndef RSPHOTOV2ITEMS_H_
#define RSPHOTOV2ITEMS_H_

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsgxsitems.h"
#include "rsitems/rsgxscommentitems.h"

#include "serialiser/rsserial.h"
#include "serialiser/rsserializer.h"

#include "retroshare/rsphoto.h"

const uint8_t RS_PKT_SUBTYPE_PHOTO_ITEM         = 0x02;
const uint8_t RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM    = 0x03;

class RsGxsPhotoAlbumItem : public RsGxsGrpItem
{

public:

	RsGxsPhotoAlbumItem():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_PHOTO,
			RS_PKT_SUBTYPE_PHOTO_ITEM) { return;}
        virtual ~RsGxsPhotoAlbumItem() { return;}

        void clear();
//	std::ostream &print(std::ostream &out, uint16_t indent = 0);

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsPhotoAlbum album;
};

class RsGxsPhotoPhotoItem : public RsGxsMsgItem
{
public:

	RsGxsPhotoPhotoItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_PHOTO, RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM) {}
	virtual ~RsGxsPhotoPhotoItem() {}
	void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsPhotoPhoto photo;
};

class RsGxsPhotoSerialiser : public RsGxsCommentSerialiser
{
public:

	RsGxsPhotoSerialiser() :RsGxsCommentSerialiser(RS_SERVICE_GXS_TYPE_PHOTO) {}
	virtual     ~RsGxsPhotoSerialiser() {}

	virtual RsItem *create_item(uint16_t service, uint8_t item_sub_id) const;
};

#endif /* RSPHOTOV2ITEMS_H_ */
