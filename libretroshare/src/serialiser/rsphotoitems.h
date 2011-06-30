#ifndef P3_PHOTO_ITEMS_H
#define P3_PHOTO_ITEMS_H

/*
 * libretroshare/src/serialiser: rsphotoitems.h
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

#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

const uint8_t RS_PKT_SUBTYPE_PHOTO_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM = 0x03;
const uint8_t RS_PKT_SUBTYPE_PHOTO_COMMENT_ITEM = 0x04;

/**************************************************************************/

class RsPhotoItem;
class RsPhotoShowItem;
class RsPhotoCommentItem;

class RsPhotoItem: public RsItem
{
	public:

	RsPhotoItem()
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_PHOTO, 
		RS_PKT_SUBTYPE_PHOTO_ITEM) { return; }
virtual ~RsPhotoItem() { return; }
virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	std::string srcId;
	std::string photoId; /* same as hash */
	uint64_t    size;    /* file size */

	std::string name;
	std::wstring comment;

	std::string location; /* TODO: change to TLV */
	std::string date;     /* TODO: change to TLV */

	/* not serialised */
	bool isAvailable;
	std::string path;
};

/* THIS must be turned into a TLV type + set (TOD) */
class RsPhotoRefItem
{
	public:
	RsPhotoRefItem();

        std::string photoId;
	std::wstring altComment;
	uint32_t    deltaT; /* in 100ths of sec? */
};

class RsPhotoShowItem: public RsItem
{
	public:
	RsPhotoShowItem()
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_PHOTO, 
		RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM) { return; }

virtual ~RsPhotoShowItem() { return; }
virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

        std::string showId;

        std::string name;
        std::wstring comment;

        std::string location; /* TODO -> TLV */
        std::string date;     /* TODO -> TLV */
        std::list<RsPhotoRefItem> photos; /* list as ordered */
};

class RsPhotoSerialiser: public RsSerialType
{
	public:
	RsPhotoSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_PHOTO)
	{ return; }
virtual     ~RsPhotoSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *) { return 0; }
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size) { return false; }
virtual	RsItem *    deserialise(void *data, uint32_t *size) { return NULL; }

	private:

	/* For RS_PKT_SUBTYPE_PHOTO_ITEM */
//virtual	uint32_t    sizeLink(RsPhotoItem *);
//virtual	bool        serialiseLink  (RsPhotoItem *item, void *data, uint32_t *size);
//virtual	RsPhotoItem *deserialiseLink(void *data, uint32_t *size);

	/* For RS_PKT_SUBTYPE_PHOTO_SHOW_ITEM */
//virtual	uint32_t    sizeLink(RsPhotoShowItem *);
//virtual	bool        serialiseLink  (RsPhotoShowItem *item, void *data, uint32_t *size);
//virtual	RsPhotoShowItem *deserialiseLink(void *data, uint32_t *size);

	/* For RS_PKT_SUBTYPE_PHOTO_COMMENT_ITEM */
//virtual	uint32_t    sizeLink(RsPhotoCommentItem *);
//virtual	bool        serialiseLink  (RsPhotoCommentItem *item, void *data, uint32_t *size);
//virtual	RsPhotoCommentItem *deserialiseLink(void *data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_PHOTO_ITEMS_H */

