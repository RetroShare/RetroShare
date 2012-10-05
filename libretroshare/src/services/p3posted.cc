#include "p3posted.h"

p3Posted::p3Posted(RsGeneralDataService *gds, RsNetworkExchangeService *nes)
    : RsGenExchange(gds, nes, NULL, RS_SERVICE_GXSV1_TYPE_POSTED), RsPosted(this)
{
}

void p3Posted::notifyChanges(std::vector<RsGxsNotify *> &changes)
{


}

bool p3Posted::getGroup(const uint32_t &token, RsPostedGroup &group)
{

}

bool p3Posted::getPost(const uint32_t &token, RsPostedPost &post)
{

}

bool p3Posted::getComment(const uint32_t &token, RsPostedComment &comment)
{

}

bool p3Posted::submitGroup(uint32_t &token, RsPostedGroup &group)
{

}

bool p3Posted::submitPost(uint32_t &token, RsPostedPost &post)
{

}

bool p3Posted::submitVote(uint32_t &token, RsPostedVote &vote)
{

}

bool p3Posted::submitComment(uint32_t &token, RsPostedComment &comment)
{

}

        // Special Ranking Request.
bool p3Posted::requestRanking(uint32_t &token, RsGxsGroupId groupId)
{

}

bool p3Posted::getRankedPost(const uint32_t &token, RsPostedPost &post)
{

}

bool p3Posted::extractPostedCache()
{

}


        // Control Ranking Calculations.
bool p3Posted::setViewMode(uint32_t mode)
{

}

bool p3Posted::setViewPeriod(uint32_t period)
{

}

bool p3Posted::setViewRange(uint32_t first, uint32_t count)
{

}

        // exposed for testing...
float p3Posted::calcPostScore(uint32_t& token, const RsGxsMessageId)
{

}
