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

#ifndef TORCONTROL_H
#define TORCONTROL_H

#include <iostream>

#include "PendingOperation.h"
#include "bytearray.h"
#include "TorControlSocket.h"

class QNetworkProxy;

namespace Tor
{

class HiddenService;
class TorControlSocket;
class TorControlCommand;

class TorControl : public TorControlSocketClient
{
//    Q_ENUMS(Status TorStatus)
//
//    // Status of the control connection
//    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
//    // Status of Tor (and whether it believes it can connect)
//    Q_PROPERTY(TorStatus torStatus READ torStatus NOTIFY torStatusChanged)
//    // Whether it's possible to make a SOCKS connection and connect
//    Q_PROPERTY(bool hasConnectivity READ hasConnectivity NOTIFY connectivityChanged)
//    Q_PROPERTY(QString torVersion READ torVersion NOTIFY connected)
//    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY statusChanged)
//    Q_PROPERTY(QVariantMap bootstrapStatus READ bootstrapStatus NOTIFY bootstrapStatusChanged)
//    Q_PROPERTY(bool hasOwnership READ hasOwnership NOTIFY hasOwnershipChanged)

public:
    enum Status
    {
        Error = -1,
        NotConnected   = 0x00,
        Connecting     = 0x01,
        Authenticating = 0x02,
        Connected      = 0x03
    };

    enum TorStatus
    {
        TorUnknown = 0x00,
        TorOffline = 0x01,
        TorReady   = 0x02
    };

    explicit TorControl();

    /* Information */
    Status status() const;
    TorStatus torStatus() const;
    std::string torVersion() const;
    bool torVersionAsNewAs(const std::string &version) const;
    std::string errorMessage() const;

    bool hasConnectivity() const;
    std::string socksAddress() const;
    quint16 socksPort() const;
    QNetworkProxy connectionProxy();

    /* Authentication */
    void setAuthPassword(const ByteArray& password);

    /* Connection */
    bool isConnected() const { return status() == Connected; }
    void connect(const std::string &address, quint16 port);

    /* Ownership means that tor is managed by this socket, and we
     * can shut it down, own its configuration, etc. */
    bool hasOwnership() const;
    void takeOwnership();

    /* Hidden Services */
    std::list<HiddenService*> hiddenServices() const;
    void addHiddenService(HiddenService *service);

    std::map<std::string, std::string> bootstrapStatus() const;
    /*Q_INVOKABLE*/ QObject *getConfiguration(const std::string &options);
    /*Q_INVOKABLE*/ QObject *setConfiguration(const std::list<std::pair<std::string, std::string> > &options);
    /*Q_INVOKABLE*/ PendingOperation *saveConfiguration();

//signals:
//    void statusChanged(int newStatus, int oldStatus);
//    void torStatusChanged(int newStatus, int oldStatus);
//    void connected();
//    void disconnected();
//    void connectivityChanged();
//    void bootstrapStatusChanged();
//    void hasOwnershipChanged();

    virtual void socketError(const std::string &s) override;

//public slots:
    /* Instruct Tor to shutdown */
    void shutdown();
    /* Call shutdown(), and wait synchronously for the command to be written */
    void shutdownSync();

    void reconnect();

private:
    TorControlSocket *mSocket;
    std::string mTorAddress;
    std::string mErrorMessage;
    std::string mTorVersion;
    ByteArray mAuthPassword;
    std::string mSocksAddress;
    std::list<HiddenService*> mServices;
    quint16 mControlPort, mSocksPort;
    TorControl::Status mStatus;
    TorControl::TorStatus mTorStatus;
    std::map<std::string,std::string> mBootstrapStatus;
    bool mHasOwnership;

    void getTorInfo();
    void getTorInfoReply();
    void setStatus(TorControl::Status n);
    void statusEvent(int code, const ByteArray &data);
    void setTorStatus(TorControl::TorStatus n);
    void updateBootstrap(const std::list<ByteArray>& data);
    void setError(const std::string& message);
    void publishServices();
    void protocolInfoReply(TorControlCommand *sender);
    void socketDisconnected();
    void socketConnected();
    void authenticateReply();
};

}

extern Tor::TorControl *torControl;

#endif // TORCONTROLMANAGER_H
