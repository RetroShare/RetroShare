#include <functional>
#include <thread>
#include "util/rstime.h"

// The Friend Server component of Retroshare automatically adds/removes some friends so that the
//
// The current strategy is:
//
//   - if total nb of friends < S
//         request new friends to the FS
//   - if total nb of friends >= S
//         do not request anymore (and unpublish the key), but keep the friends already here
//
// Possible states:
//   - not started
//   - maintain friend list
//   - actively request friends
//
// The friend server internally keeps track of which friends have been added using the friend server.
// It's important to keep the ones that are already connected because they may count on us.
// Friends supplied by the FS who never connected for a few days should be removed automatically.

class RsFriendServer
{
public:
    virtual void startServer() =0;
    virtual void stopServer() =0;

    virtual void checkServerAddress_async(const std::string& addr,uint16_t,  const std::function<void (const std::string& address,bool result_status)>& callback) =0;
    virtual void setServerAddress(const std::string&,uint16_t) =0;
    virtual void setFriendsToRequest(uint32_t) =0;
};

extern RsFriendServer *rsFriendServer;
