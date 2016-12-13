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

#include "rsgxsupdateitems.h"
#include "rsbaseserial.h"

/**********************************************************************************************/
/*                                         CLEAR                                              */
/**********************************************************************************************/

void RsGxsGrpUpdateItem::clear()
{
	grpUpdateTS = 0;
	peerID.clear();
}

void RsGxsMsgUpdateItem::clear()
{
    msgUpdateInfos.clear();
    peerID.clear();
}

void RsGxsServerMsgUpdateItem::clear()
{
    msgUpdateTS = 0;
    grpId.clear();
}

void RsGxsServerGrpUpdateItem::clear()
{
    grpUpdateTS = 0;
}



/**********************************************************************************************/
/*                                         PRINT                                              */
/**********************************************************************************************/

std::ostream& RsGxsMsgUpdateItem::print(std::ostream& out, uint16_t indent)
{
    RsPeerId peerId;
    std::map<RsGxsGroupId, uint32_t> msgUpdateTS;

    printRsItemBase(out, "RsGxsMsgUpdateItem", indent);
    uint16_t int_Indent = indent + 2;
    out << "peerId: " << peerId << std::endl;
    printIndent(out, int_Indent);

    std::map<RsGxsGroupId, MsgUpdateInfo>::const_iterator cit = msgUpdateInfos.begin();
    out << "msgUpdateTS map:" << std::endl;
    int_Indent += 2;
    for(; cit != msgUpdateInfos.end(); ++cit)
    {
        out << "grpId: " << cit->first << std::endl;
        printIndent(out, int_Indent);
        out << "Msg time stamp: " << cit->second.time_stamp << std::endl;
        printIndent(out, int_Indent);
        out << "posts available: " << cit->second.message_count << std::endl;
        printIndent(out, int_Indent);
    }

	return out;
}
std::ostream& RsGxsGrpUpdateItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsGrpUpdateItem", indent);
    uint16_t int_Indent = indent + 2;
    out << "peerId: " << peerID << std::endl;
    printIndent(out, int_Indent);
    out << "grpUpdateTS: " << grpUpdateTS << std::endl;
    printIndent(out, int_Indent);
    return out ;
}

std::ostream& RsGxsServerMsgUpdateItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsServerMsgUpdateItem", indent);
    uint16_t int_Indent = indent + 2;
    out << "grpId: " << grpId << std::endl;
    printIndent(out, int_Indent);
    out << "msgUpdateTS: " << msgUpdateTS << std::endl;
    printIndent(out, int_Indent);
    return out;
}


std::ostream& RsGxsServerGrpUpdateItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsServerGrpUpdateItem", indent);
    uint16_t int_Indent = indent + 2;
    out << "grpUpdateTS: " << grpUpdateTS << std::endl;
    printIndent(out, int_Indent);

	return out;
}

/**********************************************************************************************/
/*                                         SERIALISER                                         */
/**********************************************************************************************/

bool RsGxsNetServiceItem::serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset) const
{
	tlvsize = serial_size() ;
	offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	if(!setRsItemHeader(data, tlvsize, PacketId(), tlvsize))
	{
		std::cerr << "RsFileTransferItem::serialise_header(): ERROR. Not enough size!" << std::endl;
		return false ;
	}
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif
	offset += 8;

	return true ;
}
RsItem* RsGxsUpdateSerialiser::deserialise(void* data, uint32_t* size)
{

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsGxsUpdateSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (SERVICE_TYPE != getRsItemService(rstype)))
		return NULL; /* wrong type */

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_GXS_MSG_UPDATE:        return deserialGxsMsgUpdate(data, size);
		case RS_PKT_SUBTYPE_GXS_GRP_UPDATE:        return deserialGxsGrpUpddate(data, size);
		case RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE: return deserialGxsServerGrpUpddate(data, size);
		case RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE: return deserialGxsServerMsgUpdate(data, size);
		case RS_PKT_SUBTYPE_GXS_GRP_CONFIG:        return deserialGxsGrpConfig(data, size);

		default:
		{
#	ifdef RSSERIAL_DEBUG
			std::cerr << "RsGxsUpdateSerialiser::deserialise() : data has no type"
			          << std::endl;
#	endif
			return NULL;

		}
	}
}

/**********************************************************************************************/
/*                                         SERIAL_SIZE()                                      */
/**********************************************************************************************/


uint32_t RsGxsGrpUpdateItem::serial_size() const
{
	uint32_t s = 8; // header size
    s += peerID.serial_size();
    s += 4;	// mUpdateTS
    return s;
}

uint32_t RsGxsServerGrpUpdateItem::serial_size() const
{
        uint32_t s = 8; // header size
        s += 4; // time stamp
        return s;
}

uint32_t RsGxsMsgUpdateItem::serial_size() const
{
    uint32_t s = 8; // header size
    s += peerID.serial_size() ;//GetTlvStringSize(item->peerId);

    s += msgUpdateInfos.size() * (4 + 4 + RsGxsGroupId::serial_size());
    s += 4; // number of map items

    return s;
}

uint32_t RsGxsServerMsgUpdateItem::serial_size() const
{
        uint32_t s = 8; // header size
        s += grpId.serial_size();
        s += 4; // grp TS

        return s;
}
uint32_t RsGxsGrpConfigItem::serial_size() const
{
        uint32_t s = 8; // header size
        s += grpId.serial_size();
        s += 4; // msg_keep_delay
        s += 4; // msg_send_delay
        s += 4; // msg_req_delay

        return s;
}

/**********************************************************************************************/
/*                                          SERIALISE()                                       */
/**********************************************************************************************/

bool RsGxsGrpUpdateItem::serialise(void* data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= peerID.serialise(data, size, offset) ;
    ok &= setRawUInt32(data, size, &offset, grpUpdateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsGrpUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

    return ok;
}

bool RsGxsServerGrpUpdateItem::serialise(void* data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    /* RsGxsServerGrpUpdateItem */

    ok &= setRawUInt32(data, size, &offset, grpUpdateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerGrpUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

    return ok;
}
bool RsGxsMsgUpdateItem::serialise(void* data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= peerID.serialise(data, size, offset) ;

    std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>::const_iterator cit(msgUpdateInfos.begin());

    uint32_t numItems = msgUpdateInfos.size();
    ok &= setRawUInt32(data, size, &offset, numItems);

    for(; cit != msgUpdateInfos.end(); ++cit)
    {
    	ok &= cit->first.serialise(data, size, offset);
        ok &= setRawUInt32(data, size, &offset, cit->second.time_stamp);
		ok &= setRawUInt32(data, size, &offset, cit->second.message_count);
    }

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsMsgUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

    return ok;
}

bool RsGxsServerMsgUpdateItem::serialise( void* data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= grpId.serialise(data, size, offset) ;
    ok &= setRawUInt32(data, size, &offset, msgUpdateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerMsgUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

    return ok;
}

bool RsGxsGrpConfigItem::serialise( void* data, uint32_t& size) const
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= grpId.serialise(data, size, offset) ;
    ok &= setRawUInt32(data, size, &offset, msg_keep_delay);
    ok &= setRawUInt32(data, size, &offset, msg_send_delay);
    ok &= setRawUInt32(data, size, &offset, msg_req_delay);

	if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerMsgUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

    return ok;
}


/**********************************************************************************************/
/*                                         DESERIALISE()                                      */
/**********************************************************************************************/

RsGxsGrpConfigItem* RsGxsUpdateSerialiser::deserialGxsGrpConfig(void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerGrpUpdate()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (SERVICE_TYPE != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_GXS_GRP_CONFIG != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsGrpUpdate() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsGrpUpdate() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsGrpConfigItem* item = new RsGxsGrpConfigItem(getRsItemService(rstype));

    /* skip the header */
    offset += 8;

    ok &= item->grpId.deserialise(data, *size, offset) ;
    ok &= getRawUInt32(data, *size, &offset, &(item->msg_keep_delay));
    ok &= getRawUInt32(data, *size, &offset, &(item->msg_send_delay));
    ok &= getRawUInt32(data, *size, &offset, &(item->msg_req_delay));

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxxGrpUpdate() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsGrpUpdate() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}
RsGxsGrpUpdateItem* RsGxsUpdateSerialiser::deserialGxsGrpUpddate(void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerGrpUpdate()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_GXS_GRP_UPDATE != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsGrpUpdate() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsGrpUpdate() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsGrpUpdateItem* item = new RsGxsGrpUpdateItem(getRsItemService(rstype));

    /* skip the header */
    offset += 8;

    ok &= item->peerID.deserialise(data, *size, offset) ;
    ok &= getRawUInt32(data, *size, &offset, &(item->grpUpdateTS));

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxxGrpUpdate() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsGrpUpdate() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}

RsGxsServerGrpUpdateItem* RsGxsUpdateSerialiser::deserialGxsServerGrpUpddate(void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerGrpUpdate()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (SERVICE_TYPE != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerGrpUpdate() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerGrpUpdate() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsServerGrpUpdateItem* item = new RsGxsServerGrpUpdateItem(getRsItemService(rstype));

    /* skip the header */
    offset += 8;

    ok &= getRawUInt32(data, *size, &offset, &(item->grpUpdateTS));

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerGrpUpdate() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerGrpUpdate() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}



RsGxsMsgUpdateItem* RsGxsUpdateSerialiser::deserialGxsMsgUpdate(void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::deserialGxsMsgUpdate()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (SERVICE_TYPE != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_GXS_MSG_UPDATE != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsMsgUpdate() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsMsgUpdate() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsMsgUpdateItem* item = new RsGxsMsgUpdateItem(getRsItemService(rstype));

    /* skip the header */
    offset += 8;

    ok &= item->peerID.deserialise(data, *size, offset) ;
    uint32_t numUpdateItems;
    ok &= getRawUInt32(data, *size, &offset, &(numUpdateItems));
    std::map<RsGxsGroupId, RsGxsMsgUpdateItem::MsgUpdateInfo>& msgUpdateInfos = item->msgUpdateInfos;
    RsGxsGroupId pId;

    RsGxsMsgUpdateItem::MsgUpdateInfo info ;

    for(uint32_t i = 0; i < numUpdateItems; i++)
    {
        ok &= pId.deserialise(data, *size, offset);

        if(!ok)
            break;

        ok &= getRawUInt32(data, *size, &offset, &(info.time_stamp));
        ok &= getRawUInt32(data, *size, &offset, &(info.message_count));

        if(!ok)
            break;

        msgUpdateInfos.insert(std::make_pair(pId, info));
    }

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsMsgUpdate() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsMsgUpdate() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}

RsGxsServerMsgUpdateItem* RsGxsUpdateSerialiser::deserialGxsServerMsgUpdate(void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerMsgUpdate()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (SERVICE_TYPE != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE != getRsItemSubType(rstype)))
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerMsgUpdate() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerMsgUpdate() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsServerMsgUpdateItem* item = new RsGxsServerMsgUpdateItem(getRsItemService(rstype));

    /* skip the header */
    offset += 8;

    ok &= item->grpId.deserialise(data, *size, offset) ;
    ok &= getRawUInt32(data, *size, &offset, &(item->msgUpdateTS));

    if (offset != rssize)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerMsgUpdate() FAIL size mismatch" << std::endl;
#endif
            /* error */
            delete item;
            return NULL;
    }

    if (!ok)
    {
#ifdef RSSERIAL_DEBUG
            std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerMsgUpdate() NOK" << std::endl;
#endif
            delete item;
            return NULL;
    }

    return item;
}

