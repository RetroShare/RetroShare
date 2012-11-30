#include "p3posted.h"
#include "serialiser/rsposteditems.h"

const uint32_t RsPosted::FLAG_MSGTYPE_COMMENT = 0x0001;
const uint32_t RsPosted::FLAG_MSGTYPE_POST = 0x0002;
const uint32_t RsPosted::FLAG_MSGTYPE_VOTE = 0x0004;

RsPosted *rsPosted = NULL;

RsPostedComment::RsPostedComment(const RsGxsPostedCommentItem & item)
{
   mComment = item.mComment.mComment;
   mMeta = item.meta;
}

p3Posted::p3Posted(RsGeneralDataService *gds, RsNetworkExchangeService *nes)
    : RsGenExchange(gds, nes, new RsGxsPostedSerialiser(), RS_SERVICE_GXSV1_TYPE_POSTED), RsPosted(this), mPostedMutex("Posted")
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

bool p3Posted::getRelatedComment(const uint32_t& token, PostedRelatedCommentResult &comments)
{
    return RsGenExchange::getMsgRelatedDataT<RsGxsPostedCommentItem, RsPostedComment>(token, comments);
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
bool p3Posted::requestCommentRankings(uint32_t &token, const RankType &rType, const RsGxsGrpMsgIdPair &msgId)
{
    token = RsGenExchange::generatePublicToken();

    RsStackMutex stack(mPostedMutex);

    GxsPostedCommentRanking* gpc = new GxsPostedCommentRanking();
    gpc->msgId = msgId;
    gpc->rType = rType;
    gpc->pubToken = token;

    mPendingCommentRanks.insert(std::make_pair(token, gpc));

    return true;
}

bool p3Posted::requestMessageRankings(uint32_t &token, const RankType &rType, const RsGxsGroupId &groupId)
{
    token = RsGenExchange::generatePublicToken();

    RsStackMutex stack(mPostedMutex);
    GxsPostedPostRanking* gp = new GxsPostedPostRanking();
    gp->grpId = groupId;
    gp->rType = rType;
    gp->pubToken = token;

    mPendingPostRanks.insert(std::make_pair(token, gp));

    return true;
}

bool p3Posted::getRanking(const uint32_t &token, PostedRanking &ranking)
{

}

void p3Posted::processRankings()
{
    processMessageRanks();

    processCommentRanks();
}

void p3Posted::processMessageRanks()
{

    RsStackMutex stack(mPostedMutex);
    std::map<uint32_t, GxsPostedPostRanking*>::iterator mit =mPendingPostRanks.begin();

    for(; mit !=mPendingPostRanks.begin(); mit++)
    {
        uint32_t token;
        std::list<RsGxsGroupId> grpL;
        grpL.push_back(mit->second->grpId);
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD;
        RsGenExchange::getTokenService()->requestMsgInfo(token, GXS_REQUEST_TYPE_GROUP_DATA, opts, grpL);
        GxsPostedPostRanking* gp = mit->second;
        gp->reqToken = token;

        while(true)
        {
            uint32_t status = RsGenExchange::getTokenService()->requestStatus(token);

            if(RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE
               == status)
            {
                completePostedPostCalc(gp);
                break;
            }
            else if(RsTokenService::GXS_REQUEST_V2_STATUS_FAILED
                    == status)
            {
                discardCalc(token);
                break;
            }
        }
    }

    mPendingPostRanks.clear();


}

void p3Posted::discardCalc(const uint32_t &token)
{

}

void p3Posted::completePostedPostCalc(GxsPostedPostRanking *gpp)
{
    GxsMsgMetaMap msgMetas;
    getMsgMeta(gpp->reqToken, msgMetas);

    GxsMsgMetaMap::iterator mit = msgMetas.begin();

    for(; mit != msgMetas.end(); mit++ )
    {
        RsGxsMsgMetaData* m = NULL;
        //retrieveScores(m->mServiceString, upVotes, downVotes, nComments);

        // then dependent on rank request type process for that way
    }


}

bool p3Posted::retrieveScores(const std::string &serviceString, uint32_t &upVotes, uint32_t downVotes, uint32_t nComments)
{
    if (2 == sscanf(serviceString.c_str(), "%d %d %d", &upVotes, &downVotes, &nComments))
    {
            return true;
    }

    return false;
}

void p3Posted::processCommentRanks()
{

}
