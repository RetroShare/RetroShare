
/*
 * libretroshare/src/gxs: rsopsteditems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012 by Christopher Evi-Parker
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

#include "serialiser/rsposteditems.h"
#include "rsbaseserial.h"


uint32_t RsGxsPostedSerialiser::size(RsItem *item)
{
    RsGxsPostedPostItem* ppItem = NULL;
    RsGxsPostedCommentItem* pcItem = NULL;
    RsGxsPostedVoteItem* pvItem = NULL;
    RsGxsPostedGroupItem* pgItem = NULL;

    if((ppItem = dynamic_cast<RsGxsPostedPostItem*>(item)) != NULL)
    {
        return sizeGxsPostedPostItem(ppItem);
    }
    else if((pcItem = dynamic_cast<RsGxsPostedCommentItem*>(item)) != NULL)
    {
        return sizeGxsPostedCommentItem(pcItem);
    }else if((pvItem = dynamic_cast<RsGxsPostedVoteItem*>(item)) != NULL)
    {
        return sizeGxsPostedVoteItem(pvItem);
    }else if((pgItem = dynamic_cast<RsGxsPostedGroupItem*>(item)) != NULL)
    {
        return sizeGxsPostedGroupItem(pgItem);
    }
    else
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::size() Failed" << std::endl;
#endif
            return NULL;
    }
}

bool RsGxsPostedSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{

    RsGxsPostedPostItem* ppItem = NULL;
    RsGxsPostedCommentItem* pcItem = NULL;
    RsGxsPostedVoteItem* pvItem = NULL;
    RsGxsPostedGroupItem* pgItem = NULL;

    if((ppItem = dynamic_cast<RsGxsPostedPostItem*>(item)) != NULL)
    {
        return serialiseGxsPostedPostItem(ppItem, data, size);
    }
    else if((pcItem = dynamic_cast<RsGxsPostedCommentItem*>(item)) != NULL)
    {
        return serialiseGxsPostedCommentItem(pcItem, data, size);
    }else if((pvItem = dynamic_cast<RsGxsPostedVoteItem*>(item)) != NULL)
    {
        return serialiseGxsPostedVoteItem(pvItem, data, size);
    }else if((pgItem = dynamic_cast<RsGxsPostedGroupItem*>(item)) != NULL)
    {
        return serialiseGxsPostedGroupItem(pgItem, data, size);
    }
    else
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::serialise() FAILED" << std::endl;
#endif
            return false;
    }
}

RsItem* RsGxsPostedSerialiser::deserialise(void *data, uint32_t *size)
{
#ifdef RSSERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::deserialise()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_GXSV1_TYPE_POSTED != getRsItemService(rstype)))
    {
            return NULL; /* wrong type */
    }

    switch(getRsItemSubType(rstype))
    {

        case RS_PKT_SUBTYPE_POSTED_COMMENT_ITEM:
                return deserialiseGxsPostedCommentItem(data, size);
        case RS_PKT_SUBTYPE_POSTED_GRP_ITEM:
                return deserialiseGxsPostedGroupItem(data, size);
        case RS_PKT_SUBTYPE_POSTED_POST_ITEM:
                return deserialiseGxsPostedPostItem(data, size);
        case RS_PKT_SUBTYPE_POSTED_VOTE_ITEM:
                return deserialiseGxsPostedVoteItem(data, size);
        default:
                {
#ifdef RS_SSERIAL_DEBUG
                    std::cerr << "RsGxsPostedSerialiser::deserialise(): subtype could not be dealt with"
                        << std::endl;
#endif
                    break;
                }
    }
    return NULL;
}


uint32_t RsGxsPostedSerialiser::sizeGxsPostedPostItem(RsGxsPostedPostItem* item)
{
    RsPostedPost& p = item->mPost;

    uint32_t s = 8;

    s += GetTlvStringSize(p.mLink);
    s += GetTlvStringSize(p.mNotes);

    return s;
}

uint32_t RsGxsPostedSerialiser::sizeGxsPostedCommentItem(RsGxsPostedCommentItem* item)
{
    RsPostedComment& c = item->mComment;

    uint32_t s = 8;

    s += GetTlvStringSize(c.mComment);

    return s;
}

uint32_t RsGxsPostedSerialiser::sizeGxsPostedVoteItem(RsGxsPostedVoteItem* item)
{
    RsPostedVote& v = item->mVote;

    uint32_t s = 8;
    s += 1; // for vote direction

    return s;
}

uint32_t RsGxsPostedSerialiser::sizeGxsPostedGroupItem(RsGxsPostedGroupItem* item)
{
    RsPostedGroup& g = item->mGroup;

    uint32_t s = 8;
    return s;
}

bool RsGxsPostedSerialiser::serialiseGxsPostedPostItem(RsGxsPostedPostItem* item, void* data, uint32_t *size)
{

#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedPostItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsPostedPostItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedPostItem()()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* GxsPhotoAlbumItem */

    ok &= SetTlvString(data, tlvsize, &offset, 1, item->mPost.mLink);
    ok &= SetTlvString(data, tlvsize, &offset, 1, item->mPost.mNotes);

    if(offset != tlvsize)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedPostItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXS_POSTED_SERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedPostItem() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsGxsPostedSerialiser::serialiseGxsPostedCommentItem(RsGxsPostedCommentItem* item, void* data, uint32_t *size)
{
#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedCommentItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsPostedCommentItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedCommentItem()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* GxsPhotoAlbumItem */

    ok &= SetTlvString(data, tlvsize, &offset, 1, item->mComment.mComment);


    if(offset != tlvsize)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedCommentItem()() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXS_POSTED_SERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedCommentItem()() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsGxsPostedSerialiser::serialiseGxsPostedVoteItem(RsGxsPostedVoteItem* item, void* data, uint32_t *size)
{

#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedVoteItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsPostedVoteItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedVoteItem()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    ok &= setRawUInt8(data, tlvsize, &offset, item->mVote.mDirection);

    if(offset != tlvsize)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedVoteItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXS_POSTED_SERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedVoteItem() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsGxsPostedSerialiser::serialiseGxsPostedGroupItem(RsGxsPostedGroupItem* item, void* data, uint32_t *size)
{

#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedGroupItem()" << std::endl;
#endif

    uint32_t tlvsize = sizeGxsPostedGroupItem(item);
    uint32_t offset = 0;

    if(*size < tlvsize){
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedGroupItem()" << std::endl;
#endif
        return false;
    }

    *size = tlvsize;

    bool ok = true;

    ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

    /* skip the header */
    offset += 8;

    /* GxsPhotoAlbumItem */

    if(offset != tlvsize)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedGroupItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXS_POSTED_SERIAL_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsPostedSerialiser::serialiseGxsPostedGroupItem() NOK" << std::endl;
    }
#endif

    return ok;
}

RsGxsPostedPostItem* RsGxsPostedSerialiser::deserialiseGxsPostedPostItem(void *data, uint32_t *size)
{

#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_GXSV1_TYPE_POSTED != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_POSTED_POST_ITEM != getRsItemSubType(rstype)))
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsPostedPostItem* item = new RsGxsPostedPostItem();
    /* skip the header */
    offset += 8;


    ok &= GetTlvString(data, rssize, &offset, 1, item->mPost.mLink);
    ok &= GetTlvString(data, rssize, &offset, 1, item->mPost.mNotes);

    if (offset != rssize)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem() FAIL size mismatch" << std::endl;
#endif
        /* error */
        delete item;
        return NULL;
    }

    if (!ok)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedPostItem() NOK" << std::endl;
#endif
        delete item;
        return NULL;
    }

    return item;
}

RsGxsPostedCommentItem* RsGxsPostedSerialiser::deserialiseGxsPostedCommentItem(void *data, uint32_t *size)
{
#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedCommentItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_GXSV1_TYPE_POSTED != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_POSTED_COMMENT_ITEM != getRsItemSubType(rstype)))
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedCommentItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedCommentItem() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsPostedCommentItem* item = new RsGxsPostedCommentItem();
    /* skip the header */
    offset += 8;


    ok &= GetTlvString(data, rssize, &offset, 1, item->mComment.mComment);

    if (offset != rssize)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedCommentItem() FAIL size mismatch" << std::endl;
#endif
        /* error */
        delete item;
        return NULL;
    }

    if (!ok)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedCommentItem() NOK" << std::endl;
#endif
        delete item;
        return NULL;
    }

    return item;
}

RsGxsPostedVoteItem* RsGxsPostedSerialiser::deserialiseGxsPostedVoteItem(void *data, uint32_t *size)
{

#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedVoteItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_GXSV1_TYPE_POSTED != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_POSTED_VOTE_ITEM != getRsItemSubType(rstype)))
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedVoteItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedVoteItem() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsPostedVoteItem* item = new RsGxsPostedVoteItem();
    /* skip the header */
    offset += 8;

    ok &= getRawUInt8(data, rssize, &offset, &(item->mVote.mDirection));

    if (offset != rssize)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedVoteItem() FAIL size mismatch" << std::endl;
#endif
        /* error */
        delete item;
        return NULL;
    }

    if (!ok)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedVoteItem() NOK" << std::endl;
#endif
        delete item;
        return NULL;
    }

    return item;
}

RsGxsPostedGroupItem* RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem(void *data, uint32_t *size)
{

#ifdef GXS_POSTED_SERIAL_DEBUG
    std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem()" << std::endl;
#endif
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
            (RS_SERVICE_GXSV1_TYPE_POSTED != getRsItemService(rstype)) ||
            (RS_PKT_SUBTYPE_POSTED_VOTE_ITEM != getRsItemSubType(rstype)))
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem() FAIL wrong type" << std::endl;
#endif
            return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem() FAIL wrong size" << std::endl;
#endif
            return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsPostedGroupItem* item = new RsGxsPostedGroupItem();
    /* skip the header */
    offset += 8;

    if (offset != rssize)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
        std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem() FAIL size mismatch" << std::endl;
#endif
        /* error */
        delete item;
        return NULL;
    }

    if (!ok)
    {
#ifdef GXS_POSTED_SERIAL_DEBUG
            std::cerr << "RsGxsPostedSerialiser::deserialiseGxsPostedGroupItem() NOK" << std::endl;
#endif
        delete item;
        return NULL;
    }

    return item;
}


void RsGxsPostedPostItem::clear()
{

}

std::ostream & RsGxsPostedPostItem::print(std::ostream &out, uint16_t indent)
{
    return out;
}

void RsGxsPostedVoteItem::clear()
{
    return;
}

std::ostream & RsGxsPostedVoteItem::print(std::ostream &out, uint16_t indent)
{
    return out;
}

void RsGxsPostedCommentItem::clear()
{
    return;
}

std::ostream & RsGxsPostedCommentItem::print(std::ostream &out, uint16_t indent)
{
    return out;
}

void RsGxsPostedGroupItem::clear()
{
    return;
}

std::ostream & RsGxsPostedGroupItem::print(std::ostream &out, uint16_t indent)
{
    return out;

}
