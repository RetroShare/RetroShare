#include <unistd.h>
#include <set>

#include <QTimer>
#include <QFile>
#include <QTcpServer>
#include <QGraphicsDropShadowEffect>

#include <iostream>
#include "util/rstime.h"

#include "TorControlWindow.h"
#include "TorManager.h"
#include "TorControl.h"
#include "HiddenService.h"

TorControlDialog::TorControlDialog(Tor::TorManager *tm,QWidget */*parent*/)
	: mTorManager(tm)
{
	setupUi(this) ;

	QObject::connect(tm->control(),SIGNAL(statusChanged(int,int)),this,SLOT(statusChanged())) ;
    QObject::connect(tm->control(),SIGNAL(connected()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(disconnected()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(bootstrapStatusChanged()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(connectivityChanged()),this,SLOT(statusChanged()));
    QObject::connect(tm           ,SIGNAL(errorChanged()),this,SLOT(statusChanged()));

    //QTimer::singleShot(2000,this,SLOT(checkForHiddenService())) ;

	mIncomingServer = new QTcpServer(this) ;
	mHiddenService = nullptr ;
	mHiddenServiceStatus = HIDDEN_SERVICE_STATUS_UNKNOWN;
	//mBootstrapPhaseFinished = false ;

    connect(mIncomingServer, SIGNAL(QTcpServer::newConnection()), this, SLOT(onIncomingConnection()));

	QTimer *timer = new QTimer ;

	QObject::connect(timer,SIGNAL(timeout()),this,SLOT(showLog())) ;
	timer->start(500) ;

	// Hide some debug output for the released version

	setWindowFlags( Qt::Dialog | Qt::FramelessWindowHint );

	adjustSize();
}

void TorControlDialog::onIncomingConnection()
{
    std::cerr << "Incoming connection !!" << std::endl;
}

void TorControlDialog::statusChanged()
{
	int tor_control_status = mTorManager->control()->status();
	int torstatus = mTorManager->control()->torStatus();

	QString tor_control_status_str,torstatus_str ;

	if(mTorManager->hasError())
		mErrorMsg = mTorManager->errorMessage() ;

	switch(tor_control_status)
	{
	default:
	case Tor::TorControl::Error :			tor_control_status_str = "Error" ; break ;
	case Tor::TorControl::NotConnected:		tor_control_status_str = "Not connected" ; break ;
	case Tor::TorControl::Connecting:		tor_control_status_str = "Connecting" ; break ;
	case Tor::TorControl::Authenticating:	tor_control_status_str = "Authenticating" ; break ;
	case Tor::TorControl::Connected:		tor_control_status_str = "Connected" ; break ;
	}

	switch(torstatus)
	{
	default:
	case Tor::TorControl::TorUnknown: 	torstatus_str = "Unknown" ; break ;
	case Tor::TorControl::TorOffline: 	torstatus_str = "Tor offline" ; break ;
	case Tor::TorControl::TorReady: 	torstatus_str = "Tor ready" ; break ;
	}

	torStatus_LB->setText(torstatus_str) ;

	if(torstatus == Tor::TorControl::TorUnknown)
		torStatus_LB->setToolTip(tr("Check that Tor is accessible in your executable path")) ;
	else
		torStatus_LB->setToolTip("") ;

	QVariantMap qvm = mTorManager->control()->bootstrapStatus();
	QString bootstrapstatus_str ;

	std::cerr << "Tor control status: " << tor_control_status_str.toStdString() << std::endl;
	std::cerr << "Tor status: " << torstatus_str.toStdString() << std::endl;

	std::cerr << "Bootstrap status map: " << std::endl;

	for(auto it(qvm.begin());it!=qvm.end();++it)
		std::cerr << "  " << it.key().toStdString() << " : " << it.value().toString().toStdString() << std::endl;

	if(!qvm["progress"].toString().isNull())
		torBootstrapStatus_LB->setText(qvm["progress"].toString() + " % (" + qvm["summary"].toString() + ")") ;
	else
		torBootstrapStatus_LB->setText(tr("[Waiting for Tor...]")) ;

	QString service_id ;
	QString onion_address ;
	QHostAddress service_target_address ;
	uint16_t service_port ;
	uint16_t target_port ;

	if(mTorManager->getHiddenServiceInfo(service_id,onion_address,service_port, service_target_address,target_port))
	{
		hiddenServiceAddress_LB->setText(QString::number(service_port) + ":" + service_target_address.toString() + ":" + QString::number(target_port));
		onionAddress_LB->setText(onion_address);
	}
	else
	{
		hiddenServiceAddress_LB->setText(QString("[Not ready]")) ;
		onionAddress_LB->setText(QString("[Not ready]")) ;
	}

	showLog();
	adjustSize();

	QCoreApplication::processEvents();	// forces update
}

void TorControlDialog::showLog()
{
	static std::set<QString> already_seen ;

    QString s ;
    QStringList logmsgs = mTorManager->logMessages() ;
	bool can_print = false ;

    for(QStringList::const_iterator it(logmsgs.begin());it!=logmsgs.end();++it)
	{
        s += *it + "\n" ;

		if(already_seen.find(*it) == already_seen.end())
		{
			can_print = true ;
			already_seen.insert(*it);
		}

		if(can_print)
			std::cerr << "[TOR DEBUG LOG] " << (*it).toStdString() << std::endl;
	}

//    torLog_TB->setText(s) ;:

	std::cerr << "Connexion Proxy: " << mTorManager->control()->socksAddress().toString().toStdString() << ":" << mTorManager->control()->socksPort() << std::endl;
}

TorControlDialog::TorStatus TorControlDialog::checkForTor(QString& error_msg)
{
	if(!mErrorMsg.isNull())
	{
		error_msg = mErrorMsg ;
		return TorControlDialog::TOR_STATUS_FAIL ;
	}

	switch(mTorManager->control()->torStatus())
	{
	case Tor::TorControl::TorReady:  rstime::rs_usleep(1*1000*1000);return TOR_STATUS_OK ;
	default:
		return TOR_STATUS_UNKNOWN ;
	}
}

TorControlDialog::HiddenServiceStatus TorControlDialog::checkForHiddenService()
{
	std::cerr << "Checking for hidden services:" ;

	switch(mHiddenServiceStatus)
	{
	default:
	case HIDDEN_SERVICE_STATUS_UNKNOWN: {

		std::cerr << " trying to setup. " ;

		if(!mTorManager->setupHiddenService())
		{
			mHiddenServiceStatus = HIDDEN_SERVICE_STATUS_FAIL ;
			std::cerr << "Failed."  << std::endl;
			return mHiddenServiceStatus ;
		}
		std::cerr << "Done."  << std::endl;
		mHiddenServiceStatus = HIDDEN_SERVICE_STATUS_REQUESTED ;
		return mHiddenServiceStatus ;
	}

	case HIDDEN_SERVICE_STATUS_REQUESTED: {
		QList<Tor::HiddenService*> hidden_services = mTorManager->control()->hiddenServices();

		if(hidden_services.empty())
		{
			std::cerr << "Not ready yet." << std::endl;
			return mHiddenServiceStatus ;
		}
		else
		{
			if(mHiddenService == NULL)
				mHiddenService = *(hidden_services.begin()) ;

			Tor::HiddenService::Status hss = mHiddenService->status();

			std::cerr << "New service acquired. Status is " << hss ;

			if(hss == Tor::HiddenService::Online)
			{
				mHiddenServiceStatus = HIDDEN_SERVICE_STATUS_OK ;
				std::cerr << ": published and running!" << std::endl;

				return mHiddenServiceStatus ;
			}
			else
			{
				std::cerr << ": not ready yet." << std::endl;
				return mHiddenServiceStatus ;
			}
		}
	}
	case  HIDDEN_SERVICE_STATUS_OK :
			std::cerr << "New service acquired." << std::endl;
			return mHiddenServiceStatus ;
	}
}

