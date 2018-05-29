/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsrecogitems.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013-2013 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_GXS_RECOG_ITEMS_H
#define RS_GXS_RECOG_ITEMS_H

#include <map>

#include "rsitems/rsitem.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"

#include "serialiser/rstlvkeys.h"
#include "serialiser/rstlvidset.h"

#if 0
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"

#endif

#include "retroshare/rsgxsifacetypes.h"

/**************************************************************************/

#define	RS_PKT_SUBTYPE_RECOGN_REQ	    0x01
#define	RS_PKT_SUBTYPE_RECOGN_TAG	    0x02
#define	RS_PKT_SUBTYPE_RECOGN_SIGNER    0x03


class RsGxsRecognReqItem: public RsItem
{
public:
	RsGxsRecognReqItem()
	  :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GXS_RECOGN,  RS_PKT_SUBTYPE_RECOGN_REQ)
	  , issued_at(0), period(0), tag_class(0), tag_type(0)
	{
		setPriorityLevel(QOS_PRIORITY_DEFAULT);
		return;
	}
    virtual ~RsGxsRecognReqItem(){}
	virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	uint32_t issued_at;
	uint32_t period;
	uint16_t tag_class;
	uint16_t tag_type;

	RsGxsId identity;
	std::string nickname;
	std::string comment;

	RsTlvKeySignature sign;
};


class RsGxsRecognTagItem: public RsItem
{
	public:
	RsGxsRecognTagItem()
	  :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GXS_RECOGN,  RS_PKT_SUBTYPE_RECOGN_TAG)
	  , valid_from(0), valid_to(0), tag_class(0), tag_type(0)
	{ 
		setPriorityLevel(QOS_PRIORITY_DEFAULT);
		return; 
	}
    virtual ~RsGxsRecognTagItem(){}
	virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	uint32_t valid_from;
	uint32_t valid_to;
	uint16_t tag_class;
	uint16_t tag_type;

	RsGxsId identity;
	std::string nickname;

	RsTlvKeySignature sign;
};


class RsGxsRecognSignerItem: public RsItem
{
public:
	RsGxsRecognSignerItem()  :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GXS_RECOGN,  RS_PKT_SUBTYPE_RECOGN_SIGNER)
	{
		setPriorityLevel(QOS_PRIORITY_DEFAULT);
		return;
	}
    virtual ~RsGxsRecognSignerItem(){}
	virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsTlvServiceIdSet signing_classes;
	RsTlvPublicRSAKey key; // has from->to, and flags.
	RsTlvKeySignature sign;
};


class RsGxsRecognSerialiser: public RsServiceSerializer
{
	public:
	RsGxsRecognSerialiser() :RsServiceSerializer(RS_SERVICE_TYPE_GXS_RECOGN) {}
	virtual     ~RsGxsRecognSerialiser() {}

	virtual RsItem *create_item(uint16_t service, uint8_t item_sub_id) const;
};

/**************************************************************************/

#endif /* RS_GXS_RECOGN_ITEMS_H */


