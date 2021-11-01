#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "util/rsdir.h"
#include "util/rsbase64.h"
#include "util/radix64.h"

#include "pgp/rscertificate.h"

#include "friendserver.h"
#include "friend_server/fsitem.h"

static const rstime_t MAXIMUM_PEER_INACTIVE_DELAY    = 600;
static const rstime_t DELAY_BETWEEN_TWO_AUTOWASH     =  60;
static const rstime_t DELAY_BETWEEN_TWO_DEBUG_PRINT  =  10;

void FriendServer::threadTick()
{
    // Listen to the network interface, capture incoming data etc.

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

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
        case RS_PKT_SUBTYPE_FS_CLIENT_REMOVE: handleClientRemove(dynamic_cast<RsFriendServerClientRemoveItem*>(fsitem));
            break;
        case RS_PKT_SUBTYPE_FS_CLIENT_PUBLISH: handleClientPublish(dynamic_cast<RsFriendServerClientPublishItem*>(fsitem));
            break;
        default: ;
        }
        delete item;
    }

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
    try
    {
        RsDbg() << "Received a client publish item from " << item->PeerId() << ":";
        RsDbg() << *item ;

        // First of all, read PGP key and short invites, parse them, and check that they contain the same information

        FriendServer::handleIncomingClientData(item->pgp_public_key_b64,item->short_invite);

        // Respond with a list of potential friends


    }
    catch(std::exception& e)
    {
        RsErr() << "ERROR: " << e.what() ;
    }

    // Close client connection from server side, to tell the client that nothing more is coming.

    RsDbg() << "Closing client connection." ;

    mni->closeConnection(item->PeerId());
}

bool FriendServer::handleIncomingClientData(const std::string& pgp_public_key_b64,const std::string& short_invite_b64)
{
        RsDbg() << "  Checking item data...";

        std::string error_string;
        RsPgpId pgp_id ;
        std::vector<uint8_t> key_binary_data ;

        key_binary_data = Radix64::decode(pgp_public_key_b64);

        if(key_binary_data.empty())
            throw std::runtime_error("  Cannot decode client pgp public key: \"" + pgp_public_key_b64 + "\". Wrong format??");

// Apparently RsBase64 doesn't work correctly.
//
//        if(!RsBase64::decode(item->pgp_public_key_b64,key_binary_data))
//            throw std::runtime_error("  Cannot decode client pgp public key: \"" + item->pgp_public_key_b64 + "\". Wrong format??");

        RsDbg() << "    Public key radix is fine." ;

        if(!mPgpHandler->LoadCertificateFromBinaryData(key_binary_data.data(),key_binary_data.size(), pgp_id, error_string))
            throw std::runtime_error("Cannot load client's pgp public key into keyring: " + error_string) ;

        RsDbg() << "    Public key added to keyring.";

        RsPeerDetails shortInviteDetails;
        uint32_t errorCode = 0;

        if(short_invite_b64.empty() || !RsCertificate::decodeRadix64ShortInvite(short_invite_b64, shortInviteDetails,errorCode ))
            throw std::runtime_error("Could not parse short certificate. Error = " + RsUtil::NumberToString(errorCode));

        RsDbg() << "    Short invite is fine. PGP fingerprint: " << shortInviteDetails.fpr ;

        RsPgpFingerprint fpr_test;
        if(!mPgpHandler->getKeyFingerprint(pgp_id,fpr_test))
            throw std::runtime_error("Cannot get fingerprint from keyring for client public key. Something's really wrong.") ;

        if(fpr_test != shortInviteDetails.fpr)
            throw std::runtime_error("Cannot get fingerprint from keyring for client public key. Something's really wrong.") ;

        RsDbg() << "    Short invite PGP fingerprint matches the public key fingerprint.";
        RsDbg() << "    Sync-ing the PGP keyring on disk";

        mPgpHandler->syncDatabase();

        // Check the item's data signature. Is that needed? Not sure, since the data is sent PGP-encrypted, so only the owner
        // of the secret PGP key can actually use it.
#warning TODO

        // All good.

        // Store/update the peer info

        auto& pi(mCurrentClientPeers[shortInviteDetails.id]);

        pi.short_certificate = short_invite_b64;
        pi.last_connection_TS = time(nullptr);
        pi.last_nonce = RsRandom::random_u64();

        return true;
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

    mCurrentClientPeers.erase(it);
}

FriendServer::FriendServer(const std::string& base_dir)
{
    RsDbg() << "Creating friend server." ;
    mBaseDirectory = base_dir;

    // Create a PGP Handler

    std::string pgp_public_keyring_path  = RsDirUtil::makePath(base_dir,"pgp_public_keyring") ;
    std::string pgp_lock_path            = RsDirUtil::makePath(base_dir,"pgp_lock") ;

    std::string pgp_private_keyring_path = RsDirUtil::makePath(base_dir,"pgp_private_keyring") ;	// not used.
    std::string pgp_trustdb_path         = RsDirUtil::makePath(base_dir,"pgp_trustdb") ;	        // not used.

    mPgpHandler = new PGPHandler(pgp_public_keyring_path,pgp_private_keyring_path,pgp_trustdb_path,pgp_lock_path);
}

void FriendServer::run()
{
    // 1 - create network interface.

    mni = new FsNetworkInterface;
    mni->start();

    while(!shouldStop()) { threadTick() ; }
}

void FriendServer::autoWash()
{
    rstime_t now = time(nullptr);

    for(std::map<RsPeerId,PeerInfo>::iterator it(mCurrentClientPeers.begin());it!=mCurrentClientPeers.end();)
    {
        if(it->second.last_connection_TS + MAXIMUM_PEER_INACTIVE_DELAY < now)
        {
            RsDbg() << "Removing client peer " << it->first << " because it's inactive for more than " << MAXIMUM_PEER_INACTIVE_DELAY << " seconds." ;
            auto tmp = it;
            ++tmp;
            mCurrentClientPeers.erase(it);
            it = tmp;
        }
    }
}

void FriendServer::debugPrint()
{
    RsDbg() << "========== FriendServer statistics ============";
    RsDbg() << "  Base directory: "<< mBaseDirectory;
    RsDbg() << "  Network interface: ";
    RsDbg() << "  Current peers: " << mCurrentClientPeers.size() ;

    rstime_t now = time(nullptr);

    for(auto& it:mCurrentClientPeers)
        RsDbg() << "   " << it.first << ": nonce=" << std::hex << it.second.last_nonce << std::dec << ", last contact: " << now - it.second.last_connection_TS << " secs ago.";

    RsDbg() << "===============================================";

}




