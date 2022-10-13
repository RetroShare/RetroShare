/*
 * RetroShare Friend Server
 * Copyright (C) 2021-2021  retroshare team <retroshare.project@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "util/rsthreads.h"
#include "pqi/pqistreamer.h"
#include "pgp/pgphandler.h"
#include "retroshare/rsfriendserver.h"

#include "network.h"

class RsFriendServerClientRemoveItem;
class RsFriendServerClientPublishItem;

// Storage for peer-related information as known by the friend server.
// Peers send to the friend server the list of peers they already have, with their own friendship level with that peer.
// The FS needs to send back a list of peers, with the friendship level to the current peer.
// In the list of closest peers, the reverse friendship levels are stored: for a peer A the reverse friendship level to peer B is whether B has
// added A as friend or not.
// In the list of friends for a peer, the forward FL is stored. The forward FL of a peer A to a peer B is whether A has added B as friend or not.

struct PeerInfo
{
    typedef Sha1CheckSum PeerDistance;

    RsPgpFingerprint pgp_fingerprint;
    std::string short_certificate;
    rstime_t last_connection_TS;
    uint64_t last_identifier;

    // The following map contains the list of closest peers. The sorting is based
    // on a combination of the peer XOR distance and the friendship level, so that
    // peers which already have added a peer are considered first as potential receivers of his key.
    // The friendship level here is a reverse FL, e.g. whether each closest peer has added the current peer as friend.

    std::map<std::pair<RsFriendServer::PeerFriendshipLevel,PeerDistance>,RsPeerId > closest_peers;	// limited in size.

    // Which peers have received the key for that particular peer, along with the direct friendship level: whether current peer has added each peer.

    std::map<RsPeerId,RsFriendServer::PeerFriendshipLevel> friendship_levels;	// unlimited in size, but no distance sorting.
};

class FriendServer : public RsTickingThread
{
public:
    FriendServer(const std::string& base_directory,const std::string& listening_address,uint16_t listening_port);

private:
    // overloads RsTickingThread

    virtual void threadTick() override;
    virtual void run() override;

    // Own algorithmics

    void handleClientRemove(const RsFriendServerClientRemoveItem *item);
    void handleClientPublish(const RsFriendServerClientPublishItem *item);

    // Updates for each peer in the database, the list of closest peers w.r.t. some arbitrary distance.
    void updateClosestPeers(const RsPeerId& pid, const RsPgpFingerprint& fpr, const std::map<RsPeerId, RsFriendServer::PeerFriendshipLevel> &friended_peers);

    // removes a single peer from all lists.
    void removePeer(const RsPeerId& peer_id);

    // Adds the incoming peer data to the list of current clients and returns the
    bool handleIncomingClientData(const std::string& pgp_public_key_b64, const std::string& short_invite_b64, RsPeerId &pid);

    // Computes the appropriate list of short invites to send to a given peer.
    std::map<std::string,RsFriendServer::PeerFriendshipLevel> computeListOfFriendInvites(const RsPeerId &pid, uint32_t nb_reqs_invites,
                                                                                         const std::map<RsPeerId,RsFriendServer::PeerFriendshipLevel>& already_known_peers,
                                                                                         std::set<RsPeerId>& chosen_peers) const;

    // Compute the distance between peers using the random bias (It's not really a distance though. I'm not sure about the triangular inequality).
    PeerInfo::PeerDistance computePeerDistance(const RsPgpFingerprint &p1, const RsPgpFingerprint &p2);

    void autoWash();
    void debugPrint(bool force);
    Sha1CheckSum computeDataHash();

    // Local members

    FsNetworkInterface *mni;
    PGPHandler *mPgpHandler;

    std::string mBaseDirectory;
    RsPgpFingerprint mRandomPeerBias;

    std::map<RsPeerId, PeerInfo> mCurrentClientPeers;
    std::string mListeningAddress;
    uint16_t mListeningPort;

    Sha1CheckSum mCurrentDataHash;
};
