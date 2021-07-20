#include <unistd.h>
#include <set>

#include <QTimer>
#include <QFile>
#include <QTcpServer>
#include <QGraphicsDropShadowEffect>

#include <iostream>
#include "util/rstime.h"

#include "retroshare/rstor.h"
#include "retroshare/rsevents.h"

#include "TorControlWindow.h"
#include "util/qtthreadsutils.h"

TorControlDialog::TorControlDialog(QWidget *)
{
        setupUi(this) ;

        mEventHandlerId = 0;	// very important!

        rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
        {
                RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
        }, mEventHandlerId, RsEventType::TOR_MANAGER );

        //    QObject::connect(tm->control(),SIGNAL(statusChanged(int,int)),this,SLOT(statusChanged())) ;
        //    QObject::connect(tm->control(),SIGNAL(connected()),this,SLOT(statusChanged()));
        //    QObject::connect(tm->control(),SIGNAL(disconnected()),this,SLOT(statusChanged()));
        //    QObject::connect(tm->control(),SIGNAL(bootstrapStatusChanged()),this,SLOT(statusChanged()));
        //    QObject::connect(tm->control(),SIGNAL(connectivityChanged()),this,SLOT(statusChanged()));
        //    QObject::connect(tm           ,SIGNAL(errorChanged()),this,SLOT(statusChanged()));

        //QTimer::singleShot(2000,this,SLOT(checkForHiddenService())) ;

        mIncomingServer = new QTcpServer(this) ;
        mHiddenServiceStatus = HIDDEN_SERVICE_STATUS_UNKNOWN;

        connect(mIncomingServer, SIGNAL(QTcpServer::newConnection()), this, SLOT(onIncomingConnection()));

        QTimer *timer = new QTimer ;

        QObject::connect(timer,SIGNAL(timeout()),this,SLOT(showLog())) ;
        timer->start(500) ;

        // Hide some debug output for the released version

        setWindowFlags( Qt::Dialog | Qt::FramelessWindowHint );

        adjustSize();
}

void TorControlDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
        if(event->mType != RsEventType::TOR_MANAGER) return;

        const RsTorManagerEvent *fe = dynamic_cast<const RsTorManagerEvent*>(event.get());
        if(!fe)
                return;

        switch (fe->mTorManagerEventType)
        {
        case RsTorManagerEventCode::BOOTSTRAP_STATUS_CHANGED:
        case RsTorManagerEventCode::TOR_CONNECTIVITY_CHANGED:
        case RsTorManagerEventCode::TOR_STATUS_CHANGED:         statusChanged(fe->mTorStatus,fe->mTorConnectivityStatus);
            break;
        default:
                break;
        }
}

void TorControlDialog::onIncomingConnection()
{
    std::cerr << "Incoming connection !!" << std::endl;
}

void TorControlDialog::statusChanged(RsTorStatus torstatus, RsTorConnectivityStatus tor_control_status)
{
	QString tor_control_status_str,torstatus_str ;

    if(RsTor::hasError())
        mErrorMsg = QString::fromStdString(RsTor::errorMessage()) ;

	switch(tor_control_status)
	{
	default:
    case RsTorConnectivityStatus::ERROR :			tor_control_status_str = tr("Error") ; break ;
    case RsTorConnectivityStatus::NOT_CONNECTED:	tor_control_status_str = tr("Not connected") ; break ;
    case RsTorConnectivityStatus::CONNECTING:		tor_control_status_str = tr("Connecting") ; break ;
    case RsTorConnectivityStatus::AUTHENTICATING:	tor_control_status_str = tr("Authenticating") ; break ;
    case RsTorConnectivityStatus::CONNECTED:		tor_control_status_str = tr("Connected") ; break ;
	}

	switch(torstatus)
	{
	default:
    case RsTorStatus::UNKNOWN: 		torstatus_str = tr("Unknown") ; break ;
    case RsTorStatus::OFFLINE: 		torstatus_str = tr("Tor offline") ; break ;
    case RsTorStatus::READY: 		torstatus_str = tr("Tor ready") ; break ;
	}

	torStatus_LB->setText(torstatus_str) ;

    if(torstatus == RsTorStatus::UNKNOWN)
		torStatus_LB->setToolTip(tr("Check that Tor is accessible in your executable path")) ;
	else
		torStatus_LB->setToolTip("") ;

    std::map<std::string,std::string> qvm = RsTor::bootstrapStatus();
	QString bootstrapstatus_str ;

	std::cerr << "Tor control status: " << tor_control_status_str.toStdString() << std::endl;
	std::cerr << "Tor status: " << torstatus_str.toStdString() << std::endl;

	std::cerr << "Bootstrap status map: " << std::endl;

	for(auto it(qvm.begin());it!=qvm.end();++it)
        std::cerr << "  " << it->first << " : " << it->second << std::endl;

    if(!qvm["progress"].empty())
        torBootstrapStatus_LB->setText(QString::fromStdString(qvm["progress"]) + " % (" + QString::fromStdString(qvm["summary"]) + ")") ;
	else
		torBootstrapStatus_LB->setText(tr("[Waiting for Tor...]")) ;

    std::string service_id ;
    std::string onion_address ;
    std::string service_target_address ;
	uint16_t service_port ;
	uint16_t target_port ;

    if(RsTor::getHiddenServiceInfo(service_id,onion_address,service_port, service_target_address,target_port))
	{
        hiddenServiceAddress_LB->setText(QString::number(service_port) + ":" + QString::fromStdString(service_target_address) + ":" + QString::number(target_port));
        onionAddress_LB->setText(QString::fromStdString(onion_address));
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
    static std::set<std::string> already_seen ;

    std::string s ;
    std::list<std::string> logmsgs = RsTor::logMessages() ;
	bool can_print = false ;

    for(auto it(logmsgs.begin());it!=logmsgs.end();++it)
	{
        s += *it + "\n" ;

		if(already_seen.find(*it) == already_seen.end())
		{
			can_print = true ;
			already_seen.insert(*it);
		}

		if(can_print)
            std::cerr << "[TOR DEBUG LOG] " << *it << std::endl;
	}

//    torLog_TB->setText(s) ;:

    std::cerr << "Connexion Proxy: " << RsTor::socksAddress()  << ":" << QString::number(RsTor::socksPort()).toStdString() << std::endl;
}

TorControlDialog::TorStatus TorControlDialog::checkForTor(QString& error_msg)
{
	if(!mErrorMsg.isNull())
	{
		error_msg = mErrorMsg ;
		return TorControlDialog::TOR_STATUS_FAIL ;
	}

    switch(RsTor::torStatus())
	{
    case RsTorStatus::READY:  rstime::rs_usleep(1*1000*1000);return TOR_STATUS_OK ;
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

        if(!RsTor::setupHiddenService())
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

        std::string service_id;

        RsTorHiddenServiceStatus service_status = RsTor::getHiddenServiceStatus(service_id);

        if(service_id.empty())
        {
            std::cerr << "Not ready yet." << std::endl;
            return mHiddenServiceStatus ;
        }
        else
        {
            if(mHiddenService.empty())
                mHiddenService = service_id ;

            std::cerr << "New service acquired. Status is " << (int)service_status ;

            if(service_status == RsTorHiddenServiceStatus::ONLINE)
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

