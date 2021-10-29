#include "fsmanager.h"

RsFriendServer *rsFriendServer = nullptr;

void FriendServerManager::startServer()
{
    if(!isRunning())
    {
        std::cerr << "Starting Friend Server Manager." << std::endl;
        RsTickingThread::start() ;
    }
}
void FriendServerManager::stopServer()
{
    if(isRunning() && !shouldStop())
    {
        std::cerr << "Stopping Friend Server Manager." << std::endl;
        RsTickingThread::askForStop() ;
    }
}
void FriendServerManager::checkServerAddress_async(const std::string& addr,uint16_t,  const std::function<void (const std::string& address,bool result_status)>& callback)
{
#warning TODO
    std::this_thread::sleep_for(std::chrono::seconds(1));

    callback(addr,true);
}

void FriendServerManager::setServerAddress(const std::string& addr,uint16_t port)
{
    mServerAddress = addr;
    mServerPort = port;
}

void FriendServerManager::setFriendsToRequest(uint32_t n)
{
    mFriendsToRequest = n;
}

void FriendServerManager::threadTick()
{
    std::cerr << "Ticking FriendServerManager..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}
