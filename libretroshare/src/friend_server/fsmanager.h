#include <map>

#include "util/rsthreads.h"
#include "retroshare/rsfriendserver.h"
#include "retroshare/rspeers.h"

struct FriendServerPeerInfo
{
    enum FriendServerPeerStatus: uint8_t
    {
    UNKNOWN           = 0x00,
    LOCALLY_ACCEPTED  = 0x01,
    HAS_ACCEPTED_ME   = 0x02,
    ALREADY_CONNECTED = 0x03
    };

    uint32_t status ;
};

class FriendServerManager: public RsFriendServer, public RsTickingThread
{
public:
    virtual void startServer() override ;
    virtual void stopServer() override ;

    virtual void checkServerAddress_async(const std::string& addr,uint16_t,  const std::function<void (const std::string& address,bool result_status)>& callback) override ;
    virtual void setServerAddress(const std::string&,uint16_t) override ;
    virtual void setFriendsToRequest(uint32_t) override ;

protected:
    virtual void threadTick() override;

private:
    uint32_t mFriendsToRequest;

    // encode the current list of friends obtained through the friendserver and their status

    std::map<RsPeerId, FriendServerPeerInfo> mPeers;
    std::string mServerAddress ;
    uint16_t mServerPort;
};
