#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "util/rsdir.h"
#include "util/rsbase64.h"
#include "util/radix64.h"

#include "crypto/hashstream.h"

#include "pgp/pgpkeyutil.h"
#include "pgp/rscertificate.h"
#include "pgp/openpgpsdkhandler.h"

#include "friendserver.h"
#include "friend_server/fsitem.h"

static const rstime_t MAXIMUM_PEER_INACTIVE_DELAY    = 600;
static const rstime_t DELAY_BETWEEN_TWO_AUTOWASH     =  60;
static const rstime_t DELAY_BETWEEN_TWO_DEBUG_PRINT  =  10;
static const uint32_t MAXIMUM_PEERS_TO_REQUEST       =  10;

void FriendServer::threadTick()
{
    // Listen to the network interface, capture incoming data etc.

    RsItem *item;

    while(nullptr != (item = mni->GetItem()))
    {
        RsFriendServerItem *fsitem = dynamic_cast<RsFriendServerItem*>(item);

        if(!fsitem)
        {
            RsErr() << "Received an item of the wrong type!" ;

            continue;
        }
        std::cerr << "Received item: " << std::endl << *fsitem << std::endl;

        switch(fsitem->PacketSubType())
        {
        case RS_PKT_SUBTYPE_FS_CLIENT_PUBLISH: handleClientPublish(dynamic_cast<RsFriendServerClientPublishItem*>(fsitem));
            break;
        case RS_PKT_SUBTYPE_FS_CLIENT_REMOVE: handleClientRemove(dynamic_cast<RsFriendServerClientRemoveItem*>(fsitem));
            break;
        default: ;
        }
        delete item;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    static rstime_t last_autowash_TS = time(nullptr);
    rstime_t now = time(nullptr);

    if(last_autowash_TS + DELAY_BETWEEN_TWO_AUTOWASH < now)
    {
        last_autowash_TS = now;
        autoWash();
    }

    static rstime_t last_debugprint_TS = time(nullptr);

    if(last_debugprint_TS + DELAY_BETWEEN_TWO_DEBUG_PRINT < now)
    {
        last_debugprint_TS = now;
        debugPrint(false);
    }
}

void FriendServer::handleClientPublish(const RsFriendServerClientPublishItem *item)
{
    // We always respond with exactly one item, be it an error item or a list of friends to connect to.

    try
    {
        RsDbg() << "Received a client publish item from " << item->PeerId() << ":";
        RsDbg() << *item ;

        // First of all, read PGP key and short invites, parse them, and check that they contain the same information

        RsPeerId pid;

        if(!handleIncomingClientData(item->pgp_public_key_b64,item->short_invite,pid))
        {
            RsErr() << "Client data is dropped because of error." ;
            return ;
        }

        // All good.

        auto pi(mCurrentClientPeers.find(pid));

        // Update the list of closest peers for other peers, based on which known friends it reports, and of current peer depending
        // on friendship levels of other peers.

        updateClosestPeers(pi->first,pi->second.pgp_fingerprint,item->already_received_peers);

        RsDbg() << "Sending response item to " << item->PeerId() ;

        RsFriendServerServerResponseItem sr_item;

        std::set<RsPeerId> friends;
        sr_item.unique_identifier = pi->second.last_identifier;
        sr_item.friend_invites = computeListOfFriendInvites(pi->first,item->n_requested_friends,item->already_received_peers,friends);
        sr_item.PeerId(item->PeerId());

        RsDbg() << "  Got " << sr_item.friend_invites.size() << " closest peers not in the list." ;
        RsDbg() << "  Updating local information for destination peer." ;

        // Update friendship levels of the peer that will receive the new list

        for(auto fr:friends)
        {
            auto& p(pi->second.friendship_levels[fr]);

            RsDbg() << "  Already a friend: " << fr << ", with local status " << static_cast<int>(p) ;

            if(static_cast<int>(p) < static_cast<int>(RsFriendServer::PeerFriendshipLevel::HAS_KEY))
            {
                p = RsFriendServer::PeerFriendshipLevel::HAS_KEY;
                RsDbg() << "    --> updating status to HAS_KEY" ;
            }
        }

        // Now encrypt the item with the public PGP key of the destination. This prevents the wrong person to request for
        // someone else's data.

        RsDbg() << "  Encrypting item..." ;

        RsFriendServerEncryptedServerResponseItem *encrypted_response_item = new RsFriendServerEncryptedServerResponseItem;
        uint32_t serialized_clear_size = FsSerializer().size(&sr_item);
        RsTemporaryMemory serialized_clear_mem(serialized_clear_size);
        FsSerializer().serialise(&sr_item,serialized_clear_mem,&serialized_clear_size);

        uint32_t encrypted_mem_size = serialized_clear_size+1000;	// leave some extra space
        RsTemporaryMemory encrypted_mem(encrypted_mem_size);

        if(!mPgpHandler->encryptDataBin(PGPHandler::pgpIdFromFingerprint(pi->second.pgp_fingerprint),
                                        serialized_clear_mem,serialized_clear_size,
                                        encrypted_mem,&encrypted_mem_size))
        {
            RsErr() << "Cannot encrypt item for PGP Id/FPR " << pi->second.pgp_fingerprint << ". Something went wrong." ;
            return;
        }
        encrypted_response_item->PeerId(item->PeerId());
        encrypted_response_item->bin_len = encrypted_mem_size;
        encrypted_response_item->bin_data = malloc(encrypted_mem_size);

        memcpy(encrypted_response_item->bin_data,encrypted_mem,encrypted_mem_size);

        // Send the item.

        RsDbg() << "  Sending item..." ;
        mni->SendItem(encrypted_response_item);
    }
    catch(std::exception& e)
    {
        RsErr() << "ERROR: " << e.what() ;

        RsFriendServerStatusItem *status_item = new RsFriendServerStatusItem;
        status_item->status = RsFriendServerStatusItem::END_OF_TRANSMISSION;
        status_item->PeerId(item->PeerId());
        mni->SendItem(status_item);
        return;
    }
}

std::map<std::string,RsFriendServer::PeerFriendshipLevel> FriendServer::computeListOfFriendInvites(const RsPeerId &pid, uint32_t nb_reqs_invites,
                                              const std::map<RsPeerId,RsFriendServer::PeerFriendshipLevel>& already_known_peers,
                                              std::set<RsPeerId>& chosen_peers) const
{
    // Strategy: we want to return the same set of friends for a given PGP profile key.
    // Still, using some closest distance strategy, the n-closest peers for profile A is not the
    // same set than the n-closest peers for profile B, so some peers will not be in both sets.
    // We have multiple options:
    //
    // Option 1:
    //
    //    (1) for each profile, keep the list of n-closest peers updated (when a new peer if added/removed all lists are updated)
    //
    //   When a peer asks for k friends, read from (1), until the number of collected peers
    //   reaches the requested value. Then when a peer receives a connection request, ask the friend server if the
    //   peer has been sent your own cert.
    //
    // Option 2:
    //
    //    (1) for each profile, keep the list of n-closest peers updated (when a new peer if added/removed all lists are updated)
    //    (2) for each profile, keep the list of which peers have been sent this profile already
    //
    //   When a peer asks for k friends, read from (2) first, then (1), until the number of collected peers
    //   reaches the requested value.
    //
    // So we choose Option 2.

    std::map<std::string,RsFriendServer::PeerFriendshipLevel> res;
    chosen_peers.clear();
    auto pinfo_it(mCurrentClientPeers.find(pid));

    if(pinfo_it == mCurrentClientPeers.end())
    {
        RsErr() << "inconsistency in computeListOfFriendInvites. Something's wrong in the code." ;
        return res;
    }
    auto pinfo(pinfo_it->second);

    for(const auto& pit:pinfo.closest_peers)
    {
        if(already_known_peers.find(pit.second) == already_known_peers.end())
        {
            RsDbg() << "  peer " << pit.second << ": not in supplied list => adding it.";

            const auto p = mCurrentClientPeers.find(pit.second);

            if(p == mCurrentClientPeers.end())	// should not happen, but just an extra security.
                continue;

            auto pp = p->second.friendship_levels.find(pid);

            auto peer_friendship_level = (pp==p->second.friendship_levels.end())?(RsFriendServer::PeerFriendshipLevel::UNKNOWN):(pp->second);

            res[p->second.short_certificate] = peer_friendship_level;
            chosen_peers.insert(p->first);

            if(res.size() + already_known_peers.size() >= nb_reqs_invites)
                break;
        }
        else
        {
            auto p = already_known_peers.find(pit.second);
            RsDbg() << "  peer " << pit.second << ": already in supplied list, with status " << static_cast<int>(p->second) << ". Not adding it.";
        }
    }

    return res;
}

bool FriendServer::handleIncomingClientData(const std::string& pgp_public_key_b64,const std::string& short_invite_b64,RsPeerId& pid)
{
    // 1 - Check that the incoming data is sound.

    try
    {
        RsDbg() << "  Checking item data...";

        std::string error_string;
        std::vector<uint8_t> key_binary_data ;

        if(RsBase64::decode(pgp_public_key_b64,key_binary_data))
            throw std::runtime_error("  Cannot decode client pgp public key: \"" + pgp_public_key_b64 + "\". Wrong format??");

        RsDbg() << "    Parsing public key:" ;

        PGPKeyInfo received_key_info;

        if(!PGPKeyManagement::parsePGPPublicKey(key_binary_data.data(),key_binary_data.size(),received_key_info))
            throw std::runtime_error("Cannot parse received pgp public key.") ;

        RsDbg() << "      Issuer     : \"" << received_key_info.user_id << "\"" ;
        RsDbg() << "      Fingerprint: " << RsPgpFingerprint::fromBufferUnsafe(received_key_info.fingerprint) ;

        RsDbg() << "    Parsing short invite:" ;

        RsPeerDetails shortInviteDetails;
        uint32_t errorCode = 0;

        if(short_invite_b64.empty() || !RsCertificate::decodeRadix64ShortInvite(short_invite_b64, shortInviteDetails,errorCode ))
            throw std::runtime_error("Could not parse short certificate. Error = " + RsUtil::NumberToString(errorCode));

        RsDbg() << "      Fingerprint: " << shortInviteDetails.fpr ;
        RsDbg() << "      Peer ID:     " << shortInviteDetails.id ;

        if(shortInviteDetails.fpr != RsPgpFingerprint::fromBufferUnsafe(received_key_info.fingerprint))
            throw std::runtime_error("Fingerpring from short invite and public key are different! Very unexpected! Message will be ignored.");

        // 3 - if the key is not already here, add it to keyring.

        {
            RsPgpFingerprint fpr_test;
            if(mPgpHandler->isPgpPubKeyAvailable(RsPgpId::fromBufferUnsafe(received_key_info.fingerprint+12)))
                RsDbg() << "    PGP Key is already into keyring.";
            else
            {
                RsPgpId pgp_id;
                if(!mPgpHandler->LoadCertificateFromBinaryData(key_binary_data.data(),key_binary_data.size(), pgp_id, error_string))
                    throw std::runtime_error("Cannot load client's pgp public key into keyring: " + error_string) ;

                RsDbg() << "    Public key added to keyring.";
                RsDbg() << "    Sync-ing the PGP keyring on disk";

                mPgpHandler->syncDatabase();
            }
        }
        // Store/update the peer info

        auto& pi(mCurrentClientPeers[shortInviteDetails.id]);

        pi.short_certificate = short_invite_b64;
        pi.last_connection_TS = time(nullptr);
        pi.pgp_fingerprint = shortInviteDetails.fpr;

        while(pi.last_identifier == 0)					// reuse the same identifier (so it's not really a nonce, but it's kept secret whatsoever).
            pi.last_identifier = RsRandom::random_u64();

        pid = shortInviteDetails.id;
        return true;
    }
    catch (std::exception& e)
    {
        RsErr() << "Exception while adding client data: " << e.what() ;
        return false;
    }
}

void FriendServer::handleClientRemove(const RsFriendServerClientRemoveItem *item)
{
    RsDbg() << "Received a client remove item:" << *item ;

    auto it = mCurrentClientPeers.find(item->peer_id);

    if(it == mCurrentClientPeers.end())
    {
        RsErr() << "  ERROR: Client " << item->peer_id << " is not known to the server." ;
        return;
    }

    if(it->second.last_identifier != item->unique_identifier)
    {
        RsErr() << "  ERROR: Client supplied a nonce " << std::hex << item->unique_identifier << std::dec << " that is not correct (expected "
                << std::hex << it->second.last_identifier << std::dec << ")";
        return;
    }

    RsDbg() << "  Nonce is correct: " << std::hex << item->unique_identifier << std::dec << ". Removing peer " << item->peer_id ;

    removePeer(item->peer_id);
}

void FriendServer::removePeer(const RsPeerId& peer_id)
{
    auto it = mCurrentClientPeers.find(peer_id);

    if(it != mCurrentClientPeers.end())
        mCurrentClientPeers.erase(it);

    for(auto& it:mCurrentClientPeers)
    {
        // Also remove that peer from all n-closest lists

        for(auto pit(it.second.closest_peers.begin());pit!=it.second.closest_peers.end();)
            if(pit->second == peer_id)
            {
                RsDbg() << "  Removing from n-closest peers of peer " << it.first ;

                auto tmp(pit);
                ++tmp;
                it.second.closest_peers.erase(pit);
                pit=tmp;
            }
            else
                ++pit;

        // Also remove that peer from friendship levels of that particular peer.

        auto fit = it.second.friendship_levels.find(peer_id);

        if(fit != it.second.friendship_levels.end())
        {
            RsDbg() << "  Removing from have_added_as_friend peers of peer " << it.first ;
            it.second.friendship_levels.erase(fit);
        }
    }
}

PeerInfo::PeerDistance FriendServer::computePeerDistance(const RsPgpFingerprint& p1,const RsPgpFingerprint& p2)
{
    auto res = (p1 ^ p2)^mRandomPeerBias;
    auto res2 = RsDirUtil::sha1sum(res.toByteArray(),res.SIZE_IN_BYTES);	// sha1sum prevents reverse finding the random bias

    std::cerr << "Computing peer distance: p1=" << p1 << " p2=" << p2 << " p1^p2=" << (p1^p2) << " distance=" << res2 << std::endl;

    return res2;
}
FriendServer::FriendServer(const std::string& base_dir,const std::string& listening_address,uint16_t listening_port)
    : mListeningAddress(listening_address),mListeningPort(listening_port)
{
    RsDbg() << "Creating friend server." ;
    mBaseDirectory = base_dir;

    // Create a PGP Handler

    std::string pgp_public_keyring_path  = RsDirUtil::makePath(base_dir,"pgp_public_keyring") ;
    std::string pgp_lock_path            = RsDirUtil::makePath(base_dir,"pgp_lock") ;

    std::string pgp_private_keyring_path = RsDirUtil::makePath(base_dir,"pgp_private_keyring") ;	// not used.
    std::string pgp_trustdb_path         = RsDirUtil::makePath(base_dir,"pgp_trustdb") ;	        // not used.

    mPgpHandler = new OpenPGPSDKHandler(pgp_public_keyring_path,pgp_private_keyring_path,pgp_trustdb_path,pgp_lock_path);

    // Random bias. Should be cryptographically safe.

    mRandomPeerBias = RsPgpFingerprint::random();
}

void FriendServer::run()
{
    // 1 - create network interface.

    mni = new FsNetworkInterface(mListeningAddress,mListeningPort);
    mni->start();

    while(!shouldStop()) { threadTick() ; }
}

void FriendServer::autoWash()
{
    rstime_t now = time(nullptr);
    std::list<RsPeerId> to_remove;

    for(std::map<RsPeerId,PeerInfo>::iterator it(mCurrentClientPeers.begin());it!=mCurrentClientPeers.end();++it)
        if(it->second.last_connection_TS + MAXIMUM_PEER_INACTIVE_DELAY < now)
        {
            RsDbg() << "Removing client peer " << it->first << " because it's inactive for more than " << MAXIMUM_PEER_INACTIVE_DELAY << " seconds." ;
            to_remove.push_back(it->first);
        }

    for(auto peer_id:to_remove)
        removePeer(peer_id);
}

void FriendServer::updateClosestPeers(const RsPeerId& pid,const RsPgpFingerprint& fpr,const std::map<RsPeerId,RsFriendServer::PeerFriendshipLevel>& friended_peers)
{
    auto find_multi = [](PeerInfo::PeerDistance dist,std::map< std::pair<RsFriendServer::PeerFriendshipLevel,PeerInfo::PeerDistance>,RsPeerId >& mp)
            -> std::map< std::pair<RsFriendServer::PeerFriendshipLevel,PeerInfo::PeerDistance>,RsPeerId >::iterator
    {
        auto it = mp.find(std::make_pair(RsFriendServer::PeerFriendshipLevel::UNKNOWN,dist)) ;

        if(it == mp.end()) it = mp.find(std::make_pair(RsFriendServer::PeerFriendshipLevel::NO_KEY          ,dist));
        if(it == mp.end()) it = mp.find(std::make_pair(RsFriendServer::PeerFriendshipLevel::HAS_KEY         ,dist));
        if(it == mp.end()) it = mp.find(std::make_pair(RsFriendServer::PeerFriendshipLevel::HAS_ACCEPTED_KEY,dist));

        return it;
    };
    auto remove_from_map = [find_multi](PeerInfo::PeerDistance dist,
                                std::map< std::pair<RsFriendServer::PeerFriendshipLevel,
                                PeerInfo::PeerDistance>,RsPeerId>& mp) -> bool
    {
        auto mpit = find_multi(dist,mp);

        if(mpit != mp.end())
        {
            mp.erase(mpit);
            return true;
        }
        else
            return false;
    };

    auto& pit(mCurrentClientPeers[pid]);

    for(auto& it:mCurrentClientPeers)
        if(it.first != pid)
        {
            // 1 - for all existing peers, update the level at which the given peer has added the peer as friend.

            auto peer_iterator = friended_peers.find(it.first);
            auto peer_friendship_level = (peer_iterator==friended_peers.end())? (RsFriendServer::PeerFriendshipLevel::UNKNOWN):(peer_iterator->second);

            PeerInfo::PeerDistance d = computePeerDistance(fpr,it.second.pgp_fingerprint);

            // Remove the peer from the map. This is costly. I need to find something better. If the peer is already
            // in the list, it has a map key with the same distance.

            remove_from_map(d,it.second.closest_peers);

            it.second.closest_peers[std::make_pair(peer_friendship_level,d)] = pid;

            while(it.second.closest_peers.size() > MAXIMUM_PEERS_TO_REQUEST)
                it.second.closest_peers.erase(std::prev(it.second.closest_peers.end()));

            // 2 - for the current peer, update the list of closest peers

            auto pit2 = it.second.friendship_levels.find(pid);
            peer_friendship_level = (pit2==it.second.friendship_levels.end())? (RsFriendServer::PeerFriendshipLevel::UNKNOWN):(pit2->second);

            remove_from_map(d,pit.closest_peers);

            pit.closest_peers[std::make_pair(peer_friendship_level,d)] = it.first;

            while(pit.closest_peers.size() > MAXIMUM_PEERS_TO_REQUEST)
                pit.closest_peers.erase(std::prev(pit.closest_peers.end()));
        }

    // Also update the friendship levels for the current peer, of all friends from the list.

    for(auto it:friended_peers)
        pit.friendship_levels[it.first] = it.second;
}

Sha1CheckSum FriendServer::computeDataHash()
{
    librs::crypto::HashStream s(librs::crypto::HashStream::SHA1);

    for(auto p(mCurrentClientPeers.begin());p!=mCurrentClientPeers.end();++p)
    {
        s << p->first;

        const auto& inf(p->second);

        s << inf.pgp_fingerprint;
        s << inf.short_certificate;
        s << (uint64_t)inf.last_connection_TS;
        s << inf.last_identifier;

        for(auto d(inf.closest_peers.begin());d!=inf.closest_peers.end();++d)
        {
            s << static_cast<uint32_t>(d->first.first) ;
            s << d->first.second ;
            s << d->second;
        }
        for(auto d:inf.friendship_levels)
        {
            s << d.first ;
            s << static_cast<uint32_t>(d.second);
        }
    }
    return s.hash();
}
void FriendServer::debugPrint(bool force)
{
    auto h = computeDataHash();

    if((h != mCurrentDataHash) || force)
    {
        RsDbg() << "========== FriendServer statistics ============";
        RsDbg() << "  Base directory: "<< mBaseDirectory;
        RsDbg() << "  Random peer bias: "<< mRandomPeerBias;
        RsDbg() << "  Current hash: "<< h;
        RsDbg() << "  Network interface: ";
        RsDbg() << "  Max peers in n-closest list: " << MAXIMUM_PEERS_TO_REQUEST;
        RsDbg() << "  Current active peers: " << mCurrentClientPeers.size() ;

        rstime_t now = time(nullptr);

        for(const auto& it:mCurrentClientPeers)
        {
            RsDbg() << "   " << it.first << ": identifier=" << std::hex << it.second.last_identifier << std::dec << " fpr: " << it.second.pgp_fingerprint << ", last contact: " << now - it.second.last_connection_TS << " secs ago.";
            RsDbg() << "   Closest peers:" ;

            for(auto pit:it.second.closest_peers)
                RsDbg() << "      " << pit.second << " distance=" << pit.first.second << " Peer reciprocal status:" << static_cast<int>(pit.first.first);
        }

        RsDbg() << "===============================================";

        mCurrentDataHash = h;
    }

}




