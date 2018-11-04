/*******************************************************************************
 * libresapi/api/PeersHandler.cpp                                              *
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

#include "PeersHandler.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <util/radix64.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rsiface.h>
#include <retroshare/rsconfig.h>

#include <algorithm>
#include <time.h>

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
	std::string nodeType_string;
	if(details.isHiddenNode)
	{
		switch (details.hiddenType)
		{
			case RS_HIDDEN_TYPE_I2P:
				nodeType_string = "I2P";
			break;
			case RS_HIDDEN_TYPE_TOR:
				nodeType_string = "TOR";
			break;
			case RS_HIDDEN_TYPE_NONE:
				nodeType_string = "None";
			break;
			case RS_HIDDEN_TYPE_UNKNOWN:
				nodeType_string = "Unknown";
			break;
			default:
				nodeType_string = "Undefined";
		}
	}else{
		nodeType_string = "Normal";
	}

	stream
		<< makeKeyValueReference("pgp_id"        , details.gpg_id)
		<< makeKeyValueReference("peer_id"       , details.id)
		<< makeKeyValueReference("name"          , details.name)
		<< makeKeyValueReference("location"      , details.location)
		<< makeKeyValueReference("is_hidden_node", details.isHiddenNode)
		<< makeKeyValueReference("nodeType"      , nodeType_string )
		<< makeKeyValueReference("last_contact"  , details.lastConnect );

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

PeersHandler::PeersHandler( StateTokenServer* sts, RsNotify* notify,
                            RsPeers *peers, RsMsgs* msgs ) :
    mStateTokenServer(sts), mNotify(notify), mRsPeers(peers), mRsMsgs(msgs),
    status(0), mMtx("PeersHandler Mutex")
{
	mNotify->registerNotifyClient(this);
	mStateTokenServer->registerTickClient(this);
	addResourceHandler("*", this, &PeersHandler::handleWildcard);
	addResourceHandler("attempt_connection", this, &PeersHandler::handleAttemptConnection);
	addResourceHandler("get_state_string", this, &PeersHandler::handleGetStateString);
	addResourceHandler("set_state_string", this, &PeersHandler::handleSetStateString);
	addResourceHandler("get_custom_state_string", this, &PeersHandler::handleGetCustomStateString);
	addResourceHandler("set_custom_state_string", this, &PeersHandler::handleSetCustomStateString);
	addResourceHandler("get_network_options", this, &PeersHandler::handleGetNetworkOptions);
	addResourceHandler("set_network_options", this, &PeersHandler::handleSetNetworkOptions);
	addResourceHandler("get_pgp_options", this, &PeersHandler::handleGetPGPOptions);
	addResourceHandler("set_pgp_options", this, &PeersHandler::handleSetPGPOptions);
	addResourceHandler("get_node_name", this, &PeersHandler::handleGetNodeName);
	addResourceHandler("get_node_options", this, &PeersHandler::handleGetNodeOptions);
	addResourceHandler("set_node_options", this, &PeersHandler::handleSetNodeOptions);
	addResourceHandler("examine_cert", this, &PeersHandler::handleExamineCert);
	addResourceHandler("remove_node", this, &PeersHandler::handleRemoveNode);
	addResourceHandler("get_inactive_users", this, &PeersHandler::handleGetInactiveUsers);

}

PeersHandler::~PeersHandler()
{
	mNotify->unregisterNotifyClient(this);
	mStateTokenServer->unregisterTickClient(this);
}

void PeersHandler::notifyListChange(int list, int /* type */)
{
	if(list == NOTIFY_LIST_FRIENDS)
	{
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
		mStateTokenServer->discardToken(mStateToken);
		mStateToken = mStateTokenServer->getNewToken();
	}
}

void PeersHandler::notifyPeerStatusChanged(const std::string& /*peer_id*/, uint32_t /*state*/)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);
}

void PeersHandler::notifyPeerHasNewAvatar(std::string /*peer_id*/)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);
}

void PeersHandler::tick()
{
	std::list<RsPeerId> online;
	mRsPeers->getOnlineList(online);
	if(online != mOnlinePeers)
	{
		mOnlinePeers = online;

		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
		mStateTokenServer->discardToken(mStateToken);
		mStateToken = mStateTokenServer->getNewToken();
	}

	StatusInfo statusInfo;
	rsStatus->getOwnStatus(statusInfo);
	if(statusInfo.status != status)
	{
		status = statusInfo.status;

		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
		mStateTokenServer->discardToken(mStringStateToken);
		mStringStateToken = mStateTokenServer->getNewToken();
	}

	std::string custom_state = rsMsgs->getCustomStateString();
	if(custom_state != custom_state_string)
	{
		custom_state_string = custom_state;

		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
		mStateTokenServer->discardToken(mCustomStateToken);
		mCustomStateToken = mStateTokenServer->getNewToken();
	}
}

void PeersHandler::notifyUnreadMsgCountChanged(const RsPeerId &peer, uint32_t count)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
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
				resp.mDataStream << makeKeyValue(
				                        "cert_string",
				                        mRsPeers->GetRetroshareInvite());
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
				RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
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
				uint32_t lastConnect = 0;
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
						
						// Get latest contact timestamp
						if(vit->lastConnect > lastConnect)
							lastConnect = vit->lastConnect;

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
				itemStream << makeKeyValue("last_contact", lastConnect);
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

			uint32_t error_code;
			std::string error_string;
			RsPeerDetails peerDetails;

			RsPgpId own_pgp = mRsPeers->getGPGOwnId();
			RsPeerId ownpeer_id = mRsPeers->getOwnId();

			do
			{
				
				if (!cert_string.empty())
				{
					RsPgpId pgp_id;
					RsPeerId ssl_id;
					std::string error_string;

					if(!mRsPeers->loadCertificateFromString( cert_string,
					                                         ssl_id, pgp_id,
					                                         error_string ))
					{
						resp.mDebug << "cannot load that certificate "
						            << std::endl << error_string << std::endl;
						break;
					}
				}

				if(!mRsPeers->loadDetailsFromStringCert( cert_string,
				                                         peerDetails,
				                                         error_code ))
				{
					resp.mDebug << "Error: failed to add peer can not get "
					            << "peerDetails" << std::endl << error_code
					            << std::endl;
					break;
				}

				if(peerDetails.gpg_id.isNull())
				{
					resp.mDebug << "Error: failed to add peer gpg_id.isNull"
					            << std::endl << error_string << std::endl;
					break;
				}

				if(peerDetails.id == ownpeer_id)
				{
					std::cerr << __PRETTY_FUNCTION__ << " addPeer: is own "
					          << "peer_id, ignore" << std::endl;
					resp.mDebug << "Error: failed to add peer_id, because "
					            << "its your own peer_id" << std::endl
					            << error_string << std::endl;
					break;
				}

				if(!mRsPeers->addFriend( peerDetails.id, peerDetails.gpg_id,
				                         flags ))
				{
					std::cerr << __PRETTY_FUNCTION__
					          << "Error: failed to addFriend" << std::endl;
					resp.mDebug << "Error: failed to addFriend" << std::endl
					            << error_string << std::endl;
					break;
				}

				ok = true;

				// Retrun node details to response, after succes of adding peers
				peerDetailsToStream(resp.mDataStream, peerDetails);

				if (!peerDetails.location.empty()) 
				{
					mRsPeers->setLocation(peerDetails.id, peerDetails.location);
				}
				
				// Update new address even the peer already existed.
				if (peerDetails.isHiddenNode)
				{
					mRsPeers->setHiddenNode( peerDetails.id,
					                         peerDetails.hiddenNodeAddress,
					                         peerDetails.hiddenNodePort );
				}
				else
				{
					//let's check if there is ip adresses in the certificate.
					if (!peerDetails.extAddr.empty() && peerDetails.extPort)
						mRsPeers->setExtAddress( peerDetails.id,
						                         peerDetails.extAddr,
						                         peerDetails.extPort );
					if (!peerDetails.localAddr.empty() && peerDetails.localPort)
						mRsPeers->setLocalAddress( peerDetails.id,
						                           peerDetails.localAddr,
						                           peerDetails.localPort );
					if (!peerDetails.dyndns.empty())
						mRsPeers->setDynDNS(peerDetails.id, peerDetails.dyndns);
					for(auto&& ipr : peerDetails.ipAddressList)
						mRsPeers->addPeerLocator(
						            peerDetails.id,
						            RsUrl(ipr.substr(0, ipr.find(' '))) );

				}
			}
			while(false);
		}
	}
	if(ok) resp.setOk();
	else resp.setFail();
}

void PeersHandler::handleAttemptConnection(Request &req, Response &resp)
{
	std::string ssl_peer_id;
	req.mStream << makeKeyValueReference("peer_id", ssl_peer_id);
	RsPeerId peerId(ssl_peer_id);
	if(peerId.isNull()) resp.setFail("Invalid peer_id");
	else
	{
		mRsPeers->connectAttempt(peerId);
		resp.setOk();
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

void PeersHandler::handleRemoveNode(Request &req, Response &resp)
{
	std::string ssl_peer_id;
	req.mStream << makeKeyValueReference("peer_id", ssl_peer_id);
	RsPeerId peerId(ssl_peer_id);

	if(!peerId.isNull())
	{
		mRsPeers->removeFriendLocation(peerId);
		resp.mDataStream << makeKeyValue("peer_id", ssl_peer_id);
		resp.setOk();
	}
	else resp.setFail("handleRemoveNode Invalid peer_id");
}

void PeersHandler::handleGetInactiveUsers(Request &req, Response &resp)
{
	std::string datetime;
	req.mStream << makeKeyValueReference("datetime", datetime);

	uint32_t before;
	if(datetime.empty()) before = time(NULL);
	else before = strtoul(datetime.c_str(), NULL, 10);
	
	// list all peers
	std::list<RsPeerId> peers;
	mRsPeers->getFriendList(peers);
	
	// Make pair of gpg_id and last connect time
	std::map<RsPgpId, uint32_t> mapRsPgpId;
	
	// get all peers' infomations
	for( std::list<RsPeerId>::iterator lt = peers.begin(); lt != peers.end();
	     ++lt )
	{
		RsPeerDetails details;
		mRsPeers->getPeerDetails(*lt, details);
		// Search for exist pgp_id, update the last connect time
		if(mapRsPgpId.find(details.gpg_id) != mapRsPgpId.end())
		{
			if(mapRsPgpId[details.gpg_id] < details.lastConnect)
				mapRsPgpId[details.gpg_id] = details.lastConnect;
		}
		else
		{
			mapRsPgpId.insert(
			            std::make_pair(details.gpg_id, details.lastConnect) );
		}
	}

	// mark response as list, in case it is empty
	resp.mDataStream.getStreamToMember();

	std::map<RsPgpId, uint32_t>::iterator lit;
	for( lit = mapRsPgpId.begin(); lit != mapRsPgpId.end(); ++lit)
	{
		if (lit->second < before)
		{
			resp.mDataStream.getStreamToMember()
			        << makeKeyValue("pgp_id", lit->first.toStdString())
			        << makeKeyValue("name", mRsPeers->getGPGName(lit->first))
			        << makeKeyValue("last_contact", lit->second);
		}
	}

	resp.setOk();
}

void PeersHandler::handleGetNetworkOptions(Request& /*req*/, Response& resp)
{
	RsPeerDetails detail;
	if (!mRsPeers->getPeerDetails(mRsPeers->getOwnId(), detail))
		return;

	resp.mDataStream << makeKeyValue("local_address", detail.localAddr);
	resp.mDataStream << makeKeyValue("local_port", (int)detail.localPort);
	resp.mDataStream << makeKeyValue("external_address", detail.extAddr);
	resp.mDataStream << makeKeyValue("external_port", (int)detail.extPort);
	resp.mDataStream << makeKeyValue("dyn_dns", detail.dyndns);

	int netIndex = 0;
	switch(detail.netMode)
	{
	case RS_NETMODE_EXT:
		netIndex = 2;
		break;
	case RS_NETMODE_UDP:
		netIndex = 1;
		break;
	case RS_NETMODE_UPNP:
		netIndex = 0;
		break;
	}

	resp.mDataStream << makeKeyValue("nat_mode", netIndex);

	int discoveryIndex = 3; // NONE.
	if(detail.vs_dht != RS_VS_DHT_OFF)
	{
		if(detail.vs_disc != RS_VS_DISC_OFF)
			discoveryIndex = 0; // PUBLIC
		else
			discoveryIndex = 2; // INVERTED
	}
	else
	{
		if(detail.vs_disc != RS_VS_DISC_OFF)
			discoveryIndex = 1; // PRIVATE
		else
			discoveryIndex = 3; // NONE
	}

	resp.mDataStream << makeKeyValue("discovery_mode", discoveryIndex);

	int dlrate = 0;
	int ulrate = 0;
	rsConfig->GetMaxDataRates(dlrate, ulrate);
	resp.mDataStream << makeKeyValue("download_limit", dlrate);
	resp.mDataStream << makeKeyValue("upload_limit", ulrate);

	bool checkIP = mRsPeers->getAllowServerIPDetermination();
	resp.mDataStream << makeKeyValue("check_ip", checkIP);

	StreamBase& previousIPsStream = resp.mDataStream.getStreamToMember("previous_ips");
	previousIPsStream.getStreamToMember();
	for(std::list<std::string>::const_iterator it = detail.ipAddressList.begin(); it != detail.ipAddressList.end(); ++it)
		previousIPsStream.getStreamToMember() << makeKeyValue("ip_address", *it);

	std::list<std::string> ip_servers;
	mRsPeers->getIPServersList(ip_servers);

	StreamBase& websitesStream = resp.mDataStream.getStreamToMember("websites");
	websitesStream.getStreamToMember();

	for(std::list<std::string>::const_iterator it = ip_servers.begin(); it != ip_servers.end(); ++it)
		websitesStream.getStreamToMember() << makeKeyValue("website", *it);

	std::string proxyaddr;
	uint16_t proxyport;
	uint32_t status ;
	// Tor
	mRsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, proxyaddr, proxyport, status);
	resp.mDataStream << makeKeyValue("tor_address", proxyaddr);
	resp.mDataStream << makeKeyValue("tor_port", (int)proxyport);

	// I2P
	mRsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, proxyaddr, proxyport, status);
	resp.mDataStream << makeKeyValue("i2p_address", proxyaddr);
	resp.mDataStream << makeKeyValue("i2p_port", (int)proxyport);

	resp.setOk();
}

void PeersHandler::handleSetNetworkOptions(Request& req, Response& resp)
{
	RsPeerDetails detail;
	if (!mRsPeers->getPeerDetails(mRsPeers->getOwnId(), detail))
		return;

	int netIndex = 0;
	uint32_t natMode = 0;
	req.mStream << makeKeyValueReference("nat_mode", netIndex);

	switch(netIndex)
	{
	case 3:
		natMode = RS_NETMODE_HIDDEN;
		break;
	case 2:
		natMode = RS_NETMODE_EXT;
		break;
	case 1:
		natMode = RS_NETMODE_UDP;
		break;
	default:
	case 0:
		natMode = RS_NETMODE_UPNP;
		break;
	}

	if (detail.netMode != natMode)
		mRsPeers->setNetworkMode(mRsPeers->getOwnId(), natMode);

	int discoveryIndex;
	uint16_t vs_disc = 0;
	uint16_t vs_dht = 0;
	req.mStream << makeKeyValueReference("discovery_mode", discoveryIndex);

	switch(discoveryIndex)
	{
	    case 0:
		    vs_disc = RS_VS_DISC_FULL;
			vs_dht = RS_VS_DHT_FULL;
		    break;
	    case 1:
		    vs_disc = RS_VS_DISC_FULL;
			vs_dht = RS_VS_DHT_OFF;
		    break;
	    case 2:
		    vs_disc = RS_VS_DISC_OFF;
			vs_dht = RS_VS_DHT_FULL;
		    break;
	    case 3:
	    default:
		    vs_disc = RS_VS_DISC_OFF;
			vs_dht = RS_VS_DHT_OFF;
		    break;
	}

	if ((vs_disc != detail.vs_disc) || (vs_dht != detail.vs_dht))
		mRsPeers->setVisState(mRsPeers->getOwnId(), vs_disc, vs_dht);

	if (0 != netIndex)
	{
		std::string localAddr;
		int localPort;
		std::string extAddr;
		int extPort;

		req.mStream << makeKeyValueReference("local_address", localAddr);
		req.mStream << makeKeyValueReference("local_port", localPort);
		req.mStream << makeKeyValueReference("external_address", extAddr);
		req.mStream << makeKeyValueReference("external_port", extPort);

		mRsPeers->setLocalAddress(mRsPeers->getOwnId(), localAddr, (uint16_t)localPort);
		mRsPeers->setExtAddress(mRsPeers->getOwnId(), extAddr, (uint16_t)extPort);
	}

	std::string dynDNS;
	req.mStream << makeKeyValueReference("dyn_dns", dynDNS);
	mRsPeers->setDynDNS(mRsPeers->getOwnId(), dynDNS);

	int dlrate = 0;
	int ulrate = 0;
	req.mStream << makeKeyValueReference("download_limit", dlrate);
	req.mStream << makeKeyValueReference("upload_limit", ulrate);
	rsConfig->SetMaxDataRates(dlrate, ulrate);

	bool checkIP;
	req.mStream << makeKeyValueReference("check_ip", checkIP);
	mRsPeers->allowServerIPDetermination(checkIP) ;

	// Tor
	std::string toraddr;
	int torport;
	req.mStream << makeKeyValueReference("tor_address", toraddr);
	req.mStream << makeKeyValueReference("tor_port", torport);
	mRsPeers->setProxyServer(RS_HIDDEN_TYPE_TOR, toraddr, (uint16_t)torport);

	// I2P
	std::string i2paddr;
	int i2pport;
	req.mStream << makeKeyValueReference("i2p_address", i2paddr);
	req.mStream << makeKeyValueReference("i2p_port", i2pport);
	mRsPeers->setProxyServer(RS_HIDDEN_TYPE_I2P, i2paddr, (uint16_t)i2pport);

	resp.mStateToken = getCurrentStateToken();
	resp.setOk();
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

void PeersHandler::handleGetNodeName(Request& req, Response& resp)
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
		encryption = cdet.cipher_name;
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
			mRsPeers->setHiddenNode(peerId, local_address, local_port);
	}

	resp.setOk();
}

StateToken PeersHandler::getCurrentStateToken()
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	if(mStateToken.isNull())
		mStateToken = mStateTokenServer->getNewToken();
	return mStateToken;
}

} // namespace resource_api
