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
    SendIdentitiesListTask(RsIdentity* idservice, std::list<RsGxsId> ids, StateToken state):
        GxsResponseTask(idservice, 0), mStateToken(state)
    {
        for(std::list<RsGxsId>::iterator vit = ids.begin(); vit != ids.end(); ++vit)
        {
            requestGxsId(*vit);
            mIds.push_back(*vit);// convert from list to vector
        }
    }
private:
    std::vector<RsGxsId> mIds;
    StateToken mStateToken;
protected:
	virtual void gxsDoWork(Request& /*req*/, Response &resp)
    {
        resp.mDataStream.getStreamToMember();
        for(std::vector<RsGxsId>::iterator vit = mIds.begin(); vit != mIds.end(); ++vit)
        {
            streamGxsId(*vit, resp.mDataStream.getStreamToMember());
        }
        resp.mStateToken = mStateToken;
        resp.setOk();
        done();
    }

};

class CreateIdentityTask: public GxsResponseTask
{
public:
    CreateIdentityTask(RsIdentity* idservice):
        GxsResponseTask(idservice, idservice->getTokenService()), mState(BEGIN), mToken(0), mRsIdentity(idservice){}
private:
    enum State {BEGIN, WAIT_ACKN, WAIT_ID};
    State mState;
    uint32_t mToken;
    RsIdentity* mRsIdentity;
    RsGxsId mId;
protected:
    virtual void gxsDoWork(Request &req, Response &resp)
    {
        switch(mState)
        {
        case BEGIN:{
            RsIdentityParameters params;
            req.mStream << makeKeyValueReference("name", params.nickname) << makeKeyValueReference("pgp_linked", params.isPgpLinked);

            if(params.nickname == "")
            {
                resp.setFail("name can't be empty");
                done();
                return;
            }
            mRsIdentity->createIdentity(mToken, params);
            addWaitingToken(mToken);
            mState = WAIT_ACKN;
            break;
        }
        case WAIT_ACKN:{
            RsGxsGroupId grpId;
            if(!mRsIdentity->acknowledgeGrp(mToken, grpId))
            {
                resp.setFail("acknowledge of group id failed");
                done();
                return;
            }
            mId = RsGxsId(grpId);
            requestGxsId(mId);
            mState = WAIT_ID;
            break;
        }
        case WAIT_ID:
            streamGxsId(mId, resp.mDataStream);
            resp.setOk();
            done();
        }
    }
};

IdentityHandler::IdentityHandler(StateTokenServer *sts, RsNotify *notify, RsIdentity *identity):
    mStateTokenServer(sts), mNotify(notify), mRsIdentity(identity),
    mMtx("IdentityHandler Mtx"), mStateToken(sts->getNewToken())
{
    mNotify->registerNotifyClient(this);

    addResourceHandler("*", this, &IdentityHandler::handleWildcard);
    addResourceHandler("own", this, &IdentityHandler::handleOwn);
    addResourceHandler("create_identity", this, &IdentityHandler::handleCreateIdentity);
}

IdentityHandler::~IdentityHandler()
{
    mNotify->unregisterNotifyClient(this);
}

void IdentityHandler::notifyGxsChange(const RsGxsChanges &changes)
{
    RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    // if changes come from identity service, invalidate own state token
    if(changes.mService == mRsIdentity->getTokenService())
    {
        mStateTokenServer->replaceToken(mStateToken);
    }
}

void IdentityHandler::handleWildcard(Request & /*req*/, Response &resp)
{
	bool ok = true;

	{
		RS_STACK_MUTEX(mMtx);
		resp.mStateToken = mStateToken;
	}
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
		usleep(500*1000);
#endif
	}

	if(mRsIdentity->getTokenService()->requestStatus(token) == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
	{
		std::vector<RsGxsIdGroup> grps;
		ok &= mRsIdentity->getGroupData(token, grps);
		for(std::vector<RsGxsIdGroup>::iterator vit = grps.begin(); vit != grps.end(); vit++)
		{
			RsGxsIdGroup& grp = *vit;
			//electron: not very happy about this, i think the flags should stay hidden in rsidentities
			bool own = (grp.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
			bool pgp_linked = (grp.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility  ) ;
            resp.mDataStream.getStreamToMember()
			        << makeKeyValueReference("id", grp.mMeta.mGroupId) /// @deprecated using "id" as key can cause problems in some JS based languages like Qml @see gxs_id instead
			        << makeKeyValueReference("gxs_id", grp.mMeta.mGroupId)
			        << makeKeyValueReference("pgp_id",grp.mPgpId )
			        << makeKeyValueReference("name", grp.mMeta.mGroupName)
			        << makeKeyValueReference("own", own)
			        << makeKeyValueReference("pgp_linked", pgp_linked);
		}
	}
	else ok = false;

	if(ok) resp.setOk();
	else resp.setFail();
}

ResponseTask* IdentityHandler::handleOwn(Request & /* req */, Response &resp)
{
    StateToken state;
    {
        RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
        state = mStateToken;
    }
    std::list<RsGxsId> ids;
    if(mRsIdentity->getOwnIds(ids))
        return new SendIdentitiesListTask(mRsIdentity, ids, state);
    resp.mDataStream.getStreamToMember();
    resp.setWarning("identities not loaded yet");
    return 0;
}

ResponseTask* IdentityHandler::handleCreateIdentity(Request & /* req */, Response & /* resp */)
{
    return new CreateIdentityTask(mRsIdentity);
}

} // namespace resource_api
