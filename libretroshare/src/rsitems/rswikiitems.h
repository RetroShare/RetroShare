/*******************************************************************************
 * libretroshare/src/rsitems: rswikiitems.h                                    *
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

#ifndef RS_WIKI_ITEMS_H
#define RS_WIKI_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "rsitems/rsitem.h"
#include "rsitems/rsgxsitems.h"

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
