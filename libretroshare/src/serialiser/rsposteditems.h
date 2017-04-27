#ifndef RSPOSTEDITEMS_H
#define RSPOSTEDITEMS_H

#include "rsitems/rsserviceids.h"
#include "rsitems/rsgxscommentitems.h"
#include "rsitems/rsgxsitems.h"

#include "retroshare/rsposted.h"

const uint8_t RS_PKT_SUBTYPE_POSTED_GRP_ITEM  = 0x02;
const uint8_t RS_PKT_SUBTYPE_POSTED_POST_ITEM = 0x03;

class RsGxsPostedGroupItem : public RsGxsGrpItem
{
public:
	RsGxsPostedGroupItem() : RsGxsGrpItem(RS_SERVICE_GXS_TYPE_POSTED, RS_PKT_SUBTYPE_POSTED_GRP_ITEM) {}
	virtual ~RsGxsPostedGroupItem() {}

	void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsPostedGroup mGroup;
};

class RsGxsPostedPostItem : public RsGxsMsgItem
{
public:
	RsGxsPostedPostItem() : RsGxsMsgItem(RS_SERVICE_GXS_TYPE_POSTED, RS_PKT_SUBTYPE_POSTED_POST_ITEM) {}
	virtual ~RsGxsPostedPostItem() {}

	void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	RsPostedPost mPost;
};

class RsGxsPostedSerialiser : public RsGxsCommentSerialiser
{
public:

	RsGxsPostedSerialiser() :RsGxsCommentSerialiser(RS_SERVICE_GXS_TYPE_POSTED) {}

	virtual ~RsGxsPostedSerialiser() {}

    virtual RsItem *create_item(uint16_t service_id,uint8_t item_subtype) const ;
};


#endif // RSPOSTEDITEMS_H
