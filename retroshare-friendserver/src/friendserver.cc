#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "util/rsdir.h"
#include "util/rsbase64.h"
#include "util/radix64.h"

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
        debugPrint();
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

        std::map<RsPeerId,PeerInfo>::iterator pi = handleIncomingClientData(item->pgp_public_key_b64,item->short_invite);

        // No need to test for it==mCurrentClients.end() because it will be directly caught by the exception handling below even before.
        // Respond with a list of potential friends

        RsDbg() << "Sending response item to " << item->PeerId() ;

        RsFriendServerServerResponseItem *sr_item = new RsFriendServerServerResponseItem;

        std::map<RsPeerId,RsPgpFingerprint> friends;
        sr_item->nonce = pi->second.last_nonce;
        sr_item->friend_invites = computeListOfFriendInvites(item->n_requested_friends,pi->first,friends);
        sr_item->PeerId(item->PeerId());

        // Update the have_added_as_friend for the list of each peer. We do that before sending because sending destroys
        // the item.

        for(const auto& pid:friends)
        {
            auto& p(mCurrentClientPeers[pid.first]);
            p.have_added_this_peer[computePeerDistance(p.pgp_fingerprint, pi->second.pgp_fingerprint)] = pi->first;
        }

        // Now encrypt the item with the public PGP key of the destination. This prevents the wrong person to request for
        // someone else's data.
#warning TODO

        // Send the item.
        mni->SendItem(sr_item);

        // Update the list of closest peers for all peers currently in the database.

        updateClosestPeers(pi->first,pi->second.pgp_fingerprint);
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

std::map<std::string, bool> FriendServer::computeListOfFriendInvites(uint32_t nb_reqs_invites, const RsPeerId &pid, std::map<RsPeerId,RsPgpFingerprint>& friends)
{
    // Strategy: we want to return the same set of friends for a given PGP profile key.
    // Still, using some closest distance strategy, the n-closest peers for profile A is not the
    // same set than the n-closest peers for profile B. We have multiple options:
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

    std::map<std::string,bool> res;

    auto add_from = [&res,&friends,nb_reqs_invites,this](bool added,const std::map<PeerInfo::PeerDistance,RsPeerId>& lst) -> bool
    {
        for(const auto& pid:lst)
        {
            const auto p = mCurrentClientPeers.find(pid.second);
            res.insert(std::make_pair(p->second.short_certificate,added));
            friends.insert(std::make_pair(p->first,p->second.pgp_fingerprint));

            if(res.size() >= nb_reqs_invites)
                return true;
        }
        return false;
    };

    const auto& pinfo(mCurrentClientPeers[pid]);

    // First add from peers who already added the current peer as friend, and leave if we already have enough

    if(add_from(true,pinfo.have_added_this_peer))
        return res;

    add_from(false,pinfo.closest_peers);

    return res;
}

std::map<RsPeerId,PeerInfo>::iterator FriendServer::handleIncomingClientData(const std::string& pgp_public_key_b64,const std::string& short_invite_b64)
{
    // 1 - Check that the incoming data is sound.

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

        // All good.

        // Store/update the peer info

        auto& pi(mCurrentClientPeers[shortInviteDetails.id]);

        pi.short_certificate = short_invite_b64;
        pi.last_connection_TS = time(nullptr);
        pi.pgp_fingerprint = shortInviteDetails.fpr;

        while(pi.last_nonce == 0)					// reuse the same identifier (so it's not really a nonce, but it's kept secret whatsoever).
            pi.last_nonce = RsRandom::random_u64();

        return mCurrentClientPeers.find(shortInviteDetails.id);
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

    if(it->second.last_nonce != item->nonce)
    {
        RsErr() << "  ERROR: Client supplied a nonce " << std::hex << item->nonce << std::dec << " that is not correct (expected "
                << std::hex << it->second.last_nonce << std::dec << ")";
        return;
    }

    RsDbg() << "  Nonce is correct: " << std::hex << item->nonce << std::dec << ". Removing peer " << item->peer_id ;

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
                RsDbg() << "  Removing from n-closest peers of peer " << pit->first ;

                auto tmp(pit);
                ++tmp;
                it.second.closest_peers.erase(pit);
                pit=tmp;
            }
            else
                ++pit;

        // Also remove that peer from peers that have accepted each peer

        for(auto fit(it.second.have_added_this_peer.begin());fit!=it.second.have_added_this_peer.end();)
            if(fit->second == peer_id)
            {
                RsDbg() << "  Removing from have_added_as_friend peers of peer " << fit->first ;

                auto tmp(fit);
                ++tmp;
                it.second.closest_peers.erase(fit);
                fit=tmp;
            }
            else
                ++fit;
    }
}

PeerInfo::PeerDistance FriendServer::computePeerDistance(const RsPgpFingerprint& p1,const RsPgpFingerprint& p2)
{
    std::cerr << "Computing peer distance: p1=" << p1 << " p2=" << p2 << " p1^p2=" << (p1^p2) << " distance=" << ((p1^p2)^mRandomPeerBias) << std::endl;
    return (p1 ^ p2)^mRandomPeerBias;
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
    RsDbg() << "autoWash..." ;

    std::list<RsPeerId> to_remove;

    for(std::map<RsPeerId,PeerInfo>::iterator it(mCurrentClientPeers.begin());it!=mCurrentClientPeers.end();++it)
        if(it->second.last_connection_TS + MAXIMUM_PEER_INACTIVE_DELAY < now)
        {
            RsDbg() << "Removing client peer " << it->first << " because it's inactive for more than " << MAXIMUM_PEER_INACTIVE_DELAY << " seconds." ;
            to_remove.push_back(it->first);
        }

    for(auto peer_id:to_remove)
        removePeer(peer_id);

    RsDbg() << "done." ;
}

void FriendServer::updateClosestPeers(const RsPeerId& pid,const RsPgpFingerprint& fpr)
{
    for(auto& it:mCurrentClientPeers)
        if(it.first != pid)
        {
            PeerInfo::PeerDistance d = computePeerDistance(fpr,it.second.pgp_fingerprint);

            it.second.closest_peers.insert(std::make_pair(d,pid));

            if(it.second.closest_peers.size() > MAXIMUM_PEERS_TO_REQUEST)
                it.second.closest_peers.erase(std::prev(it.second.closest_peers.end()));
        }
}

void FriendServer::debugPrint()
{
    RsDbg() << "========== FriendServer statistics ============";
    RsDbg() << "  Base directory: "<< mBaseDirectory;
    RsDbg() << "  Random peer bias: "<< mRandomPeerBias;
    RsDbg() << "  Network interface: ";
    RsDbg() << "  Max peers in n-closest list: " << MAXIMUM_PEERS_TO_REQUEST;
    RsDbg() << "  Current active peers: " << mCurrentClientPeers.size() ;

    rstime_t now = time(nullptr);

    for(const auto& it:mCurrentClientPeers)
    {
        RsDbg() << "   " << it.first << ": nonce=" << std::hex << it.second.last_nonce << std::dec << " fpr: " << it.second.pgp_fingerprint << ", last contact: " << now - it.second.last_connection_TS << " secs ago.";
        RsDbg() << "   Closest peers:" ;

        for(const auto& pit:it.second.closest_peers)
            RsDbg() << "      " << pit.second << " distance=" << pit.first ;

        RsDbg() << "   Have added this peer:" ;

        for(const auto& pit:it.second.have_added_this_peer)
            RsDbg() << "      " << pit.second << " distance=" << pit.first ;
    }

    RsDbg() << "===============================================";

}




