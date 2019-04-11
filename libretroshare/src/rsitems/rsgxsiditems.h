/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsiditems.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_GXS_IDENTITY_ITEMS_H
#define RS_GXS_IDENTITY_ITEMS_H

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "rsgxsitems.h"
#include "retroshare/rsidentity.h"

//const uint8_t RS_PKT_SUBTYPE_GXSID_GROUP_ITEM_deprecated   = 0x02;

const uint8_t RS_PKT_SUBTYPE_GXSID_GROUP_ITEM      = 0x02;
const uint8_t RS_PKT_SUBTYPE_GXSID_OPINION_ITEM    = 0x03;
const uint8_t RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM    = 0x04;
const uint8_t RS_PKT_SUBTYPE_GXSID_LOCAL_INFO_ITEM = 0x05;

class RsGxsIdItem: public RsGxsGrpItem
{
    public:
        RsGxsIdItem(uint8_t item_subtype) : RsGxsGrpItem(RS_SERVICE_GXS_TYPE_GXSID,item_subtype) {}
};

class RsGxsIdGroupItem : public RsGxsIdItem
{
public:

    RsGxsIdGroupItem():  RsGxsIdItem(RS_PKT_SUBTYPE_GXSID_GROUP_ITEM) {}
    virtual ~RsGxsIdGroupItem() {}

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
    virtual void clear();

    bool fromGxsIdGroup(RsGxsIdGroup &group, bool moveImage);
    bool toGxsIdGroup(RsGxsIdGroup &group, bool moveImage);

    Sha1CheckSum mPgpIdHash;
    // Need a signature as proof - otherwise anyone could add others Hashes.
    // This is a string, as the length is variable.
    std::string mPgpIdSign;

    // Recognition Strings. MAX# defined above.
    std::list<std::string> mRecognTags;

    // Avatar
    RsTlvImage mImage ;
};
class RsGxsIdLocalInfoItem : public RsGxsIdItem
{

public:

    RsGxsIdLocalInfoItem():  RsGxsIdItem(RS_PKT_SUBTYPE_GXSID_LOCAL_INFO_ITEM) {}
    virtual ~RsGxsIdLocalInfoItem() {}

    virtual void clear();

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    std::map<RsGxsId,rstime_t> mTimeStamps ;
    std::set<RsGxsId> mContacts ;
};

#if 0
class RsGxsIdOpinionItem : public RsGxsMsgItem
{
public:

    RsGxsIdOpinionItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_GXSID,
			RS_PKT_SUBTYPE_GXSID_OPINION_ITEM) {return; }
        virtual ~RsGxsIdOpinionItem() { return;}
        void clear();
    virtual bool serialise(void *data,uint32_t& size) = 0 ;
    virtual uint32_t serial_size() = 0 ;

    std::ostream &print(std::ostream &out, uint16_t indent = 0);
	RsGxsIdOpinion opinion;
};

class RsGxsIdCommentItem : public RsGxsMsgItem
{
public:

    RsGxsIdCommentItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_GXSID,
                                          RS_PKT_SUBTYPE_GXSID_COMMENT_ITEM) { return; }
    virtual ~RsGxsIdCommentItem() { return; }
    void clear();
    virtual bool serialise(void *data,uint32_t& size) = 0 ;
    virtual uint32_t serial_size() = 0 ;

    std::ostream &print(std::ostream &out, uint16_t indent = 0);
    RsGxsIdComment comment;

};
#endif

class RsGxsIdSerialiser : public RsServiceSerializer
{
public:
    RsGxsIdSerialiser() :RsServiceSerializer(RS_SERVICE_GXS_TYPE_GXSID) {}
    virtual     ~RsGxsIdSerialiser() {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};

#endif /* RS_GXS_IDENTITY_ITEMS_H */
