/*
 * libretroshare/src/serialiser: rsgxscommentitems.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie
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

#ifndef RS_GXS_COMMENT_ITEMS_H
#define RS_GXS_COMMENT_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
//#include "serialiser/rstlvtypes.h"

#include "rsgxsitems.h"

#include "retroshare/rsgxscommon.h"

const uint8_t RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM = 0xf1;
const uint8_t RS_PKT_SUBTYPE_GXSCOMMENT_VOTE_ITEM = 0xf2;

class RsGxsCommentItem : public RsGxsMsgItem
{
public:

	RsGxsCommentItem(uint16_t service_type): RsGxsMsgItem(service_type,  RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM) {}
	virtual ~RsGxsCommentItem() {}
    void clear(){}

	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

	RsGxsComment mMsg;
};

class RsGxsVoteItem : public RsGxsMsgItem
{
public:

	RsGxsVoteItem(uint16_t service_type): RsGxsMsgItem(service_type, RS_PKT_SUBTYPE_GXSCOMMENT_VOTE_ITEM) {}
	virtual ~RsGxsVoteItem() {}
    void clear(){}

	virtual void serial_process(RsGenericSerializer::SerializeJob /* j */,RsGenericSerializer::SerializeContext& /* ctx */);

	RsGxsVote mMsg;
};

class RsGxsCommentSerialiser : public RsServiceSerializer
{
public:

	RsGxsCommentSerialiser(uint16_t service_type) :RsServiceSerializer(service_type) {}
	virtual     ~RsGxsCommentSerialiser() {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};

#endif /* RS_GXS_COMMENT_ITEMS_H */

