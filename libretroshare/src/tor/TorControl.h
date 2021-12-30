/* Ricochet - https://ricochet.im/
 * Copyright (C) 2014, John Brooks <john.brooks@dereferenced.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <iostream>

#include "PendingOperation.h"
#include "bytearray.h"
#include "TorControlSocket.h"


namespace Tor
{

class HiddenService;
class TorControlSocket;
class TorControlCommand;

class TorControl : public TorControlSocketClient
{
public:
    enum Status
    {
        Error              = 0x00,
        NotConnected       = 0x01,
        Connecting         = 0x02,
        SocketConnected    = 0x03,
        Authenticating     = 0x04,
        Authenticated      = 0x05,
        HiddenServiceReady = 0x06,
        Unknown            = 0x07
    };

    enum TorStatus
    {
        TorUnknown = 0x00,
        TorOffline = 0x01,
        TorReady   = 0x02
    };

    explicit TorControl();
    virtual ~TorControl();

    /* Information */
    Status status() const;
    TorStatus torStatus() const;
    std::string torVersion() const;
    bool torVersionAsNewAs(const std::string &version) const;
    std::string errorMessage() const;

    bool hasConnectivity() const;
    std::string socksAddress() const;
    uint16_t socksPort() const;

    /* Authentication */
    void setAuthPassword(const ByteArray& password);

    /* Connection */
    bool isConnected() const { return status() >= Authenticated; }
    void connect(const std::string &address, uint16_t port);
    void authenticate();

    /* Ownership means that tor is managed by this socket, and we
     * can shut it down, own its configuration, etc. */
    bool hasOwnership() const;
    void takeOwnership();

    /* Hidden Services */
    std::list<HiddenService*> hiddenServices() const;
    void addHiddenService(HiddenService *service);

    std::map<std::string, std::string> bootstrapStatus() const;
    TorControlCommand *getConfiguration(const std::string &options);
    TorControlCommand *setConfiguration(const std::list<std::pair<std::string, std::string> > &options);
    PendingOperation *saveConfiguration();

    void set_statusChanged_callback(const std::function<void(int,int)>& f) { mStatusChanged_callback = f ;}
    void set_connected_callback(const std::function<void(void)>& f) { mConnected_callback = f ;}
    void set_disconnected_callback(const std::function<void(void)>& f) { mDisconnected_callback = f ;}

    virtual void socketError(const std::string &s) override;

    /* Instruct Tor to shutdown */
    void shutdown();
    /* Call shutdown(), and wait synchronously for the command to be written */
    void shutdownSync();

    void reconnect();

    void getTorInfo();
private:
    TorControlSocket *mSocket;
    std::string mTorAddress;
    std::string mErrorMessage;
    std::string mTorVersion;
    ByteArray mAuthPassword;
    std::string mSocksAddress;
    std::list<HiddenService*> mServices;
    uint16_t mControlPort, mSocksPort;
    TorControl::Status mStatus;
    TorControl::TorStatus mTorStatus;
    std::map<std::string,std::string> mBootstrapStatus;
    bool mHasOwnership;

    void checkHiddenService(HiddenService *service);
    void getTorInfoReply(TorControlCommand *sender);
    void setStatus(TorControl::Status n);
    void statusEvent(int code, const ByteArray &data);
    void setTorStatus(TorControl::TorStatus n);
    void updateBootstrap(const std::list<ByteArray>& data);
    void setError(const std::string& message);
    void publishServices();
    void protocolInfoReply(TorControlCommand *sender);
    void socketDisconnected();
    void authenticateReply(TorControlCommand *sender);

    std::function<void(int,int)> mStatusChanged_callback;
    std::function<void(void)> mConnected_callback;
    std::function<void(void)> mDisconnected_callback;
};

}
