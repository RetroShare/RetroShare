#include <unistd.h>

#include <QTimer>
#include <QFile>
#include <QTcpServer>
#include <QGraphicsDropShadowEffect>

#include <iostream>

#include "TorControlWindow.h"
#include "TorManager.h"
#include "TorControl.h"
#include "HiddenService.h"

TorControlDialog::TorControlDialog(Tor::TorManager *tm,QWidget *parent)
	: mTorManager(tm)
{
	setupUi(this) ;

	QObject::connect(tm->control(),SIGNAL(statusChanged(int,int)),this,SLOT(statusChanged())) ;
    QObject::connect(tm->control(),SIGNAL(connected()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(disconnected()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(bootstrapStatusChanged()),this,SLOT(statusChanged()));
    QObject::connect(tm->control(),SIGNAL(connectivityChanged()),this,SLOT(statusChanged()));

    //QTimer::singleShot(2000,this,SLOT(checkForHiddenService())) ;

    mIncomingServer = new QTcpServer(this) ;
    mHiddenService = NULL ;
	mHiddenServiceStatus = HIDDEN_SERVICE_STATUS_UNKNOWN;

    connect(mIncomingServer, SIGNAL(QTcpServer::newConnection()), this, SLOT(onIncomingConnection()));

	QTimer *timer = new QTimer ;

	QObject::connect(timer,SIGNAL(timeout()),this,SLOT(showLog())) ;
	timer->start(500) ;

	// Hide some debug output for the released version
//	torLog_TB->hide();
	torBootstrapStatus_LB->hide();
	label_2->hide();

	setWindowFlags( Qt::Dialog | Qt::FramelessWindowHint );

	adjustSize();

//	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
//	effect->setBlurRadius(30.0);
//	setGraphicsEffect(effect);
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

	//torStatus_LB->setText(torstatus_str + "(" + status_str + ")") ;
	torStatus_LB->setText(status_str) ;

	QVariantMap qvm = mTorManager->control()->bootstrapStatus();
	QString bootstrapstatus_str ;

	for(auto it(qvm.begin());it!=qvm.end();++it)
		bootstrapstatus_str += it.key() + ":" + it.value().toString();

	torBootstrapStatus_LB->setText(bootstrapstatus_str) ;

	QList<Tor::HiddenService*> hidden_services = mTorManager->control()->hiddenServices();

	if(hidden_services.empty())
	{
		hiddenServiceAddress_LB->setText(QString("[Not ready]")) ;
		onionAddress_LB->setText(QString("[Not ready]")) ;
	}
	else
	{
		QString hiddenservices_str ;

		for(auto it(hidden_services.begin());it!=hidden_services.end();++it)
		{
			onionAddress_LB->setText((*it)->hostname());

			for(auto it2((*it)->targets().begin());it2!=(*it)->targets().end();++it2)
			{
				hiddenServiceAddress_LB->setText(QString::number((*it2).servicePort) + ":" + (*it2).targetAddress.toString() + ":" + QString::number((*it2).targetPort));
				break ;
			}
			break ;
		}
	}

	showLog();
	adjustSize();
}

void TorControlDialog::showLog()
{
    QString s ;
    QStringList logmsgs = mTorManager->logMessages() ;

    for(QStringList::const_iterator it(logmsgs.begin());it!=logmsgs.end();++it)
        s += *it + "\n" ;

//    torLog_TB->setText(s) ;

	 std::cerr << s.toStdString() << std::endl;
}

TorControlDialog::TorStatus TorControlDialog::checkForTor()
{
	switch(mTorManager->control()->status())
	{
	case Tor::TorControl::Connected: usleep(1*1000*1000);return TOR_STATUS_OK ;
	case Tor::TorControl::Error:     return TOR_STATUS_FAIL ;

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

