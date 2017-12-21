#include <QTimer>
#include <QFile>
#include <iostream>

#include "TorControlWindow.h"
#include "TorControl.h"
#include "HiddenService.h"

TorControlDialog::TorControlDialog(Tor::TorManager *tm,QWidget *parent)
	: mTorManager(tm)
{
	setupUi(this) ;

	QObject::connect(tm,SIGNAL(errorChanged()),this,SLOT(showLog())) ;

	QObject::connect(tm->control(),SIGNAL(statusChanged(int,int)),this,SLOT(statusChanged())) ;
    QObject::connect(tm->control(),SIGNAL(connected()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(disconnected()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(bootstrapStatusChanged()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(connectivityChanged()),this,SLOT(statusChanged()));

    QTimer::singleShot(2000,this,SLOT(checkForHiddenService())) ;
    //void configurationNeededChanged();
}

void TorControlDialog::setupHiddenService()
{
    QString keyData = m_settings->read("serviceKey").toString();
    QString legacyDir = m_settings->read("dataDirectory").toString();

    if (!keyData.isEmpty())
    {
        CryptoKey key;
        if (!key.loadFromData(QByteArray::fromBase64(keyData.toLatin1()), CryptoKey::PrivateKey, CryptoKey::DER)) {
            qWarning() << "Cannot load service key from configuration";
            return;
        }

        m_hiddenService = new Tor::HiddenService(key, legacyDir, this);
    }
    else if (!legacyDir.isEmpty() && QFile::exists(legacyDir + QLatin1String("/private_key")))
    {
        qDebug() << "Attempting to load key from legacy filesystem format in" << legacyDir;

        CryptoKey key;
        if (!key.loadFromFile(legacyDir + QLatin1String("/private_key"), CryptoKey::PrivateKey)) {
            qWarning() << "Cannot load legacy format key from" << legacyDir << "for conversion";
            return;
        } else {
            keyData = QString::fromLatin1(key.encodedPrivateKey(CryptoKey::DER).toBase64());
            m_settings->write("serviceKey", keyData);
            m_hiddenService = new Tor::HiddenService(key, legacyDir, this);
        }
    }
    else if (!m_settings->read("initializing").toBool())
    {
        qWarning() << "Missing private key for initialized identity";
        return;
    }
    else
    {
        m_hiddenService = new Tor::HiddenService(legacyDir, this);

        connect(m_hiddenService, &Tor::HiddenService::privateKeyChanged, this, [&]()
        {
                QString key = QString::fromLatin1(m_hiddenService->privateKey().encodedPrivateKey(CryptoKey::DER).toBase64());
                m_settings->write("serviceKey", key);
            }
        );
    }

    Q_ASSERT(m_hiddenService);
    connect(m_hiddenService, SIGNAL(statusChanged(int,int)), SLOT(onStatusChanged(int,int)));

    // Generally, these are not used, and we bind to localhost and port 0
    // for an automatic (and portable) selection.
    QHostAddress address(m_settings->read("localListenAddress").toString());
    if (address.isNull())
        address = QHostAddress::LocalHost;
    quint16 port = (quint16)m_settings->read("localListenPort").toInt();

    m_incomingServer = new QTcpServer(this);
    if (!m_incomingServer->listen(address, port)) {
        // XXX error case
        qWarning() << "Failed to open incoming socket:" << m_incomingServer->errorString();
        return;
    }

    connect(m_incomingServer, &QTcpServer::newConnection, this, &UserIdentity::onIncomingConnection);

    m_hiddenService->addTarget(9878, m_incomingServer->serverAddress(), m_incomingServer->serverPort());
    torControl->addHiddenService(m_hiddenService);
}

void TorControlDialog::checkForHiddenService()
{
	QList<Tor::HiddenService*> hidden_services = mTorManager->control()->hiddenServices();

	std::cerr << "Checking for hidden services:" << std::endl;

	if(hidden_services.empty())
	{
		setupHiddenService();

		QTimer::singleShot(2000,this,SLOT(checkForHiddenService())) ;
	}
}

void TorControlDialog::statusChanged()
{
	int status = mTorManager->control()->status();
	int torstatus = mTorManager->control()->torStatus();

	QString status_str,torstatus_str ;

	switch(status)
	{
	default:
	case Tor::TorControl::Error :			status_str = "Error" ; break ;
	case Tor::TorControl::NotConnected:		status_str = "Not connected" ; break ;
	case Tor::TorControl::Connecting:		status_str = "Connecting" ; break ;
	case Tor::TorControl::Authenticating:	status_str = "Authenticating" ; break ;
	case Tor::TorControl::Connected:			status_str = "Connected" ; break ;
	}

	switch(torstatus)
	{
	default:
	case Tor::TorControl::TorUnknown: 	torstatus_str = "Unknown" ; break ;
	case Tor::TorControl::TorOffline: 	torstatus_str = "Tor offline" ; break ;
	case Tor::TorControl::TorReady: 		torstatus_str = "Tor ready" ; break ;
	}

	torStatus_LB->setText(torstatus_str + "(" + status_str + ")") ;

	QVariantMap qvm = mTorManager->control()->bootstrapStatus();
	QString bootstrapstatus_str ;

	for(auto it(qvm.begin());it!=qvm.end();++it)
		bootstrapstatus_str += it.key() + ":" + it.value().toString();

	torBootstrapStatus_LB->setText(bootstrapstatus_str) ;

	QList<Tor::HiddenService*> hidden_services = mTorManager->control()->hiddenServices();

	if(hidden_services.empty())
		hiddenService_LB->setText(QString("None")) ;
	else
	{
		QString hiddenservices_str ;

		for(auto it(hidden_services.begin());it!=hidden_services.end();++it)
		{
			hiddenservices_str += (*it)->hostname();

			for(auto it2((*it)->targets().begin());it2!=(*it)->targets().end();++it2)
				hiddenservices_str += QString::number((*it2).servicePort) + ":" + (*it2).targetAddress.toString() + ":" + QString::number((*it2).targetPort) + " " ;
		}

		hiddenService_LB->setText(hiddenservices_str) ;
	}

	showLog();
}

void TorControlDialog::showLog()
{
    QString s ;
    QStringList logmsgs = mTorManager->logMessages() ;

    for(QStringList::const_iterator it(logmsgs.begin());it!=logmsgs.end();++it)
        s += *it + "\n" ;

    torLog_TB->setText(s) ;
	 QCoreApplication::processEvents() ;
}
