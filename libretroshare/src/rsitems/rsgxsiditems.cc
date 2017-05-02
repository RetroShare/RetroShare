/*
 * libretroshare/src/serialiser: rsgxsiditems.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include <iostream>

#include "rsgxsiditems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvstring.h"
#include "util/rsstring.h"

#include "serialiser/rstypeserializer.h"

#define GXSID_DEBUG	1

RsItem *RsGxsIdSerialiser::create_item(uint16_t service_id,uint8_t item_subtype) const
{
    if(service_id != RS_SERVICE_GXS_TYPE_GXSID)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_GXSID_GROUP_ITEM     : return new RsGxsIdGroupItem ();
    case RS_PKT_SUBTYPE_GXSID_LOCAL_INFO_ITEM: return new RsGxsIdLocalInfoItem() ;
    default:
        return NULL ;
    }
}
void RsGxsIdLocalInfoItem::clear()
{
    mTimeStamps.clear() ;
}
void RsGxsIdGroupItem::clear()
{
    mPgpIdHash.clear();
    mPgpIdSign.clear();

    mRecognTags.clear();
    mImage.TlvClear();
}
void RsGxsIdLocalInfoItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,mTimeStamps,"mTimeStamps") ;
    RsTypeSerializer::serial_process(j,ctx,mContacts,"mContacts") ;
}

void RsGxsIdGroupItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,mPgpIdHash,"mPgpIdHash") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_SIGN,mPgpIdSign,"mPgpIdSign") ;

    RsTlvStringSetRef rset(TLV_TYPE_RECOGNSET,mRecognTags) ;

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,rset,"mRecognTags") ;

    // image is optional

    if(j == RsGenericSerializer::DESERIALIZE && ctx.mOffset == ctx.mSize)
        return ;

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,mImage,"mImage") ;
}

bool RsGxsIdGroupItem::fromGxsIdGroup(RsGxsIdGroup &group, bool moveImage)
{
        clear();
        meta = group.mMeta;
        mPgpIdHash = group.mPgpIdHash;
        mPgpIdSign = group.mPgpIdSign;
        mRecognTags = group.mRecognTags;

        if (moveImage)
        {
            mImage.binData.bin_data = group.mImage.mData;
            mImage.binData.bin_len = group.mImage.mSize;
            group.mImage.shallowClear();
        }
        else
        {
            mImage.binData.setBinData(group.mImage.mData, group.mImage.mSize);
        }
    return true ;
}
bool RsGxsIdGroupItem::toGxsIdGroup(RsGxsIdGroup &group, bool moveImage)
{
        group.mMeta = meta;
        group.mPgpIdHash = mPgpIdHash;
        group.mPgpIdSign = mPgpIdSign;
        group.mRecognTags = mRecognTags;

        if (moveImage)
        {
            group.mImage.take((uint8_t *) mImage.binData.bin_data, mImage.binData.bin_len);
            // mImage doesn't have a ShallowClear at the moment!
            mImage.binData.TlvShallowClear();
        }
        else
        {
            group.mImage.copy((uint8_t *) mImage.binData.bin_data, mImage.binData.bin_len);
        }
    return true ;
}

