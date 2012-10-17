#ifndef RSPOSTEDITEMS_H
#define RSPOSTEDITEMS_H

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

#include "rsgxsitems.h"
#include "retroshare/rsposted.h"

const uint8_t RS_PKT_SUBTYPE_POSTED_GRP_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_POSTED_POST_ITEM = 0x03;
const uint8_t RS_PKT_SUBTYPE_POSTED_VOTE_ITEM = 0x04;
const uint8_t RS_PKT_SUBTYPE_POSTED_COMMENT_ITEM = 0x05;

class RsGxsPostedPostItem : public RsGxsMsgItem
{
public:
    RsGxsPostedPostItem() : RsGxsMsgItem(RS_SERVICE_GXSV1_TYPE_POSTED,
                                         RS_PKT_SUBTYPE_POSTED_POST_ITEM)
    {return ; }

    void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);

    RsPostedPost mPost;
};

class RsGxsPostedVoteItem : public RsGxsMsgItem
{
public:
    RsGxsPostedVoteItem() : RsGxsMsgItem(RS_SERVICE_GXSV1_TYPE_POSTED,
                                            RS_PKT_SUBTYPE_POSTED_VOTE_ITEM)
    {return ;}

    void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);

    RsPostedVote mVote;
};

class RsGxsPostedCommentItem : public RsGxsMsgItem
{
public:
    RsGxsPostedCommentItem() : RsGxsMsgItem(RS_SERVICE_GXSV1_TYPE_POSTED,
                                            RS_PKT_SUBTYPE_POSTED_COMMENT_ITEM)
    { return; }

    void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);

    RsPostedComment mComment;
};

class RsGxsPostedGroupItem : public RsGxsGrpItem
{
public:
    RsGxsPostedGroupItem() : RsGxsGrpItem(RS_SERVICE_GXSV1_TYPE_POSTED,
                                          RS_PKT_SUBTYPE_POSTED_GRP_ITEM)
    { return; }

    void clear();
    std::ostream &print(std::ostream &out, uint16_t indent = 0);

    RsPostedGroup mGroup;
};

class RsGxsPostedSerialiser : public RsSerialType
{

    RsGxsPostedSerialiser()
        : RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXSV1_TYPE_PHOTO)
    { return; }

    virtual ~RsGxsPostedSerialiser() { return; }

    uint32_t size(RsItem *item);
    bool serialise(RsItem *item, void *data, uint32_t *size);
    RsItem* deserialise(void *data, uint32_t *size);

private:

    uint32_t sizeGxsPostedPostItem(RsGxsPostedPostItem* item);
    bool serialiseGxsPostedPostItem(RsGxsPostedPostItem* item, void* data, uint32_t *size);
    RsGxsPostedPostItem* deserialiseGxsPostedPostItem(void *data, uint32_t *size);

    uint32_t sizeGxsPostedCommentItem(RsGxsPostedCommentItem* item);
    bool serialiseGxsPostedCommentItem(RsGxsPostedCommentItem* item, void* data, uint32_t *size);
    RsGxsPostedCommentItem* deserialiseGxsPostedCommentItem(void *data, uint32_t *size);

    uint32_t sizeGxsPostedVoteItem(RsGxsPostedVoteItem* item);
    bool serialiseGxsPostedVoteItem(RsGxsPostedVoteItem* item, void* data, uint32_t *size);
    RsGxsPostedVoteItem* deserialiseGxsPostedVoteItem(void *data, uint32_t *size);

    uint32_t sizeGxsPostedGroupItem(RsGxsPostedGroupItem* item);
    bool serialiseGxsPostedGroupItem(RsGxsPostedGroupItem* item, void* data, uint32_t *size);
    RsGxsPostedGroupItem* deserialiseGxsPostedGroupItem(void *data, uint32_t *size);
};


#endif // RSPOSTEDITEMS_H
