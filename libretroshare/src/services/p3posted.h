#ifndef P3POSTED_H
#define P3POSTED_H

#include <map>

#include "retroshare/rsposted.h"
#include "gxs/rsgenexchange.h"


class GxsPostedPostRanking
{
public:

    uint32_t pubToken;
    uint32_t reqToken;
    RsPosted::RankType rType;
    RsGxsGroupId grpId;
    RsPostedPostRanking rankingResult;
};

class GxsPostedCommentRanking
{
public:

    uint32_t pubToken;
    uint32_t reqToken;
    RsPosted::RankType rType;
    RsGxsGrpMsgIdPair msgId;
    PostedRanking result;
};

class PostedScore {
public:

    PostedScore() : upVotes(0), downVotes(0), commentCount(0), date(0) {}
    uint32_t upVotes, downVotes;
    uint32_t commentCount;
    time_t date;
    RsGxsMessageId msgId;
};



class p3Posted : public RsGenExchange, public RsPosted
{
public:
    p3Posted(RsGeneralDataService* gds, RsNetworkExchangeService* nes);

protected:

    /*!
     * This confirms this class as an abstract one that \n
     * should not be instantiated \n
     * The deriving class should implement this function \n
     * as it is called by the backend GXS system to \n
     * update client of changes which should \n
     * instigate client to retrieve new content from the system
     * @param changes the changes that have occured to data held by this service
     */
    void notifyChanges(std::vector<RsGxsNotify*>& changes) ;

    void service_tick();

public:

    void generateTopics();
    /*!
     * Exists solely for testing
     */
    void generatePosts();

    /*!
     * Exists solely for testing
     * Generates random votes to existing posts
     * in the system
     */
    void generateVotesAndComments();

public:

    bool getGroup(const uint32_t &token, std::vector<RsPostedGroup> &group);
    bool getPost(const uint32_t &token, PostedPostResult& posts) ;
    bool getComment(const uint32_t &token, PostedCommentResult& comments) ;
    bool getRelatedComment(const uint32_t& token, PostedRelatedCommentResult &comments);
    bool getPostRanking(const uint32_t &token, RsPostedPostRanking &ranking);

    bool submitGroup(uint32_t &token, RsPostedGroup &group);
    bool submitPost(uint32_t &token, RsPostedPost &post);
    bool submitVote(uint32_t &token, RsPostedVote &vote);
    bool submitComment(uint32_t &token, RsPostedComment &comment) ;
            // Special Ranking Request.
    bool requestPostRankings(uint32_t &token, const RankType &rType, const RsGxsGroupId &groupId);
    bool requestCommentRankings(uint32_t &token, const RankType &rType, const RsGxsGrpMsgIdPair &msgId);

    bool retrieveScores(const std::string& serviceString, uint32_t& upVotes, uint32_t& downVotes, uint32_t& nComments) const;

private:

    /* Functions for processing rankings */

    void processRankings();
    void processPostRanks();
    void processCommentRanks();
    void discardCalc(const uint32_t& token);
    bool completePostedPostCalc(GxsPostedPostRanking* gpp);
    void completePostedCommentRanking(GxsPostedCommentRanking* gpc);

    bool storeScores(std::string& serviceString, uint32_t& upVotes, uint32_t downVotes, uint32_t nComments) const;

    // for posts
    void calcPostedPostRank(const std::vector<RsMsgMetaData>, PostedRanking& ranking, bool com(const PostedScore& i, const PostedScore &j)) const;

    // for comments
    void calcPostedCommentsRank(const std::map<RsGxsMessageId, std::vector<RsGxsMessageId> >& msgBranches, std::map<RsGxsMessageId, RsMsgMetaData>& msgMetas,
                                PostedRanking& ranking, bool com(const PostedScore& i, const PostedScore &j)) const;

    /* Functions for maintaing vote counts in meta data */

    /*!
     * Update votes should only be called when a vote comes in
     * Several phases to calculating votes.
     * First get all messages for groups which you are subscribed
     * Then for these messages get all the votes accorded to them
     * Then do the calculation and update messages
     * Also stores updates for messages which have new scores
     */
    void updateVotes();
    bool updateRequestGroups();
    bool updateRequestMessages();
    bool updateRequestVotesComments();
    bool updateCompleteVotes();
    bool updateCompleteComments();

    /*!
     * The aim of this is create notifications
     * for the UI of changes to a post if their vote
     * or comment count has changed
     */
    bool updateComplete();


private:

    // for calculating ranks
    std::vector<GxsPostedPostRanking*> mPendingPostRanks;
    std::vector<GxsPostedPostRanking*> mCompletionPostRanks;
    std::map<uint32_t, RsPostedPostRanking> mCompletePostRanks;
    std::map<uint32_t, GxsPostedCommentRanking*> mPendingCommentRanks;
    std::map<uint32_t, GxsPostedCommentRanking*> mPendingCalculationCommentRanks;

    // for maintaining vote counts in msg meta
    uint32_t mUpdateRequestGroup, mUpdateRequestMessages, mUpdateRequestComments, mUpdateRequestVotes;
    bool mPostUpdate;
    uint32_t mUpdatePhase;
    std::vector<RsGxsGrpMsgIdPair> mMsgsPendingUpdate;
    time_t mLastUpdate;
    GxsMsgMetaMap mMsgMetaUpdate;
    std::map<RsGxsGrpMsgIdPair, PostedScore > mMsgCounts;
    std::vector<uint32_t> mChangeTokens;

    RsTokenService* mTokenService;
    RsMutex mPostedMutex;


    // for data generation

    bool mGeneratingPosts, mGeneratingTopics,
    mRequestPhase1, mRequestPhase2, mRequestPhase3, mGenerateVotesAndComments;
    std::vector<uint32_t> mTokens;
    uint32_t mToken;
    std::list<RsGxsGroupId> mGrpIds;

};

#endif // P3POSTED_H
