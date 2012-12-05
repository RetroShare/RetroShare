
#include <algorithm>
#include <math.h>
#include <sstream>

#include "p3posted.h"
#include "gxs/rsgxsflags.h"
#include "serialiser/rsposteditems.h"

#define NUM_TOPICS_TO_GENERATE 7
#define NUM_POSTS_TO_GENERATE 8
#define NUM_VOTES_TO_GENERATE 23

const uint32_t RsPosted::FLAG_MSGTYPE_COMMENT = 0x0001;
const uint32_t RsPosted::FLAG_MSGTYPE_POST = 0x0002;
const uint32_t RsPosted::FLAG_MSGTYPE_VOTE = 0x0004;
const uint32_t RsPosted::FLAG_MSGTYPE_MASK = 0x000f;

#define POSTED_MAX_SERVICE_STRING 50

RsPosted *rsPosted = NULL;

RsPostedComment::RsPostedComment(const RsGxsPostedCommentItem & item)
{
   mComment = item.mComment.mComment;
   mMeta = item.meta;
}

RsPostedVote::RsPostedVote(const RsGxsPostedVoteItem& item)
{
    mDirection = item.mVote.mDirection;
    mMeta = item.meta;
}

p3Posted::p3Posted(RsGeneralDataService *gds, RsNetworkExchangeService *nes)
    : RsGenExchange(gds, nes, new RsGxsPostedSerialiser(), RS_SERVICE_GXSV1_TYPE_POSTED), RsPosted(this), mPostedMutex("Posted"),
    mTokenService(NULL), mGeneratingTopics(true), mGeneratingPosts(false)
{
    mTokenService = RsGenExchange::getTokenService();
}

void p3Posted::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
    receiveChanges(changes);
}

void p3Posted::service_tick()
{
    generateTopics();

    //generatePosts();
}

void p3Posted::generatePosts()
{
    if(mGeneratingPosts)
    {
        // request topics then chose at random which one to use to generate a post about
        uint32_t token;
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
        mTokenService->requestGroupInfo(token, 0, opts);
        double timeDelta = 2.; // slow tick
        while(mTokenService->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
        {
#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif
        }

        std::list<RsGxsGroupId> grpIds;
        RsGenExchange::getGroupList(token, grpIds);


        // for each group generate NUM_POSTS_TO_GENERATE posts
        std::list<RsGxsGroupId>::iterator lit = grpIds.begin();

        for(; lit != grpIds.end(); lit++)
        {
            RsGxsGroupId& grpId = *lit;

            std::vector<uint32_t> tokens;

            for(int i=0; i < NUM_POSTS_TO_GENERATE; i++)
            {
                std::ostringstream ostrm;
                ostrm << i;
                std::string link = "link" + ostrm.str();

                RsPostedPost post;
                post.mLink = link;
                post.mNotes = link;
                post.mMeta.mMsgName = link;
                post.mMeta.mGroupId = grpId;

                submitPost(token, post);
                tokens.push_back(token);
            }

            while(!tokens.empty())
            {
                std::vector<uint32_t>::iterator vit = tokens.begin();

                for(; vit != tokens.end(); )
                {
                    if(mTokenService->requestStatus(*vit) != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
                       vit = tokens.erase(vit);
                    else
                        vit++;
                }
            }
        }




        // stop generating posts after acknowledging all the ones you created
        mGeneratingPosts = false;
    }
}

void p3Posted::generateTopics()
{
    if(mGeneratingTopics)
    {
        std::vector<uint32_t> tokens;

        for(int i=0; i < NUM_TOPICS_TO_GENERATE; i++)
        {
            std::ostringstream strm;
            strm << i;
            std::string topicName = "Topic " + strm.str();

            RsPostedGroup topic;
            topic.mMeta.mGroupName = topicName;

            uint32_t token;
            submitGroup(token, topic);
            tokens.push_back(token);
        }


        while(!tokens.empty())
        {
            std::vector<uint32_t>::iterator vit = tokens.begin();

            for(; vit != tokens.end(); )
            {
                if(mTokenService->requestStatus(*vit) != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
                   vit = tokens.erase(vit);
                else
                    vit++;
            }
        }

        mGeneratingTopics = false;
        mGeneratingPosts = true;
    }

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
    voteItem->meta.mMsgFlags |= FLAG_MSGTYPE_VOTE;

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

    // go through all pending posts
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
            uint32_t status = mTokenService->requestStatus(token);

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
    mTokenService->cancelRequest(token);
}

bool PostedTopScoreComp(const PostedScore& i, const PostedScore& j)
{
    if((i.upVotes + (-i.downVotes)) == (j.upVotes + (-j.downVotes))){
        return i.date < j.date;
    }else
        return (i.upVotes + (-i.downVotes)) < (j.upVotes + (-j.downVotes));
}

bool PostedNewScoreComp(const PostedScore& i, const PostedScore& j)
{
    return i.date < j.date;
}

bool PostedBestScoreComp(const PostedScore& i, const PostedScore& j)
{

//     n = ups + downs  if n == 0: return 0  z = 1.0
//    #1.0 = 85%, 1.6 = 95% phat = float(ups)
//     / n return sqrt(phat+z*z/(2*n)-z*((phat*(1-phat)+z*z/(4*n))/n))/(1+z*z/n)
//         def confidence(ups, downs): if ups + downs == 0: return 0 else:
//         return _confidence(ups, downs)
    // very expensive!!

    static float z = 1.0;
    float phat;

    float i_score;
    int n = i.upVotes + (-i.downVotes);
    if(n==0)
        i_score = 0.;
    else
    {
        phat = float(i.upVotes);
        i_score = sqrt(phat+z*z/(2*n)-z*((phat*(1-phat)+z*z/(4*n))/n))/(1+z*z/n);
    }

    float j_score;
    n = j.upVotes + (-j.downVotes);
    if(n==0)
        j_score = 0.;
    else
    {
        phat = float(j.upVotes);
        j_score = sqrt(phat+z*z/(2*n)-z*((phat*(1-phat)+z*z/(4*n))/n))/(1+z*z/n);
    }

    if(j_score == i_score)
        return i.date < j.date;
    else
        return i_score < j_score;
}

void p3Posted::completePostedPostCalc(GxsPostedPostRanking *gpp)
{
    GxsMsgMetaMap msgMetas;

    if(getMsgMeta(gpp->reqToken, msgMetas))
    {
        std::vector<RsMsgMetaData> msgMetaV = msgMetas[gpp->grpId];
        switch(gpp->rType)
        {
            case NewRankType:
                calcPostedPostRank(msgMetaV, gpp->rankingResult, PostedNewScoreComp);
                break;
            case TopRankType:
                calcPostedPostRank(msgMetaV, gpp->rankingResult, PostedTopScoreComp);
                break;
            default:
                std::cerr << "Unknown ranking tpye: " << gpp->rType << std::endl;
        }
    }
}


void p3Posted::calcPostedPostRank(const std::vector<RsMsgMetaData > msgMeta, PostedRanking &ranking,
                                  bool comp(const PostedScore &, const PostedScore &)) const
{

    std::vector<RsMsgMetaData>::const_iterator cit = msgMeta.begin();
    std::vector<PostedScore> scores;

    for(; cit != msgMeta.begin(); )
    {
        const RsMsgMetaData& m  = *cit;
        uint32_t upVotes, downVotes, nComments;
        retrieveScores(m.mServiceString, upVotes, downVotes, nComments);

        PostedScore c;
        c.upVotes = upVotes;
        c.downVotes = downVotes;
        c.date = m.mPublishTs;
        scores.push_back(c);
    }

    std::sort(scores.begin(), scores.end(), comp);

    std::vector<PostedScore>::iterator vit = scores.begin();

    int i = 1;
    for(; vit != scores.end(); vit)
    {
        const PostedScore& p = *vit;
        ranking.insert(std::make_pair(p.msgId, i++));
    }
}

void p3Posted::calcPostedCommentsRank(const std::map<RsGxsMessageId, std::vector<RsGxsMessageId> > &msgBranches,
                                      std::map<RsGxsMessageId, RsMsgMetaData >& msgMetas, PostedRanking &ranking, bool comp(const PostedScore &, const PostedScore &)) const
{

    std::map<RsGxsMessageId, std::vector<RsGxsMessageId> >::const_iterator cit = msgBranches.begin();

    for(; cit != msgBranches.end(); cit++)
    {
        const std::vector<RsGxsMessageId>& branch = cit->second;
        std::vector<PostedScore> scores;

        std::vector<RsGxsMessageId>::const_iterator vit = branch.begin();

        for(; vit != branch.end(); vit++)
        {

            std::map<RsGxsMessageId, RsMsgMetaData >::iterator mit =
                    msgMetas.find(*vit);

            if(mit != msgMetas.end())
            {
                uint32_t upVotes, downVotes, nComments;

                const RsMsgMetaData& m = mit->second;
                retrieveScores(m.mServiceString, upVotes, downVotes, nComments);

                PostedScore c;
                c.upVotes = upVotes;
                c.downVotes = downVotes;
                c.date = m.mPublishTs;
                scores.push_back(c);
            }
        }

        std::sort(scores.begin(), scores.end(), comp);

        std::vector<PostedScore>::iterator cvit = scores.begin();

        int i = 1;
        for(; cvit != scores.end(); cvit)
        {
            const PostedScore& p = *cvit;
            ranking.insert(std::make_pair(p.msgId, i++));
        }
    }

}

void p3Posted::completePostedCommentRanking(GxsPostedCommentRanking *gpc)
{
    GxsMsgRelatedMetaMap msgMetas;

    if(getMsgRelatedMeta(gpc->reqToken, msgMetas))
    {

        // create map of msgs
        std::vector<RsMsgMetaData>& msgV = msgMetas[gpc->msgId];
        std::map<RsGxsMessageId, std::vector<RsGxsMessageId> > msgBranches;
        std::map<RsGxsMessageId, RsMsgMetaData> remappedMsgMeta;

        std::vector<RsMsgMetaData>::iterator vit = msgV.begin();

        for(; vit != msgV.end(); vit++)
        {
           const RsMsgMetaData& m = *vit;

           if(!m.mParentId.empty())
           {
               msgBranches[m.mParentId].push_back(m.mMsgId);
           }

           remappedMsgMeta.insert(std::make_pair(m.mMsgId, m));
        }

        switch(gpc->rType)
        {
            case BestRankType:
                calcPostedCommentsRank(msgBranches, remappedMsgMeta, gpc->result, PostedBestScoreComp);
                break;
            case TopRankType:
                calcPostedCommentsRank(msgBranches, remappedMsgMeta, gpc->result, PostedTopScoreComp);
                break;
            case NewRankType:
                calcPostedCommentsRank(msgBranches, remappedMsgMeta, gpc->result, PostedNewScoreComp);
                break;
            default:
                std::cerr << "Unknown Rank type" << gpc->rType << std::endl;
                break;
        }
    }
}

bool p3Posted::retrieveScores(const std::string &serviceString, uint32_t &upVotes, uint32_t downVotes, uint32_t nComments) const
{
    if (3 == sscanf(serviceString.c_str(), "%d %d %d", &upVotes, &downVotes, &nComments))
    {
       return true;
    }

    return false;
}

bool p3Posted::storeScores(std::string &serviceString, uint32_t &upVotes, uint32_t downVotes, uint32_t nComments) const
{
    char line[POSTED_MAX_SERVICE_STRING];

    bool ok = snprintf(line, POSTED_MAX_SERVICE_STRING, "%d %d %d", upVotes, downVotes, nComments) > -1;

    serviceString = line;
    return ok;
}
void p3Posted::processCommentRanks()
{

}


void p3Posted::updateVotes()
{
    if(!mUpdateTokenQueued)
    {
        mUpdateTokenQueued = true;

        switch(mUpdatePhase)
        {
//            case UPDATE_PHASE_GRP_REQUEST:
//                {
//                    updateRequestGroups(mUpda);
//                    break;
//                }
//            case UPDATE_PHASE_GRP_MSG_REQUEST:
//                {
//                    updateRequestMessages(mVoteUpdataToken);
//                    break;
//                }
//            case UPDATE_VOTE_COMMENT_REQUEST:
//                {
//                    updateRequestVotesComments(mVoteUpdataToken);
//                    break;
//                }
//            case UPDATE_COMPLETE_UPDATE:
//            {
//                updateCompleteUpdate();
//                break;
//            }
//            default:
//                break;
        }

        // first get all msgs for groups for which you are subscribed to.
        // then request comments for them

    }
}

bool p3Posted::updateRequestGroups(uint32_t &token)
{


    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
    opts.mSubscribeMask = GXS_SERV::GROUP_SUBSCRIBE_MASK;
    opts.mSubscribeFilter = GXS_SERV::GROUP_SUBSCRIBE_ADMIN |
                            GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED;
    mTokenService->requestGroupInfo(token, 0, opts);

    mUpdatePhase = UPDATE_PHASE_GRP_MSG_REQUEST;
}

bool p3Posted::updateRequestMessages(uint32_t &token)
{

    uint32_t status = mTokenService->requestStatus(token);

    if(status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
    {
        std::list<RsGxsGroupId> grpIds;
        RsGenExchange::getGroupList(token, grpIds);
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD;
        mTokenService->requestMsgInfo(token, 0, opts, grpIds);
        mUpdatePhase = UPDATE_VOTE_COMMENT_REQUEST;
        return true;
    }
    else if(status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
    {
        mTokenService->cancelRequest(token);
        return false;
    }
}

bool p3Posted::updateRequestVotesComments(uint32_t &token)
{

    uint32_t status = mTokenService->requestStatus(token);

    if(status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
    {

        GxsMsgIdResult result;
        RsGenExchange::getMsgList(token, result);

        std::vector<RsGxsGrpMsgIdPair> msgIds;

        GxsMsgIdResult::iterator mit = result.begin();

        for(; mit != result.end(); mit++)
        {
            std::vector<RsGxsMessageId>& msgIdV = mit->second;
            std::vector<RsGxsMessageId>::const_iterator cit = msgIdV.begin();

            for(; cit != msgIdV.end(); cit++)
                msgIds.push_back(std::make_pair(mit->first, *cit));
        }

        // only need ids for comments
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_PARENT;
        opts.mMsgFlagMask = RsPosted::FLAG_MSGTYPE_MASK;
        opts.mMsgFlagFilter = RsPosted::FLAG_MSGTYPE_COMMENT;
        mTokenService->requestMsgRelatedInfo(mCommentToken, 0, opts, msgIds);

        // need actual data from votes
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_PARENT;
        opts.mMsgFlagMask = RsPosted::FLAG_MSGTYPE_MASK;
        opts.mMsgFlagFilter = RsPosted::FLAG_MSGTYPE_VOTE;
        mTokenService->requestMsgRelatedInfo(mVoteToken, 0, opts, msgIds);

        mUpdatePhase = UPDATE_COMPLETE_UPDATE;
        mMsgsPendingUpdate = msgIds;

        return true;
    }
    else if(status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
    {
        mTokenService->cancelRequest(token);
        return false;
    }
}

bool p3Posted::updateCompleteUpdate()
{
    uint32_t commentStatus = mTokenService->requestStatus(mCommentToken);
    uint32_t voteStatus = mTokenService->requestStatus(mVoteToken);

    bool ready = commentStatus == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE;
    ready &= voteStatus == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE;

    bool failed = commentStatus == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED;
    failed &= voteStatus == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED;

    if(ready)
    {
        std::map<RsGxsGrpMsgIdPair, std::vector<RsGxsMessageId> > msgCommentIds;
        std::map<RsGxsGrpMsgIdPair, std::vector<RsPostedVote> > votes;
        getMsgRelatedDataT<RsGxsPostedVoteItem, RsPostedVote>(mVoteToken, votes);
        std::vector<RsGxsGrpMsgIdPair>::iterator vit = mMsgsPendingUpdate.begin();

        for(; vit != mMsgsPendingUpdate.end();vit++)
        {
            updateMsg(*vit, votes[*vit], msgCommentIds[*vit]);
        }
        mUpdatePhase = 0;
    }
    else if(failed)
    {
        mTokenService->cancelRequest(mCommentToken);
        mTokenService->cancelRequest(mVoteToken);
        return false;
    }else
    {
        return true;
    }
}

bool p3Posted::updateMsg(const RsGxsGrpMsgIdPair& msgId, const std::vector<RsPostedVote> &msgVotes,
                         const std::vector<RsGxsMessageId>& msgCommentIds)
{

    uint32_t nComments = msgCommentIds.size();
    uint32_t nUp = 0, nDown = 0;

    std::vector<RsPostedVote>::const_iterator cit = msgVotes.begin();

    for(; cit != msgVotes.end(); cit++)
    {
        const RsPostedVote& v = *cit;

        if(v.mDirection == 0)
        {
            nDown++;
        }else
        {
            nUp++;
        }
    }
    std::string servStr;
    storeScores(servStr, nUp, nDown, nComments);
    uint32_t token;
    setMsgServiceString(token, msgId, servStr);
}

