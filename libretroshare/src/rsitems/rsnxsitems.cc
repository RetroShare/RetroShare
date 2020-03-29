/*******************************************************************************
 * libretroshare/src/rsitems: rsnxsitems.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Christopher Evi-Parker,Robert Fernie<retroshare@lunamutt.com>*
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
#include "rsnxsitems.h"
#include "util/rsprint.h"
#include <iomanip>

#include "serialiser/rstypeserializer.h"

/***
 * #define RSSERIAL_DEBUG	1
 ***/

const uint8_t RsNxsSyncGrpItem::FLAG_REQUEST   = 0x001;
const uint8_t RsNxsSyncGrpItem::FLAG_RESPONSE  = 0x002;

const uint8_t RsNxsSyncMsgItem::FLAG_REQUEST   = 0x001;
const uint8_t RsNxsSyncMsgItem::FLAG_RESPONSE  = 0x002;

const uint8_t RsNxsSyncGrpItem::FLAG_USE_SYNC_HASH       = 0x0001;
const uint8_t RsNxsSyncMsgItem::FLAG_USE_SYNC_HASH       = 0x0001;

const uint8_t RsNxsSyncMsgReqItem::FLAG_USE_HASHED_GROUP_ID = 0x02;

/** transaction state **/
const uint16_t RsNxsTransacItem::FLAG_BEGIN_P1         = 0x0001;
const uint16_t RsNxsTransacItem::FLAG_BEGIN_P2         = 0x0002;
const uint16_t RsNxsTransacItem::FLAG_END_SUCCESS      = 0x0004;
const uint16_t RsNxsTransacItem::FLAG_CANCEL           = 0x0008;
const uint16_t RsNxsTransacItem::FLAG_END_FAIL_NUM     = 0x0010;
const uint16_t RsNxsTransacItem::FLAG_END_FAIL_TIMEOUT = 0x0020;
const uint16_t RsNxsTransacItem::FLAG_END_FAIL_FULL    = 0x0040;


/** transaction type **/
const uint16_t RsNxsTransacItem::FLAG_TYPE_GRP_LIST_RESP  = 0x0100;
const uint16_t RsNxsTransacItem::FLAG_TYPE_MSG_LIST_RESP  = 0x0200;
const uint16_t RsNxsTransacItem::FLAG_TYPE_GRP_LIST_REQ   = 0x0400;
const uint16_t RsNxsTransacItem::FLAG_TYPE_MSG_LIST_REQ   = 0x0800;
const uint16_t RsNxsTransacItem::FLAG_TYPE_GRPS           = 0x1000;
const uint16_t RsNxsTransacItem::FLAG_TYPE_MSGS           = 0x2000;
const uint16_t RsNxsTransacItem::FLAG_TYPE_ENCRYPTED_DATA = 0x4000;

RsItem *RsNxsSerialiser::create_item(uint16_t service_id,uint8_t item_subtype) const
{
    if(service_id != SERVICE_TYPE)
        return NULL ;

    switch(item_subtype)
    {
        case RS_PKT_SUBTYPE_NXS_SYNC_GRP_REQ_ITEM:   return new RsNxsSyncGrpReqItem(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_SYNC_GRP_ITEM:       return new RsNxsSyncGrpItem(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_SYNC_MSG_REQ_ITEM:   return new RsNxsSyncMsgReqItem(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_SYNC_MSG_ITEM:       return new RsNxsSyncMsgItem(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_GRP_ITEM:            return new RsNxsGrp(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_MSG_ITEM:            return new RsNxsMsg(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_TRANSAC_ITEM:        return new RsNxsTransacItem(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_GRP_PUBLISH_KEY_ITEM:return new RsNxsGroupPublishKeyItem(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_ENCRYPTED_DATA_ITEM: return new RsNxsEncryptedDataItem(SERVICE_TYPE) ;
        case RS_PKT_SUBTYPE_NXS_SYNC_GRP_STATS_ITEM: return new RsNxsSyncGrpStatsItem(SERVICE_TYPE) ;

        default:
                return NULL;
	}
}

void RsNxsSyncMsgItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,transactionNumber,"transactionNumber") ;
    RsTypeSerializer::serial_process<uint8_t> (j,ctx,flag             ,"flag") ;
    RsTypeSerializer::serial_process          (j,ctx,grpId            ,"grpId") ;
    RsTypeSerializer::serial_process          (j,ctx,msgId            ,"msgId") ;
    RsTypeSerializer::serial_process          (j,ctx,authorId         ,"authorId") ;
}

void RsNxsMsg::serial_process( RsGenericSerializer::SerializeJob j,
                               RsGenericSerializer::SerializeContext& ctx )
{
	RS_SERIAL_PROCESS(transactionNumber);
	RS_SERIAL_PROCESS(pos);
	RS_SERIAL_PROCESS(msgId);
	RS_SERIAL_PROCESS(grpId);
	RS_SERIAL_PROCESS(msg);
	RS_SERIAL_PROCESS(meta);
}

void RsNxsGrp::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,transactionNumber,"transactionNumber") ;
    RsTypeSerializer::serial_process<uint8_t>  (j,ctx,pos              ,"pos") ;
    RsTypeSerializer::serial_process           (j,ctx,grpId            ,"grpId") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,grp              ,"grp") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,meta             ,"meta") ;
}

void RsNxsSyncGrpStatsItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,request_type   ,"request_type") ;
    RsTypeSerializer::serial_process           (j,ctx,grpId          ,"grpId") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,number_of_posts,"number_of_posts") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,last_post_TS   ,"last_post_TS") ;
}

void RsNxsSyncGrpReqItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,transactionNumber,"transactionNumber") ;
    RsTypeSerializer::serial_process<uint8_t> (j,ctx,flag             ,"flag") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,createdSince     ,"createdSince") ;
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_HASH_SHA1,syncHash,"syncHash") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,updateTS         ,"updateTS") ;
}

void RsNxsTransacItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,transactionNumber,"transactionNumber") ;
    RsTypeSerializer::serial_process<uint16_t>(j,ctx,transactFlag     ,"transactFlag") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,nItems           ,"nItems") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,updateTS         ,"updateTS") ;
}
void RsNxsSyncGrpItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,transactionNumber,"transactionNumber") ;
    RsTypeSerializer::serial_process<uint8_t> (j,ctx,flag             ,"flag") ;
    RsTypeSerializer::serial_process          (j,ctx,grpId            ,"grpId") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,publishTs        ,"publishTs") ;
    RsTypeSerializer::serial_process          (j,ctx,authorId         ,"authorId") ;
}
void RsNxsSyncMsgReqItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,transactionNumber,"transactionNumber") ;
    RsTypeSerializer::serial_process<uint8_t> (j,ctx,flag             ,"flag") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,createdSinceTS   ,"createdSinceTS") ;
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_HASH_SHA1,syncHash,"syncHash") ;
    RsTypeSerializer::serial_process          (j,ctx,grpId            ,"grpId") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,updateTS         ,"updateTS") ;
}
void RsNxsGroupPublishKeyItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process           (j,ctx,grpId            ,"grpId") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,private_key      ,"private_key") ;
}
void RsNxsEncryptedDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,transactionNumber,"transactionNumber") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,encrypted_data,   "encrypted_data") ;
}

int RsNxsGrp::refcount = 0;
/** print and clear functions **/
int RsNxsMsg::refcount = 0;
void RsNxsMsg::clear()
{

    msg.TlvClear();
    meta.TlvClear();
}

void RsNxsGrp::clear()
{
	grpId.clear();
	grp.TlvClear();
	meta.TlvClear();
}

RsNxsGrp* RsNxsGrp::clone() const {
	RsNxsGrp* grp = new RsNxsGrp(this->grp.tlvtype);
	*grp = *this;

	if(this->metaData)
	{
		grp->metaData = new RsGxsGrpMetaData();
		*(grp->metaData) = *(this->metaData);
	}

	return grp;
}

void RsNxsSyncGrpReqItem::clear()
{
    flag = 0;
    createdSince = 0;
    syncHash.clear();
    updateTS = 0;
}
void RsNxsGroupPublishKeyItem::clear()
{
    private_key.TlvClear();
}
void RsNxsSyncMsgReqItem::clear()
{
    grpId.clear();
    flag = 0;
    createdSinceTS = 0;
    syncHash.clear();
    updateTS = 0;
}
void RsNxsSyncGrpItem::clear()
{
    flag = 0;
    publishTs = 0;
    grpId.clear();
    authorId.clear();
}

void RsNxsSyncMsgItem::clear()
{
    flag = 0;
    msgId.clear();
    grpId.clear();
    authorId.clear();
}

void RsNxsTransacItem::clear(){
    transactFlag = 0;
    nItems = 0;
    updateTS = 0;
    timestamp = 0;
    transactionNumber = 0;
}
void RsNxsEncryptedDataItem::clear(){
    encrypted_data.TlvClear() ;
}

#ifdef SUSPENDED_CODE_27042017
void RsNxsSessionKeyItem::clear()
{
    for(std::map<RsGxsId,RsTlvBinaryData>::iterator it(encrypted_session_keys.begin());it!=encrypted_session_keys.end();++it)
        it->second.TlvClear() ;

    encrypted_session_keys.clear() ;
}
#endif


