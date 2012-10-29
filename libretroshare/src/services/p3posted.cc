#include "p3posted.h"
#include "serialiser/rsposteditems.h"

const uint32_t RsPosted::FLAG_MSGTYPE_COMMENT = 0x0001;
const uint32_t RsPosted::FLAG_MSGTYPE_POST = 0x0002;
const uint32_t RsPosted::FLAG_MSGTYPE_VOTE = 0x0004;

RsPosted *rsPosted = NULL;

p3Posted::p3Posted(RsGeneralDataService *gds, RsNetworkExchangeService *nes)
    : RsGenExchange(gds, nes, new RsGxsPostedSerialiser(), RS_SERVICE_GXSV1_TYPE_POSTED), RsPosted(this)
{
}

void p3Posted::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
    receiveChanges(changes);
}

void p3Posted::service_tick()
{

}

bool p3Posted::getGroup(const uint32_t &token, std::vector<RsPostedGroup> &groups)
{
    std::vector<RsGxsGrpItem*> grpData;
    bool ok = RsGenExchange::getGroupData(token, grpData);

    if(ok)
    {
        std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

        for(; vit != grpData.end(); vit++)
        {
                RsGxsPostedGroupItem* item = dynamic_cast<RsGxsPostedGroupItem*>(*vit);
                RsPostedGroup grp = item->mGroup;
                item->mGroup.mMeta = item->meta;
                grp.mMeta = item->mGroup.mMeta;
                delete item;
                groups.push_back(grp);
        }
    }
    return ok;
}

bool p3Posted::getPost(const uint32_t &token, PostedPostResult &posts)
{
    GxsMsgDataMap msgData;
    bool ok = RsGenExchange::getMsgData(token, msgData);

    if(ok)
    {
            GxsMsgDataMap::iterator mit = msgData.begin();

            for(; mit != msgData.end();  mit++)
            {
                    RsGxsGroupId grpId = mit->first;
                    std::vector<RsGxsMsgItem*>& msgItems = mit->second;
                    std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

                    for(; vit != msgItems.end(); vit++)
                    {
                            RsGxsPostedPostItem* item = dynamic_cast<RsGxsPostedPostItem*>(*vit);

                            if(item)
                            {
                                    RsPostedPost post = item->mPost;
                                    post.mMeta = item->meta;
                                    posts[grpId].push_back(post);
                                    delete item;
                            }else
                            {
                                std::cerr << "Not a post Item, deleting!" << std::endl;
                                delete *vit;
                            }
                    }
            }
    }

    return ok;
}

bool p3Posted::getComment(const uint32_t &token, PostedCommentResult &comments)
{
    GxsMsgDataMap msgData;
    bool ok = RsGenExchange::getMsgData(token, msgData);

    if(ok)
    {
            GxsMsgDataMap::iterator mit = msgData.begin();

            for(; mit != msgData.end();  mit++)
            {
                    RsGxsGroupId grpId = mit->first;
                    std::vector<RsGxsMsgItem*>& msgItems = mit->second;
                    std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

                    for(; vit != msgItems.end(); vit++)
                    {
                            RsGxsPostedCommentItem* item = dynamic_cast<RsGxsPostedCommentItem*>(*vit);

                            if(item)
                            {
                                    RsPostedComment comment = item->mComment;
                                    comment.mMeta = item->meta;
                                    comments[grpId].push_back(comment);
                                    delete item;
                            }else
                            {
                                std::cerr << "Not a comment Item, deleting!" << std::endl;
                                delete *vit;
                            }
                    }
            }
    }

    return ok;
}

bool p3Posted::submitGroup(uint32_t &token, RsPostedGroup &group)
{
    RsGxsPostedGroupItem* grpItem = new RsGxsPostedGroupItem();
    grpItem->mGroup = group;
    grpItem->meta = group.mMeta;
    RsGenExchange::publishGroup(token, grpItem);
    return true;
}

bool p3Posted::submitPost(uint32_t &token, RsPostedPost &post)
{
    RsGxsPostedPostItem* postItem = new RsGxsPostedPostItem();
    postItem->mPost = post;
    postItem->meta = post.mMeta;
    postItem->meta.mMsgFlags |= FLAG_MSGTYPE_POST;

    RsGenExchange::publishMsg(token, postItem);
    return true;
}

bool p3Posted::submitVote(uint32_t &token, RsPostedVote &vote)
{
    RsGxsPostedVoteItem* voteItem = new RsGxsPostedVoteItem();
    voteItem->mVote = vote;
    voteItem->meta = vote.mMeta;
    voteItem->meta.mMsgFlags |= FLAG_MSGTYPE_POST;

    RsGenExchange::publishMsg(token, voteItem);
    return true;
}

bool p3Posted::submitComment(uint32_t &token, RsPostedComment &comment)
{
    RsGxsPostedCommentItem* commentItem = new RsGxsPostedCommentItem();
    commentItem->mComment = comment;
    commentItem->meta = comment.mMeta;
    commentItem->meta.mMsgFlags |= FLAG_MSGTYPE_COMMENT;

    RsGenExchange::publishMsg(token, commentItem);
    return true;
}

        // Special Ranking Request.
bool p3Posted::requestRanking(uint32_t &token, RsGxsGroupId groupId)
{

}
