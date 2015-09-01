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
    if(!req.mPath.empty())
    {
        std::string str = req.mPath.top();
        req.mPath.pop();
        if(str != "")
        {
            //assume we have a groupID
            RsGxsGroupId grpId(str);
            std::list<RsGxsGroupId> groupIds;
            groupIds.push_back(grpId);

            uint32_t token;
            RsTokReqOptions opts;
            opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
            mRsGxsForums->getTokenService()->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds);

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
                std::vector<RsGxsForumMsg> grps;
                ok &= mRsGxsForums->getMsgData(token, grps);
                for(std::vector<RsGxsForumMsg>::iterator vit = grps.begin(); vit != grps.end(); vit++)
                {
                    RsGxsForumMsg& grp = *vit;
                    KeyValueReference<RsGxsGroupId> group_id("group_id", grp.mMeta.mGroupId);
                    resp.mDataStream.getStreamToMember()
                            << group_id
                            << makeKeyValueReference("name", grp.mMeta.mMsgName)
                            << makeKeyValueReference("id", grp.mMeta.mMsgId)
                            << makeKeyValueReference("parent_id", grp.mMeta.mParentId)
                            << makeKeyValueReference("author_id", grp.mMeta.mAuthorId)
                            << makeKeyValueReference("orig_msg_id", grp.mMeta.mOrigMsgId)
                            << makeKeyValueReference("thread_id", grp.mMeta.mThreadId)
                            << makeKeyValueReference("message", grp.mMsg);
                }
            }
            else
            {
                ok = false;
            }

        }

    }
    else
    {
        // no more path element
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
                bool subscribed = IS_GROUP_SUBSCRIBED(grp.mMeta.mSubscribeFlags);
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
                        << makeKeyValueReference("subscribed", subscribed)
                        << makeKeyValueReference("pgp_linked", pgp_linked);
            }
        }
        else
        {
            ok = false;
        }
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
