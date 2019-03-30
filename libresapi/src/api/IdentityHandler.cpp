/*******************************************************************************
 * libresapi/api/IdentityHandler.cpp                                           *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2015  electron128 <electron128@yahoo.com>                     *
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "IdentityHandler.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>
#include <util/radix64.h>
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
			req.mStream
			        << makeKeyValueReference("name", params.nickname)
			        << makeKeyValueReference("pgp_linked", params.isPgpLinked);

			std::string avatar;
			req.mStream << makeKeyValueReference("avatar", avatar);

			std::vector<uint8_t> avatar_data = Radix64::decode(avatar);
			uint8_t *p_avatar_data = &avatar_data[0];
			uint32_t size = avatar_data.size();
			params.mImage.clear();
			params.mImage.copy(p_avatar_data, size);

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
	virtual void gxsDoWork(Request &req, Response & /* resp */)
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

IdentityHandler::IdentityHandler(StateTokenServer *sts, RsNotify *notify,
                                 RsIdentity *identity) :
    mStateTokenServer(sts), mNotify(notify), mRsIdentity(identity),
    mMtx("IdentityHandler Mtx"), mStateToken(sts->getNewToken())
{
	mNotify->registerNotifyClient(this);

	addResourceHandler("*", this, &IdentityHandler::handleWildcard);
	addResourceHandler("own", this, &IdentityHandler::handleOwn);

	addResourceHandler("own_ids", this, &IdentityHandler::handleOwnIdsRequest);
	addResourceHandler("notown_ids", this,
	                   &IdentityHandler::handleNotOwnIdsRequest);

	addResourceHandler("export_key", this, &IdentityHandler::handleExportKey);
	addResourceHandler("import_key", this, &IdentityHandler::handleImportKey);

	addResourceHandler("add_contact", this, &IdentityHandler::handleAddContact);
	addResourceHandler("remove_contact", this, &IdentityHandler::handleRemoveContact);

	addResourceHandler("create_identity", this, &IdentityHandler::handleCreateIdentity);
	addResourceHandler("delete_identity", this, &IdentityHandler::handleDeleteIdentity);

	addResourceHandler("get_identity_details", this, &IdentityHandler::handleGetIdentityDetails);

	addResourceHandler("get_avatar", this, &IdentityHandler::handleGetAvatar);
	addResourceHandler("set_avatar", this, &IdentityHandler::handleSetAvatar);

	addResourceHandler("set_ban_node", this, &IdentityHandler::handleSetBanNode);
	addResourceHandler("set_opinion", this, &IdentityHandler::handleSetOpinion);
}

IdentityHandler::~IdentityHandler()
{
    mNotify->unregisterNotifyClient(this);
}

void IdentityHandler::notifyGxsChange(const RsGxsChanges &changes)
{
	RS_STACK_MUTEX(mMtx);
	// if changes come from identity service, invalidate own state token
	if(changes.mService == mRsIdentity->getTokenService())
		mStateTokenServer->replaceToken(mStateToken);
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
	mRsIdentity->getTokenService()->requestGroupInfo(
	            token, RS_TOKREQ_ANSTYPE_DATA, opts);

	time_t timeout = time(NULL)+10;
	uint8_t rStatus = mRsIdentity->getTokenService()->requestStatus(token);
	while( rStatus != RsTokenService::COMPLETE &&
	       rStatus != RsTokenService::FAILED &&
	       time(NULL) < timeout )
	{
		usleep(50*1000);
		rStatus = mRsIdentity->getTokenService()->requestStatus(token);
	}

	if(rStatus == RsTokenService::COMPLETE)
	{
		std::vector<RsGxsIdGroup> grps;
		ok &= mRsIdentity->getGroupData(token, grps);
		for( std::vector<RsGxsIdGroup>::iterator vit = grps.begin();
		     vit != grps.end(); ++vit )
		{
			RsGxsIdGroup& grp = *vit;
			/* electron: not very happy about this, i think the flags should
			 * stay hidden in rsidentities */
			bool own = (grp.mMeta.mSubscribeFlags &
			            GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
			bool pgp_linked = (grp.mMeta.mGroupFlags &
			                   RSGXSID_GROUPFLAG_REALID_kept_for_compatibility);
			resp.mDataStream.getStreamToMember()
#warning Gioacchino Mazzurco 2017-03-24: @deprecated using "id" as key can cause problems in some JS based \
	        languages like Qml @see gxs_id instead
			        << makeKeyValueReference("id", grp.mMeta.mGroupId)
			        << makeKeyValueReference("gxs_id", grp.mMeta.mGroupId)
			        << makeKeyValueReference("pgp_id",grp.mPgpId )
			        << makeKeyValueReference("name", grp.mMeta.mGroupName)
			        << makeKeyValueReference("contact", grp.mIsAContact)
			        << makeKeyValueReference("own", own)
			        << makeKeyValueReference("pgp_linked", pgp_linked)
			        << makeKeyValueReference("is_contact", grp.mIsAContact);
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
	mRsIdentity->getTokenService()->requestGroupInfo(
	            token, RS_TOKREQ_ANSTYPE_DATA, opts);

	time_t timeout = time(NULL)+10;
	uint8_t rStatus = mRsIdentity->getTokenService()->requestStatus(token);
	while( rStatus != RsTokenService::COMPLETE &&
	       rStatus != RsTokenService::FAILED &&
	       time(NULL) < timeout )
	{
		usleep(50*1000);
		rStatus = mRsIdentity->getTokenService()->requestStatus(token);
	}

	if(rStatus == RsTokenService::COMPLETE)
	{
		std::vector<RsGxsIdGroup> grps;
		ok &= mRsIdentity->getGroupData(token, grps);
		for(std::vector<RsGxsIdGroup>::iterator vit = grps.begin();
		    vit != grps.end(); vit++)
		{
			RsGxsIdGroup& grp = *vit;
			if(!(grp.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN))
			{
				bool pgp_linked = (
				            grp.mMeta.mGroupFlags &
				            RSGXSID_GROUPFLAG_REALID_kept_for_compatibility );
				resp.mDataStream.getStreamToMember()
				        << makeKeyValueReference("gxs_id", grp.mMeta.mGroupId)
				        << makeKeyValueReference("pgp_id",grp.mPgpId )
				        << makeKeyValueReference("name", grp.mMeta.mGroupName)
				        << makeKeyValueReference("pgp_linked", pgp_linked)
				        << makeKeyValueReference("is_contact", grp.mIsAContact);
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
	while((mRsIdentity->getTokenService()->requestStatus(token) != RsTokenService::COMPLETE)
	      &&(mRsIdentity->getTokenService()->requestStatus(token) != RsTokenService::FAILED)
	      &&((time(NULL) < (start+10)))
	      )
	{
#ifdef WINDOWS_SYS
		Sleep(500);
#else
		usleep(500*1000);
#endif
	}

	if(mRsIdentity->getTokenService()->requestStatus(token) == RsTokenService::COMPLETE)
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
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
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
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
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
	while((mRsIdentity->getTokenService()->requestStatus(token) != RsTokenService::COMPLETE)
	      &&(mRsIdentity->getTokenService()->requestStatus(token) != RsTokenService::FAILED)
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

	if(datavector.empty())
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

	resp.mDataStream << makeKeyValue("last_usage", (uint32_t)data.mLastUsageTS);

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

	RsReputationInfo info;
	rsReputations->getReputationInfo(RsGxsId(data.mMeta.mGroupId), data.mPgpId, info);
	resp.mDataStream << makeKeyValue("friends_positive_votes", info.mFriendsPositiveVotes);
	resp.mDataStream << makeKeyValue("friends_negative_votes", info.mFriendsNegativeVotes);
	resp.mDataStream << makeKeyValue("overall_reputation_level", (int)info.mOverallReputationLevel);
	resp.mDataStream << makeKeyValue("own_opinion", (int)info.mOwnOpinion);

	RsIdentityDetails details;
	mRsIdentity->getIdDetails(RsGxsId(data.mMeta.mGroupId), details);

	std::string base64Avatar;
	Radix64::encode(details.mAvatar.mData, details.mAvatar.mSize, base64Avatar);
	resp.mDataStream << makeKeyValue("avatar", base64Avatar);

	StreamBase& usagesStream = resp.mDataStream.getStreamToMember("usages");
	usagesStream.getStreamToMember();

	for(auto it(details.mUseCases.begin()); it != details.mUseCases.end(); ++it)
	{
		usagesStream.getStreamToMember()
		        << makeKeyValue("usage_time", (uint32_t)data.mLastUsageTS)
		        << makeKeyValue("usage_service", (int)(it->first.mServiceId))
		        << makeKeyValue("usage_case", (int)(it->first.mUsageCode));
	}

	resp.setOk();
}

void IdentityHandler::handleSetAvatar(Request& req, Response& resp)
{
	std::string gxs_id;
	std::string avatar;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);
	req.mStream << makeKeyValueReference("avatar", avatar);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(RsGxsGroupId(gxs_id));
	mRsIdentity->getTokenService()->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds);

	time_t start = time(NULL);
	while((mRsIdentity->getTokenService()->requestStatus(token) != RsTokenService::COMPLETE)
	      &&(mRsIdentity->getTokenService()->requestStatus(token) != RsTokenService::FAILED)
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

	if(datavector.empty())
	{
		resp.setFail();
		return;
	}

	data = datavector[0];

	if(!avatar.empty())
	{
		std::vector<uint8_t> avatar_data = Radix64::decode(avatar);
		uint8_t *p_avatar_data = &avatar_data[0];
		uint32_t size = avatar_data.size();
		data.mImage.clear();
		data.mImage.copy(p_avatar_data, size);

		std::string base64Avatar;
		Radix64::encode(data.mImage.mData, data.mImage.mSize, base64Avatar);
		resp.mDataStream << makeKeyValue("avatar", base64Avatar);
	}
	else
		data.mImage.clear();

	uint32_t dummyToken = 0;
	mRsIdentity->updateIdentity(dummyToken, data);

	resp.setOk();
}

void IdentityHandler::handleGetAvatar(Request& req, Response& resp)
{
	std::string gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);

	RsIdentityDetails details;
	bool got = mRsIdentity->getIdDetails(RsGxsId(gxs_id), details);

	std::string base64Avatar;
	Radix64::encode(details.mAvatar.mData, details.mAvatar.mSize, base64Avatar);
	resp.mDataStream << makeKeyValue("avatar", base64Avatar);

	if(got)
		resp.setOk();
	else
		resp.setFail();
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

	RsOpinion opinion;
	switch(own_opinion)
	{
	case 0: opinion = RsOpinion::NEGATIVE; break;
	case 1: opinion = RsOpinion::NEUTRAL; break;
	case 2: opinion = RsOpinion::POSITIVE; break;
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

void IdentityHandler::handleExportKey(Request& req, Response& resp)
{
	RsGxsId gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);

	std::string radix;
	time_t timeout = time(NULL)+2;
	bool found = mRsIdentity->serialiseIdentityToMemory(gxs_id, radix);
	while(!found && time(nullptr) < timeout)
	{
		usleep(5000);
		found = mRsIdentity->serialiseIdentityToMemory(gxs_id, radix);
	}

	if(found)
	{
		resp.mDataStream << makeKeyValueReference("gxs_id", gxs_id)
		                 << makeKeyValueReference("radix", radix);

		resp.setOk();
		return;
	}

	resp.setFail();
}

void IdentityHandler::handleImportKey(Request& req, Response& resp)
{
	std::string radix;
	req.mStream << makeKeyValueReference("radix", radix);

	RsGxsId gxs_id;
	if(mRsIdentity->deserialiseIdentityFromMemory(radix, &gxs_id))
	{
		resp.mDataStream << makeKeyValueReference("gxs_id", gxs_id)
		                 << makeKeyValueReference("radix", radix);
		resp.setOk();
		return;
	}

	resp.setFail();
}

ResponseTask* IdentityHandler::handleDeleteIdentity(Request& /*req*/,
                                                    Response& /*resp*/)
{ return new DeleteIdentityTask(mRsIdentity); }

} // namespace resource_api
