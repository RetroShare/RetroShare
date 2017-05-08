#include "PeersHandler.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <util/radix64.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rsiface.h>

#include <algorithm>

#include "Operators.h"
#include "ApiTypes.h"

namespace resource_api
{

#define PEER_STATE_ONLINE       1
#define PEER_STATE_BUSY         2
#define PEER_STATE_AWAY         3
#define PEER_STATE_AVAILABLE    4
#define PEER_STATE_INACTIVE     5
#define PEER_STATE_OFFLINE      6
// todo: groups, add friend, remove friend, permissions

void peerDetailsToStream(StreamBase& stream, RsPeerDetails& details)
{
    stream
        << makeKeyValueReference("peer_id", details.id)
        << makeKeyValueReference("name", details.name)
        << makeKeyValueReference("location", details.location)
        << makeKeyValueReference("pgp_id", details.gpg_id)
	    << makeKeyValueReference("pgp_id", details.gpg_id);

	if(details.state & RS_PEER_STATE_CONNECTED)
	{
		std::list<StatusInfo> statusInfo;
		rsStatus->getStatusList(statusInfo);

		std::string state_string;
		std::list<StatusInfo>::iterator it;
		for (it = statusInfo.begin(); it != statusInfo.end(); ++it)
		{
			if (it->id == details.id)
			{
				switch (it->status)
				{
				case RS_STATUS_INACTIVE:
					state_string = "inactive";
					break;

				case RS_STATUS_ONLINE:
					state_string = "online";
					break;

				case RS_STATUS_AWAY:
					state_string = "away";
					break;

				case RS_STATUS_BUSY:
					state_string = "busy";
					break;
				default:
					state_string = "undefined";
					break;
				}
				break;
			}
		}
		stream << makeKeyValueReference("state_string", state_string);
	}
	else
	{
		std::string state_string = "undefined";
		stream << makeKeyValueReference("state_string", state_string);
	}
}

bool peerInfoToStream(StreamBase& stream, RsPeerDetails& details, RsPeers* peers, std::list<RsGroupInfo>& grpInfo, bool have_avatar)
{
    bool ok = true;
    peerDetailsToStream(stream, details);
    stream << makeKeyValue("is_online", peers->isOnline(details.id))
	       << makeKeyValue("chat_id", ChatId(details.id).toStdString())
	       << makeKeyValue("custom_state_string", rsMsgs->getCustomStateString(details.id));


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

std::string peerStateString(int peerState)
{
	if (peerState & RS_PEER_STATE_CONNECTED) {
		return "Connected";
	} else if (peerState & RS_PEER_STATE_UNREACHABLE) {
		return "Unreachable";
	} else if (peerState & RS_PEER_STATE_ONLINE) {
		return "Available";
	} else if (peerState & RS_PEER_STATE_FRIEND) {
		return "Offline";
	}

	return "Neighbor";
}

std::string connectStateString(RsPeerDetails &details)
{
	std::string stateString;
	bool isConnected = false;

	switch (details.connectState) {
	case 0:
		stateString = peerStateString(details.state);
		break;
	case RS_PEER_CONNECTSTATE_TRYING_TCP:
		stateString = "Trying TCP";
		break;
	case RS_PEER_CONNECTSTATE_TRYING_UDP:
		stateString = "Trying UDP";
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_TCP:
		stateString = "Connected: TCP";
		isConnected = true;
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_UDP:
		stateString = "Connected: UDP";
		isConnected = true;
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_TOR:
		stateString = "Connected: Tor";
		isConnected = true;
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_I2P:
		stateString = "Connected: I2P";
		isConnected = true;
		break;
	case RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN:
		stateString = "Connected: Unknown";
		isConnected = true;
		break;
	}

	if(isConnected) {
		stateString += " ";
		if(details.actAsServer)
			stateString += "inbound connection";
		else
			stateString += "outbound connection";
	}

	if (details.connectStateString.empty() == false) {
		if (stateString.empty() == false) {
			stateString += ": ";
		}
		stateString += details.connectStateString;
	}

	/* HACK to display DHT Status info too */
	if (details.foundDHT) {
		if (stateString.empty() == false) {
			stateString += ", ";
		}
		stateString += "DHT: Contact";
	}

	return stateString;
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
	addResourceHandler("get_state_string", this, &PeersHandler::handleGetStateString);
	addResourceHandler("set_state_string", this, &PeersHandler::handleSetStateString);
	addResourceHandler("get_custom_state_string", this, &PeersHandler::handleGetCustomStateString);
	addResourceHandler("set_custom_state_string", this, &PeersHandler::handleSetCustomStateString);
	addResourceHandler("get_pgp_options", this, &PeersHandler::handleGetPGPOptions);
	addResourceHandler("set_pgp_options", this, &PeersHandler::handleSetPGPOptions);
	addResourceHandler("get_node_options", this, &PeersHandler::handleGetNodeOptions);
	addResourceHandler("set_node_options", this, &PeersHandler::handleSetNodeOptions);
	addResourceHandler("examine_cert", this, &PeersHandler::handleExamineCert);
}

PeersHandler::~PeersHandler()
{
    mNotify->unregisterNotifyClient(this);
    mStateTokenServer->unregisterTickClient(this);
}

void PeersHandler::notifyListChange(int list, int /* type */)
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    if(list == NOTIFY_LIST_FRIENDS)
    {
        mStateTokenServer->discardToken(mStateToken);
        mStateToken = mStateTokenServer->getNewToken();
    }
}

void PeersHandler::notifyPeerStatusChanged(const std::string& /*peer_id*/, uint32_t /*state*/)
{
	RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
	mStateTokenServer->replaceToken(mStateToken);
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

	StatusInfo statusInfo;
	rsStatus->getOwnStatus(statusInfo);
	if(statusInfo.status != status)
	{
		status = statusInfo.status;

		RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
		mStateTokenServer->discardToken(mStringStateToken);
		mStringStateToken = mStateTokenServer->getNewToken();
	}

	std::string custom_state = rsMsgs->getCustomStateString();
	if(custom_state != custom_state_string)
	{
		custom_state_string = custom_state;

		RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
		mStateTokenServer->discardToken(mCustomStateToken);
		mCustomStateToken = mStateTokenServer->getNewToken();
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

void PeersHandler::handleGetStateString(Request& /*req*/, Response& resp)
{
	{
		RS_STACK_MUTEX(mMtx);
		resp.mStateToken = mStringStateToken;
	}

	std::string state_string;
	StatusInfo statusInfo;
	if (rsStatus->getOwnStatus(statusInfo))
	{
		if(statusInfo.status == RS_STATUS_ONLINE)
			state_string = "online";
		else if(statusInfo.status == RS_STATUS_BUSY)
			state_string = "busy";
		else if(statusInfo.status == RS_STATUS_AWAY)
			state_string = "away";
		else if(statusInfo.status == RS_STATUS_INACTIVE)
			state_string = "inactive";
		else
			state_string = "undefined";
	}
	else
		state_string = "undefined";

	resp.mDataStream << makeKeyValueReference("state_string", state_string);
	resp.setOk();
}

void PeersHandler::handleSetStateString(Request& req, Response& resp)
{
	std::string state_string;
	req.mStream << makeKeyValueReference("state_string", state_string);

	uint32_t status = RS_STATUS_OFFLINE;
	if(state_string == "online")
		status = RS_STATUS_ONLINE;
	else if(state_string == "busy")
		status = RS_STATUS_BUSY;
	else if(state_string == "away")
		status = RS_STATUS_AWAY;

	rsStatus->sendStatus(RsPeerId(), status);
	resp.setOk();
}

void PeersHandler::handleGetCustomStateString(Request& /*req*/, Response& resp)
{
	{
		RS_STACK_MUTEX(mMtx);
		resp.mStateToken = mCustomStateToken;
	}

	std::string custom_state_string = rsMsgs->getCustomStateString();
	resp.mDataStream << makeKeyValueReference("custom_state_string", custom_state_string);
	resp.setOk();
}

void PeersHandler::handleSetCustomStateString(Request& req, Response& resp)
{
	std::string custom_state_string;
	req.mStream << makeKeyValueReference("custom_state_string", custom_state_string);

	rsMsgs->setCustomStateString(custom_state_string);
	resp.setOk();
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
			std::list<StatusInfo> statusInfo;
			rsStatus->getStatusList(statusInfo);

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
            // mark response as list, in case it is empty
            resp.mDataStream.getStreamToMember();
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

				int bestPeerState = 0;
				unsigned int bestRSState = 0;
				std::string bestCustomStateString;

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


						/* Custom state string */
						std::string customStateString;
						if (vit->state & RS_PEER_STATE_CONNECTED)
						{
							customStateString = rsMsgs->getCustomStateString(vit->id);
						}

						int peerState = 0;

						if (vit->state & RS_PEER_STATE_CONNECTED)
						{
							// get the status info for this ssl id
							int rsState = 0;
							std::list<StatusInfo>::iterator it;
							for (it = statusInfo.begin(); it != statusInfo.end(); ++it)
							{
								if (it->id == vit->id)
								{
									rsState = it->status;
									switch (rsState)
									{
									case RS_STATUS_INACTIVE:
										peerState = PEER_STATE_INACTIVE;
										break;

									case RS_STATUS_ONLINE:
										peerState = PEER_STATE_ONLINE;
										break;

									case RS_STATUS_AWAY:
										peerState = PEER_STATE_AWAY;
										break;

									case RS_STATUS_BUSY:
										peerState = PEER_STATE_BUSY;
										break;
									}

									/* find the best ssl contact for the gpg item */
									if (bestPeerState == 0 || peerState < bestPeerState)
									{
										bestPeerState = peerState;
										bestRSState = rsState;
										bestCustomStateString = customStateString;
									}
									else if (peerState == bestPeerState)
									{
										/* equal state */
										if (bestCustomStateString.empty() && !customStateString.empty())
										{
											bestPeerState = peerState;
											bestRSState = rsState;
											bestCustomStateString = customStateString;
										}
									}
									break;
								}
							}
						}
					}
                }
				itemStream << makeKeyValue("custom_state_string", bestCustomStateString);

				std::string state_string;

				if(bestRSState == RS_STATUS_ONLINE)
					state_string = "online";
				else if(bestRSState == RS_STATUS_BUSY)
					state_string = "busy";
				else if(bestRSState == RS_STATUS_AWAY)
					state_string = "away";
				else if(bestRSState == RS_STATUS_INACTIVE)
					state_string = "inactive";
				else
					state_string = "undefined";

				itemStream << makeKeyValue("state_string", state_string);
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

void PeersHandler::handleGetPGPOptions(Request& req, Response& resp)
{
	std::string pgp_id;
	req.mStream << makeKeyValueReference("pgp_id", pgp_id);

	RsPgpId pgp(pgp_id);
	RsPeerDetails detail;

	if(!mRsPeers->getGPGDetails(pgp, detail))
	{
		resp.setFail();
		return;
	}

	std::string pgp_key = mRsPeers->getPGPKey(detail.gpg_id, false);

	resp.mDataStream << makeKeyValue("pgp_fingerprint", detail.fpr.toStdString());
	resp.mDataStream << makeKeyValueReference("pgp_key", pgp_key);

	resp.mDataStream << makeKeyValue("direct_transfer", detail.service_perm_flags & RS_NODE_PERM_DIRECT_DL);
	resp.mDataStream << makeKeyValue("allow_push", detail.service_perm_flags & RS_NODE_PERM_ALLOW_PUSH);
	resp.mDataStream << makeKeyValue("require_WL", detail.service_perm_flags & RS_NODE_PERM_REQUIRE_WL);

	resp.mDataStream << makeKeyValue("own_sign", detail.ownsign);
	resp.mDataStream << makeKeyValue("trustLvl", detail.trustLvl);

	uint32_t max_upload_speed = 0;
	uint32_t max_download_speed = 0;

	mRsPeers->getPeerMaximumRates(pgp, max_upload_speed, max_download_speed);

	resp.mDataStream << makeKeyValueReference("maxUploadSpeed", max_upload_speed);
	resp.mDataStream << makeKeyValueReference("maxDownloadSpeed", max_download_speed);

	StreamBase& signersStream = resp.mDataStream.getStreamToMember("gpg_signers");

	// mark as list (in case list is empty)
	signersStream.getStreamToMember();

	for(std::list<RsPgpId>::const_iterator it(detail.gpgSigners.begin()); it != detail.gpgSigners.end(); ++it)
	{
		RsPeerDetails detail;
		if(!mRsPeers->getGPGDetails(*it, detail))
			continue;

		std::string pgp_id = (*it).toStdString();
		std::string name = detail.name;

		signersStream.getStreamToMember()
		    << makeKeyValueReference("pgp_id", pgp_id)
		    << makeKeyValueReference("name", name);
	}

	resp.setOk();
}

void PeersHandler::handleSetPGPOptions(Request& req, Response& resp)
{
	std::string pgp_id;
	req.mStream << makeKeyValueReference("pgp_id", pgp_id);

	RsPgpId pgp(pgp_id);
	RsPeerDetails detail;

	if(!mRsPeers->getGPGDetails(pgp, detail))
	{
		resp.setFail();
		return;
	}

	int trustLvl;
	req.mStream << makeKeyValueReference("trustLvl", trustLvl);

	if(trustLvl != (int)detail.trustLvl)
		mRsPeers->trustGPGCertificate(pgp, trustLvl);

	int max_upload_speed;
	int max_download_speed;

	req.mStream << makeKeyValueReference("max_upload_speed", max_upload_speed);
	req.mStream << makeKeyValueReference("max_download_speed", max_download_speed);

	mRsPeers->setPeerMaximumRates(pgp, (uint32_t)max_upload_speed, (uint32_t)max_download_speed);

	bool direct_transfer;
	bool allow_push;
	bool require_WL;

	req.mStream << makeKeyValueReference("direct_transfer", direct_transfer);
	req.mStream << makeKeyValueReference("allow_push", allow_push);
	req.mStream << makeKeyValueReference("require_WL", require_WL);

	ServicePermissionFlags flags(0);

	if(direct_transfer)
		flags = flags | RS_NODE_PERM_DIRECT_DL;
	if(allow_push)
		flags = flags | RS_NODE_PERM_ALLOW_PUSH;
	if(require_WL)
		flags = flags | RS_NODE_PERM_REQUIRE_WL;

	mRsPeers->setServicePermissionFlags(pgp, flags);

	bool own_sign;
	req.mStream << makeKeyValueReference("own_sign", own_sign);

	if(own_sign)
		mRsPeers->signGPGCertificate(pgp);

	resp.mStateToken = getCurrentStateToken();

	resp.setOk();
}

void PeersHandler::handleGetNodeOptions(Request& req, Response& resp)
{
	std::string peer_id;
	req.mStream << makeKeyValueReference("peer_id", peer_id);

	RsPeerId peerId(peer_id);
	RsPeerDetails detail;
	if(!mRsPeers->getPeerDetails(peerId, detail))
	{
		resp.setFail();
		return;
	}

	resp.mDataStream << makeKeyValue("peer_id", detail.id.toStdString());
	resp.mDataStream << makeKeyValue("name", detail.name);
	resp.mDataStream << makeKeyValue("location", detail.location);
	resp.mDataStream << makeKeyValue("pgp_id", detail.gpg_id.toStdString());
	resp.mDataStream << makeKeyValue("last_contact", detail.lastConnect);

	std::string status_message = mRsMsgs->getCustomStateString(detail.id);
	resp.mDataStream << makeKeyValueReference("status_message", status_message);

	std::string encryption;
	RsPeerCryptoParams cdet;
	if(RsControl::instance()->getPeerCryptoDetails(detail.id, cdet) && cdet.connexion_state != 0)
	{
		encryption = cdet.cipher_version;
		encryption += ": ";
		encryption += cdet.cipher_name;

		if(cdet.cipher_version != "TLSv1.2")
			encryption += cdet.cipher_bits_1;
	}
	else
		encryption = "Not connected";

	resp.mDataStream << makeKeyValueReference("encryption", encryption);

	resp.mDataStream << makeKeyValue("is_hidden_node", detail.isHiddenNode);
	if (detail.isHiddenNode)
	{
		resp.mDataStream << makeKeyValue("local_address", detail.hiddenNodeAddress);
		resp.mDataStream << makeKeyValue("local_port", (int)detail.hiddenNodePort);
		resp.mDataStream << makeKeyValue("ext_address", std::string("none"));
		resp.mDataStream << makeKeyValue("ext_port", 0);
		resp.mDataStream << makeKeyValue("dyn_dns", std::string("none"));
	}
	else
	{
		resp.mDataStream << makeKeyValue("local_address", detail.localAddr);
		resp.mDataStream << makeKeyValue("local_port", (int)detail.localPort);
		resp.mDataStream << makeKeyValue("ext_address", detail.extAddr);
		resp.mDataStream << makeKeyValue("ext_port", (int)detail.extPort);
		resp.mDataStream << makeKeyValue("dyn_dns", detail.dyndns);
	}

	resp.mDataStream << makeKeyValue("connection_status", connectStateString(detail));

	StreamBase& addressesStream = resp.mDataStream.getStreamToMember("ip_addresses");

	// mark as list (in case list is empty)
	addressesStream.getStreamToMember();

	for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin()); it != detail.ipAddressList.end(); ++it)
	{
		addressesStream.getStreamToMember() << makeKeyValue("ip_address", (*it));
	}

	std::string certificate = mRsPeers->GetRetroshareInvite(detail.id, false);
	resp.mDataStream << makeKeyValueReference("certificate", certificate);

	resp.setOk();
}

void PeersHandler::handleSetNodeOptions(Request& req, Response& resp)
{
	std::string peer_id;
	req.mStream << makeKeyValueReference("peer_id", peer_id);

	RsPeerId peerId(peer_id);
	RsPeerDetails detail;
	if(!mRsPeers->getPeerDetails(peerId, detail))
	{
		resp.setFail();
		return;
	}

	std::string local_address;
	std::string ext_address;
	std::string dyn_dns;
	int local_port;
	int ext_port;

	req.mStream << makeKeyValueReference("local_address", local_address);
	req.mStream << makeKeyValueReference("local_port", local_port);
	req.mStream << makeKeyValueReference("ext_address", ext_address);
	req.mStream << makeKeyValueReference("ext_port", ext_port);
	req.mStream << makeKeyValueReference("dyn_dns", dyn_dns);

	if(!detail.isHiddenNode)
	{
		if(detail.localAddr != local_address || (int)detail.localPort != local_port)
			mRsPeers->setLocalAddress(peerId, local_address, local_port);
		if(detail.extAddr != ext_address || (int)detail.extPort != ext_port)
			mRsPeers->setExtAddress(peerId, ext_address, ext_port);
		if(detail.dyndns != dyn_dns)
			mRsPeers->setDynDNS(peerId, dyn_dns);
	}
	else
	{
		if(detail.hiddenNodeAddress != local_address || detail.hiddenNodePort != local_port)
			rsPeers->setHiddenNode(peerId, local_address, local_port);
	}

	resp.setOk();
}

StateToken PeersHandler::getCurrentStateToken()
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    if(mStateToken.isNull())
        mStateToken = mStateTokenServer->getNewToken();
    return mStateToken;
}

} // namespace resource_api
