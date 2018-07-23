/*******************************************************************************
 * libresapi/api/PeersHandler.h                                                *
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
#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"
#include "ChatHandler.h"
#include <retroshare/rsnotify.h>
#include <util/rsthreads.h>

class RsPeers;
class RsMsgs;

namespace resource_api
{

class PeersHandler: public ResourceRouter, NotifyClient, Tickable, public UnreadMsgNotify
{
public:
	PeersHandler(StateTokenServer* sts, RsNotify* notify, RsPeers* peers, RsMsgs* msgs);
	virtual ~PeersHandler();

	// from NotifyClient
	// note: this may get called from foreign threads
	virtual void notifyListChange(int list, int type); // friends list change
	virtual void notifyPeerStatusChanged(const std::string& /*peer_id*/, uint32_t /*state*/);
	virtual void notifyPeerHasNewAvatar(std::string /*peer_id*/);

	// from Tickable
	virtual void tick();

	// from UnreadMsgNotify
	// ChatHandler calls this to tell us about unreadmsgs
	// this allows to merge unread msgs info with the peers list
	virtual void notifyUnreadMsgCountChanged(const RsPeerId& peer, uint32_t count);

private:
	void handleWildcard(Request& req, Response& resp);

	void handleAttemptConnection(Request& req, Response& resp);

	void handleExamineCert(Request& req, Response& resp);

	void handleGetStateString(Request& req, Response& resp);
	void handleSetStateString(Request& req, Response& resp);

	void handleGetCustomStateString(Request& req, Response& resp);
	void handleSetCustomStateString(Request& req, Response& resp);

	void handleGetNetworkOptions(Request& req, Response& resp);
	void handleSetNetworkOptions(Request& req, Response& resp);

	void handleGetPGPOptions(Request& req, Response& resp);
	void handleSetPGPOptions(Request& req, Response& resp);

	void handleGetNodeName(Request& req, Response& resp);
	void handleGetNodeOptions(Request& req, Response& resp);
	void handleSetNodeOptions(Request& req, Response& resp);

	/**
	 *  \brief Remove specific location from trusted nodes
	 *
	 *  \param [in] req request from user either peer_id is needed.
	 *  \param [out] resp response to user
	 */
	void handleRemoveNode(Request &req, Response &resp);

	/**
	 *  \brief Retrieve inactive user list before specific UNIX time
	 *
	 *  \param [in] req request from user, datatime in UNIX timestamp.
	 *  \param [in] resp response to request
	 *  \return a pgp_id list.
	 */
	void handleGetInactiveUsers(Request &req, Response &resp);

	/// Helper which ensures proper mutex locking
	StateToken getCurrentStateToken();

	StateTokenServer* mStateTokenServer;
	RsNotify* mNotify;
	RsPeers* mRsPeers;
	RsMsgs* mRsMsgs; // required for avatar data

	std::list<RsPeerId> mOnlinePeers;
	uint32_t status;
	std::string custom_state_string;

	RsMutex mMtx;
	StateToken mStateToken; // mutex protected
	StateToken mStringStateToken; // mutex protected
	StateToken mCustomStateToken; // mutex protected

	std::map<RsPeerId, uint32_t> mUnreadMsgsCounts;
};
} // namespace resource_api
