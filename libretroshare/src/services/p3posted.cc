
#include <algorithm>
#include <math.h>
#include <sstream>
#include <stdio.h>

#include "p3posted.h"
#include "gxs/rsgxsflags.h"
#include "serialiser/rsposteditems.h"

#define UPDATE_PHASE_GRP_REQUEST 1
#define UPDATE_PHASE_MSG_REQUEST 2
#define UPDATE_PHASE_VOTE_COMMENT_REQUEST 3
#define UPDATE_PHASE_VOTE_COUNT 4
#define UPDATE_PHASE_COMMENT_COUNT 5
#define UPDATE_PHASE_COMPLETE 6

#define NUM_TOPICS_TO_GENERATE 5
#define NUM_POSTS_TO_GENERATE 7
#define NUM_VOTES_TO_GENERATE 11
#define NUM_COMMENTS_TO_GENERATE 3

#define VOTE_UPDATE_PERIOD 5 // 20 seconds
#define HOT_PERIOD 4 // 4 secs for testing prob 1 to 2 days in practice

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
    mTokenService(NULL), mGeneratingTopics(true), mGeneratingPosts(false), mRequestPhase1(true), mRequestPhase2(false),
    mRequestPhase3(false), mGenerateVotesAndComments(false)
{
    mPostUpdate = false;
    mLastUpdate = time(NULL);
    mUpdatePhase = UPDATE_PHASE_GRP_REQUEST;

    mTokenService = RsGenExchange::getTokenService();
}

void p3Posted::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
    receiveChanges(changes);
}

void p3Posted::service_tick()
{

    generateTopics();
    generatePosts();
    generateVotesAndComments();

//    time_t now = time(NULL);
//
//    if((now > (time_t) (VOTE_UPDATE_PERIOD + mLastUpdate)) &&
//       (mUpdatePhase == UPDATE_PHASE_GRP_REQUEST))
//    {
//        mPostUpdate = true;
//        mLastUpdate = time(NULL);
//    }
//
//    updateVotes();
//
//    processRankings();
}

void p3Posted::generateVotesAndComments()
{
    if(mGenerateVotesAndComments)
    {

        if(mRequestPhase1)
        {
            // request topics then chose at random which one to use to generate a post about
            RsTokReqOptions opts;
            opts.mReqType = GXS_REQUEST_TYPE_MSG_IDS;
            opts.mMsgFlagFilter = RsPosted::FLAG_MSGTYPE_POST;
            opts.mMsgFlagMask = RsPosted::FLAG_MSGTYPE_MASK;

            mTokenService->requestMsgInfo(mToken, 0, opts, mGrpIds);
            mRequestPhase1 = false;
            mRequestPhase2 = true;
            return;
        }

        if(mRequestPhase2)
        {

            if(mTokenService->requestStatus(mToken) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
            {
                GxsMsgIdResult msgIds;
                RsGenExchange::getMsgList(mToken, msgIds);

                // for each msg generate a random number of votes and comments


                GxsMsgIdResult::iterator mit = msgIds.begin();

                for(; mit != msgIds.end(); mit++)
                {
                    const RsGxsGroupId& grpId = mit->first;
                    std::vector<RsGxsMessageId>& msgIdsV = mit->second;
                    std::vector<RsGxsMessageId>::iterator vit = msgIdsV.begin();

                    for(; vit != msgIdsV.end(); vit++)
                    {
                        const RsGxsMessageId& msgId = *vit;

                        int nVotes = rand()%NUM_VOTES_TO_GENERATE;
                        uint32_t token;

                        for(int i=1; i < nVotes; i++)
                        {
                            RsPostedVote v;

                            v.mDirection = (rand()%10 > 3) ? 0 : 1;
                            v.mMeta.mParentId = msgId;
                            v.mMeta.mGroupId = grpId;

                            std::ostringstream ostrm;
                            ostrm << i;

                            v.mMeta.mMsgName = "Vote " + ostrm.str();



                            submitVote(token, v);
                            mTokens.push_back(token);

                        }

                        int nComments = rand()%NUM_COMMENTS_TO_GENERATE;

                        // single level comments for now
                        for(int i=1; i < nComments; i++)
                        {
                            RsPostedComment c;

                            std::ostringstream ostrm;
                            ostrm << i;

                            c.mComment = "Comment " + ostrm.str();
                            c.mMeta.mParentId = msgId;
                            c.mMeta.mGroupId = grpId;
                            c.mMeta.mThreadId = msgId;

                            submitComment(token, c);
                            mTokens.push_back(token);

                        }

                    }
                }
                mRequestPhase2 = false;
                mRequestPhase3 = true;
            }
        }

        if(mRequestPhase3)
        {
            if(!mTokens.empty())
            {
                std::vector<uint32_t>::iterator vit = mTokens.begin();

                for(; vit != mTokens.end(); )
                {
                    if(mTokenService->requestStatus(*vit) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
                       vit = mTokens.erase(vit);
                    else
                       vit++;
                }
            }else
            {
                // stop generating posts after acknowledging all the ones you created
                mGeneratingPosts = false;
                mRequestPhase3 = false;
            }
        }

    }
}

void p3Posted::generatePosts()
{
    if(mGeneratingPosts)
    {


        if(mRequestPhase1)
        {
            // request topics then chose at random which one to use to generate a post about
            RsTokReqOptions opts;
            opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
            mTokenService->requestGroupInfo(mToken, 0, opts);
            mRequestPhase1 = false;
            return;
        }
        else if(!mRequestPhase2)
        {
            if(mTokenService->requestStatus(mToken) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
            {
                RsGenExchange::getGroupList(mToken, mGrpIds);

                mRequestPhase2 = true;
            }
        }

        if(mRequestPhase2)
        {
            // for each group generate NUM_POSTS_TO_GENERATE posts
            std::list<RsGxsGroupId>::iterator lit = mGrpIds.begin();

            for(; lit != mGrpIds.end(); lit++)
            {
                RsGxsGroupId& grpId = *lit;

                for(int i=0; i < NUM_POSTS_TO_GENERATE; i++)
                {
                    std::ostringstream ostrm;
                    ostrm << i;
                    std::string link = "link " + ostrm.str();

                    RsPostedPost post;
                    post.mLink = link;
                    post.mNotes = link;
                    post.mMeta.mMsgName = link;
                    post.mMeta.mGroupId = grpId;

                    uint32_t token;
                    submitPost(token, post);
                    mTokens.push_back(token);
                }
            }

            mRequestPhase2 = false;
            mRequestPhase3 = true;

        }
        else if(mRequestPhase3)
        {

            if(!mTokens.empty())
            {
                std::vector<uint32_t>::iterator vit = mTokens.begin();

                for(; vit != mTokens.end(); )
                {
                    if(mTokenService->requestStatus(*vit) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
                       vit = mTokens.erase(vit);
                    else
                        vit++;
                }
            }else
            {
                // stop generating posts after acknowledging all the ones you created
                mGeneratingPosts = false;
                mRequestPhase3 = false;
                mGenerateVotesAndComments = true;
                mRequestPhase1 = true;
            }

        }
    }
}

void p3Posted::generateTopics()
{
    if(mGeneratingTopics)
    {
        if(mRequestPhase1)
        {


            for(int i=0; i < NUM_TOPICS_TO_GENERATE; i++)
            {
                std::ostringstream strm;
                strm << i;
                std::string topicName = "Topic " + strm.str();

                RsPostedGroup topic;
                topic.mMeta.mGroupName = topicName;

                uint32_t token;
                submitGroup(token, topic);
                mTokens.push_back(token);
            }

            mRequestPhase1 = false;
        }
        else
        {

            if(!mTokens.empty())
            {
                std::vector<uint32_t>::iterator vit = mTokens.begin();

                for(; vit != mTokens.end(); )
                {
                    if(mTokenService->requestStatus(*vit) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
                    {
                        RsGxsGroupId grpId;
                        RsGenExchange::acknowledgeTokenGrp(*vit, grpId);
                        vit = mTokens.erase(vit);
                    }
                    else
                        vit++;
                }
            }
            else
            {
                mGeneratingPosts = true;
                mGeneratingTopics = false;
                mRequestPhase1 = true;
            }
        }
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

bool p3Posted::requestPostRankings(uint32_t &token, const RankType &rType, const RsGxsGroupId &groupId)
{
    token = RsGenExchange::generatePublicToken();

    RsStackMutex stack(mPostedMutex);
    GxsPostedPostRanking* gp = new GxsPostedPostRanking();
    gp->grpId = groupId;
    gp->rType = rType;
    gp->pubToken = token;
    gp->rankingResult.rType = gp->rType;
    gp->rankingResult.grpId = gp->grpId;

    mPendingPostRanks.push_back(gp);

    return true;
}

bool p3Posted::getPostRanking(const uint32_t &token, RsPostedPostRanking &ranking)
{
    RsStackMutex stack(mPostedMutex);

    if( mCompletePostRanks.find(token) == mCompletePostRanks.end()) return false;

    ranking = mCompletePostRanks[token];
    mCompletePostRanks.erase(token);

    // put this in service tick as it's blocking
    // and likely costly
    disposeOfPublicToken(token);

    return true;
}

void p3Posted::processRankings()
{
      processPostRanks();

    //processCommentRanks();
}

void p3Posted::processPostRanks()
{

    RsStackMutex stack(mPostedMutex);

    std::vector<GxsPostedPostRanking*>::iterator vit = mPendingPostRanks.begin();

    // go through all pending posts
    for(; vit !=mPendingPostRanks.end(); )
    {
        GxsPostedPostRanking* gp = *vit;
        uint32_t token;
        std::list<RsGxsGroupId> grpL;
        grpL.push_back(gp->grpId);

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_META;
        opts.mMsgFlagFilter = RsPosted::FLAG_MSGTYPE_POST;
        opts.mMsgFlagMask = RsPosted::FLAG_MSGTYPE_MASK;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD;

        RsGenExchange::getTokenService()->requestMsgInfo(token, GXS_REQUEST_TYPE_GROUP_DATA, opts, grpL);


        gp->reqToken = token;

        vit = mPendingPostRanks.erase(vit);
        mCompletionPostRanks.push_back(gp);
    }

    mPendingPostRanks.clear();

    vit = mCompletionPostRanks.begin();

    for(; vit != mCompletionPostRanks.end(); )
    {
        bool ok = false;
        GxsPostedPostRanking *gp = *vit;
        uint32_t status = mTokenService->requestStatus(gp->reqToken);

        if(RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE
           == status)
        {
            ok = completePostedPostCalc(gp);

            if(ok)
            {
                mCompletePostRanks.insert(
                    std::make_pair(gp->pubToken, gp->rankingResult));
            }
        }
        else if(RsTokenService::GXS_REQUEST_V2_STATUS_FAILED
                == status)
        {
            discardCalc(gp->reqToken);
            ok = false;
        }else
        {
            vit++;
            continue;
        }

        if(ok)
        {
            updatePublicRequestStatus(gp->pubToken, RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE);
        }
        else
        {
            updatePublicRequestStatus(gp->pubToken, RsTokenService::GXS_REQUEST_V2_STATUS_FAILED);
        }

        vit = mCompletionPostRanks.erase(vit);
        delete gp;
    }

}

void p3Posted::discardCalc(const uint32_t &token)
{
    mTokenService->cancelRequest(token);
}

bool PostedTopScoreComp(const PostedScore& i, const PostedScore& j)
{
    int32_t i_score = (int32_t)i.upVotes - (int32_t)i.downVotes;
    int32_t j_score = (int32_t)j.upVotes - (int32_t)j.downVotes;

    if(i_score == j_score){
        return i.date > j.date;
    }else
    {
        return i_score > j_score;
    }
}


bool PostedHotScoreComp(const PostedScore& i, const PostedScore& j)
{
    int32_t i_score = (int32_t)i.upVotes - (int32_t)i.downVotes;
    int32_t j_score = (int32_t)j.upVotes - (int32_t)j.downVotes;

    time_t now = time(NULL);
    long long td = now - i.date;

    if(td == 0)
    {
        return i.date > j.date;
    }

    int y;
    if(i_score > 0)
    {
        y = 1;
    }else if(i_score == 0)
    {
        y = 0;
    }else
    {
        y = -1;
    }

    double z;
    if(abs(i_score) >= 1)
    {
        z = abs(i_score);
    }
    else
    {
        z = 1;
    }

    double i_hot_score = log10(z) + ( ((double)(y*td))
                                      / HOT_PERIOD );

    if(j_score > 0)
    {
        y = 1;
    }else if(j_score == 0)
    {
        y = 0;
    }else
    {
        y = -1;
    }


    if(abs(j_score) >= 1)
    {
        z = abs(j_score);
    }
    else
    {
        z = 1;
    }

    td = now - j.date;

    double j_hot_score = log10(z) + ( ((double)(y*td))
                                      / HOT_PERIOD );
    if(i_hot_score == j_hot_score)
    {
        return i.date > j.date;
    }else
    {
        return i_hot_score > j_hot_score;
    }

}

bool PostedNewScoreComp(const PostedScore& i, const PostedScore& j)
{
    return i.date > j.date;
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

bool p3Posted::completePostedPostCalc(GxsPostedPostRanking *gpp)
{
    GxsMsgMetaMap msgMetas;

    if(getMsgMeta(gpp->reqToken, msgMetas))
    {
        std::vector<RsMsgMetaData>& msgMetaV = msgMetas[gpp->grpId];
        switch(gpp->rType)
        {
            case NewRankType:
                calcPostedPostRank(msgMetaV, gpp->rankingResult.ranking, PostedNewScoreComp);
                break;
            case HotRankType:
                calcPostedPostRank(msgMetaV, gpp->rankingResult.ranking, PostedHotScoreComp);
                break;
            case TopRankType:
                calcPostedPostRank(msgMetaV, gpp->rankingResult.ranking, PostedTopScoreComp);
                break;
            default:
                std::cerr << "Unknown ranking tpye: " << gpp->rType << std::endl;
                break;
        }
        return true;
    }else
        return false;
}


void p3Posted::calcPostedPostRank(const std::vector<RsMsgMetaData > msgMeta, PostedRanking &ranking,
                                  bool comp(const PostedScore &, const PostedScore &)) const
{

    std::vector<RsMsgMetaData>::const_iterator cit = msgMeta.begin();
    std::vector<PostedScore> scores;

    for(; cit != msgMeta.end(); cit++)
    {
        const RsMsgMetaData& m  = *cit;
        uint32_t upVotes, downVotes, nComments;
        retrieveScores(m.mServiceString, upVotes, downVotes, nComments);

        PostedScore c;
        c.upVotes = upVotes;
        c.downVotes = downVotes;
        c.date = m.mPublishTs;
        c.msgId = m.mMsgId;
        scores.push_back(c);
    }

    std::sort(scores.begin(), scores.end(), comp);


    for(int i = 0; i < scores.size(); i++)
    {
        const PostedScore& p = scores[i];
        ranking.insert(std::make_pair(i+1, p.msgId));
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
            ranking.insert(std::make_pair(i++, p.msgId));
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
            case HotRankType:
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

bool p3Posted::retrieveScores(const std::string &serviceString, uint32_t &upVotes, uint32_t& downVotes, uint32_t& nComments) const
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
    // any request failure stops  update process

    if(mPostUpdate)
    {
        switch(mUpdatePhase)
        {
            case UPDATE_PHASE_GRP_REQUEST:
                {
                    mPostUpdate = updateRequestGroups();
                    break;
                }
            case UPDATE_PHASE_MSG_REQUEST:
                {
                    mPostUpdate = updateRequestMessages();
                    break;
                }
            case UPDATE_PHASE_VOTE_COMMENT_REQUEST:
                {
                    mPostUpdate = updateRequestVotesComments();
                    break;
                }
            case UPDATE_PHASE_VOTE_COUNT:
                {
                    mPostUpdate = updateCompleteVotes();
                    break;
                }
            case UPDATE_PHASE_COMMENT_COUNT:
                {
                    mPostUpdate = updateCompleteComments();
                    break;
                }
            case UPDATE_PHASE_COMPLETE:
                {
                    updateComplete();
                    break;
                }
            default:
                {
                    std::cerr << "Unknown update phase, we should not be here!" << std::endl;
                    break;
                }
        }
    }
}

bool p3Posted::updateRequestGroups()
{


    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
   // opts.mSubscribeMask = GXS_SERV::GROUP_SUBSCRIBE_MASK;
//    opts.mSubscribeFilter = GXS_SERV::GROUP_SUBSCRIBE_ADMIN |
//                            GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED;
    mTokenService->requestGroupInfo(mUpdateRequestGroup, 0, opts);

    mUpdatePhase = UPDATE_PHASE_MSG_REQUEST;
}

bool p3Posted::updateRequestMessages()
{

    uint32_t status = mTokenService->requestStatus(mUpdateRequestGroup);

    if(status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
    {
        std::list<RsGxsGroupId> grpIds;
        RsGenExchange::getGroupList(mUpdateRequestGroup, grpIds);
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_MSG_META;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD;
        mTokenService->requestMsgInfo(mUpdateRequestMessages, 0, opts, grpIds);
        mUpdatePhase = UPDATE_PHASE_VOTE_COMMENT_REQUEST;
        return true;
    }
    else if(status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
    {
        mTokenService->cancelRequest(mUpdateRequestGroup);
        return false;
    }

    return true;
}

bool p3Posted::updateRequestVotesComments()
{

    uint32_t status = mTokenService->requestStatus(mUpdateRequestMessages);

    if(status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
    {

        RsGenExchange::getMsgMeta(mUpdateRequestMessages, mMsgMetaUpdate);

        std::vector<RsGxsGrpMsgIdPair> msgIds;

        GxsMsgMetaMap::iterator mit = mMsgMetaUpdate.begin();

        for(; mit != mMsgMetaUpdate.end(); mit++)
        {
            std::vector<RsMsgMetaData>& msgIdV = mit->second;
            std::vector<RsMsgMetaData>::const_iterator cit = msgIdV.begin();

            for(; cit != msgIdV.end(); cit++)
                msgIds.push_back(std::make_pair(mit->first, cit->mMsgId));
        }

        RsTokReqOptions opts;
        // only need ids for comments

        opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_IDS;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_PARENT;
        opts.mMsgFlagMask = RsPosted::FLAG_MSGTYPE_MASK;
        opts.mMsgFlagFilter = RsPosted::FLAG_MSGTYPE_COMMENT;
        mTokenService->requestMsgRelatedInfo(mUpdateRequestComments, 0, opts, msgIds);

        // need actual data for votes
        opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
        opts.mOptions = RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_PARENT;
        opts.mMsgFlagMask = RsPosted::FLAG_MSGTYPE_MASK;
        opts.mMsgFlagFilter = RsPosted::FLAG_MSGTYPE_VOTE;
        mTokenService->requestMsgRelatedInfo(mUpdateRequestVotes, 0, opts, msgIds);

        mUpdatePhase = UPDATE_PHASE_VOTE_COUNT;

        return true;
    }
    else if(status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
    {
        mTokenService->cancelRequest(mUpdateRequestMessages);
        return false;
    }

    return true;
}


bool p3Posted::updateCompleteVotes()
{
    uint32_t status = mTokenService->requestStatus(mUpdateRequestVotes);

    if(status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
    {
        std::map<RsGxsGrpMsgIdPair, std::vector<RsPostedVote> > votes;
        getMsgRelatedDataT<RsGxsPostedVoteItem, RsPostedVote>(mUpdateRequestVotes,
                                                              votes);

        // now for each msg count the number of votes and thats it

        std::map<RsGxsGrpMsgIdPair, std::vector<RsPostedVote> >::iterator mit = votes.begin();

        for(; mit != votes.end(); mit++)
        {
            const std::vector<RsPostedVote>& v = mit->second;
            std::vector<RsPostedVote>::const_iterator cit = v.begin();

            for(; cit != v.end(); cit++)
            {
                const RsPostedVote& vote = *cit;

                if(vote.mDirection)
                {
                    mMsgCounts[mit->first].upVotes++;
                }else
                {
                    mMsgCounts[mit->first].downVotes++;
                }
            }
        }
        mUpdatePhase = UPDATE_PHASE_COMMENT_COUNT;
    }
    else if(status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
    {
        mTokenService->cancelRequest(mUpdateRequestVotes);
        return false;
    }
    return true;
}

bool p3Posted::updateCompleteComments()
{
    uint32_t status = mTokenService->requestStatus(mUpdateRequestComments);

    if(status == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
    {
        MsgRelatedIdResult commentIds;
        RsGenExchange::getMsgRelatedList(mUpdateRequestComments, commentIds);

        // now for each msg count the number of votes and thats it

        MsgRelatedIdResult::iterator mit = commentIds.begin();

        for(; mit != commentIds.end(); mit++)
        {
            const std::vector<RsGxsMessageId>& v = mit->second;
            std::vector<RsGxsMessageId>::const_iterator cit = v.begin();

            for(; cit != v.end(); cit++)
            {
                mMsgCounts[mit->first].commentCount++;
            }
        }
        mUpdatePhase = UPDATE_PHASE_COMPLETE;
    }
    else if(status == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
    {
        mTokenService->cancelRequest(mUpdateRequestComments);
        return false;
    }
    return true;
}

bool p3Posted::updateComplete()
{
    // now compare with msg meta to see what currently store there

    GxsMsgMetaMap::iterator mit = mMsgMetaUpdate.begin();

    for(; mit != mMsgMetaUpdate.end(); mit++)
    {
        const std::vector<RsMsgMetaData>& msgMetaV = mit->second;
        std::vector<RsMsgMetaData>::const_iterator cit = msgMetaV.begin();

        for(; cit != msgMetaV.end(); cit++)
        {
            const RsMsgMetaData& msgMeta = *cit;
            uint32_t upVotes, downVotes, nComments;
            retrieveScores(msgMeta.mServiceString, upVotes, downVotes, nComments);

            RsGxsGrpMsgIdPair msgId;
            msgId.first = mit->first;
            msgId.second = msgMeta.mMsgId;
            PostedScore& sc = mMsgCounts[msgId];

            bool changed = (sc.upVotes != upVotes) || (sc.downVotes != downVotes)
                           || (sc.commentCount != nComments);

            if(changed)
            {
                std::string servStr;
                storeScores(servStr, sc.upVotes, sc.downVotes, sc.commentCount);
                uint32_t token;
                setMsgServiceString(token, msgId, servStr);
                mChangeTokens.push_back(token);
            }
            else
            {
                mMsgCounts.erase(msgId);
            }
        }
    }

    mPostUpdate = false;
    mUpdatePhase = UPDATE_PHASE_GRP_REQUEST;

    if(!mMsgCounts.empty())
    {
        RsGxsMsgChange* msgChange = new RsGxsMsgChange();

        std::map<RsGxsGrpMsgIdPair, PostedScore >::iterator mit_c = mMsgCounts.begin();

        for(; mit_c != mMsgCounts.end(); mit_c++)
        {
            const RsGxsGrpMsgIdPair& msgId = mit_c->first;
            msgChange->msgChangeMap[msgId.first].push_back(msgId.second);
        }

        std::vector<RsGxsNotify*> n;
        n.push_back(msgChange);
        notifyChanges(n);

        mMsgCounts.clear();
    }

}
