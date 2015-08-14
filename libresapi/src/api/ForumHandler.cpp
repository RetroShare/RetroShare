#include "ForumHandler.h"

#include <retroshare/rsgxsforums.h>
#include <time.h>

#include "Operators.h"
#include "ApiTypes.h"
#include "GxsResponseTask.h"
#ifndef WINDOWS_SYS
#include "unistd.h"
#endif

namespace resource_api
{
ForumHandler::ForumHandler(RsGxsForums* forums):
    mRsGxsForums(forums)
{
    addResourceHandler("*", this, &ForumHandler::handleWildcard);
}

void ForumHandler::handleWildcard(Request &req, Response &resp)
{
    bool ok = true;

    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
    uint32_t token;
    mRsGxsForums->getTokenService()->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts);

    time_t start = time(NULL);
    while((mRsGxsForums->getTokenService()->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
          &&(mRsGxsForums->getTokenService()->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
          &&((time(NULL) < (start+10)))
          )
    {
#ifdef WINDOWS_SYS
        Sleep(500);
#else
        usleep(500*1000) ;
#endif
    }
    if(mRsGxsForums->getTokenService()->requestStatus(token) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
    {
        std::vector<RsGxsForumGroup> grps;
        ok &= mRsGxsForums->getGroupData(token, grps);
        for(std::vector<RsGxsForumGroup>::iterator vit = grps.begin(); vit != grps.end(); vit++)
        {
            RsGxsForumGroup& grp = *vit;
            KeyValueReference<RsGxsGroupId> id("id", grp.mMeta.mGroupId);
            KeyValueReference<u_int32_t> vis_msg("visible_msg_count", grp.mMeta.mVisibleMsgCount);
            //KeyValueReference<RsPgpId> pgp_id("pgp_id",grp.mPgpId );
            // not very happy about this, i think the flags should stay hidden in rsidentities
            bool own = (grp.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
            bool pgp_linked = (grp.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID);
            resp.mDataStream.getStreamToMember()
                    << id
                    //<< pgp_id
                    << makeKeyValueReference("name", grp.mMeta.mGroupName)
                    //<< makeKeyValueReference("last_post", grp.mMeta.mLastPost)
                    << makeKeyValueReference("pop", grp.mMeta.mPop)
                    //<< makeKeyValueReference("publish_ts", grp.mMeta.mPublishTs)
                    << vis_msg
                    << makeKeyValueReference("group_status", grp.mMeta.mGroupStatus)
                    << makeKeyValueReference("author_id", grp.mMeta.mAuthorId)
                    << makeKeyValueReference("parent_grp_id", grp.mMeta.mParentGrpId)
                    << makeKeyValueReference("description", grp.mDescription)
                    << makeKeyValueReference("own", own)
                    << makeKeyValueReference("pgp_linked", pgp_linked);
        }
    }
    else
    {
        ok = false;
    }


    if(ok)
    {
        resp.setOk();
    }
    else
    {
        resp.setFail();
    }
}

} // namespace resource_api
