/*
 * libretroshare/src/serialiser: rsgxsforumitems.h
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

#ifndef RS_GXS_FORUM_ITEMS_H
#define RS_GXS_FORUM_ITEMS_H

#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

#include "rsgxsitems.h"
#include "retroshare/rsgxsforums.h"

const uint8_t RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSFORUM_MESSAGE_ITEM = 0x03;

class RsGxsForumGroupItem : public RsGxsGrpItem
{

public:

	RsGxsForumGroupItem():  RsGxsGrpItem(RS_SERVICE_GXSV1_TYPE_FORUMS,
			RS_PKT_SUBTYPE_GXSFORUM_GROUP_ITEM) { return;}
        virtual ~RsGxsForumGroupItem() { return;}

        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);


	RsGxsForumGroup mGroup;
};

class RsGxsForumMsgItem : public RsGxsMsgItem
{
public:

	RsGxsForumMsgItem(): RsGxsMsgItem(RS_SERVICE_GXSV1_TYPE_FORUMS,
			RS_PKT_SUBTYPE_GXSFORUM_MESSAGE_ITEM) {return; }
        virtual ~RsGxsForumMsgItem() { return;}
        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);

	RsGxsForumMsg mMsg;
};

class RsGxsForumSerialiser : public RsSerialType
{
public:

	RsGxsForumSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV1_TYPE_FORUMS)
	{ return; }
	virtual     ~RsGxsForumSerialiser() { return; }

	uint32_t    size(RsItem *item);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	uint32_t    sizeGxsForumGroupItem(RsGxsForumGroupItem *item);
	bool        serialiseGxsForumGroupItem  (RsGxsForumGroupItem *item, void *data, uint32_t *size);
	RsGxsForumGroupItem *    deserialiseGxsForumGroupItem(void *data, uint32_t *size);

	uint32_t    sizeGxsForumMsgItem(RsGxsForumMsgItem *item);
	bool        serialiseGxsForumMsgItem  (RsGxsForumMsgItem *item, void *data, uint32_t *size);
	RsGxsForumMsgItem *    deserialiseGxsForumMsgItem(void *data, uint32_t *size);

};

#endif /* RS_GXS_FORUM_ITEMS_H */
