#include <functional>
#include <thread>
#include "util/rstime.h"

class RsFriendServer
{
public:
    void start() {}
    void stop() {}

    void checkServerAddress_async(const std::string& addr,uint16_t,  const std::function<void (const std::string& address,bool result_status)>& callback)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        callback(addr,true);
    }
    void setServerAddress(const std::string&,uint16_t) {}
    void setFriendsToRequest(uint32_t) {}
};

extern RsFriendServer *rsFriendServer;
