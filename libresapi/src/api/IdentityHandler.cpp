#include "IdentityHandler.h"

#include <retroshare/rsidentity.h>
#include <time.h>

#include "Operators.h"
#include "ApiTypes.h"
#include "GxsResponseTask.h"
#ifndef WINDOWS_SYS
#include "unistd.h"
#endif

namespace resource_api
{

class SendIdentitiesListTask: public GxsResponseTask
{
public:
    SendIdentitiesListTask(RsIdentity* idservice, std::list<RsGxsId> ids):
        GxsResponseTask(idservice, 0)
    {
        for(std::list<RsGxsId>::iterator vit = ids.begin(); vit != ids.end(); ++vit)
        {
            requestGxsId(*vit);
            mIds.push_back(*vit);// convert fro list to vector
        }
    }
private:
    std::vector<RsGxsId> mIds;
protected:
    virtual void gxsDoWork(Request &req, Response &resp)
    {
        resp.mDataStream.getStreamToMember();
        for(std::vector<RsGxsId>::iterator vit = mIds.begin(); vit != mIds.end(); ++vit)
        {
            streamGxsId(*vit, resp.mDataStream.getStreamToMember());
        }
        resp.setOk();
        done();
    }

};

IdentityHandler::IdentityHandler(RsIdentity *identity):
    mRsIdentity(identity)
{
    addResourceHandler("*", this, &IdentityHandler::handleWildcard);
    addResourceHandler("own", this, &IdentityHandler::handleOwn);
}

void IdentityHandler::handleWildcard(Request &req, Response &resp)
{
    bool ok = true;

    if(req.isPut())
    {
        RsIdentityParameters params;
        req.mStream << makeKeyValueReference("name", params.nickname);
        if(req.mStream.isOK())
        {
            uint32_t token;
            mRsIdentity->createIdentity(token, params);
            // not sure if should acknowledge the token
            // for now go the easier way
        }
        else
        {
            ok = false;
        }
    }
    else
    {
        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
        uint32_t token;
        mRsIdentity->getTokenService()->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts);

        time_t start = time(NULL);
        while((mRsIdentity->getTokenService()->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
              &&(mRsIdentity->getTokenService()->requestStatus(token) != RsTokenService::GXS_REQUEST_V2_STATUS_FAILED)
              &&((time(NULL) < (start+10)))
              )
        {
#ifdef WINDOWS_SYS
            Sleep(500);
#else
            usleep(500*1000) ;
#endif
        }

        if(mRsIdentity->getTokenService()->requestStatus(token) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
        {
            std::vector<RsGxsIdGroup> grps;
            ok &= mRsIdentity->getGroupData(token, grps);
            for(std::vector<RsGxsIdGroup>::iterator vit = grps.begin(); vit != grps.end(); vit++)
            {
                RsGxsIdGroup& grp = *vit;
                KeyValueReference<RsGxsGroupId> id("id", grp.mMeta.mGroupId);
                KeyValueReference<RsPgpId> pgp_id("pgp_id",grp.mPgpId );
                // not very happy about this, i think the flags should stay hidden in rsidentities
                bool own = (grp.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
                bool pgp_linked = (grp.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID);
                resp.mDataStream.getStreamToMember()
                        << id
                        << pgp_id
                        << makeKeyValueReference("name", grp.mMeta.mGroupName)
                        << makeKeyValueReference("own", own)
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

ResponseTask* IdentityHandler::handleOwn(Request &req, Response &resp)
{
    std::list<RsGxsId> ids;
    mRsIdentity->getOwnIds(ids);
    return new SendIdentitiesListTask(mRsIdentity, ids);
}

} // namespace resource_api
