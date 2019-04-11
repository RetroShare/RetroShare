/*******************************************************************************
 * unittests/libretroshare/gxs/gen_exchange/rsdummyservices.cc                 *
 *                                                                             *
 * Copyright (C) 2013, Crispy <retroshare.team@gmailcom>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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
 ******************************************************************************/

#include "rsdummyservices.h"


uint32_t RsDummySerialiser::size(RsItem *item)
{
    RsDummyMsg* msg;
    RsDummyGrp* grp;

    if( (msg = dynamic_cast<RsDummyMsg*>(item)) != NULL )
    {
        return sizeDummyMsgItem(msg);
    }else if( (grp = dynamic_cast<RsDummyGrp*>(item)) != NULL )
    {
        return sizeDummyGrpItem(grp);
    }else
    {
        std::cerr << "RsDummySerialiser::size(RsItem *item) Error with dummy cast!\n";
    }

    return 0;
}


bool RsDummySerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
    RsDummyMsg* msg;
    RsDummyGrp* grp;

    if( (msg = dynamic_cast<RsDummyMsg*>(item)) != NULL )
    {
        return serialiseDummyMsgItem(msg, data, size);
    }else if( (grp = dynamic_cast<RsDummyGrp*>(item)) != NULL )
    {
        return serialiseDummyGrpItem(grp, data, size);
    }else
    {
        std::cerr << "RsDummySerialiser::size(RsItem *item) Error with dummy cast!\n";
    }

    return false;
}

RsItem* RsDummySerialiser::deserialise(void *data, uint32_t *size)
{
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_TYPE_DUMMY != getRsItemService(rstype)))
    {
            return NULL; /* wrong type */
    }

    switch(getRsItemSubType(rstype))
    {
            case RS_PKT_SUBTYPE_DUMMY_MSG:
                    return deserialiseDummyMsgItem(data, size);
            case RS_PKT_SUBTYPE_DUMMY_GRP:
                    return deserialiseDummyGrpItem(data, size);
            default:
                    return NULL;
    }

    return NULL;
}


uint32_t    RsDummySerialiser::sizeDummyMsgItem(RsDummyMsg *item)
{
    uint32_t s = 8; // header
    s += GetTlvStringSize(item->msgData);

    return s;
}

bool        RsDummySerialiser::serialiseDummyMsgItem  (RsDummyMsg *item, void *data, uint32_t *size)
{
    uint32_t tlvsize = sizeDummyMsgItem(item);
    uint32_t offset = 0;

    if (*size < tlvsize)
            return false; /* not enough space */

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* RsDistribMsg first */

    ok &= SetTlvString(data, *size, &offset, 1, item->msgData);

    if (offset != tlvsize)
    {
            ok = false;
            std::cerr << "RsDummySerialiser::serialiseDummyMsgItem Size Error! " << std::endl;
    }

    return ok;
}

RsDummyMsg *    RsDummySerialiser::deserialiseDummyMsgItem(void *data, uint32_t *size)
{
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_TYPE_DUMMY != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_DUMMY_MSG != getRsItemSubType(rstype)))
    {
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
            return NULL; /* not enough data */

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    /* ready to load */
    RsDummyMsg *item = new RsDummyMsg();

    /* skip the header */
    offset += 8;

    ok &= GetTlvString(data, *size, &offset, 1, item->msgData);

    if (!ok)
    {
            delete item;
            return NULL;
    }

    return item;
}

uint32_t    RsDummySerialiser::sizeDummyGrpItem(RsDummyGrp *item)
{
    uint32_t s = 8;
    s += GetTlvStringSize(item->grpData);

    return s;
}

bool        RsDummySerialiser::serialiseDummyGrpItem  (RsDummyGrp *item, void *data, uint32_t *size)
{
    uint32_t tlvsize = sizeDummyGrpItem(item);
    uint32_t offset = 0;

    if (*size < tlvsize)
            return false; /* not enough space */

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    ok &= SetTlvString(data, *size, &offset, 1, item->grpData);

    if (offset != tlvsize)
    {
            ok = false;
            std::cerr << "RsDummySerialiser::serialiseDummyGrpItem Size Error! " << std::endl;
    }

    return ok;
}

RsDummyGrp *    RsDummySerialiser::deserialiseDummyGrpItem(void *data, uint32_t *size)
{
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_TYPE_DUMMY != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_DUMMY_GRP != getRsItemSubType(rstype)))
    {
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
            return NULL; /* not enough data */

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    /* ready to load */
    RsDummyGrp *item = new RsDummyGrp();

    /* skip the header */
    offset += 8;

    ok &= GetTlvString(data, *size, &offset, 1, item->grpData);

    if (!ok)
    {
            delete item;
            return NULL;
    }

    return item;
}


