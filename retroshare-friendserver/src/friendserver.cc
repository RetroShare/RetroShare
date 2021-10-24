#include "util/rsdebug.h"

#include "friendserver.h"
#include "friend_server/fsitem.h"

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
}

void FriendServer::handleClientPublish(const RsFriendServerClientPublishItem *item)
{
    RsDbg() << "Received a client publish item:" << *item ;
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

