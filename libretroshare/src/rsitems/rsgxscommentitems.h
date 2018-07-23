/*******************************************************************************
 * libretroshare/src/rsitems: rsgxscommentitems.h                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_GXS_COMMENT_ITEMS_H
#define RS_GXS_COMMENT_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"

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

