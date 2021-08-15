/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsiditems.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
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
#pragma once

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "rsgxsitems.h"
#include "retroshare/rsidentity.h"

//const uint8_t RS_PKT_SUBTYPE_GXSID_GROUP_ITEM_deprecated   = 0x02;

const uint8_t RS_PKT_SUBTYPE_GXSID_GROUP_ITEM      = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSID_OPINION_ITEM    = 0x03;
const uint8_t RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM    = 0x04;
const uint8_t RS_PKT_SUBTYPE_GXSID_LOCAL_INFO_ITEM = 0x05;

class RsGxsIdItem: public RsGxsGrpItem
{
    public:
        RsGxsIdItem(uint8_t item_subtype) : RsGxsGrpItem(RS_SERVICE_GXS_TYPE_GXSID,item_subtype) {}
};

class RsGxsIdGroupItem : public RsGxsIdItem
{
public:

    RsGxsIdGroupItem():  RsGxsIdItem(RS_PKT_SUBTYPE_GXSID_GROUP_ITEM) {}
    virtual ~RsGxsIdGroupItem() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
    virtual void clear();

    bool fromGxsIdGroup(RsGxsIdGroup &group, bool moveImage);
    bool toGxsIdGroup(RsGxsIdGroup &group, bool moveImage);

    Sha1CheckSum mPgpIdHash;

	/** Need a signature as proof - otherwise anyone could add others Hashes.
	 * This is a string, as the length is variable.
	 * TODO: this should actually be a byte array (pointer+size), using an
	 * std::string breaks the JSON serialization.
	 * Be careful refactoring this as it may break retrocompatibility as this
	 * item is sent over the network */
	std::string mPgpIdSign;

	/// Unused
	RS_DEPRECATED std::list<std::string> mRecognTags;

	/// Avatar
	RsTlvImage mImage;
};

struct RsGxsIdLocalInfoItem : public RsGxsIdItem
{
    RsGxsIdLocalInfoItem():  RsGxsIdItem(RS_PKT_SUBTYPE_GXSID_LOCAL_INFO_ITEM) {}
    virtual ~RsGxsIdLocalInfoItem() {}

    virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    std::map<RsGxsId,rstime_t> mTimeStamps ;
    std::set<RsGxsId> mContacts ;
};

class RsGxsIdSerialiser : public RsServiceSerializer
{
public:
    RsGxsIdSerialiser() :RsServiceSerializer(RS_SERVICE_GXS_TYPE_GXSID) {}
    virtual     ~RsGxsIdSerialiser() {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};
