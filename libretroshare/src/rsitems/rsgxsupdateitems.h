#ifndef RSGXSUPDATEITEMS_H_
#define RSGXSUPDATEITEMS_H_

/*
 * libretroshare/src/serialiser: rsgxsupdateitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012 Christopher Evi-Parker
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



#if 0
#include <map>
#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"
#endif

#include "gxs/rsgxs.h"
#include "gxs/rsgxsdata.h"
#include "serialiser/rstlvidset.h"


const uint8_t RS_PKT_SUBTYPE_GXS_GRP_UPDATE             = 0x01;
const uint8_t RS_PKT_SUBTYPE_GXS_MSG_UPDATE_deprecated  = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXS_MSG_UPDATE             = 0x03;
const uint8_t RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE      = 0x04;
const uint8_t RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE      = 0x08;
const uint8_t RS_PKT_SUBTYPE_GXS_GRP_CONFIG             = 0x09;

class RsGxsNetServiceItem: public RsItem
{
public:
    RsGxsNetServiceItem(uint16_t serv_type,uint8_t subtype) : RsItem(RS_PKT_VERSION_SERVICE, serv_type, subtype) {}

    virtual ~RsGxsNetServiceItem() {}
	virtual void clear() = 0 ;
};

class RsGxsGrpConfig
{
public:
	RsGxsGrpConfig()
	{
		msg_keep_delay = RS_GXS_DEFAULT_MSG_STORE_PERIOD ;
		msg_send_delay = RS_GXS_DEFAULT_MSG_SEND_PERIOD ;
		msg_req_delay  = RS_GXS_DEFAULT_MSG_REQ_PERIOD ;

		max_visible_count = 0 ;
		update_TS = 0 ;
	}

	uint32_t     msg_keep_delay ;	// delay after which we discard the posts
	uint32_t     msg_send_delay ;	// delay after which we dont send the posts anymore
	uint32_t     msg_req_delay ;	// delay after which we dont get the posts from friends

	RsTlvPeerIdSet suppliers;		// list of friends who feed this group
	uint32_t max_visible_count ;	// max visible count reported by contributing friends
	time_t update_TS ;				// last time the max visible count was updated.
};

class RsGxsGrpConfigItem : public RsGxsNetServiceItem, public RsGxsGrpConfig
{
public:
    RsGxsGrpConfigItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_GRP_CONFIG) {}
    RsGxsGrpConfigItem(const RsGxsGrpConfig& m,uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_GRP_CONFIG),RsGxsGrpConfig(m) {}
    virtual ~RsGxsGrpConfigItem() {}

    virtual void clear() {}
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsGxsGroupId grpId ;
};

class RsGxsGrpUpdate
{
public:
    RsGxsGrpUpdate() { grpUpdateTS=0;}

    uint32_t grpUpdateTS;
};

class RsGxsGrpUpdateItem : public RsGxsNetServiceItem, public RsGxsGrpUpdate
{
public:
    RsGxsGrpUpdateItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_GRP_UPDATE) {clear();}
    RsGxsGrpUpdateItem(const RsGxsGrpUpdate& u,uint16_t serv_type) : RsGxsNetServiceItem(serv_type, RS_PKT_SUBTYPE_GXS_GRP_UPDATE), RsGxsGrpUpdate(u) {}

    virtual ~RsGxsGrpUpdateItem() {}

    virtual void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    RsPeerId peerID;
};

class RsGxsServerGrpUpdate
{
public:
    RsGxsServerGrpUpdate() { grpUpdateTS = 0 ; }

	uint32_t grpUpdateTS;
};

class RsGxsServerGrpUpdateItem : public RsGxsNetServiceItem, public RsGxsServerGrpUpdate
{
public:
    RsGxsServerGrpUpdateItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE) { clear();}
    RsGxsServerGrpUpdateItem(const RsGxsServerGrpUpdate& u,uint16_t serv_type) : RsGxsNetServiceItem(serv_type, RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE), RsGxsServerGrpUpdate(u) {}

    virtual ~RsGxsServerGrpUpdateItem() {}

	virtual void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsGxsMsgUpdate
{
public:
    struct MsgUpdateInfo
    {
        MsgUpdateInfo(): time_stamp(0), message_count(0) {}

        uint32_t time_stamp ;
        uint32_t message_count ;
    };

    std::map<RsGxsGroupId, MsgUpdateInfo> msgUpdateInfos;
};

class RsGxsMsgUpdateItem : public RsGxsNetServiceItem, public RsGxsMsgUpdate
{
public:
    RsGxsMsgUpdateItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_MSG_UPDATE) { clear();}
    RsGxsMsgUpdateItem(const RsGxsMsgUpdate& m,uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_MSG_UPDATE), RsGxsMsgUpdate(m) {}

    virtual ~RsGxsMsgUpdateItem() {}

    virtual void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    RsPeerId peerID;
};

class RsGxsServerMsgUpdate
{
public:
    RsGxsServerMsgUpdate() { msgUpdateTS = 0 ;}

	uint32_t msgUpdateTS; // local time stamp this group last received a new msg
};

class RsGxsServerMsgUpdateItem : public RsGxsNetServiceItem, public RsGxsServerMsgUpdate
{
public:
    RsGxsServerMsgUpdateItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE) { clear();}
    RsGxsServerMsgUpdateItem(const RsGxsServerMsgUpdate& m,uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE),RsGxsServerMsgUpdate(m) {}
    virtual ~RsGxsServerMsgUpdateItem() {}

	virtual void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsGxsGroupId grpId;
};


class RsGxsUpdateSerialiser : public RsServiceSerializer
{
public:

	RsGxsUpdateSerialiser(uint16_t servtype) : RsServiceSerializer(servtype), SERVICE_TYPE(servtype) {}

	virtual ~RsGxsUpdateSerialiser() {}

	virtual RsItem* create_item(uint16_t service,uint8_t item_subtype) const ;

	const uint16_t SERVICE_TYPE;
};




#endif /* RSGXSUPDATEITEMS_H_ */
