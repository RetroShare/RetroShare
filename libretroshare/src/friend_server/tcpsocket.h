#include <string>
#include "util/rsthreads.h"
#include "friend_server/fsbio.h"

class TcpSocket: public FsBioInterface
{
public:
    TcpSocket(const std::string& tcp_address,uint16_t tcp_port);

    enum State: uint8_t {
        UNKNOWN      = 0x00,
        DISCONNECTED = 0x01,
        CONNECTED    = 0x02
    };

    // Return 1 when OK, 0 otherwise.
    int connect();

    // Returns 1 when OK, 0 otherwise.
    int close();

    State connectionState() const { return mState; }
    const std::string& connectAddress() const { return mConnectAddress ; }
    uint16_t connectPort() const { return mConnectPort ; }

private:
    State       mState;
    std::string mConnectAddress;
    uint16_t    mConnectPort;
    int         mSocket;
};

class ThreadedTcpSocket: public TcpSocket, public RsThread
{
public:
    ThreadedTcpSocket(const std::string& tcp_address,uint16_t tcp_port);
    virtual ~ThreadedTcpSocket();

    virtual void run() override;
};

