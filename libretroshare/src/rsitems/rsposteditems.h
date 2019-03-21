/*******************************************************************************
 * libretroshare/src/rsitems: rsposteditems.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef RSPOSTEDITEMS_H
#define RSPOSTEDITEMS_H

#include "rsitems/rsserviceids.h"
#include "rsitems/rsgxscommentitems.h"
#include "rsitems/rsgxsitems.h"
#include "serialiser/rstlvimage.h"

#include "retroshare/rsposted.h"

const uint8_t RS_PKT_SUBTYPE_POSTED_GRP_ITEM  = 0x02;
const uint8_t RS_PKT_SUBTYPE_POSTED_POST_ITEM = 0x03;

class RsGxsPostedGroupItem : public RsGxsGrpItem
{
public:
	RsGxsPostedGroupItem() : RsGxsGrpItem(RS_SERVICE_GXS_TYPE_POSTED, RS_PKT_SUBTYPE_POSTED_GRP_ITEM) {}
	virtual ~RsGxsPostedGroupItem() {}

	void clear();
	
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
	
	RsPostedGroup mGroup;
	

};

class RsGxsPostedPostItem : public RsGxsMsgItem
{
public:
	RsGxsPostedPostItem() : RsGxsMsgItem(RS_SERVICE_GXS_TYPE_POSTED, RS_PKT_SUBTYPE_POSTED_POST_ITEM) {}
	virtual ~RsGxsPostedPostItem() {}

	void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
	
	// Slightly unusual structure.
	// use conversion functions to transform:
	bool fromPostedPost(RsPostedPost &post, bool moveImage);
	bool toPostedPost(RsPostedPost &post, bool moveImage);

	RsPostedPost mPost;
	RsTlvImage mImage;
};

class RsGxsPostedSerialiser : public RsGxsCommentSerialiser
{
public:

	RsGxsPostedSerialiser() :RsGxsCommentSerialiser(RS_SERVICE_GXS_TYPE_POSTED) {}

	virtual ~RsGxsPostedSerialiser() {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};


#endif // RSPOSTEDITEMS_H
