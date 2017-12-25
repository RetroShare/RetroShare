#include <unistd.h>

#include <QTimer>
#include <QFile>
#include <QTcpServer>

#include <iostream>

#include "TorControlWindow.h"
#include "TorManager.h"
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

    //QTimer::singleShot(2000,this,SLOT(checkForHiddenService())) ;

    mIncomingServer = new QTcpServer(this) ;
    mHiddenService = NULL ;

    connect(mIncomingServer, SIGNAL(QTcpServer::newConnection()), this, SLOT(onIncomingConnection()));
}

void TorControlDialog::onIncomingConnection()
{
    std::cerr << "Incoming connection !!" << std::endl;
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
	case Tor::TorControl::Connected:		status_str = "Connected" ; break ;
	}

	switch(torstatus)
	{
	default:
	case Tor::TorControl::TorUnknown: 	torstatus_str = "Unknown" ; break ;
	case Tor::TorControl::TorOffline: 	torstatus_str = "Tor offline" ; break ;
	case Tor::TorControl::TorReady: 	torstatus_str = "Tor ready" ; break ;
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

TorControlDialog::TorStatus TorControlDialog::checkForTor()
{
	switch(mTorManager->control()->status())
	{
	case Tor::TorControl::Connected: usleep(2*1000*1000);return TOR_STATUS_OK ;
	case Tor::TorControl::Error:     return TOR_STATUS_FAIL ;

	default:
		return TOR_STATUS_UNKNOWN ;
	}
}

TorControlDialog::HiddenServiceStatus TorControlDialog::checkForHiddenService()
{
	std::cerr << "Checking for hidden services:" << std::endl;

	switch(mHiddenServiceStatus)
	{
	case HIDDEN_SERVICE_STATUS_UNKNOWN: {

		if(!mTorManager->setupHiddenService())
		{
			mHiddenServiceStatus = HIDDEN_SERVICE_STATUS_FAIL ;
			return mHiddenServiceStatus ;
		}
		mHiddenServiceStatus = HIDDEN_SERVICE_STATUS_REQUESTED ;
		break ;
	}

	case HIDDEN_SERVICE_STATUS_REQUESTED: {
		QList<Tor::HiddenService*> hidden_services = mTorManager->control()->hiddenServices();

		if(mHiddenService == NULL)
			mHiddenService = *(hidden_services.begin()) ;
	}
	case  HIDDEN_SERVICE_STATUS_OK : break;

	default: break ;
	}

	return mHiddenServiceStatus ;
}

