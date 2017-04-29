/*
 * libretroshare/src/serialiser: rswikiitems.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#ifndef RS_WIKI_ITEMS_H
#define RS_WIKI_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"

#include "serialiser/rsgxsitems.h"
#include "retroshare/rswiki.h"

const uint8_t RS_PKT_SUBTYPE_WIKI_COLLECTION_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_WIKI_SNAPSHOT_ITEM   = 0x03;
const uint8_t RS_PKT_SUBTYPE_WIKI_COMMENT_ITEM    = 0x04;

class RsGxsWikiCollectionItem : public RsGxsGrpItem
{
public:
	RsGxsWikiCollectionItem():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_WIKI, RS_PKT_SUBTYPE_WIKI_COLLECTION_ITEM) {}
	virtual ~RsGxsWikiCollectionItem() {}

	void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsWikiCollection collection;
};

class RsGxsWikiSnapshotItem : public RsGxsMsgItem
{
public:

	RsGxsWikiSnapshotItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_WIKI, RS_PKT_SUBTYPE_WIKI_SNAPSHOT_ITEM) {}
	virtual ~RsGxsWikiSnapshotItem() {}
	void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsWikiSnapshot snapshot;
};

class RsGxsWikiCommentItem : public RsGxsMsgItem
{
public:

    RsGxsWikiCommentItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_WIKI, RS_PKT_SUBTYPE_WIKI_COMMENT_ITEM) {}
    virtual ~RsGxsWikiCommentItem() {}
    void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    RsWikiComment comment;

};

class RsGxsWikiSerialiser : public RsServiceSerializer
{
public:

	RsGxsWikiSerialiser() :RsServiceSerializer(RS_SERVICE_GXS_TYPE_WIKI) {}
	virtual     ~RsGxsWikiSerialiser() {}

	virtual RsItem *create_item(uint16_t /* service */, uint8_t /* item_sub_id */) const;
};

#endif /* RS_WIKI_ITEMS_H */
