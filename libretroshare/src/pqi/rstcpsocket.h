#include <string>
#include "util/rsthreads.h"
#include "pqi/pqifdbin.h"

class RsTcpSocket: public RsFdBinInterface
{
public:
    RsTcpSocket(const std::string& tcp_address,uint16_t tcp_port);
    RsTcpSocket();
    virtual ~RsTcpSocket()=default;

    enum State: uint8_t {
        UNKNOWN      = 0x00,
        DISCONNECTED = 0x01,
        CONNECTED    = 0x02
    };

    // Return true when OK, false otherwise.
    bool connect();

    bool connect(const std::string& tcp_address,uint16_t tcp_port);

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

class RsThreadedTcpSocket: public RsTcpSocket, public RsThread
{
public:
    RsThreadedTcpSocket(const std::string& tcp_address,uint16_t tcp_port);
    RsThreadedTcpSocket();
    virtual ~RsThreadedTcpSocket();

    virtual void run() override;
};

