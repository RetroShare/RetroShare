#include "PeersHandler.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <util/radix64.h>

#include <algorithm>

#include "Operators.h"
#include "ApiTypes.h"

namespace resource_api
{

// todo: groups, add friend, remove friend, permissions

void peerDetailsToStream(StreamBase& stream, RsPeerDetails& details)
{
    stream
        << makeKeyValueReference("peer_id", details.id)
        << makeKeyValueReference("name", details.name)
        << makeKeyValueReference("location", details.location)
        << makeKeyValueReference("pgp_id", details.gpg_id)
            ;
}

bool peerInfoToStream(StreamBase& stream, RsPeerDetails& details, RsPeers* peers, std::list<RsGroupInfo>& grpInfo, bool have_avatar)
{
    bool ok = true;
    peerDetailsToStream(stream, details);
    stream << makeKeyValue("is_online", peers->isOnline(details.id))
           << makeKeyValue("chat_id", ChatId(details.id).toStdString());

    std::string avatar_address = "/"+details.id.toStdString()+"/avatar_image";

    if(!have_avatar)
        avatar_address = "";

    stream << makeKeyValue("avatar_address", avatar_address);

    StreamBase& grpStream = stream.getStreamToMember("groups");

    for(std::list<RsGroupInfo>::iterator lit = grpInfo.begin(); lit != grpInfo.end(); lit++)
    {
        RsGroupInfo& grp = *lit;
        if(std::find(grp.peerIds.begin(), grp.peerIds.end(), details.gpg_id) != grp.peerIds.end())
        {
            grpStream.getStreamToMember()
                    << makeKeyValueReference("group_name", grp.name)
                    << makeKeyValueReference("group_id", grp.id);
        }
    }

    return ok;
}

PeersHandler::PeersHandler(StateTokenServer* sts, RsNotify* notify, RsPeers *peers, RsMsgs* msgs):
    mStateTokenServer(sts),
    mNotify(notify),
    mRsPeers(peers), mRsMsgs(msgs),
    mMtx("PeersHandler Mutex")
{
    mNotify->registerNotifyClient(this);
    mStateTokenServer->registerTickClient(this);
    addResourceHandler("*", this, &PeersHandler::handleWildcard);
    addResourceHandler("examine_cert", this, &PeersHandler::handleExamineCert);
}

PeersHandler::~PeersHandler()
{
    mNotify->unregisterNotifyClient(this);
    mStateTokenServer->unregisterTickClient(this);
}

void PeersHandler::notifyListChange(int list, int type)
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    if(list == NOTIFY_LIST_FRIENDS)
    {
        mStateTokenServer->discardToken(mStateToken);
        mStateToken = mStateTokenServer->getNewToken();
    }
}

void PeersHandler::notifyPeerHasNewAvatar(std::string /*peer_id*/)
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    mStateTokenServer->replaceToken(mStateToken);
}

void PeersHandler::tick()
{
    std::list<RsPeerId> online;
    mRsPeers->getOnlineList(online);
    if(online != mOnlinePeers)
    {
        mOnlinePeers = online;

        RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
        mStateTokenServer->discardToken(mStateToken);
        mStateToken = mStateTokenServer->getNewToken();
    }
}

void PeersHandler::notifyUnreadMsgCountChanged(const RsPeerId &peer, uint32_t count)
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    mUnreadMsgsCounts[peer] = count;
    mStateTokenServer->replaceToken(mStateToken);
}

static bool have_avatar(RsMsgs* msgs, const RsPeerId& id)
{
    // check if avatar data is available
    // requests missing avatar images as a side effect
    unsigned char *data = NULL ;
    int size = 0 ;
    msgs->getAvatarData(id, data,size) ;
    std::vector<uint8_t> avatar(data, data+size);
    delete[] data;
    return size != 0;
}

void PeersHandler::handleWildcard(Request &req, Response &resp)
{
    bool ok = false;
    if(!req.mPath.empty())
    {
        std::string str = req.mPath.top();
        req.mPath.pop();
        if(str != "")
        {
            if(str == "self" && !req.mPath.empty() && req.mPath.top() == "certificate")
            {
                resp.mDataStream << makeKeyValue("cert_string", mRsPeers->GetRetroshareInvite(false));
                resp.setOk();
                return;
            }
            // assume the path element is a peer id
            // sometimes it is a peer id for location info
            // another time it is a pgp id
            // this will confuse the client developer
            if(!req.mPath.empty() && req.mPath.top() == "avatar_image")
            {
                // the avatar image
                // better have this extra, else have to load all avatar images
                // only to see who is online
                unsigned char *data = NULL ;
                int size = 0 ;
                mRsMsgs->getAvatarData(RsPeerId(str),data,size) ;
                std::vector<uint8_t> avatar(data, data+size);
                delete[] data;
                resp.mDataStream << avatar;
            }
            else if(!req.mPath.empty() && req.mPath.top() == "delete")
            {
                mRsPeers->removeFriend(RsPgpId(str));
            }
            else
            {
                std::list<RsGroupInfo> grpInfo;
                mRsPeers->getGroupInfoList(grpInfo);
                RsPeerDetails details;
                ok &= mRsPeers->getPeerDetails(RsPeerId(str), details);
                ok = peerInfoToStream(resp.mDataStream, details, mRsPeers, grpInfo, have_avatar(mRsMsgs, RsPeerId(str)));
            }
        }
    }
    else
    {
        // no more path element
        if(req.isGet())
        {
            std::map<RsPeerId, uint32_t> unread_msgs;
            {
                RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
                unread_msgs = mUnreadMsgsCounts;
            }
            // list all peers
            ok = true;
            std::list<RsPgpId> identities;
            ok &= mRsPeers->getGPGAcceptedList(identities);
            RsPgpId own_pgp = mRsPeers->getGPGOwnId();
            identities.push_back(own_pgp);
            std::list<RsPeerId> peers;
            ok &= mRsPeers->getFriendList(peers);
            std::list<RsGroupInfo> grpInfo;
            mRsPeers->getGroupInfoList(grpInfo);
            std::vector<RsPeerDetails> detailsVec;
            for(std::list<RsPeerId>::iterator lit = peers.begin(); lit != peers.end(); ++lit)
            {
                RsPeerDetails details;
                ok &= mRsPeers->getPeerDetails(*lit, details);
                detailsVec.push_back(details);
            }
            for(std::list<RsPgpId>::iterator lit = identities.begin(); lit != identities.end(); ++lit)
            {
                // if no own ssl id is known, then hide the own id from the friendslist
                if(*lit == own_pgp)
                {
                    bool found = false;
                    for(std::vector<RsPeerDetails>::iterator vit = detailsVec.begin(); vit != detailsVec.end(); ++vit)
                    {
                        if(vit->gpg_id == *lit)
                            found = true;
                    }
                    if(!found)
                        continue;
                }
                StreamBase& itemStream = resp.mDataStream.getStreamToMember();
                itemStream << makeKeyValueReference("pgp_id", *lit);
                itemStream << makeKeyValue("name", mRsPeers->getGPGName(*lit));
                itemStream << makeKeyValue("is_own", *lit == own_pgp);
                StreamBase& locationStream = itemStream.getStreamToMember("locations");
                // mark as list (in case list is empty)
                locationStream.getStreamToMember();
                for(std::vector<RsPeerDetails>::iterator vit = detailsVec.begin(); vit != detailsVec.end(); ++vit)
                {
                    if(vit->gpg_id == *lit)
                    {
                        StreamBase& stream = locationStream.getStreamToMember();
                        double unread = 0;
                        if(unread_msgs.find(vit->id) != unread_msgs.end())
                            unread = unread_msgs.find(vit->id)->second;
                        stream << makeKeyValueReference("unread_msgs", unread);
                        peerInfoToStream(stream,*vit, mRsPeers, grpInfo, have_avatar(mRsMsgs, vit->id));
                    }
                }
            }
            resp.mStateToken = getCurrentStateToken();
        }
        else if(req.isPut())
        {
            std::string cert_string;
            req.mStream << makeKeyValueReference("cert_string", cert_string);

            ServicePermissionFlags flags;
            StreamBase& flags_stream = req.mStream.getStreamToMember("flags");
            if(req.mStream.isOK())
            {
                bool direct_dl = RS_NODE_PERM_DEFAULT & RS_NODE_PERM_DIRECT_DL;
                flags_stream << makeKeyValueReference("allow_direct_download", direct_dl);
                if(direct_dl) flags |= RS_NODE_PERM_DIRECT_DL;

                bool allow_push = RS_NODE_PERM_DEFAULT & RS_NODE_PERM_ALLOW_PUSH;
                flags_stream << makeKeyValueReference("allow_push", allow_push);
                if(allow_push) flags |= RS_NODE_PERM_ALLOW_PUSH;

                bool require_whitelist = RS_NODE_PERM_DEFAULT & RS_NODE_PERM_REQUIRE_WL;
                flags_stream << makeKeyValueReference("require_whitelist", require_whitelist);
                if(require_whitelist) flags |= RS_NODE_PERM_REQUIRE_WL;
            }
            else
            {
                flags = RS_NODE_PERM_DEFAULT;
            }
            RsPeerId peer_id;
            RsPgpId pgp_id;
            std::string error_string;
            if(mRsPeers->loadCertificateFromString(cert_string, peer_id, pgp_id, error_string)
                    && mRsPeers->addFriend(peer_id, pgp_id, flags))
            {
                ok = true;
                resp.mDataStream << makeKeyValueReference("pgp_id", pgp_id);
                resp.mDataStream << makeKeyValueReference("peer_id", peer_id);
            }
            else
            {
                resp.mDebug << "Error: failed to add peer" << std::endl;
                resp.mDebug << error_string << std::endl;
            }
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

void PeersHandler::handleExamineCert(Request &req, Response &resp)
{
    std::string cert_string;
    req.mStream << makeKeyValueReference("cert_string", cert_string);
    RsPeerDetails details;
    uint32_t error_code;
    if(mRsPeers->loadDetailsFromStringCert(cert_string, details, error_code))
    {
        peerDetailsToStream(resp.mDataStream, details);
        resp.setOk();
    }
    else
    {
        resp.setFail("failed to load certificate");
    }
}

StateToken PeersHandler::getCurrentStateToken()
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    if(mStateToken.isNull())
        mStateToken = mStateTokenServer->getNewToken();
    return mStateToken;
}

} // namespace resource_api
