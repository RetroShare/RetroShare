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
const uint8_t RS_PKT_SUBTYPE_NXS_GRP     = 0x0004;
const uint8_t RS_PKT_SUBTYPE_SYNC_MSG      = 0x0008;
const uint8_t RS_PKT_SUBTYPE_SYNC_MSG_LIST = 0x0010;
const uint8_t RS_PKT_SUBTYPE_NXS_MSG      = 0x0020;
const uint8_t RS_PKT_SUBTYPE_SEARCH_REQ    = 0x0040;

// possibility create second service to deal with this functionality

const uint8_t RS_PKT_SUBTYPE_NXS_EXTENDED      = 0x0080; // in order to extend supported pkt subtypes
const uint8_t RS_PKT_SUBTYPE_EXT_SEARCH_GRP   = 0x0001;
const uint8_t RS_PKT_SUBTYPE_EXT_SEARCH_MSG   = 0x0002;
const uint8_t RS_PKT_SUBTYPE_EXT_DELETE_GRP   = 0x0004;
const uint8_t RS_PKT_SUBTYPE_EXT_DELETE_MSG   = 0x0008;


/*!
 * Base class for Network exchange service
 * Main purpose is for rtti based routing used in the
 * serialisation and deserialisation of NXS packets
 *
 * Service type is set by plugin service
 */
class RsNxsItem : public RsItem
{

public:
    RsNxsItem(uint16_t servtype, uint8_t subtype)
        : RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype) { return; }

    virtual void clear() = 0;
    virtual std::ostream &print(std::ostream &out, uint16_t indent = 0) = 0;
};


/*!
 * Use to request grp list from peer
 * Server may advise client peer to use sync file
 * while serving his request. This results
 */
class RsSyncGrp : public RsNxsItem {

public:

    static const uint8_t FLAG_USE_SYNC_HASH;
    static const uint8_t FLAG_ONLY_CURRENT; // only send most current sycn list

    RsSyncGrp(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_SYNC_GRP) { return;}

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    uint8_t flag; // advises whether to use sync hash
    uint32_t syncAge; // how far back to sync data
    std::string syncHash; // use to determine if changes that have occured since last hash


};

typedef std::map<std::string, std::list<RsTlvKeySignature> > SyncList;

/*!
 * Use to send to peer list of grps
 * held by server peer
 */
class RsSyncGrpList : public RsNxsItem
{

public:

    static const uint8_t FLAG_REQUEST;
    static const uint8_t FLAG_RESPONSE;
    static const uint8_t FLAG_USE_SYNC_HASH;

    RsSyncGrpList(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_SYNC_GRP_LIST) { return ; }

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);


    uint8_t flag; // request or response

    /// groups held by sending peer
     SyncList grps; // grpId/ sign pair

};

/*!
 * Contains serialised group items
 * Each item corresponds to a group which needs to be
 * deserialised
 */
class RsNxsGrp : public RsNxsItem
{

public:

    RsNxsGrp(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_GRP), grp(servtype) { return; }


    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    std::string grpId; /// group Id, needed to complete version Id (ncvi)
    uint32_t timeStamp; /// UTC time, ncvi
    RsTlvBinaryData grp; /// actual group data
    RsTlvKeySignature adminSign; /// signature of admin (needed to complete version Id
    RsTlvSecurityKeySet keys;
    std::string identity;
    RsTlvKeySignature idSign; /// identity sign if applicable
    uint32_t grpFlag;
};

/*!
 * Use to request list of msg held by peer
 * for a given group
 */
class RsSyncGrpMsg : public RsNxsItem
{

public:

    static const uint8_t FLAG_USE_SYNC_HASH;

    RsSyncGrpMsg(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_SYNC_MSG) {return; }


    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    std::string grpId;
    uint8_t flag;
    uint32_t syncAge;
    std::string syncHash;
};

/*!
 * Use to send list msgs for a group held by
 * a peer
 */
class RsSyncGrpMsgList : public RsNxsItem
{
public:

    static const uint8_t FLAG_REQUEST;
    static const uint8_t FLAG_RESPONSE;
    static const uint8_t FLAG_USE_SYNC_HASH;
    RsSyncGrpMsgList(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_SYNC_MSG_LIST) { return; }


    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    uint8_t flag; // response/req
    std::string grpId;
    SyncList msgs; // msg/versions pairs

};


/*!
 * Used to respond to a RsGrpMsgsReq
 * with message items satisfying request
 */
class RsNxsMsg : public RsNxsItem
{
public:



    RsNxsMsg(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_MSG), msg(servtype) { return; }


    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    std::string grpId; /// group id, forms part of version id
    std::string msgId; /// msg id
    uint32_t msgFlag;
    uint32_t timeStamp; /// UTC time create,
    RsTlvBinaryData msg;
    RsTlvKeySignature publishSign; /// publish signature
    RsTlvKeySignature idSign; /// identity signature
    std::string identity;

};

/*!
 * Used to request a search of user data
 */
class RsNxsSearchReq : public RsNxsItem
{
public:

    RsNxsSearchReq(uint16_t servtype): RsNxsItem(servtype, RS_PKT_SUBTYPE_SEARCH_REQ), serviceSearchItem(servtype) { return; }
    virtual ~RsNxsSearchReq() { return;}

    virtual void clear() { return;}
    virtual std::ostream &print(std::ostream &out, uint16_t indent) { return out; }

    uint8_t nHops; /// how many peers to jump to
    uint32_t token; // search token
    RsTlvBinaryData serviceSearchItem; // service aware of item class
    uint32_t expiration; // expiration date
};


/*!
 * used to extend data types processed
 */
class RsNxsExtended : public RsNxsItem
{

public:

    RsNxsExtended(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_EXTENDED), extData(servtype) { return; }
    virtual ~RsNxsExtended() { return; }

    virtual void clear() {}
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    RsTlvBinaryData extData;
    uint32_t type;
};


/*!
 * Used to respond to a RsGrpSearchReq
 * with grpId/MsgIds that satisfy search request
 */
class RsNxsSearchResultMsg
{
public:

    RsNxsSearchResultMsg() : context(0) { return;}
    void clear() {}
    std::ostream &print(std::ostream &out, uint16_t indent) { return out; }

    uint32_t token; // search token to be redeemed
    RsTlvBinaryData context; // used by client service
    std::string msgId;
    std::string grpId;
    RsTlvKeySignature idSign;

    uint32_t expiration; // expiration date
};

/*!
 * Used to respond to a RsGrpSearchReq
 * with grpId/MsgIds that satisfy search request
 */
class RsNxsSearchResultGrp
{
public:

    RsNxsSearchResultGrp();
    void clear() {}
    std::ostream &print(std::ostream &out, uint16_t indent) { return out; }

    uint32_t token; // search token to be redeemed

    RsTlvBinaryData context; // used by client service

    std::string grpId;
    RsTlvKeySignature adminSign;

    uint32_t expiration; // expiration date
};


class RsNxsDeleteMsg
{
public:

    RsNxsDeleteMsg() { return; }

    std::string msgId;
    std::string grpId;
    RsTlvKeySignature idSign;
    RsTlvKeySignature deleteSign; // ( msgId + grpId + msg data ) sign //TODO: add warning not to place msgId+grpId in msg!

};

class RsNxsDeleteGrp
{
public:

    RsNxsDeleteGrp() { return;}

    std::string grpId;
    RsTlvKeySignature idSign;
    RsTlvKeySignature deleteSign; // (grpId + grp data) sign // TODO: add warning not to place grpId in msg
};



class RsNxsSerialiser : public RsSerialType
{
public:

    RsNxsSerialiser(uint16_t servtype) :
            RsSerialType(RS_PKT_VERSION_SERVICE, servtype), SERVICE_TYPE(servtype) { return; }

    virtual ~RsNxsSerialiser() { return; }

    virtual uint32_t size(RsItem *item);
    virtual bool serialise(RsItem *item, void *data, uint32_t *size);
    virtual RsItem* deserialise(void *data, uint32_t *size);

private:


    /* for RS_PKT_SUBTYPE_SYNC_GRP */

    virtual uint32_t sizeSyncGrp(RsSyncGrp* item);
    virtual bool serialiseSyncGrp(RsSyncGrp *item, void *data, uint32_t *size);
    virtual RsSyncGrp* deserialSyncGrp(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_SYNC_GRP_LIST */

    virtual uint32_t sizeSyncGrpList(RsSyncGrpList* item);
    virtual bool serialiseSyncGrpList(RsSyncGrpList *item, void *data, uint32_t *size);
    virtual RsSyncGrpList* deserialSyncGrpList(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_NXS_GRP */

    virtual uint32_t sizeNxsGrp(RsNxsGrp* item);
    virtual bool serialiseNxsGrp(RsNxsGrp *item, void *data, uint32_t *size);
    virtual RsNxsGrp* deserialNxsGrp(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_SYNC_MSG */

    virtual uint32_t sizeSyncGrpMsg(RsSyncGrpMsg* item);
    virtual bool serialiseSyncGrpMsg(RsSyncGrpMsg *item, void *data, uint32_t *size);
    virtual RsSyncGrpMsg* deserialSyncGrpMsg(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_SYNC_MSG_LIST */

    virtual uint32_t sizeSyncGrpMsgList(RsSyncGrpMsgList* item);
    virtual bool serialiseSynGrpMsgList(RsSyncGrpMsgList* item, void *data, uint32_t* size);
    virtual RsSyncGrpMsgList* deserialSyncGrpMsgList(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_NXS_MSG */

    virtual uint32_t sizeNxsMsg(RsNxsMsg* item);
    virtual bool serialiseNxsMsg(RsNxsMsg* item, void* data, uint32_t* size);
    virtual RsNxsMsg* deserialNxsMsg(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_SEARCH_REQ */
    virtual uint32_t sizeNxsSearchReq(RsNxsSearchReq* item);
    virtual bool serialiseNxsSearchReq(RsNxsSearchReq* item, void* data, uint32_t* size);
    virtual RsNxsSearchReq* deserialNxsSearchReq(void* data, uint32_t *size);

    /* RS_PKT_SUBTYPE_EXTENDED */
    virtual RsNxsExtended* deserialNxsExtended(void* data, uint32_t *size);
    virtual uint32_t sizeNxsExtended(RsNxsExtended* item);
    virtual bool serialiseNxsExtended(RsNxsExtended* item, void* data, uint32_t* size);

private:

    const uint16_t SERVICE_TYPE;
};


#endif // RSNXSITEMS_H
