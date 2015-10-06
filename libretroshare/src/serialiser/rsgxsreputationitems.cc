/*
 * libretroshare/src/serialiser: rsbanlist.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011 by Robert Fernie.
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

#include <stdio.h>
#include "serialiser/rsbaseserial.h"
#include "serialiser/rsgxsreputationitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

bool RsReputationItem::serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset) const
{
	tlvsize = serial_size() ;
	offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	if(!setRsItemHeader(data, tlvsize, PacketId(), tlvsize))
	{
		std::cerr << "RsReputationItem::serialise_header(): ERROR. Not enough size!" << std::endl;
		return false ;
	}
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsReputationItem::serialise() Header: " << ok << std::endl;
#endif
	offset += 8;

	return true ;
}

/*************************************************************************/

void RsGxsReputationSetItem::clear()
{
	mOpinions.clear() ;
}

void RsGxsReputationUpdateItem::clear()
{
	mOpinions.clear() ;
}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

std::ostream& RsGxsReputationConfigItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsReputationConfigItem", indent);
	uint16_t int_Indent = indent + 2;

    	out << "mPeerId: " << mPeerId << std::endl;
    	out << "last update: " << time(NULL) - mLatestUpdate << " secs ago." << std::endl;
    	out << "last query : " << time(NULL) - mLastQuery << " secs ago." << std::endl;
        
        printRsItemEnd(out, "RsReputationConfigItem", indent);
        return out;
}

std::ostream& RsGxsReputationSetItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsReputationSetItem", indent);
	uint16_t int_Indent = indent + 2;

    	out << "GxsId: " << mGxsId << std::endl;
    	out << "mOwnOpinion: " << mOwnOpinion << std::endl;
    	out << "mOwnOpinionTS : " << time(NULL) - mOwnOpinionTS << " secs ago." << std::endl;
    	out << "mReputation: " << mReputation << std::endl;
        out << "Opinions from neighbors: " << std::endl;
        
        for(std::map<RsPeerId,uint32_t>::const_iterator it(mOpinions.begin());it!=mOpinions.end();++it)
        	out << "  " << it->first << ": " << it->second << std::endl;
        
        printRsItemEnd(out, "RsReputationSetItem", indent);
        return out;
}
std::ostream& RsGxsReputationUpdateItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsReputationUpdateItem", indent);
	uint16_t int_Indent = indent + 2;

    	out << "from: " << PeerId() << std::endl;
    	out << "last update: " << time(NULL) - mLatestUpdate << " secs ago." << std::endl;
        
        for(std::map<RsGxsId,uint32_t>::const_iterator it(mOpinions.begin());it!=mOpinions.end();++it)
        	out << "  " << it->first << ": " << it->second << std::endl;
        
        printRsItemEnd(out, "RsReputationUpdateItem", indent);
        return out;
}
std::ostream& RsGxsReputationRequestItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsReputationRequestItem", indent);
	uint16_t int_Indent = indent + 2;

    	out << "last update: " << time(NULL) - mLastUpdate << " secs ago." << std::endl;
        
        printRsItemEnd(out, "RsReputationRequestItem", indent);
        return out;
}
/*************************************************************************/

uint32_t    RsGxsReputationConfigItem::serial_size() const
{
    uint32_t s = 8; /* header */

    s += mPeerId.serial_size() ;	// PeerId
    s += 4 ;				// mLatestUpdate
    s += 4 ;				// mLastQuery
    
    return s ;
}

uint32_t    RsGxsReputationSetItem::serial_size() const
{
	uint32_t s = 8; /* header */
    
    	s += mGxsId.serial_size() ;
        s += 4 ; 			// mOwnOpinion
        s += 4 ; 			// mOwnOpinionTS
        s += 4 ; 			// mReputation
        
        s += 4 ; 			// mOpinions.size()
        
        s += (4+RsPeerId::serial_size()) * mOpinions.size() ;
                
	return s ;
}

uint32_t    RsGxsReputationUpdateItem::serial_size() const
{
	uint32_t s = 8; /* header */
    
    	s += 4 ; 	// mLatestUpdate
        s += 4 ; 	// mOpinions.size();
        
        s += (RsGxsId::serial_size() + 4) * mOpinions.size() ;
        
	return s ;
}

uint32_t    RsGxsReputationRequestItem::serial_size() const
{
	uint32_t s = 8; /* header */
    
	s += 4 ; // mLastUpdate

	return s;
}

/*************************************************************************/

bool RsGxsReputationConfigItem::serialise(void *data, uint32_t& pktsize) const
{
	uint32_t tlvsize ;
    	uint32_t offset=0;

	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	bool ok = true;
	
	ok &= mPeerId.serialise(data,tlvsize,offset) ;
	ok &= setRawUInt32(data, tlvsize, &offset, mLatestUpdate);
	ok &= setRawUInt32(data, tlvsize, &offset, mLastQuery);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGRouterGenericDataItem::serialisedata() size error! " << std::endl;
	}

	return ok;
}
bool RsGxsReputationSetItem::serialise(void *data, uint32_t& pktsize) const
{
	uint32_t tlvsize ;
    	uint32_t offset=0;

	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	bool ok = true;
	
	ok &= mGxsId.serialise(data,tlvsize,offset) ;
	ok &= setRawUInt32(data, tlvsize, &offset, mOwnOpinion);
	ok &= setRawUInt32(data, tlvsize, &offset, mOwnOpinionTS);
	ok &= setRawUInt32(data, tlvsize, &offset, mReputation);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGRouterGenericDataItem::serialisedata() size error! " << std::endl;
	}

	return ok;
}
bool RsGxsReputationUpdateItem::serialise(void *data, uint32_t& pktsize) const
{
	uint32_t tlvsize ;
    	uint32_t offset=0;

	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	bool ok = true;
	
	ok &= setRawUInt32(data, tlvsize, &offset, mLatestUpdate);
	ok &= setRawUInt32(data, tlvsize, &offset, mOpinions.size());
    
    	for(std::map<RsGxsId,uint32_t>::const_iterator it(mOpinions.begin());ok && it!=mOpinions.end();++it)
        {
	    ok &= it->first.serialise(data, tlvsize, offset) ;
	    ok &= setRawUInt32(data, tlvsize, &offset, it->second) ;
        }

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGRouterGenericDataItem::serialisedata() size error! " << std::endl;
	}

	return ok;
}
/* serialise the data to the buffer */
bool RsGxsReputationRequestItem::serialise(void *data, uint32_t& pktsize) const
{
	uint32_t tlvsize ;
    	uint32_t offset=0;

	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	bool ok = true;
	
	ok &= setRawUInt32(data, tlvsize, &offset, mLastUpdate);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGRouterGenericDataItem::serialisedata() size error! " << std::endl;
	}

	return ok;
}
/*************************************************************************/

RsGxsReputationConfigItem *RsGxsReputationSerialiser::deserialiseReputationConfigItem(void *data,uint32_t size)
{
    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);
    bool ok = true ;

    RsGxsReputationConfigItem *item = new RsGxsReputationConfigItem() ;

    /* add mandatory parts first */
    ok &= item->mPeerId.deserialise(data, size, offset) ;
    ok &= getRawUInt32(data, size, &offset, &item->mLatestUpdate);
    ok &= getRawUInt32(data, size, &offset, &item->mLastQuery);

    if (offset != rssize || !ok)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
	delete item;
        return NULL ;
    }

    return item;
}

RsGxsReputationSetItem *RsGxsReputationSerialiser::deserialiseReputationSetItem(void *data,uint32_t tlvsize)
{
    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);
    bool ok = true ;

    RsGxsReputationSetItem *item = new RsGxsReputationSetItem() ;

    /* add mandatory parts first */
    ok &= item->mGxsId.deserialise(data, tlvsize, offset) ;
    ok &= getRawUInt32(data, tlvsize, &offset, &item->mOwnOpinion);
    ok &= getRawUInt32(data, tlvsize, &offset, &item->mOwnOpinionTS);
    ok &= getRawUInt32(data, tlvsize, &offset, &item->mReputation);
    
    uint32_t S ;
    ok &= getRawUInt32(data, tlvsize, &offset, &S);
    
    for(int i=0;ok && i<S;++i)
    {
            RsPeerId pid ;
            uint32_t op ;
            
	    ok &= pid.deserialise(data, tlvsize, offset) ;
	    ok &= getRawUInt32(data, tlvsize, &offset, &op);
        
        if(ok)
		item->mOpinions[pid] = op ;
    }

    if (offset != rssize || !ok)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
	delete item;
        return NULL ;
    }

    return item;
}

RsGxsReputationUpdateItem *RsGxsReputationSerialiser::deserialiseReputationUpdateItem(void *data,uint32_t tlvsize)
{
    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);
    bool ok = true ;

    RsGxsReputationUpdateItem *item = new RsGxsReputationUpdateItem() ;

    /* add mandatory parts first */
    ok &= getRawUInt32(data, tlvsize, &offset, &item->mLatestUpdate);
    
    uint32_t S ;
    ok &= getRawUInt32(data, tlvsize, &offset, &S) ;
    
    for(uint32_t i=0;ok && i<S;++i)
    {
            RsGxsId gid ;
            uint32_t op ;
            
	    ok &= gid.deserialise(data, tlvsize, offset) ;
	    ok &= getRawUInt32(data, tlvsize, &offset, &op);
        
        if(ok)
		item->mOpinions[gid] = op ;
        
    }

    if (offset != rssize || !ok)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
	delete item;
        return NULL ;
    }

    return item;
}

RsGxsReputationRequestItem *RsGxsReputationSerialiser::deserialiseReputationRequestItem(void *data,uint32_t tlvsize)
{
    uint32_t offset = 8; // skip the header
    uint32_t rssize = getRsItemSize(data);
    bool ok = true ;

    RsGxsReputationRequestItem *item = new RsGxsReputationRequestItem() ;

    /* add mandatory parts first */
    ok &= getRawUInt32(data, tlvsize, &offset, &item->mLastUpdate);

    if (offset != rssize || !ok)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
	delete item;
        return NULL ;
    }

    return item;
}
/*************************************************************************/

RsItem *RsGxsReputationSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_GXS_TYPE_REPUTATION != getRsItemService(rstype)))
    {
            std::cerr << "RsReputationSerialiser::deserialise(): wrong item type " << std::hex << rstype << std::dec << std::endl;
	    return NULL; /* wrong type */
    }

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_GXS_REPUTATION_SET_ITEM    : return deserialiseReputationSetItem    (data, *pktsize);
		case RS_PKT_SUBTYPE_GXS_REPUTATION_UPDATE_ITEM : return deserialiseReputationUpdateItem (data, *pktsize);
		case RS_PKT_SUBTYPE_GXS_REPUTATION_REQUEST_ITEM: return deserialiseReputationRequestItem(data, *pktsize);
		case RS_PKT_SUBTYPE_GXS_REPUTATION_CONFIG_ITEM : return deserialiseReputationConfigItem (data, *pktsize);
        
		default:
        		std::cerr << "RsGxsReputationSerialiser::deserialise(): unknown item subtype " << std::hex<< rstype << std::dec << std::endl;
			return NULL;
			break;
	}
}

/*************************************************************************/



