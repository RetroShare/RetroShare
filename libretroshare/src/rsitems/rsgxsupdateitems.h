/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsupdateitems.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Christopher Evi-Parker                                       *
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
#ifndef RSGXSUPDATEITEMS_H_
#define RSGXSUPDATEITEMS_H_

#include "gxs/rsgxs.h"
#include "gxs/rsgxsdata.h"
#include "gxs/rsgxsnettunnel.h"
#include "serialiser/rstlvidset.h"
#include "serialiser/rstypeserializer.h"
#include "serialiser/rsserializable.h"


const uint8_t RS_PKT_SUBTYPE_GXS_GRP_UPDATE             = 0x01;
const uint8_t RS_PKT_SUBTYPE_GXS_MSG_UPDATE_deprecated  = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXS_MSG_UPDATE             = 0x03;
const uint8_t RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE      = 0x04;
const uint8_t RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE      = 0x08;
const uint8_t RS_PKT_SUBTYPE_GXS_GRP_CONFIG             = 0x09;
const uint8_t RS_PKT_SUBTYPE_GXS_RANDOM_BIAS            = 0x0a;

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
		statistics_update_TS = 0 ;
		last_group_modification_TS = 0 ;
	}

	uint32_t     msg_keep_delay ;	// delay after which we discard the posts
	uint32_t     msg_send_delay ;	// delay after which we dont send the posts anymore
	uint32_t     msg_req_delay ;	// delay after which we dont get the posts from friends

	RsTlvPeerIdSet suppliers;		// list of friends who feed this group
	uint32_t max_visible_count ;	// max visible count reported by contributing friends
	rstime_t statistics_update_TS ;	// last time the max visible count was updated.
	rstime_t last_group_modification_TS ;	// last time the group was modified, either in meta data or in the list of messages posted in it.
};

class RsGxsGrpConfigItem : public RsGxsNetServiceItem, public RsGxsGrpConfig
{
public:
    explicit RsGxsGrpConfigItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_GRP_CONFIG) {}
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
    explicit RsGxsGrpUpdateItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_GRP_UPDATE) {clear();}
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
    explicit RsGxsServerGrpUpdateItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE) { clear();}
    RsGxsServerGrpUpdateItem(const RsGxsServerGrpUpdate& u,uint16_t serv_type) : RsGxsNetServiceItem(serv_type, RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE), RsGxsServerGrpUpdate(u) {}

    virtual ~RsGxsServerGrpUpdateItem() {}

	virtual void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsGxsMsgUpdate
{
public:
	struct MsgUpdateInfo : RsSerializable
    {
        MsgUpdateInfo(): time_stamp(0), message_count(0) {}

        uint32_t time_stamp ;
        uint32_t message_count ;

		/// @see RsSerializable
		void serial_process(RsGenericSerializer::SerializeJob j,
		                    RsGenericSerializer::SerializeContext& ctx)
		{
			RS_SERIAL_PROCESS(time_stamp);
			RS_SERIAL_PROCESS(message_count);
		}
    };

    std::map<RsGxsGroupId, MsgUpdateInfo> msgUpdateInfos;
};


class RsGxsMsgUpdateItem : public RsGxsNetServiceItem, public RsGxsMsgUpdate
{
public:
    explicit RsGxsMsgUpdateItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_MSG_UPDATE) { clear();}
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
    explicit RsGxsServerMsgUpdateItem(uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE) { clear();}
    RsGxsServerMsgUpdateItem(const RsGxsServerMsgUpdate& m,uint16_t servType) : RsGxsNetServiceItem(servType, RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE),RsGxsServerMsgUpdate(m) {}
    virtual ~RsGxsServerMsgUpdateItem() {}

	virtual void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsGxsGroupId grpId;
};

class RsGxsUpdateSerialiser : public RsServiceSerializer
{
public:

	explicit RsGxsUpdateSerialiser(uint16_t servtype) : RsServiceSerializer(servtype), SERVICE_TYPE(servtype) {}

	virtual ~RsGxsUpdateSerialiser() {}

	virtual RsItem* create_item(uint16_t service,uint8_t item_subtype) const ;

	const uint16_t SERVICE_TYPE;
};




#endif /* RSGXSUPDATEITEMS_H_ */
