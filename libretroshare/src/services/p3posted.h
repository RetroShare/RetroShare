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
    PostedRanking rankingResult;
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
    int32_t upVotes, downVotes;
    time_t date;
    RsGxsMessageId msgId;
};


#define UPDATE_PHASE_GRP_REQUEST 1
#define UPDATE_PHASE_GRP_MSG_REQUEST 2
#define UPDATE_VOTE_COMMENT_REQUEST 3
#define UPDATE_COMPLETE_UPDATE 4

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
    void generateVotes();

public:

    bool getGroup(const uint32_t &token, std::vector<RsPostedGroup> &group);
    bool getPost(const uint32_t &token, PostedPostResult& posts) ;
    bool getComment(const uint32_t &token, PostedCommentResult& comments) ;
    bool getRelatedComment(const uint32_t& token, PostedRelatedCommentResult &comments);
    bool getRanking(const uint32_t &token, PostedRanking &ranking);

    bool submitGroup(uint32_t &token, RsPostedGroup &group);
    bool submitPost(uint32_t &token, RsPostedPost &post);
    bool submitVote(uint32_t &token, RsPostedVote &vote);
    bool submitComment(uint32_t &token, RsPostedComment &comment) ;
            // Special Ranking Request.
    bool requestMessageRankings(uint32_t &token, const RankType &rType, const RsGxsGroupId &groupId);
    bool requestCommentRankings(uint32_t &token, const RankType &rType, const RsGxsGrpMsgIdPair &msgId);

private:

    /* Functions for processing rankings */

    void processRankings();
    void processMessageRanks();
    void processCommentRanks();
    void discardCalc(const uint32_t& token);
    void completePostedPostCalc(GxsPostedPostRanking* gpp);
    void completePostedCommentRanking(GxsPostedCommentRanking* gpc);
    bool retrieveScores(const std::string& serviceString, uint32_t& upVotes, uint32_t downVotes, uint32_t nComments) const;
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
    bool updateRequestGroups(uint32_t& token);
    bool updateRequestMessages(uint32_t& token);
    bool updateRequestVotesComments(uint32_t& token);
    bool updateCompleteUpdate();
    bool updateMsg(const RsGxsGrpMsgIdPair& msgId, const std::vector<RsPostedVote>& msgVotes,
                        const std::vector<RsGxsMessageId>& msgCommentIds);

private:

    // for calculating ranks
    std::map<uint32_t, GxsPostedPostRanking*> mPendingPostRanks;
    std::map<uint32_t, GxsPostedPostRanking*> mPendingCalculationPostRanks;
    std::map<uint32_t, GxsPostedCommentRanking*> mPendingCommentRanks;
    std::map<uint32_t, GxsPostedCommentRanking*> mPendingCalculationCommentRanks;

    // for maintaining vote counts in msg meta
    uint32_t mVoteUpdataToken, mVoteToken, mCommentToken;
    bool mUpdateTokenQueued;
    uint32_t mUpdatePhase;
    std::vector<RsGxsGrpMsgIdPair> mMsgsPendingUpdate;

    RsTokenService* mTokenService;
    RsMutex mPostedMutex;


    // for data generation

    bool mGeneratingPosts, mGeneratingTopics;
};

#endif // P3POSTED_H
