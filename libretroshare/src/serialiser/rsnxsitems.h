#ifndef RSNXSITEMS_H
#define RSNXSITEMS_H

/*
 * libretroshare/src/serialiser: rsnxssitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012 Christopher Evi-Parker, Robert Fernie.
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


const uint8_t RS_PKT_SUBTYPE_SYNC_GRP      = 0x0001;
const uint8_t RS_PKT_SUBTYPE_SYNC_GRP_LIST = 0x0002;
const uint8_t RS_PKT_SUBTYPE_GRPS_REQ      = 0x0004;
const uint8_t RS_PKT_SUBTYPE_GRPS_RESP     = 0x0008;
const uint8_t RS_PKT_SUBTYPE_SYNC_MSG      = 0x0010;
const uint8_t RS_PKT_SUBTYPE_SYNC_MSG_LIST = 0x0020;
const uint8_t RS_PKT_SUBTYPE_MSG_REQ       = 0x0040;
const uint8_t RS_PKT_SUBTYPE_MSG_RESP      = 0x0080;
const uint8_t RS_PKT_SUBTYPE_SEARCH_REQ    = 0x0100;
const uint8_t RS_PKT_SUBTYPE_SEARCH_RESP   = 0x0200;


/*!
 * Base class for Network exchange service
 * Main purpose is rtti based routing of for serialisation
 * and deserialisation
 *
 * Service type is set by plugin service
 */
class RsNxs : public RsItem
{

public:
    RsNxs(uint16_t servtype, uint8_t subtype)
        : RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype) { return; }
};


/*!
 * Use to request grp list from peer
 * Server may advise client peer to use sync file
 * while serving his request. This results
 */
class RsSyncGrp : RsNxs {

public:

    static const uint8_t FLAG_USE_SYNC_HASH;

    RsSyncGrp(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_SYNC_GRP) { return;}

    uint8_t flag; // advises whether to use sync hash
    uint32_t syncAge; // how far back to sync data
    std::string syncHash; // use to determine if changes that have occured since last hash


};

/*!
 * Use to send to peer list of grps
 * held by server peer
 */
class RsSyncGrpList : public RsNxs
{

public:
    RsSynchGrpList(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_SYNC_GRP_LIST) { return ; }

    /// groups held by sending peer
    std::map<std::string, std::list<uint32_t> > grps; // grpId/ versions pair

};

/*!
 * Use to send to peer list grps
 * wanted to by server peer
 */
class RsGrpReq : public RsNxs
{
public:

    RsGrpReq(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_GRPS_REQ) { return; }

    std::map<std::string, std::list<uint32_t> > grps; // grp/versions pair
};

/*!
 * Contains serialised group items
 * Each item corresponds to a group which needs to be
 * deserialised
 */
class RsGrpResp : public RsNxs
{

public:
    RsGrpResp(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_GRPS_RESP) { return; }

    std::list<RsTlvBinaryData> grps; /// each entry corresponds to a group item

};

/*!
 * Use to request list of msg held by peer
 * for a given group
 */
class RsSyncGrpMsg : public RsNxs
{

public:

    RsSyncGrpMsgs(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_SYNC_MSG) {return; }

    std::string grpId;
    uint32_t flag;
    uint32_t syncHash;
};

/*!
 * Use to send list msgs for a group held by
 * a peer
 */
class RsSyncGrpMsgList : public RsNxs
{
public:

    RsSyncGrpMsgList(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_SYNC_MSG_LIST) { return; }

    std::string grpId;
    std::map<std::string, std::list<uint32_t> > msgs; // msg/versions pairs

};

/*!
 * Use to request msgs from a peer
 * for a given group
 */
class RsGrpMsgReq : public RsNxs
{
public:

    RsGrpMsgReq(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_MSG_REQ) { return; }

    std::string grpId;
    std::map<std::string, std::list<uint32_t> > msgs;
};

/*!
 * Used to respond to a RsGrpMsgsReq
 * with message items satisfying request
 */
class RsGrpMsgResp : public RsNxs
{
public:

    RsGrpMsgResp(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_MSG_RESP) { return; }

    std::list<RsTlvBinaryData> msgs; // each entry corresponds to a message item
};

/*!
 * Used to request a search of user data
 */
class RsNxsSearchReq : public RsNxs
{
public:

    RsNxsSearchReq(uint16_t servtype): RsNxs(servtype, RS_PKT_SUBTYPE_SEARCH_REQ) { return; }
    uint16_t token;
    RsTlvBinaryData searchTerm; // service aware of item class
};


/*!
 * Used to respond to a RsGrpSearchReq
 * with grpId/MsgIds that satisfy search request
 */
class RsNxsSearchResp : public RsNxs
{
public:

    RsNxsSearchResp(uint16_t servtype) : RsNxs(servtype, RS_PKT_SUBTYPE_SEARCH_RESP) { return; }

    uint16_t token; // search token to be redeemed
    std::list<RsTlvBinaryData> msgs; // msgs with search terms present
    std::list<RsTlvBinaryData> grps; // grps with search terms present
};


class RsNxsSerialiser : public RsSerialType
{

    RsNxsSerialiser(uint16_t servtype) :
            RsSerialType(RS_PKT_VERSION_SERVICE, servtype) { return; }

    virtual ~RsNxsSerialiser() { return; }

    virtual uint32_t size(RsItem *);
    virtual bool serialise(RsItem *item, void *data, uint32_t *size);
    virtual RsItem* deserialise(void *data, uint32_t *size);

private:


    /* for RS_PKT_SUBTYPE_SYNC_GRP */

    virtual uint32_t sizeSyncGrp(RsItem* item);
    virtual bool serialiseSyncGrp(RsSyncGrp *item, void *data, uint32_t *size);
    virtual RsSyncGrp* deserialise(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_SYNC_GRP_LIST */

    virtual uint32_t sizeSyncGrpList(RsItem* item);
    virtual bool serialiseSyncGrpList(RsSyncGrpList *item, void *data, uint32_t *size);
    virtual RsSyncGrpList* deserialise(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GRPS_REQ */

    virtual uint32_t sizeGrpReq(RsGrpReq* item);
    virtual bool serialiseGrpReq(RsGrpReq *item, void *data, uint32_t *size);
    virtual RsGrpReq* deserialise(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_GRPS_RESP */

    virtual uint32_t sizeGrpResp(RsGrpResp* item);
    virtual bool serialiseGrpResp(RsGrpResp *item, void *data, uint32_t *size);
    virtual RsGrpResp* deserialise(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_SYNC_MSG */

    virtual uint32_t sizeSyncGrpMsg(RsSyncGrpMsg* item);
    virtual bool serialiseSyncGrpMsg(RsItem *item, void *data, uint32_t *size);
    virtual RsSyncGrpMsg* deserialise(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_SYNC_MSG_LIST */

    virtual uint32_t sizeSynGrpMsgList(RsSyncGrpMsgList* item);
    virtual bool serialiseSynGrpMsgList(RsSyncGrpMsgList* item, void *data, uint32_t* size);
    virtual RsSyncGrpMsgList* deserialise(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_MSG_REQ */

    virtual uint32_t sizeGrpMsgReq(RsGrpMsgReq* item);
    virtual bool serialiseGrpMsgReq(RsGrpMsgReq* item, void* data, uint32_t* size);
    virtual RsGrpMsgReq deserialise(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_MSG_RESP */

    virtual uint32_t sizeGrpMsgResp(RsGrpMsgResp* item);
    virtual bool serialiseGrpMsgResp(RsGrpMsgResp* item, void* data, uint32_t* size);
    virtual RsGrpMsgResp* deserialise(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_SEARCH_REQ */

    virtual uint32_t sizeNxsSearchReq(RsNxsSearchReq* item);
    virtual bool serialiseNxsSearchReq(RsNxsSearchReq* item, void* data, uint32_t* size);
    virtual RsNxsSearchResp* deserialise(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_SEARCH_RESP */

    virtual uint32_t sizeNxsSearchResp(RsNxsSearchResp *item);
    virtual bool serialiseNxsSearchResp(RsNxsSearchResp *item, void *data, uint32_t *size);
    virtual RsNxsSearchResp* deserialise(void *data, uint32_t *size);

};

#endif // RSNXSITEMS_H
