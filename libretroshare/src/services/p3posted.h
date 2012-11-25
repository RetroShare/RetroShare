#ifndef P3POSTED_H
#define P3POSTED_H

#include "retroshare/rsposted.h"
#include "gxs/rsgenexchange.h"

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
    bool getGroupRank(const uint32_t& token, GroupRank& grpRank);

    bool submitGroup(uint32_t &token, RsPostedGroup &group);
    bool submitPost(uint32_t &token, RsPostedPost &post);
    bool submitVote(uint32_t &token, RsPostedVote &vote);
    bool submitComment(uint32_t &token, RsPostedComment &comment) ;
            // Special Ranking Request.
    bool requestRanking(uint32_t &token, RsGxsGroupId groupId) ;

            // Control Ranking Calculations.
//    bool setViewMode(uint32_t mode);
//    bool setViewPeriod(uint32_t period);
//    bool setViewRange(uint32_t first, uint32_t count);

};

#endif // P3POSTED_H
