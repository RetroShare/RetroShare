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

#ifndef RSPHOTOV2ITEMS_H_
#define RSPHOTOV2ITEMS_H_

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsgxsitems.h"

#include "serialiser/rsserial.h"
#include "serialization/rsserializer.h"

#include "retroshare/rsphoto.h"

const uint8_t RS_PKT_SUBTYPE_PHOTO_ITEM         = 0x02;
const uint8_t RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM    = 0x03;
const uint8_t RS_PKT_SUBTYPE_PHOTO_COMMENT_ITEM = 0x04;

class RsGxsPhotoAlbumItem : public RsGxsGrpItem
{

public:

	RsGxsPhotoAlbumItem():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_PHOTO,
			RS_PKT_SUBTYPE_PHOTO_ITEM) { return;}
        virtual ~RsGxsPhotoAlbumItem() { return;}

        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);

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

class RsGxsPhotoCommentItem : public RsGxsMsgItem
{
public:

    RsGxsPhotoCommentItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_PHOTO, RS_PKT_SUBTYPE_PHOTO_COMMENT_ITEM) {}
    virtual ~RsGxsPhotoCommentItem() {}
    void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    RsPhotoComment comment;
};

class RsGxsPhotoSerialiser : public RsServiceSerializer
{
public:

	RsGxsPhotoSerialiser() :RsServiceSerializer(RS_SERVICE_GXS_TYPE_PHOTO) {}
	virtual     ~RsGxsPhotoSerialiser() {}

	virtual RsItem *create_item(uint16_t service, uint8_t item_sub_id) const;
};

#endif /* RSPHOTOV2ITEMS_H_ */
