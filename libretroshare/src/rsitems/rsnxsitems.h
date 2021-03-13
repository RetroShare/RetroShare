/*******************************************************************************
 * libretroshare/src/rsitems: rsnxsitems.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012  Christopher Evi-Parker                                  *
 * Copyright (C) 2012  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2021  Gioacchino Mazzurco <gio@altermundi.net>                *
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
#include <openssl/ssl.h>

#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvitem.h"
#include "serialiser/rstlvkeys.h"
#include "gxs/rsgxsdata.h"

enum class RsNxsSubtype : uint8_t
{
	PULL_REQUEST = 0x90 /// @see RsNxsPullRequestItem
};

// These items have "flag type" numbers, but this is not used.
// TODO: refactor as C++11 enum class
const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_GRP_REQ_ITEM    = 0x01;
const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM        = 0x02;
const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_GRP_STATS_ITEM  = 0x03;
const uint8_t RS_PKT_SUBTYPE_NXS_GRP_ITEM             = 0x04;
const uint8_t RS_PKT_SUBTYPE_NXS_ENCRYPTED_DATA_ITEM  = 0x05;
const uint8_t RS_PKT_SUBTYPE_NXS_SESSION_KEY_ITEM     = 0x06;
const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM        = 0x08;
const uint8_t RS_PKT_SUBTYPE_NXS_SYNC_MSG_REQ_ITEM    = 0x10;
const uint8_t RS_PKT_SUBTYPE_NXS_MSG_ITEM             = 0x20;
const uint8_t RS_PKT_SUBTYPE_NXS_TRANSAC_ITEM         = 0x40;
const uint8_t RS_PKT_SUBTYPE_NXS_GRP_PUBLISH_KEY_ITEM = 0x80;


#ifdef RS_DEAD_CODE
// possibility create second service to deal with this functionality
const uint8_t RS_PKT_SUBTYPE_EXT_SEARCH_GRP   = 0x0001;
const uint8_t RS_PKT_SUBTYPE_EXT_SEARCH_MSG   = 0x0002;
const uint8_t RS_PKT_SUBTYPE_EXT_DELETE_GRP   = 0x0004;
const uint8_t RS_PKT_SUBTYPE_EXT_DELETE_MSG   = 0x0008;
const uint8_t RS_PKT_SUBTYPE_EXT_SEARCH_REQ   = 0x0010;
#endif // def RS_DEAD_CODE

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
	RsNxsItem(uint16_t servtype, uint8_t subtype):
	    RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype), transactionNumber(0)
	{ setPriorityLevel(QOS_PRIORITY_RS_GXS_NET); }

	virtual ~RsNxsItem() = default;

	uint32_t transactionNumber; // set to zero if this is not a transaction item
};


/*!
 * Use to request grp list from peer
 * Server may advise client peer to use sync file
 * while serving his request. This results
 */
class RsNxsSyncGrpReqItem : public RsNxsItem 
{
public:

	static const uint8_t FLAG_USE_SYNC_HASH;
	static const uint8_t FLAG_ONLY_CURRENT; // only send most current version of grps / ignores sync hash

	explicit RsNxsSyncGrpReqItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_GRP_REQ_ITEM) { clear();}
	virtual void clear() override;

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

	uint8_t flag; // advises whether to use sync hash
	uint32_t createdSince; // how far back to sync data
	uint32_t updateTS; // time of last group update
	std::string syncHash; // use to determine if changes that have occured since last hash
};

/*!
 * Use to request statistics about a particular group
 */
class RsNxsSyncGrpStatsItem : public RsNxsItem 
{
public:

	explicit RsNxsSyncGrpStatsItem(uint16_t servtype)
	  : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_GRP_STATS_ITEM)
	  , request_type(0), number_of_posts(0), last_post_TS(0)
	{}

    virtual void clear() override {}

    static const uint8_t GROUP_INFO_TYPE_REQUEST  = 0x01;
    static const uint8_t GROUP_INFO_TYPE_RESPONSE = 0x02; 
    
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

    uint32_t request_type;	// used to determine the type of request
    RsGxsGroupId grpId;		// id of the group
    uint32_t number_of_posts;	// number of posts in that group
    uint32_t last_post_TS; 	// time_stamp of last post
};

/*!
 * Use to request grp list from peer
 * Server may advise client peer to use sync file
 * while serving his request. This results
 */
class RsNxsGroupPublishKeyItem : public RsNxsItem
{
public:
	explicit RsNxsGroupPublishKeyItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_GRP_PUBLISH_KEY_ITEM) { clear(); }

    virtual void clear() override;

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

    RsGxsGroupId grpId ;
    RsTlvPrivateRSAKey private_key ;
};



/*!
 * This RsNxsItem is for use in enabling transactions
 * in order to guaranttee a collection of item have been
 * received
 */
class RsNxsTransacItem: public RsNxsItem {

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
    static const uint16_t FLAG_TYPE_ENCRYPTED_DATA;

    explicit RsNxsTransacItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_TRANSAC_ITEM) { clear(); }
    virtual ~RsNxsTransacItem() {}

    virtual void clear() override;

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

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
class RsNxsSyncGrpItem: public RsNxsItem
{

public:

    static const uint8_t FLAG_REQUEST;
    static const uint8_t FLAG_RESPONSE;
    static const uint8_t FLAG_USE_SYNC_HASH;

    explicit RsNxsSyncGrpItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM) { clear();}
    virtual ~RsNxsSyncGrpItem() {}

    virtual void clear() override;

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

    uint8_t flag; // request or response
    uint32_t publishTs; // to compare to Ts of receiving peer's grp of same id

    /// grpId of grp held by sending peer
    RsGxsGroupId grpId;
    RsGxsId authorId;

};

#ifdef SUSPENDED_CODE_27042017
/*!
 * Use to send to peer list of grps
 * held by server peer
 */
class RsNxsSessionKeyItem : public RsNxsItem
{

public:

    explicit RsNxsSessionKeyItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SESSION_KEY_ITEM) { clear(); }
    virtual ~RsNxsSessionKeyItem() {}

    virtual void clear() override;

    /// Session key encrypted for the whole group
    /// 
    uint8_t iv[EVP_MAX_IV_LENGTH] ;					// initialisation vector
    std::map<RsGxsId, RsTlvBinaryData> encrypted_session_keys;	// encrypted session keys
};
#endif
/*!
 * Use to send to peer list of grps
 * held by server peer
 */
class RsNxsEncryptedDataItem : public RsNxsItem
{

public:

	explicit RsNxsEncryptedDataItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_ENCRYPTED_DATA_ITEM),encrypted_data(servtype)
	{
		encrypted_data.tlvtype = TLV_TYPE_BIN_ENCRYPTED ;
		clear();
	}
    virtual ~RsNxsEncryptedDataItem() {}
    virtual void clear() override;

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

    /// grpId of grp held by sending peer
    /// 
    RsTlvBinaryData encrypted_data ;
};


/*!
 * Contains serialised group items
 * Each item corresponds to a group which needs to be
 * deserialised
 */
class RsNxsGrp : public RsNxsItem
{

public:

	explicit RsNxsGrp(uint16_t servtype)
	    : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_GRP_ITEM)
	    , pos(0), count(0), meta(servtype), grp(servtype), metaData(NULL)
	{ clear(); }
	virtual ~RsNxsGrp() { delete metaData; }

	RsNxsGrp* clone() const;

	virtual void clear() override;

	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx ) override;

	uint8_t pos; /// used for splitting up grp
	uint8_t count; /// number of split up messages
	RsGxsGroupId grpId; /// group Id, needed to complete version Id (ncvi)
	static int refcount;

	/*!
	 * This should contains all data
	 * which is not specific to the Gxs service data
	 */
	// This is the binary data for the group meta that is sent to friends. It *should not* contain any private
	// key parts. This is ensured in RsGenExchange

	RsTlvBinaryData meta;

	RsTlvBinaryData grp; /// actual group data

	// Deserialised metaData, this is not serialised by the serialize() method. So it may contain private key parts in some cases.
	RsGxsGrpMetaData* metaData;
};

/*!
 * Use to request list of msg held by peer
 * for a given group
 */
class RsNxsSyncMsgReqItem : public RsNxsItem
{

public:
 
#ifdef UNUSED_CODE
    static const uint8_t FLAG_USE_SYNC_HASH;
#endif
    static const uint8_t FLAG_USE_HASHED_GROUP_ID;

    explicit RsNxsSyncMsgReqItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_MSG_REQ_ITEM) { clear(); }

    virtual void clear() override;

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

    RsGxsGroupId grpId;
    uint8_t flag;
    uint32_t createdSinceTS;
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
    explicit RsNxsSyncMsgItem(uint16_t servtype) : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM) { clear(); }

    virtual void clear() override;

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

    uint8_t flag; // response/req
    RsGxsGroupId grpId;
    RsGxsMessageId msgId;
    RsGxsId authorId;

};

/*!
 * Used to request to a peer pull updates from us ASAP without waiting GXS sync
 * timer */
struct RsNxsPullRequestItem: RsItem
{
	explicit RsNxsPullRequestItem(RsServiceType servtype):
	    RsItem( RS_PKT_VERSION_SERVICE,
	            servtype,
	            static_cast<uint8_t>(RsNxsSubtype::PULL_REQUEST),
	            QOS_PRIORITY_RS_GXS_NET ) {}

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob,
	                     RsGenericSerializer::SerializeContext& ) override {}
};


/*!
 * Used to respond to a RsGrpMsgsReq
 * with message items satisfying request
 */
struct RsNxsMsg : RsNxsItem
{
	explicit RsNxsMsg(uint16_t servtype)
	  : RsNxsItem(servtype, RS_PKT_SUBTYPE_NXS_MSG_ITEM)
	  , pos(0), count(0), meta(servtype), msg(servtype), metaData(NULL)
	{ clear(); }
	virtual ~RsNxsMsg() { delete metaData; }

	virtual void clear() override;

	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx ) override;

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
	 * only client of API knows how to decode this
	 */
	RsTlvBinaryData msg;

	RsGxsMsgMetaData* metaData;
};

#ifdef RS_DEAD_CODE
/*!
 * Used to request a search of user data
 */
class RsNxsSearchReqItem : public RsNxsItem
{
public:

	explicit RsNxsSearchReqItem(uint16_t servtype)
	  : RsNxsItem(servtype, RS_PKT_SUBTYPE_EXT_SEARCH_REQ)
	  , nHops(0), token(0), serviceSearchItem(servtype), expiration(0)
	{}
	virtual ~RsNxsSearchReqItem() {}
	virtual void clear() override {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx) override;

    uint8_t nHops; /// how many peers to jump to
    uint32_t token; // search token
    RsTlvBinaryData serviceSearchItem; // service aware of item class
    uint32_t expiration; // expiration date
};
#endif //def RS_DEAD_CODE

#ifdef UNUSED_CODE

/*!
 * Used to respond to a RsGrpSearchReq
 * with grpId/MsgIds that satisfy search request
 */
class RsNxsSearchResultMsgItem
{
public:

    RsNxsSearchResultMsgItem()
		  : token(0), context(0), expiration(0)
		{}
    
    void clear() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

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
class RsNxsSearchResultGrpItem
{
public:

    RsNxsSearchResultGrpItem();
    
    void clear() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

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
#endif


class RsNxsSerialiser : public RsServiceSerializer
{
public:

	explicit RsNxsSerialiser(uint16_t servtype):
	    RsServiceSerializer(servtype), SERVICE_TYPE(servtype) {}
	virtual ~RsNxsSerialiser() = default;


    virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
protected:
    const uint16_t SERVICE_TYPE;
};
