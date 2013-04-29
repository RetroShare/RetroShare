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

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

#include "rsgxsitems.h"
#include "retroshare/rswiki.h"

const uint8_t RS_PKT_SUBTYPE_WIKI_COLLECTION_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_WIKI_SNAPSHOT_ITEM = 0x03;
const uint8_t RS_PKT_SUBTYPE_WIKI_COMMENT_ITEM = 0x04;

class RsGxsWikiCollectionItem : public RsGxsGrpItem
{

public:

	RsGxsWikiCollectionItem():  RsGxsGrpItem(RS_SERVICE_GXSV2_TYPE_WIKI,
			RS_PKT_SUBTYPE_WIKI_COLLECTION_ITEM) { return;}
        virtual ~RsGxsWikiCollectionItem() { return;}

        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);


	RsWikiCollection collection;
};

class RsGxsWikiSnapshotItem : public RsGxsMsgItem
{
public:

	RsGxsWikiSnapshotItem(): RsGxsMsgItem(RS_SERVICE_GXSV2_TYPE_WIKI,
			RS_PKT_SUBTYPE_WIKI_SNAPSHOT_ITEM) {return; }
        virtual ~RsGxsWikiSnapshotItem() { return;}
        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);
	RsWikiSnapshot snapshot;
};

class RsGxsWikiCommentItem : public RsGxsMsgItem
{
public:

    RsGxsWikiCommentItem(): RsGxsMsgItem(RS_SERVICE_GXSV2_TYPE_WIKI,
                                          RS_PKT_SUBTYPE_WIKI_COMMENT_ITEM) { return; }
    virtual ~RsGxsWikiCommentItem() { return; }
    void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);
    RsWikiComment comment;

};

class RsGxsWikiSerialiser : public RsSerialType
{
public:

	RsGxsWikiSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV2_TYPE_WIKI)
	{ return; }
	virtual     ~RsGxsWikiSerialiser() { return; }

	uint32_t    size(RsItem *item);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	uint32_t    sizeGxsWikiCollectionItem(RsGxsWikiCollectionItem *item);
	bool        serialiseGxsWikiCollectionItem  (RsGxsWikiCollectionItem *item, void *data, uint32_t *size);
	RsGxsWikiCollectionItem *    deserialiseGxsWikiCollectionItem(void *data, uint32_t *size);

	uint32_t    sizeGxsWikiSnapshotItem(RsGxsWikiSnapshotItem *item);
	bool        serialiseGxsWikiSnapshotItem  (RsGxsWikiSnapshotItem *item, void *data, uint32_t *size);
	RsGxsWikiSnapshotItem *    deserialiseGxsWikiSnapshotItem(void *data, uint32_t *size);

        uint32_t    sizeGxsWikiCommentItem(RsGxsWikiCommentItem *item);
        bool        serialiseGxsWikiCommentItem  (RsGxsWikiCommentItem *item, void *data, uint32_t *size);
        RsGxsWikiCommentItem *    deserialiseGxsWikiCommentItem(void *data, uint32_t *size);

};

#endif /* RS_WIKI_ITEMS_H */
