#include "util/rsdebug.h"

#include "friendserver.h"
#include "fsitem.h"

void FriendServer::threadTick()
{
    // Listen to the network interface, capture incoming data etc.

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

FriendServer::FriendServer(const std::string& base_dir)
{
    RsDbg() << "Creating friend server." << std::endl;
    mBaseDirectory = base_dir;
}

void FriendServer::run()
{
    // 1 - create network interface.

    mni = new FsNetworkInterface;

    RsSerialiser *rss = new RsSerialiser ;
    rss->addSerialType(new FsSerializer) ;

    pqi = new pqistreamer(rss, RsPeerId(), mni,BIN_FLAGS_READABLE);

    while(!shouldStop()) { threadTick() ; }
}

