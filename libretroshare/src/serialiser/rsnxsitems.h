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
#include "serialiser/rstlvitem.h"
#include "serialiser/rstlvkeys.h"
#include "gxs/rsgxsdata.h"


const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_GRP      = 0x0001;
const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM = 0x0002;
const uint8_t RS_PKT_SUBTYPE_NXS_GRP     = 0x0004;
const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM      = 0x0008;
const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_MSG = 0x0010;
const uint8_t RS_PKT_SUBTYPE_NXS_MSG      = 0x0020;
const uint8_t RS_PKT_SUBTYPE_NXS_TRANS      = 0x0040;


// possibility create second service to deal with this functionality

const uint8_t RS_PKT_SUBTYPE_EXT_SEARCH_GRP   = 0x0001;
const uint8_t RS_PKT_SUBTYPE_EXT_SEARCH_MSG   = 0x0002;
const uint8_t RS_PKT_SUBTYPE_EXT_DELETE_GRP   = 0x0004;
const uint8_t RS_PKT_SUBTYPE_EXT_DELETE_MSG   = 0x0008;
const uint8_t RS_PKT_SUBTYPE_EXT_SEARCH_REQ    = 0x0010;


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
        : RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype), transactionNumber(0)
    {
        setPriorityLevel(QOS_PRIORITY_RS_GXS_NET);
        return;
    }
    virtual ~RsNxsItem(){ return; }

    virtual void clear() = 0;
    virtual std::ostream &print(std::ostream &out, uint16_t indent = 0) = 0;

    uint32_t transactionNumber; // set to zero if this is not a transaction item
};


/*!
 * Use to request grp list from peer
 * Server may advise client peer to use sync file
 * while serving his request. This results
 */
class RsNxsSyncGrp : public RsNxsItem {

public:

    static const uint8_t FLAG_USE_SYNC_HASH;
    static const uint8_t FLAG_ONLY_CURRENT; // only send most current version of grps / ignores sync hash

    RsNxsSyncGrp(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_GRP) { clear(); return;}

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    uint8_t flag; // advises whether to use sync hash
    uint32_t createdSince; // how far back to sync data
    uint32_t updateTS; // time of last group update
    std::string syncHash; // use to determine if changes that have occured since last hash


};



/*!
 * This RsNxsItem is for use in enabling transactions
 * in order to guaranttee a collection of item have been
 * received
 */
class RsNxsTransac : public RsNxsItem {

public:

    static const uint16_t FLAG_STATE_MASK = 0xff;
    static const uint16_t FLAG_TYPE_MASK = 0xff00;

    /** transaction state **/
    static const uint16_t FLAG_BEGIN_P1;
    static const uint16_t FLAG_BEGIN_P2;
    static const uint16_t FLAG_END_SUCCESS;
    static const uint16_t FLAG_CANCEL;
    static const uint16_t FLAG_END_FAIL_NUM;
    static const uint16_t FLAG_END_FAIL_TIMEOUT;
    static const uint16_t FLAG_END_FAIL_FULL;


    /** transaction type **/
    static const uint16_t FLAG_TYPE_GRP_LIST_RESP;
    static const uint16_t FLAG_TYPE_MSG_LIST_RESP;
    static const uint16_t FLAG_TYPE_GRP_LIST_REQ;
    static const uint16_t FLAG_TYPE_MSG_LIST_REQ;
    static const uint16_t FLAG_TYPE_GRPS;
    static const uint16_t FLAG_TYPE_MSGS;

    RsNxsTransac(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_TRANS) { clear(); return; }
    virtual ~RsNxsTransac() { return ; }

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    uint16_t transactFlag;
    uint32_t nItems;
    uint32_t updateTS;

    // not serialised
    uint32_t timestamp;
};

/*!
 * Use to send to peer list of grps
 * held by server peer
 */
class RsNxsSyncGrpItem : public RsNxsItem
{

public:

    static const uint8_t FLAG_REQUEST;
    static const uint8_t FLAG_RESPONSE;
    static const uint8_t FLAG_USE_SYNC_HASH;

    RsNxsSyncGrpItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM) { clear(); return ; }
    virtual ~RsNxsSyncGrpItem() { return; }

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);


    uint8_t flag; // request or response

    uint32_t publishTs; // to compare to Ts of receiving peer's grp of same id

    /// grpId of grp held by sending peer
    RsGxsGroupId grpId;
    RsGxsId authorId;

};

/*!
 * Contains serialised group items
 * Each item corresponds to a group which needs to be
 * deserialised
 */
class RsNxsGrp : public RsNxsItem
{

public:

    RsNxsGrp(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_GRP), grp(servtype), meta(servtype),
    metaData(NULL) { clear();
    //std::cout << "\nGrp refcount++ : " << ++refcount << std::endl;
    return; }
    virtual ~RsNxsGrp() { if(metaData) delete metaData;
    //std::cout << "\nGrp refcount-- : " << --refcount << std::endl;
    }

    RsNxsGrp* clone() const;

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    RsGxsGroupId grpId; /// group Id, needed to complete version Id (ncvi)
    static int refcount;
    RsTlvBinaryData grp; /// actual group data
    uint8_t pos; /// used for splitting up grp
    uint8_t count; /// number of split up messages

    /*!
     * This should contains all data
     * which is not specific to the Gxs service data
     */
    RsTlvBinaryData meta;

    // deserialised metaData, this is not serialised
    RsGxsGrpMetaData* metaData;
};

/*!
 * Use to request list of msg held by peer
 * for a given group
 */
class RsNxsSyncMsg : public RsNxsItem
{

public:

    static const uint8_t FLAG_USE_SYNC_HASH;

    RsNxsSyncMsg(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_MSG) { clear(); return; }


    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    RsGxsGroupId grpId;
    uint8_t flag;
    uint32_t createdSince;
    uint32_t updateTS; // time of last update
    std::string syncHash;
};

/*!
 * Use to send list msgs for a group held by
 * a peer
 */
class RsNxsSyncMsgItem : public RsNxsItem
{
public:

    static const uint8_t FLAG_REQUEST;
    static const uint8_t FLAG_RESPONSE;
    static const uint8_t FLAG_USE_SYNC_HASH;
    RsNxsSyncMsgItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM) { clear(); return; }

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    uint8_t flag; // response/req
    RsGxsGroupId grpId;
    RsGxsMessageId msgId;
    RsGxsId authorId;

};


/*!
 * Used to respond to a RsGrpMsgsReq
 * with message items satisfying request
 */
class RsNxsMsg : public RsNxsItem
{
public:

    RsNxsMsg(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_MSG), meta(servtype), msg(servtype),
    metaData(NULL) {
    //	std::cout << "\nrefcount++ : " << ++refcount << std::endl;
    	clear(); return;
    }
    virtual ~RsNxsMsg()
    {
    	//std::cout << "\nrefcount-- : " << --refcount << std::endl;
    	if(metaData){
    		//std::cout << "\ndeleted\n";
    		delete metaData;
    	}
    }

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent);

    uint8_t pos; /// used for splitting up msg
    uint8_t count; /// number of split up messages
    RsGxsGroupId grpId; /// group id, forms part of version id
    RsGxsMessageId msgId; /// msg id
    static int refcount;

    /*!
     * This should contains all the data
     * which is not specific to the Gxs service data
     */
    RsTlvBinaryData meta;

    /*!
     * This contains Gxs specific data
     * only client of API knows who to decode this
     */
    RsTlvBinaryData msg;

    RsGxsMsgMetaData* metaData;

};

/*!
 * Used to request a search of user data
 */
class RsNxsSearchReq : public RsNxsItem
{
public:

    RsNxsSearchReq(uint16_t servtype): RsNxsItem(servtype, RS_PKT_SUBTYPE_EXT_SEARCH_REQ), serviceSearchItem(servtype) { return; }
    virtual ~RsNxsSearchReq() { return;}

    virtual void clear() { return;}
    virtual std::ostream &print(std::ostream &out, uint16_t /*indent*/) { return out; }

    uint8_t nHops; /// how many peers to jump to
    uint32_t token; // search token
    RsTlvBinaryData serviceSearchItem; // service aware of item class
    uint32_t expiration; // expiration date
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
    std::ostream &print(std::ostream &out, uint16_t /*indent*/) { return out; }

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
    std::ostream &print(std::ostream &out, uint16_t /*indent*/) { return out; }

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

    virtual uint32_t sizeNxsSyncGrp(RsNxsSyncGrp* item);
    virtual bool serialiseNxsSyncGrp(RsNxsSyncGrp *item, void *data, uint32_t *size);
    virtual RsNxsSyncGrp* deserialNxsSyncGrp(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_SYNC_GRP_ITEM */

    virtual uint32_t sizeNxsSyncGrpItem(RsNxsSyncGrpItem* item);
    virtual bool serialiseNxsSyncGrpItem(RsNxsSyncGrpItem *item, void *data, uint32_t *size);
    virtual RsNxsSyncGrpItem* deserialNxsSyncGrpItem(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_NXS_GRP */

    virtual uint32_t sizeNxsGrp(RsNxsGrp* item);
    virtual bool serialiseNxsGrp(RsNxsGrp *item, void *data, uint32_t *size);
    virtual RsNxsGrp* deserialNxsGrp(void *data, uint32_t *size);

    /* for RS_PKT_SUBTYPE_SYNC_MSG */

    virtual uint32_t sizeNxsSyncMsg(RsNxsSyncMsg* item);
    virtual bool serialiseNxsSyncMsg(RsNxsSyncMsg *item, void *data, uint32_t *size);
    virtual RsNxsSyncMsg* deserialNxsSyncMsg(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_SYNC_MSG_ITEM */

    virtual uint32_t sizeNxsSyncMsgItem(RsNxsSyncMsgItem* item);
    virtual bool serialiseNxsSynMsgItem(RsNxsSyncMsgItem* item, void *data, uint32_t* size);
    virtual RsNxsSyncMsgItem* deserialNxsSyncMsgItem(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_NXS_MSG */

    virtual uint32_t sizeNxsMsg(RsNxsMsg* item);
    virtual bool serialiseNxsMsg(RsNxsMsg* item, void* data, uint32_t* size);
    virtual RsNxsMsg* deserialNxsMsg(void *data, uint32_t *size);

    /* RS_PKT_SUBTYPE_NXS_TRANS */
    virtual uint32_t sizeNxsTrans(RsNxsTransac* item);
    virtual bool serialiseNxsTrans(RsNxsTransac* item, void* data, uint32_t* size);
    virtual RsNxsTransac* deserialNxsTrans(void* data, uint32_t *size);

private:

    const uint16_t SERVICE_TYPE;
};


#endif // RSNXSITEMS_H
