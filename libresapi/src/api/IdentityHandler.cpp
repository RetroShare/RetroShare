#include "IdentityHandler.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>
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

class DeleteIdentityTask : public GxsResponseTask
{
public:
	DeleteIdentityTask(RsIdentity* idservice) :
	    GxsResponseTask(idservice, idservice->getTokenService()),
	    mToken(0),
	    mRsIdentity(idservice)
	{}

protected:
	virtual void gxsDoWork(Request &req, Response &resp)
	{
		RsGxsIdGroup group;
		std::string gxs_id;

		req.mStream << makeKeyValueReference("gxs_id", gxs_id);
		group.mMeta.mGroupId = RsGxsGroupId(gxs_id);

		mRsIdentity->deleteIdentity(mToken, group);
		addWaitingToken(mToken);

		done();
		return;
	}

private:
	uint32_t mToken;
	RsIdentity* mRsIdentity;
	RsGxsId mId;
};

IdentityHandler::IdentityHandler(StateTokenServer *sts, RsNotify *notify, RsIdentity *identity):
    mStateTokenServer(sts), mNotify(notify), mRsIdentity(identity),
    mMtx("IdentityHandler Mtx"), mStateToken(sts->getNewToken())
{
    mNotify->registerNotifyClient(this);

	addResourceHandler("*", this, &IdentityHandler::handleWildcard);
	addResourceHandler("own", this, &IdentityHandler::handleOwn);

	addResourceHandler("own_ids", this, &IdentityHandler::handleOwnIdsRequest);
	addResourceHandler("notown_ids", this, &IdentityHandler::handleNotOwnIdsRequest);

	addResourceHandler("add_contact", this, &IdentityHandler::handleAddContact);
	addResourceHandler("remove_contact", this, &IdentityHandler::handleRemoveContact);

	addResourceHandler("create_identity", this, &IdentityHandler::handleCreateIdentity);
	addResourceHandler("delete_identity", this, &IdentityHandler::handleDeleteIdentity);

	addResourceHandler("get_identity_details", this, &IdentityHandler::handleGetIdentityDetails);

	addResourceHandler("set_ban_node", this, &IdentityHandler::handleSetBanNode);
	addResourceHandler("set_opinion", this, &IdentityHandler::handleSetOpinion);
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
			        << makeKeyValueReference("contact", grp.mIsAContact)
			        << makeKeyValueReference("own", own)
			        << makeKeyValueReference("pgp_linked", pgp_linked);
		}
	}
	else ok = false;

	if(ok) resp.setOk();
	else resp.setFail();
}


void IdentityHandler::handleNotOwnIdsRequest(Request & /*req*/, Response &resp)
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
			if(!(grp.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) && grp.mIsAContact)
			{
				bool pgp_linked = (grp.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility  ) ;
				resp.mDataStream.getStreamToMember()
				        << makeKeyValueReference("id", grp.mMeta.mGroupId) /// @deprecated using "id" as key can cause problems in some JS based languages like Qml @see gxs_id instead
				        << makeKeyValueReference("gxs_id", grp.mMeta.mGroupId)
				        << makeKeyValueReference("pgp_id",grp.mPgpId )
				        << makeKeyValueReference("name", grp.mMeta.mGroupName)
				        << makeKeyValueReference("pgp_linked", pgp_linked);
			}
		}
	}
	else ok = false;

	if(ok) resp.setOk();
	else resp.setFail();
}

void IdentityHandler::handleOwnIdsRequest(Request & /*req*/, Response &resp)
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
			if(vit->mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
			{
				bool pgp_linked = (grp.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility  ) ;
				resp.mDataStream.getStreamToMember()
				        << makeKeyValueReference("own_gxs_id", grp.mMeta.mGroupId)
				        << makeKeyValueReference("pgp_id",grp.mPgpId )
				        << makeKeyValueReference("name", grp.mMeta.mGroupName)
				        << makeKeyValueReference("pgp_linked", pgp_linked);
			}
		}

	}
	else
		ok = false;

	if(ok) resp.setOk();
	else resp.setFail();
}

void IdentityHandler::handleAddContact(Request& req, Response& resp)
{
	std::string gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);

	mRsIdentity->setAsRegularContact(RsGxsId(gxs_id), true);

	{
		RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
		mStateTokenServer->replaceToken(mStateToken);
	}

	resp.setOk();
}

void IdentityHandler::handleRemoveContact(Request& req, Response& resp)
{
	std::string gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);

	mRsIdentity->setAsRegularContact(RsGxsId(gxs_id), false);

	{
		RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
		mStateTokenServer->replaceToken(mStateToken);
	}

	resp.setOk();
}

void IdentityHandler::handleGetIdentityDetails(Request& req, Response& resp)
{
	std::string gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(RsGxsGroupId(gxs_id));
	mRsIdentity->getTokenService()->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds);

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

	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	if (!mRsIdentity->getGroupData(token, datavector))
	{
		resp.setFail();
		return;
	}

	data = datavector[0];

	resp.mDataStream << makeKeyValue("gxs_name", data.mMeta.mGroupName);
	resp.mDataStream << makeKeyValue("gxs_id", data.mMeta.mGroupId.toStdString());

	resp.mDataStream << makeKeyValue("pgp_id_known", data.mPgpKnown);
	resp.mDataStream << makeKeyValue("pgp_id", data.mPgpId.toStdString());

	std::string pgp_name;
	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		pgp_name = details.name;
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
			pgp_name = "[Unknown node]";
		else
			pgp_name = "Anonymous Id";
	}
	resp.mDataStream << makeKeyValue("pgp_name", pgp_name);

	time_t now = time(NULL);
	resp.mDataStream << makeKeyValue("last_usage", std::to_string(now - data.mLastUsageTS));

	bool isAnonymous = false;
	if(!data.mPgpKnown)
	{
		if (!(data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility))
			isAnonymous = true;
	}
	resp.mDataStream << makeKeyValue("anonymous", isAnonymous);


	bool isOwnId = (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
	resp.mDataStream << makeKeyValue("own", isOwnId);

	std::string type;
	if(isOwnId)
	{
		if (data.mPgpKnown && !data.mPgpId.isNull())
			type = "Identity owned by you, linked to your Retroshare node";
		else
			type = "Anonymous identity, owned by you";
	}
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
	{
		if (data.mPgpKnown)
		{
			if (rsPeers->isGPGAccepted(data.mPgpId))
				type = "Linked to a friend Retroshare node";
			else
				type = "Linked to a known Retroshare node";
		}
		else
			type = "Linked to unknown Retroshare node";
	}
	else
		type = "Anonymous identity";

	resp.mDataStream << makeKeyValue("type", type);

	resp.mDataStream << makeKeyValue("bannned_node", rsReputations->isNodeBanned(data.mPgpId));

	RsReputations::ReputationInfo info;
	rsReputations->getReputationInfo(RsGxsId(data.mMeta.mGroupId), data.mPgpId, info);
	resp.mDataStream << makeKeyValue("friends_positive_votes", info.mFriendsPositiveVotes);
	resp.mDataStream << makeKeyValue("friends_negative_votes", info.mFriendsNegativeVotes);
	resp.mDataStream << makeKeyValue("overall_reputation_level", (int)info.mOverallReputationLevel);
	resp.mDataStream << makeKeyValue("own_opinion", (int)info.mOwnOpinion);

	RsIdentityDetails details;
	mRsIdentity->getIdDetails(RsGxsId(data.mMeta.mGroupId), details);
	StreamBase& usagesStream = resp.mDataStream.getStreamToMember("usages");
	usagesStream.getStreamToMember();

	for(std::map<RsIdentityUsage,time_t>::const_iterator it(details.mUseCases.begin()); it != details.mUseCases.end(); ++it)
	{
		usagesStream.getStreamToMember() << makeKeyValue("usage_time", std::to_string(now - it->second));
		usagesStream.getStreamToMember() << makeKeyValue("usage_service", (int)(it->first.mServiceId));
		usagesStream.getStreamToMember() << makeKeyValue("usage_case", (int)(it->first.mUsageCode));
	}

	resp.setOk();
}

void IdentityHandler::handleSetBanNode(Request& req, Response& resp)
{
	std::string pgp_id;
	req.mStream << makeKeyValueReference("pgp_id", pgp_id);
	RsPgpId pgpId(pgp_id);

	bool banned_node;
	req.mStream << makeKeyValueReference("banned_node", banned_node);
	rsReputations->banNode(pgpId, banned_node);

	resp.setOk();
}

void IdentityHandler::handleSetOpinion(Request& req, Response& resp)
{
	std::string gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);
	RsGxsId gxsId(gxs_id);

	int own_opinion;
	req.mStream << makeKeyValueReference("own_opinion", own_opinion);

	RsReputations::Opinion opinion;
	switch(own_opinion)
	{
	    case 0:
		    opinion = RsReputations::OPINION_NEGATIVE;
		    break;
	    case 1: opinion =
		    RsReputations::OPINION_NEUTRAL;
		    break;
	    case 2:
		    opinion = RsReputations::OPINION_POSITIVE;
		    break;
	default:
		resp.setFail();
		return;
	}
	rsReputations->setOwnOpinion(gxsId, opinion);

	resp.setOk();
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

ResponseTask* IdentityHandler::handleDeleteIdentity(Request& req, Response& resp)
{
	return new DeleteIdentityTask(mRsIdentity);
}

} // namespace resource_api
