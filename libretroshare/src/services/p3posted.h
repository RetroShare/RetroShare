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
    PostedRanking result;
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

    void processRankings();
    void processMessageRanks();
    void processCommentRanks();
    void discardCalc(const uint32_t& token);
    void completePostedPostCalc(GxsPostedPostRanking* gpp);
    bool retrieveScores(const std::string& serviceString, uint32_t& upVotes, uint32_t downVotes, uint32_t nComments);

private:

    std::map<uint32_t, GxsPostedPostRanking*> mPendingPostRanks;
    std::map<uint32_t, GxsPostedPostRanking*> mPendingCalculationPostRanks;

    std::map<uint32_t, GxsPostedCommentRanking*> mPendingCommentRanks;
    std::map<uint32_t, GxsPostedCommentRanking*> mPendingCalculationCommentRanks;

    RsMutex mPostedMutex;
};

#endif // P3POSTED_H
