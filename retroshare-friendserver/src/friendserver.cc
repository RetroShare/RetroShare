#include "util/rsdebug.h"

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
    RsDbg() << "Received a client publish item from " << item->PeerId() << ":" << *item ;

    // Respond with a list of potential friends

    // Close client connection from server side, to tell the client that nothing more is coming.

    RsDbg() << "Closing client connection." ;

    mni->closeConnection(item->PeerId());
}
void FriendServer::handleClientRemove(const RsFriendServerClientRemoveItem *item)
{
    RsDbg() << "Received a client remove item:" << *item ;
}
FriendServer::FriendServer(const std::string& base_dir)
{
    RsDbg() << "Creating friend server." ;
    mBaseDirectory = base_dir;
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
        RsDbg() << "   " << it.first << ": " << "last contact: " << now - it.second.last_connection_TS;

    RsDbg() << "===============================================";

}




