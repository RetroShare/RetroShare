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

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

#include "rsgxsitems.h"
#include "rsphotoitems.h"
#include "retroshare/rsphotoV2.h"


class RsGxsPhotoAlbumItem : public RsGxsGrpItem
{

public:

	RsGxsPhotoAlbumItem():  RsGxsGrpItem(RS_SERVICE_TYPE_PHOTO,
			RS_PKT_SUBTYPE_PHOTO_ITEM) { return;}
        virtual ~RsGxsPhotoAlbumItem() { return;}

        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);


	RsPhotoAlbum album;
};

class RsGxsPhotoPhotoItem : public RsGxsMsgItem
{
public:

	RsGxsPhotoPhotoItem(): RsGxsMsgItem(RS_SERVICE_TYPE_PHOTO,
			RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM) {return; }
        virtual ~RsGxsPhotoPhotoItem() { return;}
        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);
	RsPhotoPhoto photo;
};

class RsGxsPhotoSerialiser : public RsSerialType
{
public:

	RsGxsPhotoSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_PHOTO)
	{ return; }
	virtual     ~RsGxsPhotoSerialiser() { return; }

	uint32_t    size(RsItem *item);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	uint32_t    sizeGxsPhotoAlbumItem(RsGxsPhotoAlbumItem *item);
	bool        serialiseGxsPhotoAlbumItem  (RsGxsPhotoAlbumItem *item, void *data, uint32_t *size);
	RsGxsPhotoAlbumItem *    deserialiseGxsPhotoAlbumItem(void *data, uint32_t *size);

	uint32_t    sizeGxsPhotoPhotoItem(RsGxsPhotoPhotoItem *item);
	bool        serialiseGxsPhotoPhotoItem  (RsGxsPhotoPhotoItem *item, void *data, uint32_t *size);
	RsGxsPhotoPhotoItem *    deserialiseGxsPhotoPhotoItem(void *data, uint32_t *size);


};

#endif /* RSPHOTOV2ITEMS_H_ */
