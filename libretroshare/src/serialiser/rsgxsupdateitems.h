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


#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"
#include "gxs/rsgxsdata.h"


const uint8_t RS_PKT_SUBTYPE_GXS_GRP_UPDATE             = 0x0001;
const uint8_t RS_PKT_SUBTYPE_GXS_MSG_UPDATE             = 0x0002;
const uint8_t RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE      = 0x0004;
const uint8_t RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE      = 0x0008;

class RsGxsGrpUpdateItem : public RsItem {
public:
    RsGxsGrpUpdateItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE, servType,
                                                   RS_PKT_SUBTYPE_GXS_GRP_UPDATE)
    {}
            virtual ~RsGxsGrpUpdateItem() {}

	virtual void clear();
	virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::string peerId;
	uint32_t grpUpdateTS;
};

class RsGxsServerGrpUpdateItem : public RsItem {
public:
    RsGxsServerGrpUpdateItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE, servType,
                                                         RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE)
        {}
    virtual ~RsGxsServerGrpUpdateItem() {}

        virtual void clear();
        virtual std::ostream &print(std::ostream &out, uint16_t indent);

        uint32_t grpUpdateTS;
};

class RsGxsMsgUpdateItem : public RsItem
{
public:
    RsGxsMsgUpdateItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE, servType, RS_PKT_SUBTYPE_GXS_MSG_UPDATE)
    {}
    virtual ~RsGxsMsgUpdateItem() {}

	virtual void clear();
	virtual std::ostream &print(std::ostream &out, uint16_t indent);

	std::string peerId;
	std::map<std::string, uint32_t> msgUpdateTS;
};

class RsGxsServerMsgUpdateItem : public RsItem
{
public:
    RsGxsServerMsgUpdateItem(uint16_t servType) : RsItem(RS_PKT_VERSION_SERVICE,
                                                         servType, RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE)
    {}
    virtual ~RsGxsServerMsgUpdateItem() {}

        virtual void clear();
        virtual std::ostream &print(std::ostream &out, uint16_t indent);

        std::string grpId;
        uint32_t msgUpdateTS; // the last time this group received a new msg
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

    virtual uint32_t sizeGxsGrpUpdate(RsGxsGrpUpdateItem* item);
    virtual bool serialiseGxsGrpUpdate(RsGxsGrpUpdateItem *item, void *data, uint32_t *size);
    virtual RsGxsGrpUpdateItem* deserialGxsGrpUpddate(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GRP_SERVER_UPDATE_ITEM */

    virtual uint32_t sizeGxsServerGrpUpdate(RsGxsServerGrpUpdateItem* item);
    virtual bool serialiseGxsServerGrpUpdate(RsGxsServerGrpUpdateItem *item, void *data, uint32_t *size);
    virtual RsGxsServerGrpUpdateItem* deserialGxsServerGrpUpddate(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GXS_MSG_UPDATE_ITEM */

    virtual uint32_t sizeGxsMsgUpdate(RsGxsMsgUpdateItem* item);
    virtual bool serialiseGxsMsgUpdate(RsGxsMsgUpdateItem *item, void *data, uint32_t *size);
    virtual RsGxsMsgUpdateItem* deserialGxsMsgUpdate(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GXS_SERVER_UPDATE_ITEM */

    virtual uint32_t sizeGxsServerMsgUpdate(RsGxsServerMsgUpdateItem* item);
    virtual bool serialiseGxsServerMsgUpdate(RsGxsServerMsgUpdateItem *item, void *data, uint32_t *size);
    virtual RsGxsServerMsgUpdateItem* deserialGxsServerMsgUpdate(void *data, uint32_t *size);

private:

    const uint16_t SERVICE_TYPE;
};



#endif /* RSGXSUPDATEITEMS_H_ */
