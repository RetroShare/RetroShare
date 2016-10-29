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
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"
#endif

#include "gxs/rsgxsdata.h"
#include "serialiser/rstlvidset.h"


const uint8_t RS_PKT_SUBTYPE_GXS_GRP_UPDATE             = 0x01;
const uint8_t RS_PKT_SUBTYPE_GXS_MSG_UPDATE_deprecated  = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXS_MSG_UPDATE             = 0x03;
const uint8_t RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE      = 0x04;
const uint8_t RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE      = 0x08;
const uint8_t RS_PKT_SUBTYPE_GXS_GRP_CONFIG             = 0x09;

class RsGxsGrpConfigItem : public RsItem {
public:
    RsGxsGrpConfigItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE, servType, RS_PKT_SUBTYPE_GXS_GRP_CONFIG) {}
    virtual ~RsGxsGrpConfigItem() {}

    virtual void clear() {}
    virtual std::ostream &print(std::ostream &out, uint16_t indent) { return out;}

    RsGxsGroupId grpId ;
    uint32_t     msg_keep_delay ;
    uint32_t     msg_send_delay ;
    uint32_t     msg_req_delay ;

    RsTlvPeerIdSet suppliers;
    uint32_t max_visible_count ;
    time_t update_TS ;
};

class RsGxsGrpUpdateItem : public RsItem {
public:
    RsGxsGrpUpdateItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE, servType,
                                                   RS_PKT_SUBTYPE_GXS_GRP_UPDATE)
    {clear();}
    virtual ~RsGxsGrpUpdateItem() {}

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    RsPeerId peerId;
    uint32_t grpUpdateTS;
};

class RsGxsServerGrpUpdateItem : public RsItem {
public:
    RsGxsServerGrpUpdateItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE, servType,
                                                         RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE)
        { clear();}
    virtual ~RsGxsServerGrpUpdateItem() {}

        virtual void clear();
        virtual std::ostream &print(std::ostream &out, uint16_t indent);

        uint32_t grpUpdateTS;
};

class RsGxsMsgUpdateItem : public RsItem
{
public:
    RsGxsMsgUpdateItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE, servType, RS_PKT_SUBTYPE_GXS_MSG_UPDATE)
    { clear();}
    virtual ~RsGxsMsgUpdateItem() {}

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    struct MsgUpdateInfo
    {
        MsgUpdateInfo(): time_stamp(0), message_count(0) {}
        
        uint32_t time_stamp ;
        uint32_t message_count ;
    };

    RsPeerId peerId;
    std::map<RsGxsGroupId, MsgUpdateInfo> msgUpdateInfos;
};

class RsGxsServerMsgUpdateItem : public RsItem
{
public:
    RsGxsServerMsgUpdateItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE,
                                                         servType, RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE)
    { clear();}
    virtual ~RsGxsServerMsgUpdateItem() {}

        virtual void clear();
        virtual std::ostream &print(std::ostream &out, uint16_t indent);

        RsGxsGroupId grpId;
        uint32_t msgUpdateTS; // local time stamp this group last received a new msg
};


class RsGxsUpdateSerialiser : public RsSerialType
{
public:

	RsGxsUpdateSerialiser(uint16_t servtype) :
            RsSerialType(RS_PKT_VERSION_SERVICE, servtype), SERVICE_TYPE(servtype) { return; }

    virtual ~RsGxsUpdateSerialiser() { return; }

    virtual uint32_t size(RsItem *item);
    virtual bool serialise(RsItem *item, void *data, uint32_t *size);
    virtual RsItem* deserialise(void *data, uint32_t *size);

private:


    /* for RS_PKT_SUBTYPE_GRP_UPDATE_ITEM */

    uint32_t sizeGxsGrpUpdate(RsGxsGrpUpdateItem* item);
    bool serialiseGxsGrpUpdate(RsGxsGrpUpdateItem *item, void *data, uint32_t *size);
    RsGxsGrpUpdateItem* deserialGxsGrpUpddate(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GRP_SERVER_UPDATE_ITEM */

    uint32_t sizeGxsServerGrpUpdate(RsGxsServerGrpUpdateItem* item);
    bool serialiseGxsServerGrpUpdate(RsGxsServerGrpUpdateItem *item, void *data, uint32_t *size);
    RsGxsServerGrpUpdateItem* deserialGxsServerGrpUpddate(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GXS_MSG_UPDATE_ITEM */

    uint32_t sizeGxsMsgUpdate(RsGxsMsgUpdateItem* item);
    bool serialiseGxsMsgUpdate(RsGxsMsgUpdateItem *item, void *data, uint32_t *size);
    RsGxsMsgUpdateItem* deserialGxsMsgUpdate(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GXS_SERVER_UPDATE_ITEM */

    uint32_t sizeGxsServerMsgUpdate(RsGxsServerMsgUpdateItem* item);
    bool serialiseGxsServerMsgUpdate(RsGxsServerMsgUpdateItem *item, void *data, uint32_t *size);
    RsGxsServerMsgUpdateItem* deserialGxsServerMsgUpdate(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GXS_CONFIG */

    uint32_t sizeGxsGrpConfig(RsGxsGrpConfigItem* item);
    bool serialiseGxsGrpConfig(RsGxsGrpConfigItem *item, void *data, uint32_t *size);
    RsGxsGrpConfigItem* deserialGxsGrpConfig(void *data, uint32_t *size);

private:

    const uint16_t SERVICE_TYPE;
};



#endif /* RSGXSUPDATEITEMS_H_ */
