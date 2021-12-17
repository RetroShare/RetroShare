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

#include <time.h>
#include <fstream>
#include "util/rsdir.h"

#include "retroshare/rstor.h"
#include "TorControl.h"
#include "TorControlSocket.h"
#include "HiddenService.h"
#include "ProtocolInfoCommand.h"
#include "AuthenticateCommand.h"
#include "SetConfCommand.h"
#include "GetConfCommand.h"
#include "AddOnionCommand.h"
#include "StrUtil.h"
#include "PendingOperation.h"

Tor::TorControl *torControl = 0;

class nullstream: public std::ostream {};

static std::ostream& torctrldebug()
{
    static nullstream null ;

    if(true)
        return std::cerr << time(NULL) << ":TOR CONTROL: " ;
    else
        return null ;
}

#define torCtrlDebug torctrldebug


using namespace Tor;

namespace Tor {

// class TorControlPrivate : public QObject
// {
//     Q_OBJECT
//
// public:
//     TorControl *q;
//
//     TorControlSocket *socket;
//     std::string torAddress;
//     std::string errorMessage;
//     std::string torVersion;
//     ByteArray authPassword;
//     std::string socksAddress;
//     QList<HiddenService*> services;
//     uint16_t controlPort, socksPort;
//     TorControl::Status status;
//     TorControl::TorStatus torStatus;
//     std::map<std::string,std::string> bootstrapStatus;
//     bool hasOwnership;
//
//     TorControlPrivate(TorControl *parent);
//
//     void setStatus(TorControl::Status status);
//     void setTorStatus(TorControl::TorStatus status);
//
//     void getTorInfo();
//     void publishServices();
//
// public slots:
//     void socketConnected();
//     void socketDisconnected();
//     void socketError();
//
//     void authenticateReply();
//     void protocolInfoReply();
//     void getTorInfoReply();
//     void setError(const std::string &message);
//
//     void statusEvent(int code, const ByteArray &data);
//     void updateBootstrap(const std::list<ByteArray> &data);
// };

}

TorControl::TorControl()
{
    mSocket = new TorControlSocket(this);
}

// TorControlPrivate::TorControlPrivate(TorControl *parent)
//     : QObject(parent), q(parent), controlPort(0), socksPort(0),
//       status(TorControl::NotConnected), torStatus(TorControl::TorUnknown),
//       hasOwnership(false)
// {
//     socket = new TorControlSocket();
//
//     // QObject::connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
//     // QObject::connect(socket, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
//     // QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError()));
//     // QObject::connect(socket, SIGNAL(error(QString)), this, SLOT(setError(QString)));
// }

// QNetworkProxy TorControl::connectionProxy()
// {
//     return QNetworkProxy(QNetworkProxy::Socks5Proxy, d->socksAddress.toString(), d->socksPort);
// }

static RsTorConnectivityStatus torConnectivityStatus(Tor::TorControl::Status t)
{
    switch(t)
    {
    default:
    case TorControl::Error:          return RsTorConnectivityStatus::ERROR;
    case TorControl::NotConnected:   return RsTorConnectivityStatus::NOT_CONNECTED;
    case TorControl::Connecting:     return RsTorConnectivityStatus::CONNECTING;
    case TorControl::Authenticating: return RsTorConnectivityStatus::AUTHENTICATING;
    case TorControl::Authenticated:      return RsTorConnectivityStatus::AUTHENTICATED;
    }
}
static RsTorStatus torStatus(Tor::TorControl::TorStatus t)
{
    switch(t)
    {
    default:
    case TorControl::TorUnknown:   return RsTorStatus::UNKNOWN;
    case TorControl::TorOffline:   return RsTorStatus::OFFLINE;
    case TorControl::TorReady:     return RsTorStatus::READY;
    }
}

void TorControl::setStatus(TorControl::Status n)
{
    if (n == mStatus)
        return;

    TorControl::Status old = mStatus;
    mStatus = n;

    if (old == TorControl::Error)
        mErrorMessage.clear();

    if(rsEvents)
    {
        auto ev = std::make_shared<RsTorManagerEvent>();

        ev->mTorManagerEventType    = RsTorManagerEventCode::TOR_STATUS_CHANGED;
        ev->mTorStatus = ::torStatus(mTorStatus);
        ev->mTorConnectivityStatus  = torConnectivityStatus(mStatus);

        rsEvents->sendEvent(ev);
    }
    mStatusChanged_callback(mStatus, old);

//    if (mStatus == TorControl::Connected && old < TorControl::Connected)
//        socketConnected();//mConnected_callback();
//    else if (mStatus < TorControl::Connected && old >= TorControl::Connected)
//        socketDisconnected();//mDisconnected_callback();
}

void TorControl::setTorStatus(TorControl::TorStatus n)
{
    if (n == mTorStatus)
        return;

    TorControl::TorStatus old = mTorStatus;
    mTorStatus = n;

    if(rsEvents)
    {
        auto ev = std::make_shared<RsTorManagerEvent>();

        ev->mTorManagerEventType = RsTorManagerEventCode::TOR_STATUS_CHANGED;
        ev->mTorConnectivityStatus  = torConnectivityStatus(mStatus);
        ev->mTorStatus = ::torStatus(mTorStatus);
        rsEvents->sendEvent(ev);
    }
}

void TorControl::setError(const std::string &message)
{
    mErrorMessage = message;
    setStatus(TorControl::Error);

    RsWarn() << "torctrl: Error:" << mErrorMessage;
}

TorControl::Status TorControl::status() const
{
    return mStatus;
}

TorControl::TorStatus TorControl::torStatus() const
{
    return mTorStatus;
}

std::string TorControl::torVersion() const
{
    return mTorVersion;
}

std::string TorControl::errorMessage() const
{
    return mErrorMessage;
}

bool TorControl::hasConnectivity() const
{
    return torStatus() == TorReady && !mSocksAddress.empty();
}

std::string TorControl::socksAddress() const
{
    return mSocksAddress;
}

uint16_t TorControl::socksPort() const
{
    return mSocksPort;
}

std::list<HiddenService*> TorControl::hiddenServices() const
{
    return mServices;
}

std::map<std::string,std::string> TorControl::bootstrapStatus() const
{
    return mBootstrapStatus;
}

void TorControl::setAuthPassword(const ByteArray &password)
{
    mAuthPassword = password;
}

void TorControl::connect(const std::string &address, uint16_t port)
{
    if (status() > Connecting)
    {
        torCtrlDebug() << "Ignoring TorControl::connect due to existing connection" << std::endl;
        return;
    }

    mTorAddress = address;
    mControlPort = port;
    setTorStatus(TorUnknown);

    //bool b = d->socket->blockSignals(true);
    if(mSocket->isRunning())
        mSocket->fullstop();
    //d->socket->blockSignals(b);

    setStatus(Connecting);

    if(mSocket->connectToHost(address, port))
        setStatus(SocketConnected);
}

void TorControl::reconnect()
{
    assert(!mTorAddress.empty() && mControlPort);

    if (mTorAddress.empty() || !mControlPort || status() >= Connecting)
        return;

    setStatus(Connecting);
    mSocket->connectToHost(mTorAddress, mControlPort);
}

void TorControl::authenticateReply(TorControlCommand *sender)
{
    AuthenticateCommand *command = dynamic_cast<AuthenticateCommand*>(sender);
    assert(command);
    assert(mStatus == TorControl::Authenticating);
    if (!command)
        return;

    if (!command->isSuccessful()) {
        setError(command->errorMessage());
        return;
    }

    torCtrlDebug() << "torctrl: Authentication successful" << std::endl;
    setStatus(TorControl::Authenticated);

    setTorStatus(TorControl::TorUnknown);

    TorControlCommand *clientEvents = new TorControlCommand;
    clientEvents->set_replyLine_callback([this](int code, const ByteArray &data)
    {
        statusEvent(code,data);	// no async needed here.
    });

    mSocket->registerEvent(ByteArray("STATUS_CLIENT"), clientEvents);

    getTorInfo();
    publishServices();

    // XXX Fix old configurations that would store unwanted options in torrc.
    // This can be removed some suitable amount of time after 1.0.4.
    if (mHasOwnership)
        saveConfiguration();
}


void TorControl::authenticate()
{
    assert(mStatus == TorControl::SocketConnected);

    setStatus(TorControl::Authenticating);
    torCtrlDebug() << "torctrl: Connected socket; querying information for authentication" << std::endl;

    ProtocolInfoCommand *command = new ProtocolInfoCommand(this);

    command->set_finished_callback( [this](TorControlCommand *sender) { protocolInfoReply(sender); });
    //connect(command, &TorControlCommand::finished, this, &protocolInfoReply);
    mSocket->sendCommand(command, command->build());
}

void TorControl::socketDisconnected()
{
    /* Clear some internal state */
    mTorVersion.clear();
    mSocksAddress.clear();
    mSocksPort = 0;
    setTorStatus(TorControl::TorUnknown);

    /* This emits the disconnected() signal as well */
    setStatus(TorControl::NotConnected);
}

void TorControl::socketError(const std::string& s)
{
    setError("Connection failed: " + s);
}

void TorControl::protocolInfoReply(TorControlCommand *sender)
{
    ProtocolInfoCommand *info = dynamic_cast<ProtocolInfoCommand*>(sender);
    if (!info)
        return;

    mTorVersion = info->torVersion();

    if (mStatus == TorControl::Authenticating)
    {
        AuthenticateCommand *auth = new AuthenticateCommand;
        //connect(auth, &TorControlCommand::finished, this, &TorControl::authenticateReply);
        auth->set_finished_callback( [this](TorControlCommand *sender) { authenticateReply(sender); });

        ByteArray data;
        ProtocolInfoCommand::AuthMethod methods = info->authMethods();

        if(methods & ProtocolInfoCommand::AuthNull)
        {
            torCtrlDebug() << "torctrl: Using null authentication" << std::endl;
            data = auth->build();
        }
        else if ((methods & ProtocolInfoCommand::AuthCookie) && !info->cookieFile().empty())
        {
            std::string cookieFile = info->cookieFile();
            std::string cookieError;
            torCtrlDebug() << "torctrl: Using cookie authentication with file" << cookieFile << std::endl;

            FILE *f = fopen(cookieFile.c_str(),"r");

            if(f)
            {
                std::string cookie;
                char c;
                while((c=getc(f))!=EOF)
                    cookie += c;
                fclose(f);

                /* Simple test to avoid a vulnerability where any process listening on what we think is
                 * the control port could trick us into sending the contents of an arbitrary file */
                if (cookie.size() == 32)
                    data = auth->build(cookie);
                else
                    cookieError = "Unexpected file size";
            }
            else
                cookieError = "Cannot open file " + cookieFile + ". errno=" + RsUtil::NumberToString(errno);

            if (!cookieError.empty() || data.isNull())
            {
                /* If we know a password and password authentication is allowed, try using that instead.
                 * This is a strange corner case that will likely never happen in a normal configuration,
                 * but it has happened. */
                if ((methods & ProtocolInfoCommand::AuthHashedPassword) && !mAuthPassword.empty())
                {
                    torCtrlDebug() << "torctrl: Unable to read authentication cookie file:" << cookieError << std::endl;
                    goto usePasswordAuth;
                }

                setError("Unable to read authentication cookie file: " + cookieError);
                delete auth;
                return;
            }
        }
        else if ((methods & ProtocolInfoCommand::AuthHashedPassword) && !mAuthPassword.empty())
        {
            usePasswordAuth:
            torCtrlDebug() << "torctrl: Using hashed password authentication with AuthPasswd=\"" << mAuthPassword.toString() << "\"" << std::endl;
            data = auth->build(mAuthPassword);
        }
        else
        {
            if (methods & ProtocolInfoCommand::AuthHashedPassword)
                setError("Tor requires a control password to connect, but no password is configured.");
            else
                setError("Tor is not configured to accept any supported authentication methods.");
            delete auth;
            return;
        }

        mSocket->sendCommand(auth, data);
    }
}

void TorControl::getTorInfo()
{
    assert(isConnected());

    GetConfCommand *command = new GetConfCommand(GetConfCommand::GetInfo);
    //connect(command, &TorControlCommand::finished, this, &TorControl::getTorInfoReply);
    command->set_finished_callback( [this](TorControlCommand *sender) { getTorInfoReply(sender); });

    std::list<std::string> keys{ "status/circuit-established","status/bootstrap-phase" };

#ifdef TODO
    /* If these are set in the config, they override the automatic behavior. */
    SettingsObject settings("tor");
    QHostAddress forceAddress(settings.read("socksAddress").toString());
    uint16_t port = (uint16_t)settings.read("socksPort").toInt();

    if (!forceAddress.isNull() && port) {
        torCtrlDebug() << "torctrl: Using manually specified SOCKS connection settings";
        socksAddress = forceAddress;
        socksPort = port;

        if(rsEvents)
        {
            auto ev = std::make_shared<RsTorManagerEvent>();

            ev->mTorManagerEventType = RsTorManagerEventCode::TOR_CONNECTIVITY_CHANGED;
            rsEvents->sendEvent(ev);
        }
    }
    else
#endif
        keys .push_back("net/listeners/socks");

    mSocket->sendCommand(command, command->build(keys));
}

void TorControl::getTorInfoReply(TorControlCommand *sender)
{
    GetConfCommand *command = dynamic_cast<GetConfCommand*>(sender);
    if (!command || !isConnected())
        return;

    std::list<ByteArray> listenAddresses = splitQuotedStrings(command->get("net/listeners/socks").front(), ' ');

    for (const auto& add:listenAddresses) {
        ByteArray value = unquotedString(add);
        int sepp = value.indexOf(':');
        std::string address(value.mid(0, sepp).toString());
        uint16_t port = (uint16_t)value.mid(sepp+1).toInt();

        /* Use the first address that matches the one used for this control connection. If none do,
         * just use the first address and rely on the user to reconfigure if necessary (not a problem;
         * their setup is already very customized) */
        if (mSocksAddress.empty() || address == mSocket->peerAddress()) {
            mSocksAddress = address;
            mSocksPort = port;
            if (address == mSocket->peerAddress())
                break;
        }
    }

    /* It is not immediately an error to have no SOCKS address; when DisableNetwork is set there won't be a
     * listener yet. To handle that situation, we'll try to read the socks address again when TorReady state
     * is reached. */
    if (!mSocksAddress.empty()) {
        torCtrlDebug() << "torctrl: SOCKS address is " << mSocksAddress << ":" << mSocksPort << std::endl;

        if(rsEvents)
        {
            auto ev = std::make_shared<RsTorManagerEvent>();

            ev->mTorManagerEventType = RsTorManagerEventCode::TOR_CONNECTIVITY_CHANGED;
            ev->mTorConnectivityStatus  = torConnectivityStatus(mStatus);
            ev->mTorStatus = ::torStatus(mTorStatus);
            rsEvents->sendEvent(ev);
        }
    }

    if (ByteArray(command->get("status/circuit-established").front()).toInt() == 1)
    {
        torCtrlDebug() << "torctrl: Tor indicates that circuits have been established; state is TorReady" << std::endl;
        setTorStatus(TorControl::TorReady);
    }
    else
        setTorStatus(TorControl::TorOffline);

    auto bootstrap = command->get("status/bootstrap-phase");
    if (!bootstrap.empty())
        updateBootstrap(splitQuotedStrings(bootstrap.front(), ' '));
}

void TorControl::addHiddenService(HiddenService *service)
{
    if (std::find(mServices.begin(),mServices.end(),service) != mServices.end())
        return;

    mServices.push_back(service);
}

void TorControl::publishServices()
{
	torCtrlDebug() << "Publish Services... " ;

    assert(isConnected());
    if (mServices.empty())
	{
		std::cerr << "No service regstered!" << std::endl;
        return;
	}
	std::cerr << std::endl;

#ifdef TODO
    SettingsObject settings("tor");
    if (settings.read("neverPublishServices").toBool())
    {
        torCtrlDebug() << "torctrl: Skipping service publication because neverPublishService is enabled" << std::endl;

        /* Call servicePublished under the assumption that they're published externally. */
        for (QList<HiddenService*>::Iterator it = services.begin(); it != services.end(); ++it)
            (*it)->servicePublished();

        return;
    }
#endif

    if (torVersionAsNewAs("0.2.7")) {
        for(HiddenService *service: mServices)
        {
            if (service->hostname().empty())
                torCtrlDebug() << "torctrl: Creating a new hidden service" << std::endl;
            else
                torCtrlDebug() << "torctrl: Publishing hidden service: " << service->hostname() << std::endl;
            AddOnionCommand *onionCommand = new AddOnionCommand(service);
            //protocolInfoReplyQObject::connect(onionCommand, &AddOnionCommand::succeeded, service, &HiddenService::servicePublished);
            onionCommand->set_succeeded_callback( [this,service]() { checkHiddenService(service) ; });
            mSocket->sendCommand(onionCommand, onionCommand->build());
        }
    } else {
        torCtrlDebug() << "torctrl: Using legacy SETCONF hidden service configuration for tor" << mTorVersion << std::endl;
        SetConfCommand *command = new SetConfCommand;
        std::list<std::pair<std::string,std::string> > torConfig;

        for(HiddenService *service: mServices)
        {
            if (service->dataPath().empty())
                continue;

            if (service->privateKey().isLoaded() && !RsDirUtil::fileExists(service->dataPath() + "/private_key")) {
                // This case can happen if tor is downgraded after the profile is created
                RsWarn() << "Cannot publish ephemeral hidden services with this version of tor; skipping";
                continue;
            }

            torCtrlDebug() << "torctrl: Configuring hidden service at" << service->dataPath() << std::endl;

            torConfig.push_back(std::make_pair("HiddenServiceDir", service->dataPath()));

            const std::list<HiddenService::Target> &targets = service->targets();
            for (auto tit:targets)
            {
                std::string target = RsUtil::NumberToString(tit.servicePort) + " "
                                    +tit.targetAddress + ":"
                                    +RsUtil::NumberToString(tit.targetPort);
                torConfig.push_back(std::make_pair("HiddenServicePort", target));
            }

            command->set_ConfSucceeded_callback( [this,service]() { checkHiddenService(service); });
            //QObject::connect(command, &SetConfCommand::setConfSucceeded, service, &HiddenService::servicePublished);
        }

        if (!torConfig.empty())
            mSocket->sendCommand(command, command->build(torConfig));
    }
}

void TorControl::checkHiddenService(HiddenService *service)
{
    service->servicePublished();

    if(service->status() == HiddenService::Online)
    {
        RsDbg() << "Hidden service published and ready!" ;

        setStatus(TorControl::HiddenServiceReady);
    }
}

void TorControl::shutdown()
{
    if (!hasOwnership()) {
        RsWarn() << "torctrl: Ignoring shutdown command for a tor instance I don't own";
        return;
    }

    mSocket->sendCommand(ByteArray("SIGNAL SHUTDOWN\r\n"));
}

void TorControl::shutdownSync()
{
    if (!hasOwnership()) {
        RsWarn() << "torctrl: Ignoring shutdown command for a tor instance I don't own";
        return;
    }

    shutdown();
    while (mSocket->moretowrite(0))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    mSocket->close();
}

void TorControl::statusEvent(int /* code */, const ByteArray &data)
{
    std::list<ByteArray> tokens = splitQuotedStrings(data.trimmed(), ' ');
    if (tokens.size() < 3)
        return;

    torCtrlDebug() << "torctrl: status event:" << data.trimmed().toString() << std::endl;
    const ByteArray& tok2 = *(++tokens.begin());

    if (tok2 == "CIRCUIT_ESTABLISHED") {
        setTorStatus(TorControl::TorReady);
    } else if (tok2 == "CIRCUIT_NOT_ESTABLISHED") {
        setTorStatus(TorControl::TorOffline);
    } else if (tok2 == "BOOTSTRAP") {
        tokens.pop_front();
        updateBootstrap(tokens);
    }
}

void TorControl::updateBootstrap(const std::list<ByteArray> &data)
{
    mBootstrapStatus.clear();
    // WARN or NOTICE
    mBootstrapStatus["severity"] = (*data.begin()).toString();

    auto dat = data.begin();
    ++dat;

    for(;dat!=data.end();++dat) {               // for(int i = 1; i < data.size(); i++) {
        int equals = (*dat).indexOf('=');
        ByteArray key = (*dat).mid(0, equals);
        ByteArray value;

        if (equals >= 0)
            value = unquotedString((*dat).mid(equals + 1));

        mBootstrapStatus[key.toLower().toString()] = value.toString();
    }

    //torCtrlDebug() << bootstrapStatus << std::endl;

    if(rsEvents)
    {
        auto ev = std::make_shared<RsTorManagerEvent>();

        ev->mTorManagerEventType = RsTorManagerEventCode::BOOTSTRAP_STATUS_CHANGED;
        ev->mTorConnectivityStatus  = torConnectivityStatus(mStatus);
        ev->mTorStatus = ::torStatus(mTorStatus);
        rsEvents->sendEvent(ev);
    }
}

TorControlCommand *TorControl::getConfiguration(const std::string& options)
{
    GetConfCommand *command = new GetConfCommand(GetConfCommand::GetConf);
    mSocket->sendCommand(command, command->build(options));

    //QQmlEngine::setObjectOwnership(command, QQmlEngine::CppOwnership);
    return command;
}

TorControlCommand *TorControl::setConfiguration(const std::list<std::pair<std::string,std::string> >& options)
{
    SetConfCommand *command = new SetConfCommand;
    command->setResetMode(true);
    mSocket->sendCommand(command, command->build(options));

    //QQmlEngine::setObjectOwnership(command, QQmlEngine::CppOwnership);
    return command;
}

namespace Tor {

class SaveConfigOperation : public PendingOperation
{
public:
    SaveConfigOperation()
        : PendingOperation(), command(0)
    {
    }

    void start(TorControlSocket *socket)
    {
        assert(!command);
        command = new GetConfCommand(GetConfCommand::GetInfo);
        //QObject::connect(command, &TorControlCommand::finished, this, &SaveConfigOperation::configTextReply);
        command->set_finished_callback([this](TorControlCommand *sender){ configTextReply(sender); });

        socket->sendCommand(command, command->build(std::list<std::string> { "config-text" , "config-file" } ));
    }

    void configTextReply(TorControlCommand * /*sender*/)
    {
        assert(command);
        if (!command)
            return;

        auto lpath = command->get("config-file");
        std::string path = (lpath.empty()?std::string():lpath.front());

        if (path.empty()) {
            finishWithError("Cannot write torrc without knowing its path");
            return;
        }

        // Out of paranoia, refuse to write any file not named 'torrc', or if the
        // file doesn't exist

        auto filename = RsDirUtil::getFileName(path);

        if(filename != "torrc" || !RsDirUtil::fileExists(path))
        {
            finishWithError("Refusing to write torrc to unacceptable path " + path);
            return;
        }

        std::ofstream file(path);

        if (!file.is_open()) {
            finishWithError("Failed opening torrc file for writing: permissions error?");
            return;
        }

        // Remove these keys when writing torrc; they are set at runtime and contain
        // absolute paths or port numbers
        static const char *bannedKeys[] = {
            "ControlPortWriteToFile",
            "DataDirectory",
            "HiddenServiceDir",
            "HiddenServicePort",
            0
        };

        auto configText = command->get("config-text") ;

        for(const auto& value: configText)
        {
            ByteArray line(value);

            bool skip = false;
            for (const char **key = bannedKeys; *key; key++) {
                if (line.startsWith(*key)) {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;

            file << line.toString() << std::endl;
        }

        file.close();

        torCtrlDebug() << "torctrl: Wrote torrc file" << std::endl;
        finishWithSuccess();
    }

private:
    GetConfCommand *command;
};

}

PendingOperation *TorControl::saveConfiguration()
{
    if (!hasOwnership()) {
        RsWarn() << "torctrl: Ignoring save configuration command for a tor instance I don't own";
        return 0;
    }

    SaveConfigOperation *operation = new SaveConfigOperation();

    //QObject::connect(operation, &PendingOperation::finished, operation, &QObject::deleteLater);
    operation->set_finished_callback( [operation]() { delete operation; });
    operation->start(mSocket);

    //QQmlEngine::setObjectOwnership(operation, QQmlEngine::CppOwnership);
    return operation;
}

bool TorControl::hasOwnership() const
{
    return mHasOwnership;
}

void TorControl::takeOwnership()
{
    mHasOwnership = true;
    mSocket->sendCommand(ByteArray("TAKEOWNERSHIP\r\n"));

    // Reset PID-based polling
    std::list<std::pair<std::string,std::string> > options;
    options.push_back(std::make_pair("__OwningControllerProcess",std::string()));
    setConfiguration(options);
}

bool TorControl::torVersionAsNewAs(const std::string& match) const
{
    auto split = ByteArray(torVersion()).split(ByteArray(".-"));
    auto matchSplit = ByteArray(match).split(ByteArray(".-"));

    int split_size = split.size();
    int i=0;
    const auto& b_split(split.begin());

    for(const auto& b_matchsplit:matchSplit)
    {
        if (i >= split_size)
            return false;
        int currentVal,matchVal;
        bool ok1 = RsUtil::StringToInt((*b_split).toString(),currentVal);
        bool ok2 = RsUtil::StringToInt(b_matchsplit.toString(),matchVal);

        if (!ok1 || !ok2)
            return false;
        if (currentVal > matchVal)
            return true;
        if (currentVal < matchVal)
            return false;

        ++i;
    }

    // Versions are equal, up to the length of match
    return true;
}


