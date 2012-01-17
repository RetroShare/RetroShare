/****************************************************************
 *  RetroShare QT Gui is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QObject>
#include <QMessageBox>
#include <QSplashScreen>
#include <rshare.h>
#ifndef MINIMAL_RSGUI
#include "gui/MainWindow.h"
#include "gui/FriendsDialog.h"
#include "gui/SearchDialog.h"
#include "gui/TransfersDialog.h"
#include "gui/MessagesDialog.h"
#include "gui/SharedFilesDialog.h"
#include "gui/NetworkDialog.h"
#include "gui/chat/ChatDialog.h"
#include "gui/QuickStartWizard.h"
#endif // MINIMAL_RSGUI
#include "gui/MessengerWindow.h"
#include "gui/StartDialog.h"
#include "gui/GenCertDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/connect/ConfCertDialog.h"
#include "idle/idle.h"
#include "gui/common/Emoticons.h"
#include "util/EventReceiver.h"
#include "gui/RetroShareLink.h"

/*** WINDOWS DON'T LIKE THIS - REDEFINES VER numbers.
#include <gui/qskinobject/qskinobject.h>
****/

#include <util/stringutil.h>
#include <retroshare/rsinit.h>
#include <retroshare/rsiface.h>
#include "gui/notifyqt.h"

int main(int argc, char *argv[])
{ 
#ifdef WINDOWS_SYS
	{
		/* Set the current directory to the application dir,
		   because the start dir with autostart from the registry run key is not the exe dir */
		QApplication app(argc, argv);
		QDir::setCurrent(QCoreApplication::applicationDirPath());
	}
#endif

  QStringList args = char_array_to_stringlist(argv+1, argc-1);
  
  Q_INIT_RESOURCE(images);

	rsiface = NULL;

	NotifyQt *notify = NotifyQt::Create();
	createRsIface(*notify);
	createRsControl(*rsiface, *notify);

        /* RetroShare Core Objects */
	RsInit::InitRsConfig();
	int initResult = RsInit::InitRetroShare(argc, argv);

	if (initResult < 0) {
		/* Error occured */
		QApplication dummyApp (argc, argv); // needed for QMessageBox
		QMessageBox mb(QMessageBox::Critical, QObject::tr("RetroShare"), "", QMessageBox::Ok);
		mb.setWindowIcon(QIcon(":/images/rstray3.png"));

		switch (initResult) {
		case RS_INIT_AUTH_FAILED:
			std::cerr << "RsInit::InitRetroShare AuthGPG::InitAuth failed" << std::endl;
			mb.setText(QObject::tr("Inititialize failed. Wrong or missing installation of gpg."));
			break;
		default:
			/* Unexpected return code */
			std::cerr << "RsInit::InitRetroShare unexpected return code " << initResult << std::endl;
			mb.setText(QObject::tr("An unexpected error occured. Please report 'RsInit::InitRetroShare unexpected return code %1'.").arg(initResult));
			break;
		}
		mb.exec();
		return 1;
	}

	/* create global settings object
	   path maybe wrong, when no profile exist
	   in this case it can be use only for default values */
	RshareSettings::Create ();

	/* Setup The GUI Stuff */
	Rshare rshare(args, argc, argv, 
		QString::fromUtf8(RsInit::RsConfigDirectory().c_str()));

	std::string url = RsInit::getRetroShareLink();
	if (!url.empty()) {
		/* start with RetroShare link */
		EventReceiver eventReceiver;
		if (eventReceiver.sendRetroShareLink(QString::fromStdString(url))) {
			return 0;
		}

		/* Start RetroShare */
	}

	QSplashScreen splashScreen(QPixmap(":/images/splash.png")/* , Qt::WindowStaysOnTopHint*/);

	switch (initResult) {
	case RS_INIT_OK:
		{
			/* Login Dialog */
			/* check for existing Certificate */
			StartDialog *sd = NULL;
			bool genCert = false;
			std::list<std::string> accountIds;
			if (RsInit::getAccountIds(accountIds) && (accountIds.size() > 0))
			{
				sd = new StartDialog();
				sd->show();

				while(sd -> isVisible())
				{
					rshare.processEvents();
#ifdef WIN32
					Sleep(10);
#else // __LINUX__
					usleep(10000);
#endif
				}

				/* if we're logged in */
				genCert = sd->requestedNewCert();
				delete (sd);
			}
			else
			{
				genCert = true;
			}

			if (genCert)
			{
				GenCertDialog gd;
				gd.exec ();
			}

			splashScreen.show();
		}
		break;
	case RS_INIT_HAVE_ACCOUNT:
		{
			splashScreen.show();
			splashScreen.showMessage(rshare.translate("SplashScreen", "Load profile"), Qt::AlignHCenter | Qt::AlignBottom);

			std::string preferredId, gpgId, gpgName, gpgEmail, sslName;
			RsInit::getPreferedAccountId(preferredId);

			if (RsInit::getAccountDetails(preferredId, gpgId, gpgName, gpgEmail, sslName))
			{
				RsInit::SelectGPGAccount(gpgId);
			}

			// true: note auto-login is active
                        std::string lockFile;
                        int retVal = RsInit::LockAndLoadCertificates(true, lockFile);
			switch(retVal)
			{
				case 0:	break;
				case 1:	QMessageBox::warning(	0,
												QObject::tr("Multiple instances"),
												QObject::tr("Another RetroShare using the same profile is "
                                                                                                                "already running on your system. Please close "
                                                                                                                "that instance first\n Lock file:\n") +
                                                                                                                QString::fromStdString(lockFile));
						return 1;
				case 2:	QMessageBox::critical(	0,
												QObject::tr("Multiple instances"),
												QObject::tr("An unexpected error occurred when Retroshare"
                                                                                                                     "tried to acquire the single instance lock\n Lock file:\n") +
                                                                                                                     QString::fromStdString(lockFile));
						return 1;
				case 3: QMessageBox::critical(	0,
												QObject::tr("Login Failure"),
												QObject::tr("Maybe password is wrong") );
						return 1;
				default: std::cerr << "StartDialog::loadCertificates() unexpected switch value " << retVal << std::endl;
			}
		}
		break;
	default:
		/* Unexpected return code */
		std::cerr << "RsInit::InitRetroShare unexpected return code " << initResult << std::endl;
		QMessageBox::warning(0, QObject::tr("RetroShare"), QObject::tr("An unexpected error occured. Please report 'RsInit::InitRetroShare unexpected return code %1'.").arg(initResult));
		return 1;
	}

        /* recreate global settings object, now with correct path */
	RshareSettings::Create(true);
	Rshare::resetLanguageAndStyle();

	splashScreen.showMessage(rshare.translate("SplashScreen", "Load configuration"), Qt::AlignHCenter | Qt::AlignBottom);

	rsicontrol->StartupRetroShare();
	Rshare::initPlugins();

	splashScreen.showMessage(rshare.translate("SplashScreen", "Create interface"), Qt::AlignHCenter | Qt::AlignBottom);

	RsharePeerSettings::Create();

#ifdef MINIMAL_RSGUI
	MessengerWindow::showYourself();

	rshare.setQuitOnLastWindowClosed(true);

	splashScreen.hide();
#else
	Emoticons::load();

	if (Settings->value(QString::fromUtf8("FirstRun"), true).toBool()) {
		splashScreen.hide();

		Settings->setValue(QString::fromUtf8("FirstRun"), false);
		QuickStartWizard qstartWizard;
		qstartWizard.exec();
	}

	MainWindow *w = MainWindow::Create ();
	splashScreen.finish(w);

	EventReceiver *eventReceiver = NULL;
	if (Settings->getRetroShareProtocol()) {
		/* Create event receiver */
		eventReceiver = new EventReceiver;
		if (eventReceiver->start()) {
			QObject::connect(eventReceiver, SIGNAL(linkReceived(const QUrl&)), w, SLOT(linkActivated(const QUrl&)));
		}
	}

	if (!url.empty()) {
		/* Now use link from the command line, because no RetroShare was running */
		RetroShareLink link(QString::fromStdString(url));
		if (link.valid()) {
			w->linkActivated(link.toUrl());
		}
	}

	// I'm using a signal to transfer the hashing info to the mainwindow, because Qt schedules signals properly to
	// avoid clashes between infos from threads.
	//

	qRegisterMetaType<FileDetail>("FileDetail") ;

	std::cerr << "connecting signals and slots" << std::endl ;
	QObject::connect(notify,SIGNAL(gotTurtleSearchResult(qulonglong,FileDetail)),w->searchDialog	,SLOT(updateFiles(qulonglong,FileDetail))) ;
	QObject::connect(notify,SIGNAL(diskFull(int,int))						,w                   		,SLOT(displayDiskSpaceWarning(int,int))) ;
	QObject::connect(notify,SIGNAL(filesPreModChanged(bool))          ,w->sharedfilesDialog		,SLOT(preModDirectories(bool)          )) ;
	QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w->sharedfilesDialog		,SLOT(postModDirectories(bool)         )) ;
	QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w                            ,SLOT(postModDirectories(bool)         )) ;
	QObject::connect(notify,SIGNAL(transfersChanged())                ,w->transfersDialog  		,SLOT(insertTransfers()                )) ;
	QObject::connect(notify,SIGNAL(publicChatChanged(int))            ,w->friendsDialog      		,SLOT(publicChatChanged(int)           ));
	QObject::connect(notify,SIGNAL(privateChatChanged(int, int))      ,w                   		,SLOT(privateChatChanged(int, int)     ));
	QObject::connect(notify,SIGNAL(neighboursChanged())               ,w->networkDialog    		,SLOT(insertConnect()                  )) ;
	QObject::connect(notify,SIGNAL(messagesChanged())                 ,w->messagesDialog   		,SLOT(insertMessages()                 )) ;
	QObject::connect(notify,SIGNAL(messagesTagsChanged())             ,w->messagesDialog   		,SLOT(messagesTagsChanged()            )) ;
	QObject::connect(notify,SIGNAL(messagesChanged())                 ,w                   		,SLOT(updateMessages()                 )) ;
	QObject::connect(notify,SIGNAL(forumsChanged())                   ,w                   		,SLOT(updateForums()                   ), Qt::QueuedConnection);
	QObject::connect(notify,SIGNAL(channelsChanged(int))              ,w                   		,SLOT(updateChannels(int)              ), Qt::QueuedConnection);
	QObject::connect(notify,SIGNAL(downloadCompleteCountChanged(int)) ,w                   		,SLOT(updateTransfers(int)             ));

	QObject::connect(notify,SIGNAL(chatStatusChanged(const QString&,const QString&,bool)),w->friendsDialog,SLOT(updatePeerStatusString(const QString&,const QString&,bool)));
	QObject::connect(notify,SIGNAL(ownStatusMessageChanged()),w->friendsDialog,SLOT(loadmypersonalstatus()));

	QObject::connect(notify,SIGNAL(logInfoChanged(const QString&))		,w->networkDialog,SLOT(setLogInfo(QString))) ;
	QObject::connect(notify,SIGNAL(discInfoChanged())						,w->networkDialog,SLOT(updateNewDiscoveryInfo()),Qt::QueuedConnection) ;
	QObject::connect(notify,SIGNAL(errorOccurred(int,int,const QString&)),w,SLOT(displayErrorMessage(int,int,const QString&))) ;

	w->installGroupChatNotifier();

	/* only show window, if not startMinimized */
	if (RsInit::getStartMinimised() || Settings->getStartMinimized())
	{
		splashScreen.close();
	} else {
		w->show();
	}

	/* Startup a Timer to keep the gui's updated */
	QTimer *timer = new QTimer(w);
	timer -> connect(timer, SIGNAL(timeout()), notify, SLOT(UpdateGUI()));
	timer->start(1000);
#endif // MINIMAL_RSGUI

	/* dive into the endless loop */
	int ti = rshare.exec();
#ifndef MINIMAL_RSGUI
	delete w ;

	if (eventReceiver) {
		/* Destroy event receiver */
		delete eventReceiver;
		eventReceiver = NULL;
	}

	/* cleanup */
	ChatDialog::cleanupChat();
#endif // MINIMAL_RSGUI

	rsicontrol->rsGlobalShutDown();

	Settings->sync();
	delete Settings;

	return ti ;
}
