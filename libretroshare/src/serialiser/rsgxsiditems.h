/*
 * libretroshare/src/serialiser: rsgxsiditems.h
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

#ifndef RS_GXS_IDENTITY_ITEMS_H
#define RS_GXS_IDENTITY_ITEMS_H

#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

#include "rsgxsitems.h"
#include "retroshare/rsidentity.h"

const uint8_t RS_PKT_SUBTYPE_GXSID_GROUP_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSID_OPINION_ITEM = 0x03;
const uint8_t RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM = 0x04;

class RsGxsIdGroupItem : public RsGxsGrpItem
{

public:

	RsGxsIdGroupItem():  RsGxsGrpItem(RS_SERVICE_GXSV2_TYPE_GXSID,
			RS_PKT_SUBTYPE_GXSID_GROUP_ITEM) { return;}
        virtual ~RsGxsIdGroupItem() { return;}

        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);


	RsGxsIdGroup group;
};

class RsGxsIdOpinionItem : public RsGxsMsgItem
{
public:

	RsGxsIdOpinionItem(): RsGxsMsgItem(RS_SERVICE_GXSV2_TYPE_GXSID,
			RS_PKT_SUBTYPE_GXSID_OPINION_ITEM) {return; }
        virtual ~RsGxsIdOpinionItem() { return;}
        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);
	RsGxsIdOpinion opinion;
};

class RsGxsIdCommentItem : public RsGxsMsgItem
{
public:

    RsGxsIdCommentItem(): RsGxsMsgItem(RS_SERVICE_GXSV2_TYPE_GXSID,
                                          RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM) { return; }
    virtual ~RsGxsIdCommentItem() { return; }
    void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);
    RsGxsIdComment comment;

};

class RsGxsIdSerialiser : public RsSerialType
{
public:

	RsGxsIdSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV2_TYPE_GXSID)
	{ return; }
	virtual     ~RsGxsIdSerialiser() { return; }

	uint32_t    size(RsItem *item);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	uint32_t    sizeGxsIdGroupItem(RsGxsIdGroupItem *item);
	bool        serialiseGxsIdGroupItem  (RsGxsIdGroupItem *item, void *data, uint32_t *size);
	RsGxsIdGroupItem *    deserialiseGxsIdGroupItem(void *data, uint32_t *size);

	uint32_t    sizeGxsIdOpinionItem(RsGxsIdOpinionItem *item);
	bool        serialiseGxsIdOpinionItem  (RsGxsIdOpinionItem *item, void *data, uint32_t *size);
	RsGxsIdOpinionItem *    deserialiseGxsIdOpinionItem(void *data, uint32_t *size);

        uint32_t    sizeGxsIdCommentItem(RsGxsIdCommentItem *item);
        bool        serialiseGxsIdCommentItem  (RsGxsIdCommentItem *item, void *data, uint32_t *size);
        RsGxsIdCommentItem *    deserialiseGxsIdCommentItem(void *data, uint32_t *size);

};

#endif /* RS_GXS_IDENTITY_ITEMS_H */
