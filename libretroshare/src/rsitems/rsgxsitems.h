#ifndef RSGXSITEMS_H
#define RSGXSITEMS_H

/*
 * libretroshare/src/serialiser: rsgxsitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012   Christopher Evi-Parker, Robert Fernie.
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

#include "rsitems/rsitem.h"
#include "rsitems/rsserviceids.h"

#include "retroshare/rsgxsifacetypes.h"

std::ostream &operator<<(std::ostream &out, const RsGroupMetaData &meta);
std::ostream &operator<<(std::ostream &out, const RsMsgMetaData &meta);

class RsGxsGrpItem : public RsItem
{

public:

    RsGxsGrpItem(uint16_t service, uint8_t subtype)
    : RsItem(RS_PKT_VERSION_SERVICE, service, subtype) { return; }
    virtual ~RsGxsGrpItem(){}

    RsGroupMetaData meta;
};

class RsGxsMsgItem : public RsItem
{

public:
    RsGxsMsgItem(uint16_t service, uint8_t subtype)
    : RsItem(RS_PKT_VERSION_SERVICE, service, subtype) { return; }
    virtual ~RsGxsMsgItem(){}

    RsMsgMetaData meta;
};

// We should make these items templates or generic classes so that each GXS service will handle them on its own.

static const uint8_t  RS_PKT_SUBTYPE_GXS_SUBSTRING_SEARCH_ITEM = 0x20 ;
static const uint8_t  RS_PKT_SUBTYPE_GXS_GROUP_SEARCH_ITEM     = 0x21 ;
static const uint8_t  RS_PKT_SUBTYPE_GXS_GROUP_SUMMARY_ITEM    = 0x22 ;
static const uint8_t  RS_PKT_SUBTYPE_GXS_GROUP_DATA_ITEM       = 0x23 ;

class RsGxsTurtleSubStringSearchItem: public RsItem
{
	public:
		RsGxsTurtleSubStringSearchItem(uint16_t service): RsItem(RS_PKT_VERSION_SERVICE,service,RS_PKT_SUBTYPE_GXS_SUBSTRING_SEARCH_ITEM) {}
        virtual ~RsGxsTurtleSubStringSearchItem() {}

		std::string match_string ;	// string to match

		std::string GetKeywords() { return match_string; }
        void clear() { match_string.clear() ; }

	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsGxsTurtleGroupSearchItem: public RsItem
{
	public:
		RsGxsTurtleGroupSearchItem(uint16_t service): RsItem(RS_PKT_VERSION_SERVICE,service,RS_PKT_SUBTYPE_GXS_GROUP_SEARCH_ITEM) {}
        virtual ~RsGxsTurtleGroupSearchItem() {}

        uint16_t service_id ;		// searvice to search
		Sha1CheckSum hashed_group_id ;	// the group ID is hashed in order to keep it private.

		std::string GetKeywords() { return std::string("Group request for [hashed] ")+hashed_group_id.toStdString() ; }
        void clear() { hashed_group_id.clear() ; }

	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

struct TurtleGxsInfo
{
	uint16_t     service_id ;
	RsGxsGroupId group_id ;
	RsGxsId      author;
	std::string  name ;
	std::string  description ;
	time_t       last_post ;
	uint32_t     number_of_posts ;
};

class RsTurtleGxsSearchResultGroupSummaryItem: public RsItem
{
	public:
        RsTurtleGxsSearchResultGroupSummaryItem(uint16_t service) : RsItem(RS_PKT_VERSION_SERVICE,service,RS_PKT_SUBTYPE_GXS_GROUP_SUMMARY_ITEM){}
        virtual ~RsTurtleGxsSearchResultGroupSummaryItem() {}

		std::list<TurtleGxsInfo> result ;

        void clear() { result.clear() ; }
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

class RsTurtleGxsSearchResultGroupDataItem: public RsItem
{
	public:
        RsTurtleGxsSearchResultGroupDataItem(uint16_t service) : RsItem(RS_PKT_VERSION_SERVICE,service,RS_PKT_SUBTYPE_GXS_GROUP_DATA_ITEM){}
        virtual ~RsTurtleGxsSearchResultGroupDataItem() {}

        unsigned char *encrypted_nxs_group_data;	// data is encrypted with group ID. Only the requester, or anyone who already know the group id can decrypt.
        uint32_t encrypted_nxs_group_data_len ;

        void clear() { free(encrypted_nxs_group_data); encrypted_nxs_group_data=NULL; encrypted_nxs_group_data_len=0; }
	protected:
		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};





#endif // RSGXSITEMS_H
