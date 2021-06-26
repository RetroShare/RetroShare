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

#include <iostream>

#include "TorManager.h"
#include "TorProcess.h"
#include "TorControl.h"
#include "CryptoKey.h"
#include "HiddenService.h"
#include "GetConfCommand.h"
#include "Settings.h"
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QTcpServer>
#include <QTextStream>

using namespace Tor;

namespace Tor
{

class TorManagerPrivate : public QObject
{
    Q_OBJECT

public:
    TorManager *q;
    TorProcess *process;
    TorControl *control;
    QString dataDir;
    QString hiddenServiceDir;
    QStringList logMessages;
    QString errorMessage;
    bool configNeeded;

	HiddenService *hiddenService ;

    explicit TorManagerPrivate(TorManager *parent = 0);

    QString torExecutablePath() const;
    bool createDataDir(const QString &path);
    bool createDefaultTorrc(const QString &path);

    void setError(const QString &errorMessage);

public slots:
    void processStateChanged(int state);
    void processErrorChanged(const QString &errorMessage);
    void processLogMessage(const QString &message);
    void controlStatusChanged(int status);
    void getConfFinished();
};

}

TorManager::TorManager(QObject *parent)
    : QObject(parent), d(new TorManagerPrivate(this))
{
}

TorManagerPrivate::TorManagerPrivate(TorManager *parent)
    : QObject(parent)
    , q(parent)
    , process(0)
    , control(new TorControl(this))
    , configNeeded(false)
    , hiddenService(NULL)
{
    connect(control, SIGNAL(statusChanged(int,int)), SLOT(controlStatusChanged(int)));
}

TorManager *TorManager::instance()
{
    static TorManager *p = 0;
    if (!p)
        p = new TorManager(qApp);
    return p;
}

TorControl *TorManager::control()
{
    return d->control;
}

TorProcess *TorManager::process()
{
    return d->process;
}

QString TorManager::torDataDirectory() const
{
    return d->dataDir;
}

void TorManager::setTorDataDirectory(const QString &path)
{
    d->dataDir = QDir::fromNativeSeparators(path);

    if (!d->dataDir.isEmpty() && !d->dataDir.endsWith(QLatin1Char('/')))
        d->dataDir.append(QLatin1Char('/'));
}

QString TorManager::hiddenServiceDirectory() const
{
    return d->hiddenServiceDir;
}
void TorManager::setHiddenServiceDirectory(const QString &path)
{
    d->hiddenServiceDir = QDir::fromNativeSeparators(path);

    if (!d->hiddenServiceDir.isEmpty() && !d->hiddenServiceDir.endsWith(QLatin1Char('/')))
        d->hiddenServiceDir.append(QLatin1Char('/'));
}

bool TorManager::setupHiddenService()
{
	if(d->hiddenService != NULL)
	{
		std::cerr << "TorManager: setupHiddenService() called twice! Not doing anything this time." << std::endl;
		return true ;
	}

    QString keyData   ;//= m_settings->read("serviceKey").toString();
    QString legacyDir = d->hiddenServiceDir;

	std::cerr << "TorManager: setting up hidden service." << std::endl;

	if(legacyDir.isNull())
	{
		std::cerr << "legacy dir not set! Cannot proceed." << std::endl;
		return false ;
	}

	std::cerr << "Using legacy dir: " << legacyDir.toStdString() << std::endl;

	if (!legacyDir.isEmpty() && QFile::exists(legacyDir + QLatin1String("/private_key")))
    {
        std::cerr << "Attempting to load key from legacy filesystem format in " << legacyDir.toStdString() << std::endl;

        CryptoKey key;
        if (!key.loadFromFile(legacyDir + QLatin1String("/private_key")))
        {
            qWarning() << "Cannot load legacy format key from" << legacyDir << "for conversion";
            return false;
        }

		d->hiddenService = new Tor::HiddenService(key, legacyDir, this);

		std::cerr << "Got key from legacy dir: " << std::endl;
        std::cerr << key.bytes().toStdString() << std::endl;
    }
    else
    {
        d->hiddenService = new Tor::HiddenService(legacyDir, this);

		std::cerr << "Creating new hidden service." << std::endl;

        connect(d->hiddenService, SIGNAL(privateKeyChanged()), this, SLOT(hiddenServicePrivateKeyChanged())) ;
        connect(d->hiddenService, SIGNAL(hostnameChanged()), this, SLOT(hiddenServiceHostnameChanged())) ;
    }

    Q_ASSERT(d->hiddenService);
    connect(d->hiddenService, SIGNAL(statusChanged(int,int)), this, SLOT(hiddenServiceStatusChanged(int,int)));

    // Generally, these are not used, and we bind to localhost and port 0
    // for an automatic (and portable) selection.

    QHostAddress address = QHostAddress::LocalHost;	// we only listen from localhost

    quint16 port = 7934;//(quint16)m_settings->read("localListenPort").toInt();

	std::cerr << "Testing host address: " << address.toString().toStdString() << ":" << port ;

    if (!QTcpServer().listen(address, port))
    {
        // XXX error case
		std::cerr << " Failed to open incoming socket" << std::endl;
        return false;
    }

	std::cerr << " OK - Adding hidden service to TorControl." << std::endl;

    //d->hiddenService->addTarget(9878, mIncomingServer->serverAddress(), mIncomingServer->serverPort());
    d->hiddenService->addTarget(9878, QHostAddress::LocalHost,7934);
    control()->addHiddenService(d->hiddenService);

	return true ;
}

void TorManager::hiddenServiceStatusChanged(int old_status,int new_status)
{
	std::cerr << "Hidden service status changed from " << old_status << " to " << new_status << std::endl;
}

void TorManager::hiddenServicePrivateKeyChanged()
{
    QString key = QString::fromLatin1(d->hiddenService->privateKey().bytes());

	QFile outfile(d->hiddenServiceDir + QLatin1String("/private_key")) ;
    outfile.open( QIODevice::WriteOnly | QIODevice::Text );
    QTextStream s(&outfile);

#ifdef TO_REMOVE
	s << "-----BEGIN RSA PRIVATE KEY-----" << endl;

    for(int i=0;i<key.length();i+=64)
	    s << key.mid(i,64) << endl ;

	s << "-----END RSA PRIVATE KEY-----" << endl;
#endif
    s << key ;

    outfile.close();

	std::cerr << "Hidden service private key changed!" << std::endl;
	std::cerr << key.toStdString() << std::endl;
}

void TorManager::hiddenServiceHostnameChanged()
{
    QFile outfile2(d->hiddenServiceDir + QLatin1String("/hostname")) ;
    outfile2.open( QIODevice::WriteOnly | QIODevice::Text );
    QTextStream t(&outfile2);

    QString hostname(d->hiddenService->hostname());

    t << hostname << endl;
    outfile2.close();

    std::cerr << "Hidden service hostname changed: " << hostname.toStdString() << std::endl;
}

bool TorManager::configurationNeeded() const
{
    return d->configNeeded;
}

QStringList TorManager::logMessages() const
{
    return d->logMessages;
}

bool TorManager::hasError() const
{
    return !d->errorMessage.isEmpty();
}

QString TorManager::errorMessage() const
{
    return d->errorMessage;
}

bool TorManager::start()
{
    if (!d->errorMessage.isEmpty()) {
        d->errorMessage.clear();
        emit errorChanged();
    }

    SettingsObject settings(QStringLiteral("tor"));

    // If a control port is defined by config or environment, skip launching tor
    if (!settings.read("controlPort").isUndefined() ||
        !qEnvironmentVariableIsEmpty("TOR_CONTROL_PORT"))
    {
        QHostAddress address(settings.read("controlAddress").toString());
        quint16 port = (quint16)settings.read("controlPort").toInt();
        QByteArray password = settings.read("controlPassword").toString().toLatin1();

        if (!qEnvironmentVariableIsEmpty("TOR_CONTROL_HOST"))
            address = QHostAddress(QString::fromLatin1(qgetenv("TOR_CONTROL_HOST")));

        if (!qEnvironmentVariableIsEmpty("TOR_CONTROL_PORT")) {
            bool ok = false;
            port = qgetenv("TOR_CONTROL_PORT").toUShort(&ok);
            if (!ok)
                port = 0;
        }

        if (!qEnvironmentVariableIsEmpty("TOR_CONTROL_PASSWD"))
            password = qgetenv("TOR_CONTROL_PASSWD");

        if (!port) {
            d->setError(QStringLiteral("Invalid control port settings from environment or configuration"));
            return false;
        }

        if (address.isNull())
            address = QHostAddress::LocalHost;

        d->control->setAuthPassword(password);
        d->control->connect(address, port);
    } else {
        // Launch a bundled Tor instance
        QString executable = d->torExecutablePath();

        std::cerr << "Executable path: " << executable.toStdString() << std::endl;

        if (executable.isEmpty()) {
            d->setError(QStringLiteral("Cannot find tor executable"));
            return false;
        }

        if (!d->process) {
            d->process = new TorProcess(this);
            connect(d->process, SIGNAL(stateChanged(int)), d, SLOT(processStateChanged(int)));
            connect(d->process, SIGNAL(errorMessageChanged(QString)), d,
                    SLOT(processErrorChanged(QString)));
            connect(d->process, SIGNAL(logMessage(QString)), d, SLOT(processLogMessage(QString)));
        }

        if (!QFile::exists(d->dataDir) && !d->createDataDir(d->dataDir)) {
            d->setError(QStringLiteral("Cannot write data location: %1").arg(d->dataDir));
            return false;
        }

        QString defaultTorrc = d->dataDir + QStringLiteral("default_torrc");
        if (!QFile::exists(defaultTorrc) && !d->createDefaultTorrc(defaultTorrc)) {
            d->setError(QStringLiteral("Cannot write data files: %1").arg(defaultTorrc));
            return false;
        }

        QFile torrc(d->dataDir + QStringLiteral("torrc"));
        if (!torrc.exists() || torrc.size() == 0) {
            d->configNeeded = true;
            emit configurationNeededChanged();
        }

        d->process->setExecutable(executable);
        d->process->setDataDir(d->dataDir);
        d->process->setDefaultTorrc(defaultTorrc);
        d->process->start();
    }
	return true ;
}

bool TorManager::getProxyServerInfo(QHostAddress& proxy_server_adress,uint16_t& proxy_server_port)
{
	proxy_server_adress = control()->socksAddress();
	proxy_server_port   = control()->socksPort();

	return proxy_server_port > 1023 ;
}

bool TorManager::getHiddenServiceInfo(QString& service_id,QString& service_onion_address,uint16_t& service_port, QHostAddress& service_target_address,uint16_t& target_port)
{
	QList<Tor::HiddenService*> hidden_services = control()->hiddenServices();

	if(hidden_services.empty())
		return false ;

	// Only return the first one.

	for(auto it(hidden_services.begin());it!=hidden_services.end();++it)
	{
		service_onion_address = (*it)->hostname();
        service_id = (*it)->serviceId();

		for(auto it2((*it)->targets().begin());it2!=(*it)->targets().end();++it2)
		{
			service_port = (*it2).servicePort ;
			service_target_address = (*it2).targetAddress ;
			target_port = (*it2).targetPort;
			break ;
		}
		break ;
	}
	return true ;
}

void TorManagerPrivate::processStateChanged(int state)
{
    std::cerr << Q_FUNC_INFO << "state: " << state << " passwd=\"" << QString(process->controlPassword()).toStdString() << "\" " << process->controlHost().toString().toStdString()
	         << ":" << process->controlPort() << std::endl;
    if (state == TorProcess::Ready) {
        control->setAuthPassword(process->controlPassword());
        control->connect(process->controlHost(), process->controlPort());
    }
}

void TorManagerPrivate::processErrorChanged(const QString &errorMessage)
{
    std::cerr << "tor error:" << errorMessage.toStdString() << std::endl;
    setError(errorMessage);
}

void TorManagerPrivate::processLogMessage(const QString &message)
{
    std::cerr << "tor:" << message.toStdString() << std::endl;
    if (logMessages.size() >= 50)
        logMessages.takeFirst();
    logMessages.append(message);
}

void TorManagerPrivate::controlStatusChanged(int status)
{
    if (status == TorControl::Connected) {
        if (!configNeeded) {
            // If DisableNetwork is 1, trigger configurationNeeded
            connect(control->getConfiguration(QStringLiteral("DisableNetwork")),
                    SIGNAL(finished()), SLOT(getConfFinished()));
        }

        if (process) {
            // Take ownership via this control socket
            control->takeOwnership();
        }
    }
}

void TorManagerPrivate::getConfFinished()
{
    GetConfCommand *command = qobject_cast<GetConfCommand*>(sender());
    if (!command)
        return;

    if (command->get("DisableNetwork").toInt() == 1 && !configNeeded) {
        configNeeded = true;
        emit q->configurationNeededChanged();
    }
}

QString TorManagerPrivate::torExecutablePath() const
{
    SettingsObject settings(QStringLiteral("tor"));
    QString path = settings.read("executablePath").toString();

    if (!path.isEmpty() && QFile::exists(path))
        return path;

#ifdef Q_OS_WIN
    QString filename(QStringLiteral("/tor/tor.exe"));
#else
    QString filename(QStringLiteral("/tor"));
#endif

    path = qApp->applicationDirPath();

    if (QFile::exists(path + filename))
        return path + filename;

#ifdef BUNDLED_TOR_PATH
    path = QStringLiteral(BUNDLED_TOR_PATH);
    if (QFile::exists(path + filename))
        return path + filename;
#endif

#ifdef __APPLE__
    // on MacOS, try traditional brew installation path

    path = QStringLiteral("/usr/local/opt/tor/bin") ;

    if (QFile::exists(path + filename))
        return path + filename;
#endif

    // Try $PATH
    return filename.mid(1);
}

bool TorManagerPrivate::createDataDir(const QString &path)
{
    QDir dir(path);
    return dir.mkpath(QStringLiteral("."));
}

bool TorManagerPrivate::createDefaultTorrc(const QString &path)
{
    static const char defaultTorrcContent[] =
        "SocksPort auto\n"
        "AvoidDiskWrites 1\n"
//        "DisableNetwork 1\n"	// (cyril) I removed this because it prevents Tor to bootstrap.
        "__ReloadTorrcOnSIGHUP 0\n";

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    if (file.write(defaultTorrcContent) < 0)
        return false;
    return true;
}

void TorManagerPrivate::setError(const QString &message)
{
    errorMessage = message;
    emit q->errorChanged();
}

#include "TorManager.moc"

bool RsTor::isTorAvailable()
{
    return !instance()->d->torExecutablePath().isNull();
}

bool RsTor::getHiddenServiceInfo(std::string& service_id,
                                 std::string& service_onion_address,
                                 uint16_t& service_port,
                                 std::string& service_target_address,
                                 uint16_t& target_port)
{
    QString sid;
    QString soa;
    QHostAddress sta;

    if(!instance()->getHiddenServiceInfo(sid,soa,service_port,sta,target_port))
        return false;

    service_id = sid.toStdString();
    service_onion_address = soa.toStdString();
    service_target_address = sta.toString().toStdString();

    return true;
}

std::list<std::string> RsTor::logMessages()
{
    QStringList qs = instance()->logMessages();

    std::list<std::string> s;
    for(auto& ss:qs)
        s.push_back(ss.toStdString());

    return s;
}

std::string RsTor::socksAddress()
{
    return instance()->control()->socksAddress().toString().toStdString();
}
uint16_t RsTor::socksPort()
{
    return instance()->control()->socksPort();
}

RsTorStatus RsTor::torStatus()
{
    TorControl::TorStatus ts = instance()->control()->torStatus();

    switch(ts)
    {
    case TorControl::TorOffline: return RsTorStatus::OFFLINE;
    case TorControl::TorReady:   return RsTorStatus::READY;

    default:
    case TorControl::TorUnknown: return RsTorStatus::UNKNOWN;
    }
}

RsTorControlStatus RsTor::torControlStatus()
{
    TorControl::Status ts = instance()->control()->status();

    switch(ts)
    {
    default:
    case Tor::TorControl::Error :          return RsTorControlStatus::ERROR;
    case Tor::TorControl::NotConnected :   return RsTorControlStatus::NOT_CONNECTED;
    case Tor::TorControl::Authenticating:  return RsTorControlStatus::AUTHENTICATING;
    case Tor::TorControl::Connecting:      return RsTorControlStatus::CONNECTING;
    case Tor::TorControl::Connected :      return RsTorControlStatus::CONNECTED;
    }
}

bool RsTor::setupHiddenService()
{
    return instance()->setupHiddenService();
}

RsTorHiddenServiceStatus RsTor::getHiddenServiceStatus(std::string& service_id)
{
    service_id.clear();
    auto list = instance()->control()->hiddenServices();

    if(list.empty())
        return RsTorHiddenServiceStatus::NOT_CREATRED;

    service_id = (*list.begin())->serviceId().toStdString();

    switch((*list.begin())->status())
    {
    default:
    case Tor::HiddenService::NotCreated: return RsTorHiddenServiceStatus::NOT_CREATRED;
    case Tor::HiddenService::Offline   : return RsTorHiddenServiceStatus::OFFLINE;
    case Tor::HiddenService::Online    : return RsTorHiddenServiceStatus::ONLINE;
    }
}

std::map<std::string,std::string> RsTor::bootstrapStatus()
{
    QVariantMap m = instance()->control()->bootstrapStatus();
    std::map<std::string,std::string> res;

    for(auto it(m.begin());it!=m.end();++it)
        res.insert(std::make_pair(it.key().toStdString(),it.value().toString().toStdString()));

    return res;
}

bool RsTor::hasError()
{
    return instance()->hasError();
}
std::string RsTor::errorMessage()
{
    return instance()->errorMessage().toStdString();
}

void RsTor::getProxyServerInfo(std::string& server_address,  uint16_t& server_port)
{
    QHostAddress qserver_address;
    instance()->getProxyServerInfo(qserver_address,server_port);

    server_address = qserver_address.toString().toStdString();
}

bool RsTor::start()
{
    return instance()->start();
}

void RsTor::setTorDataDirectory(const std::string& dir)
{
    instance()->setTorDataDirectory(QString::fromStdString(dir));
}
void RsTor::setHiddenServiceDirectory(const std::string& dir)
{
    instance()->setHiddenServiceDirectory(QString::fromStdString(dir));
}

TorManager *RsTor::instance()
{
    assert(getpid() == gettid());	// make sure we're not in a thread

    static TorManager *rsTor = nullptr;

    if(rsTor == nullptr)
        rsTor = new TorManager;

    return rsTor;
}
