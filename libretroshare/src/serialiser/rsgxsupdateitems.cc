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





void RsGxsGrpUpdateItem::clear()
{
	grpUpdateTS = 0;
	peerId.clear();
}

std::ostream& RsGxsGrpUpdateItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsGrpUpdateItem", indent);
    uint16_t int_Indent = indent + 2;
    out << "peerId: " << peerId << std::endl;
    printIndent(out, int_Indent);
    out << "grpUpdateTS: " << grpUpdateTS << std::endl;
    printIndent(out, int_Indent);
	return out ;
}



void RsGxsMsgUpdateItem::clear()
{
    msgUpdateTS.clear();
    peerId.clear();
}

std::ostream& RsGxsMsgUpdateItem::print(std::ostream& out, uint16_t indent)
{
    RsPeerId peerId;
    std::map<RsGxsGroupId, uint32_t> msgUpdateTS;

    printRsItemBase(out, "RsGxsMsgUpdateItem", indent);
    uint16_t int_Indent = indent + 2;
    out << "peerId: " << peerId << std::endl;
    printIndent(out, int_Indent);

    std::map<RsGxsGroupId, uint32_t>::const_iterator cit = msgUpdateTS.begin();
    out << "msgUpdateTS map:" << std::endl;
    int_Indent += 2;
    for(; cit != msgUpdateTS.end(); ++cit)
    {
    	out << "grpId: " << cit->first << std::endl;
		printIndent(out, int_Indent);
		out << "Msg time stamp: " << cit->second << std::endl;
		printIndent(out, int_Indent);
    }

	return out;
}



void RsGxsServerMsgUpdateItem::clear()
{
    msgUpdateTS = 0;
    grpId.clear();
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


void RsGxsServerGrpUpdateItem::clear()
{
    grpUpdateTS = 0;
}

std::ostream& RsGxsServerGrpUpdateItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsServerGrpUpdateItem", indent);
    uint16_t int_Indent = indent + 2;
    out << "grpUpdateTS: " << grpUpdateTS << std::endl;
    printIndent(out, int_Indent);

	return out;
}



uint32_t RsGxsUpdateSerialiser::size(RsItem* item)
{
	RsGxsMsgUpdateItem* mui = NULL;
	RsGxsGrpUpdateItem* gui = NULL;
        RsGxsServerGrpUpdateItem* gsui = NULL;
        RsGxsServerMsgUpdateItem* msui = NULL;

    if((mui = dynamic_cast<RsGxsMsgUpdateItem*>(item))  != NULL)
    {
        return sizeGxsMsgUpdate(mui);
    }else if(( gui = dynamic_cast<RsGxsGrpUpdateItem*>(item)) != NULL){
        return sizeGxsGrpUpdate(gui);
    }else if((gsui = dynamic_cast<RsGxsServerGrpUpdateItem*>(item)) != NULL)
    {
        return sizeGxsServerGrpUpdate(gsui);
    }else if((msui = dynamic_cast<RsGxsServerMsgUpdateItem*>(item)) != NULL)
    {
        return sizeGxsServerMsgUpdate(msui);
    }else
    {
#ifdef RSSERIAL_DEBUG
    	std::cerr << "RsGxsUpdateSerialiser::size(): Could not find appropriate size function"
    			  << std::endl;
#endif
    	return 0;
    }
}

bool RsGxsUpdateSerialiser::serialise(RsItem* item, void* data,
		uint32_t* size)
{
    RsGxsMsgUpdateItem* mui;
    RsGxsGrpUpdateItem* gui;
    RsGxsServerGrpUpdateItem* gsui;
    RsGxsServerMsgUpdateItem* msui;

    if((mui = dynamic_cast<RsGxsMsgUpdateItem*>(item)) != NULL)
        return serialiseGxsMsgUpdate(mui, data, size);
    else if((gui = dynamic_cast<RsGxsGrpUpdateItem*>(item)) != NULL)
        return serialiseGxsGrpUpdate(gui, data, size);
    else if((msui = dynamic_cast<RsGxsServerMsgUpdateItem*>(item)) != NULL)
        return serialiseGxsServerMsgUpdate(msui, data, size);
    else if((gsui = dynamic_cast<RsGxsServerGrpUpdateItem*>(item)) != NULL)
        return serialiseGxsServerGrpUpdate(gsui, data, size);
    else
    {
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::serialise() item does not caste to known type"
              << std::endl;
#endif

    return false;
    }
}

RsItem* RsGxsUpdateSerialiser::deserialise(void* data, uint32_t* size)
{

#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
                        (SERVICE_TYPE != getRsItemService(rstype)))
	{
			return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{

	case RS_PKT_SUBTYPE_GXS_MSG_UPDATE:
		return deserialGxsMsgUpdate(data, size);
	case RS_PKT_SUBTYPE_GXS_GRP_UPDATE:
		return deserialGxsGrpUpddate(data, size);
        case RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE:
                return deserialGxsServerGrpUpddate(data, size);
        case RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE:
                return deserialGxsServerMsgUpdate(data, size);
	default:
		{
#ifdef RSSERIAL_DEBUG
			std::cerr << "RsGxsUpdateSerialiser::deserialise() : data has no type"
					  << std::endl;
#endif
			return NULL;

		}
	}
}

uint32_t RsGxsUpdateSerialiser::sizeGxsGrpUpdate(RsGxsGrpUpdateItem* item)
{
	uint32_t s = 8; // header size
    s += item->peerId.serial_size();
	s += 4;
	return s;
}

uint32_t RsGxsUpdateSerialiser::sizeGxsServerGrpUpdate(RsGxsServerGrpUpdateItem* /* item */)
{
        uint32_t s = 8; // header size
        s += 4; // time stamp
        return s;
}

bool RsGxsUpdateSerialiser::serialiseGxsGrpUpdate(RsGxsGrpUpdateItem* item,
		void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::serialiseGxsGrpUpdate()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsGrpUpdate(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsGrpUpdate() size do not match" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsGxsGrpUpdateItem */


    ok &= item->peerId.serialise(data, *size, offset) ;
    ok &= setRawUInt32(data, *size, &offset, item->grpUpdateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsGrpUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsGrpUpdate() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsGxsUpdateSerialiser::serialiseGxsServerGrpUpdate(RsGxsServerGrpUpdateItem* item,
                void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerGrpUpdate()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsServerGrpUpdate(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerGrpUpdate() size do not match" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsGxsServerGrpUpdateItem */

    ok &= setRawUInt32(data, *size, &offset, item->grpUpdateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerGrpUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerGrpUpdate() NOK" << std::endl;
    }
#endif

    return ok;
}

RsGxsGrpUpdateItem* RsGxsUpdateSerialiser::deserialGxsGrpUpddate(void* data,
		uint32_t* size)
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

    ok &= item->peerId.deserialise(data, *size, offset) ;
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

RsGxsServerGrpUpdateItem* RsGxsUpdateSerialiser::deserialGxsServerGrpUpddate(void* data,
                uint32_t* size)
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
            (RS_PKT_SUBTYPE_GXS_SERVER_GRP_UPDATE != getRsItemSubType(rstype)))
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

uint32_t RsGxsUpdateSerialiser::sizeGxsMsgUpdate(RsGxsMsgUpdateItem* item)
{
	uint32_t s = 8; // header size
    s += item->peerId.serial_size() ;//GetTlvStringSize(item->peerId);

    const std::map<RsGxsGroupId, uint32_t>& msgUpdateTS = item->msgUpdateTS;
    std::map<RsGxsGroupId, uint32_t>::const_iterator cit = msgUpdateTS.begin();

	for(; cit != msgUpdateTS.end(); ++cit)
	{
		s += cit->first.serial_size();
        s += 4;
	}

        s += 4; // number of map items

	return s;
}

uint32_t RsGxsUpdateSerialiser::sizeGxsServerMsgUpdate(RsGxsServerMsgUpdateItem* item)
{
        uint32_t s = 8; // header size
        s += item->grpId.serial_size();
        s += 4; // grp TS

        return s;
}

bool RsGxsUpdateSerialiser::serialiseGxsMsgUpdate(RsGxsMsgUpdateItem* item,
		void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::serialiseGxsMsgUpdate()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsMsgUpdate(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsMsgUpdate() size do not match" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsGxsMsgUpdateItem */


    ok &= item->peerId.serialise(data, *size, offset) ;

    const std::map<RsGxsGroupId, uint32_t>& msgUpdateTS = item->msgUpdateTS;
    std::map<RsGxsGroupId, uint32_t>::const_iterator cit = msgUpdateTS.begin();

    uint32_t numItems = msgUpdateTS.size();
    ok &= setRawUInt32(data, *size, &offset, numItems);

    for(; cit != msgUpdateTS.end(); ++cit)
    {
    	ok &= cit->first.serialise(data, *size, offset);
        ok &= setRawUInt32(data, *size, &offset, cit->second);
    }

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsMsgUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsMsgUpdate() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsGxsUpdateSerialiser::serialiseGxsServerMsgUpdate(RsGxsServerMsgUpdateItem* item,
                void* data, uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerMsgUpdate()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsServerMsgUpdate(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerMsgUpdate() size do not match" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsNxsSyncm */


    ok &= item->grpId.serialise(data, *size, offset) ;
    ok &= setRawUInt32(data, *size, &offset, item->msgUpdateTS);

    if(offset != tlvsize){
#ifdef RSSERIAL_DEBUG
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerMsgUpdate() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef RSSERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsUpdateSerialiser::serialiseGxsServerMsgUpdate() NOK" << std::endl;
    }
#endif

    return ok;
}

RsGxsMsgUpdateItem* RsGxsUpdateSerialiser::deserialGxsMsgUpdate(void* data,
		uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::deserialGxsMsgUpdate()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_GXS_MSG_UPDATE != getRsItemSubType(rstype)))
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

    ok &= item->peerId.deserialise(data, *size, offset) ;
    uint32_t numUpdateItems;
    ok &= getRawUInt32(data, *size, &offset, &(numUpdateItems));
    std::map<RsGxsGroupId, uint32_t>& msgUpdateItem = item->msgUpdateTS;
    RsGxsGroupId pId;
    uint32_t updateTS;
    for(uint32_t i = 0; i < numUpdateItems; i++)
    {
        ok &= pId.deserialise(data, *size, offset);

        if(!ok)
            break;

        ok &= getRawUInt32(data, *size, &offset, &(updateTS));

        if(!ok)
            break;

        msgUpdateItem.insert(std::make_pair(pId, updateTS));
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

RsGxsServerMsgUpdateItem* RsGxsUpdateSerialiser::deserialGxsServerMsgUpdate(void* data,
                uint32_t* size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsUpdateSerialiser::deserialGxsServerMsgUpdate()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (SERVICE_TYPE != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_GXS_SERVER_MSG_UPDATE != getRsItemSubType(rstype)))
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

