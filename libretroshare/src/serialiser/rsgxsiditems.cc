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

#define GXSID_DEBUG	1

RsItem* RsGxsIdSerialiser::deserialise(void* data, uint32_t* size)
{
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)))
        return NULL; /* wrong type */

    switch(getRsItemSubType(rstype))
    {
        case RS_PKT_SUBTYPE_GXSID_GROUP_ITEM:        return deserialise_GxsIdGroupItem(data, size);
        case RS_PKT_SUBTYPE_GXSID_LOCAL_INFO_ITEM:   return deserialise_GxsIdLocalInfoItem(data, size);
#if 0
        case RS_PKT_SUBTYPE_GXSID_OPINION_ITEM: return deserialise_GxsIdOpinionItem(data, size);
        case RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM: return deserialise_GxsIdCommentItem(data, size);
#endif
        default:
#ifdef GXSID_DEBUG
            std::cerr << "RsGxsIdSerialiser::deserialise(): unknown subtype";
            std::cerr << std::endl;
#endif
            break;
    }
    return NULL;
}

bool RsGxsIdItem::serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset)
{
    tlvsize = serial_size() ;
    offset = 0;

    if (pktsize < tlvsize)
        return false; /* not enough space */

    pktsize = tlvsize;

    if(!setRsItemHeader(data, tlvsize, PacketId(), tlvsize))
    {
        std::cerr << "RsItem::serialise_header(): ERROR. Not enough size!" << std::endl;
        return false ;
    }
    offset += 8;

    return true ;
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


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
uint32_t RsGxsIdLocalInfoItem::serial_size()
{
    uint32_t s = 8 ;	// header
    s += 4 ; 		// number of items
    s += mTimeStamps.size() * (RsGxsId::SIZE_IN_BYTES + 8) ;
    s += 4 ; 		// number of contacts
    s += mContacts.size() * (RsGxsId::SIZE_IN_BYTES + 8) ;

    return s;
}

std::ostream& RsGxsIdLocalInfoItem::print(std::ostream& out, uint16_t indent)
{
    printRsItemBase(out, "RsGxsIdLocalInfoItem", indent);
    uint16_t int_Indent = indent + 2;

    // convert from binary to hex.
    for(std::map<RsGxsId,time_t>::const_iterator it(mTimeStamps.begin());it!=mTimeStamps.end();++it)
        out << it->first << " : " << it->second << std::endl;

    printRsItemEnd(out ,"RsGxsIdLocalInfoItem", indent);
    return out;
}
std::ostream& RsGxsIdGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsIdGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "MetaData: " << meta << std::endl;
	printIndent(out, int_Indent);
    out << "PgpIdHash: " << mPgpIdHash << std::endl;
	printIndent(out, int_Indent);

	std::string signhex;
	// convert from binary to hex.
    for(unsigned int i = 0; i < mPgpIdSign.length(); i++)
	{
        rs_sprintf_append(signhex, "%02x", (uint32_t) ((uint8_t) mPgpIdSign[i]));
	}
	out << "PgpIdSign: " << signhex << std::endl;
	printIndent(out, int_Indent);
	out << "RecognTags:" << std::endl;

    RsTlvStringSetRef set(TLV_TYPE_RECOGNSET, mRecognTags);
	set.print(out, int_Indent + 2);
  
	printRsItemEnd(out ,"RsGxsIdGroupItem", indent);
	return out;
}


uint32_t RsGxsIdGroupItem::serial_size()
{
    uint32_t s = 8; // header

    s += Sha1CheckSum::SIZE_IN_BYTES;
    s += GetTlvStringSize(mPgpIdSign);

    RsTlvStringSetRef set(TLV_TYPE_RECOGNSET, mRecognTags);
    s += set.TlvSize();
    s += mImage.TlvSize() ;

    return s;
}


bool RsGxsIdLocalInfoItem::serialise(void *data, uint32_t& size)
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    ok &= setRawUInt32(data, tlvsize, &offset, mTimeStamps.size()) ;

    for(std::map<RsGxsId,time_t>::const_iterator it = mTimeStamps.begin();it!=mTimeStamps.end();++it)
    {
        ok &= it->first.serialise(data,tlvsize,offset) ;
        ok &= setRawTimeT(data,tlvsize,&offset,it->second) ;
    }
    ok &= setRawUInt32(data, tlvsize, &offset, mContacts.size()) ;
    
    for(std::set<RsGxsId>::const_iterator it(mContacts.begin());it!=mContacts.end();++it)
	    ok &= (*it).serialise(data,tlvsize,offset) ;
    
    if(offset != tlvsize)
    {
#ifdef GXSID_DEBUG
        std::cerr << "RsGxsIdSerialiser::serialiseGxsIdGroupItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXSID_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsIdSerialiser::serialiseGxsIdgroupItem() NOK" << std::endl;
    }
#endif

    return ok;
}

bool RsGxsIdGroupItem::serialise(void *data, uint32_t& size)
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;

    /* GxsIdGroupItem */
    ok &= mPgpIdHash.serialise(data, tlvsize, offset);
    ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_SIGN, mPgpIdSign);

    RsTlvStringSetRef set(TLV_TYPE_RECOGNSET, mRecognTags);
    ok &= set.SetTlv(data, tlvsize, &offset);

    ok &= mImage.SetTlv(data,tlvsize,&offset) ;

    if(offset != tlvsize)
    {
#ifdef GXSID_DEBUG
        std::cerr << "RsGxsIdSerialiser::serialiseGxsIdGroupItem() FAIL Size Error! " << std::endl;
#endif
        ok = false;
    }

#ifdef GXSID_DEBUG
    if (!ok)
    {
        std::cerr << "RsGxsIdSerialiser::serialiseGxsIdgroupItem() NOK" << std::endl;
    }
#endif

    return ok;
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
RsGxsIdGroupItem* RsGxsIdSerialiser::deserialise_GxsIdGroupItem(void *data, uint32_t *size)
{
    /* get the type and size */
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

    uint32_t offset = 0;


    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
        (RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)) ||
        (RS_PKT_SUBTYPE_GXSID_GROUP_ITEM != getRsItemSubType(rstype)))
    {
#ifdef GXSID_DEBUG
        std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL wrong type" << std::endl;
#endif
        return NULL; /* wrong type */
    }

    if (*size < rssize)    /* check size */
    {
#ifdef GXSID_DEBUG
        std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL wrong size" << std::endl;
#endif
        return NULL; /* not enough data */
    }

    /* set the packet length */
    *size = rssize;

    bool ok = true;

    RsGxsIdGroupItem* item = new RsGxsIdGroupItem();
    /* skip the header */
    offset += 8;

    ok &= item->mPgpIdHash.deserialise(data, rssize, offset);
    ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_SIGN, item->mPgpIdSign);

    RsTlvStringSetRef set(TLV_TYPE_RECOGNSET, item->mRecognTags);
    ok &= set.GetTlv(data, rssize, &offset);

    // image is optional,so that we can continue reading old items.
    if(offset < rssize)
        ok &= item->mImage.GetTlv(data,rssize,&offset) ;

    if (offset != rssize)
    {
#ifdef GXSID_DEBUG
        std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL size mismatch" << std::endl;
#endif
        /* error */
        delete item;
        return NULL;
    }

    if (!ok)
    {
#ifdef GXSID_DEBUG
        std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() NOK" << std::endl;
#endif
        delete item;
        return NULL;
    }

    return item;
}
RsGxsIdLocalInfoItem *RsGxsIdSerialiser::deserialise_GxsIdLocalInfoItem(void *data, uint32_t *size)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)) ||
        (RS_PKT_SUBTYPE_GXSID_LOCAL_INFO_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
    RsGxsIdLocalInfoItem* item = new RsGxsIdLocalInfoItem();
	/* skip the header */
	offset += 8;

    uint32_t n=0 ;
    ok &= getRawUInt32(data, rssize, &offset, &n) ;

    for(int i=0;ok && i<n;++i)
    {
        RsGxsId gxsid ;
        time_t TS ;

        ok &= gxsid.deserialise(data,rssize,offset) ;
        ok &= getRawTimeT(data,rssize,&offset,TS) ;

        item->mTimeStamps[gxsid] = TS ;
    }

    if (offset < rssize)	// backward compatibility, making that section optional.
    {
	ok &= getRawUInt32(data, rssize, &offset, &n) ;
    	RsGxsId gxsid ;
    
    	for(int i=0;ok && i<n;++i)
        {
		ok &= gxsid.deserialise(data,rssize,offset) ;
        
        item->mContacts.insert(gxsid) ;
        }
    }
    
    if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

#if 0

void RsGxsIdOpinionItem::clear()
{
	opinion.mOpinion = 0;
	opinion.mReputation = 0;
	opinion.mComment = "";
}

std::ostream& RsGxsIdOpinionItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsIdOpinionItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Opinion: " << opinion.mOpinion << std::endl;
	printIndent(out, int_Indent);
	out << "Reputation: " << opinion.mReputation << std::endl;
	printIndent(out, int_Indent);
	out << "Comment: " << opinion.mComment << std::endl;
  
	printRsItemEnd(out ,"RsGxsIdOpinionItem", indent);
	return out;
}


uint32_t RsGxsIdOpinionItem::serial_size()
{

	const RsGxsIdOpinion& opinion = item->opinion;
	uint32_t s = 8; // header

	s += 4; // mOpinion.
	s += 4; // mReputation.
	s += GetTlvStringSize(opinion.mComment);

	return s;
}

bool RsGxsIdOpinionItem::serialise(void *data, uint32_t *size)
{
    uint32_t tlvsize,offset=0;
    bool ok = true;

    if(!serialise_header(data,size,tlvsize,offset))
        return false ;
	
	/* GxsIdOpinionItem */
	ok &= setRawUInt32(data, tlvsize, &offset, item->opinion.mOpinion);
	ok &= setRawUInt32(data, tlvsize, &offset, item->opinion.mReputation);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_COMMENT, item->opinion.mComment);
	
	if(offset != tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdOpinionItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSID_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdgroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsIdOpinionItem* RsGxsIdSerialiser::deserialise_GxsIdOpinionItem(void *data, uint32_t *size)
{
	
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSID_OPINION_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdOpinionItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdOpinionItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsIdOpinionItem* item = new RsGxsIdOpinionItem();
	/* skip the header */
	offset += 8;
	
	ok &= getRawUInt32(data, rssize, &offset, &(item->opinion.mOpinion));
	ok &= getRawUInt32(data, rssize, &offset, &(item->opinion.mReputation));
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_COMMENT, item->opinion.mComment);
	
	if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdOpinionItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdOpinionItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}


/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsIdCommentItem::clear()
{
	comment.mComment.clear();
}

std::ostream& RsGxsIdCommentItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsIdCommentItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Comment: " << comment.mComment << std::endl;
  
	printRsItemEnd(out ,"RsGxsIdCommentItem", indent);
	return out;
}


uint32_t RsGxsIdCommentItem::serial_size()
{

	const RsGxsIdComment& comment = item->comment;
	uint32_t s = 8; // header

	s += GetTlvStringSize(comment.mComment);

	return s;
}

bool RsGxsIdCommentItem::serialise(void *data, uint32_t *size)
{
        uint32_t tlvsize,offset=0;
        bool ok = true;

        if(!serialise_header(data,size,tlvsize,offset))
            return false ;
	
	/* GxsIdCommentItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_COMMENT, item->comment.mComment);
	
	if(offset != tlvsize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdCommentItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef GXSID_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsIdSerialiser::serialiseGxsIdgroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
}
	
RsGxsIdCommentItem* RsGxsIdSerialiser::deserialise_GxsIdCommentItem(void *data, uint32_t *size)
{
	
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_GXSID != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM != getRsItemSubType(rstype)))
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdCommentItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdCommentItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsIdCommentItem* item = new RsGxsIdCommentItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_COMMENT, item->comment.mComment);
	
	if (offset != rssize)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdCommentItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef GXSID_DEBUG
		std::cerr << "RsGxsIdSerialiser::deserialiseGxsIdCommentItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}

#endif

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

