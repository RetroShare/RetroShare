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
#include <rshare.h>
#ifndef MINIMAL_RSGUI
#include "gui/MainWindow.h"
#include "gui/PeersDialog.h"
#include "gui/SearchDialog.h"
#include "gui/TransfersDialog.h"
#include "gui/MessagesDialog.h"
#include "gui/SharedFilesDialog.h"
#include "gui/NetworkDialog.h"
#include "gui/chat/PopupChatDialog.h"
#endif // MINIMAL_RSGUI
#include "gui/MessengerWindow.h"
#include "gui/StartDialog.h"
#include "gui/GenCertDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/connect/ConfCertDialog.h"
#include "idle/idle.h"

/*** WINDOWS DON'T LIKE THIS - REDEFINES VER numbers.
#include <gui/qskinobject/qskinobject.h>
****/

#include <util/stringutil.h>
#include <retroshare/rsinit.h>
#include <retroshare/rsiface.h>
#include "gui/notifyqt.h"

int main(int argc, char *argv[])
{ 

  QStringList args = char_array_to_stringlist(argv+1, argc-1);
  
  Q_INIT_RESOURCE(images);

	rsiface = NULL;

	NotifyQt *notify = NotifyQt::Create();
	createRsIface(*notify);
	createRsControl(*rsiface, *notify);

        /* RetroShare Core Objects */
	RsInit::InitRsConfig();
	bool okStart = RsInit::InitRetroShare(argc, argv);

	/* create global settings object
	   path maybe wrong, when no profile exist
	   in this case it can be use only for default values */
	RshareSettings::Create ();

	/*
	Function RsConfigMinimised is not available in SVN, so I commented it out.
        bool startMinimised = RsConfigStartMinimised(config);
	*/

	//bool startMinimized = false;

	/* Setup The GUI Stuff */
	Rshare rshare(args, argc, argv, 
		QString::fromStdString(RsInit::RsConfigDirectory()));

	/* Login Dialog */
  	if (!okStart)
	{
		/* check for existing Certificate */
		std::string userName;

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
	}
	else
	{

            std::string preferredId, gpgId, gpgName, gpgEmail, sslName;
            RsInit::getPreferedAccountId(preferredId);

            if (RsInit::getAccountDetails(preferredId,
                            gpgId, gpgName, gpgEmail, sslName))
            {
                    RsInit::SelectGPGAccount(gpgId);
            }

            // true: note auto-login is active
			int retVal = RsInit::LockAndLoadCertificates(true);
			switch(retVal)
			{
				case 0:	break;
				case 1:	QMessageBox::warning(	0,
												QObject::tr("Multiple instances"),
												QObject::tr("Another RetroShare using the same profile is "
															"already running on your system. Please close "
															"that instance first") );
						return 1;
				case 2:	QMessageBox::critical(	0,
												QObject::tr("Multiple instances"),
												QObject::tr("An unexpected error occurred when Retroshare"
															"tried to acquire the single instance lock") );
						return 1;
				case 3: QMessageBox::critical(	0,
												QObject::tr("Login Failure"),
												QObject::tr("Maybe password is wrong") );
						return 1;
				default: std::cerr << "StartDialog::loadCertificates() unexpected switch value " << retVal << std::endl;
			}
	}

	rsicontrol->StartupRetroShare();

	/* recreate global settings object, now with correct path */
	RshareSettings::Create ();

#ifdef MINIMAL_RSGUI
	MessengerWindow::showYourself();

	rshare.setQuitOnLastWindowClosed(true);
#else
	MainWindow *w = MainWindow::Create ();

	// I'm using a signal to transfer the hashing info to the mainwindow, because Qt schedules signals properly to
	// avoid clashes between infos from threads.
	//

	qRegisterMetaType<FileDetail>("FileDetail") ;

	std::cerr << "connecting signals and slots" << std::endl ;
	QObject::connect(notify,SIGNAL(gotTurtleSearchResult(qulonglong,FileDetail)),w->searchDialog	,SLOT(updateFiles(qulonglong,FileDetail))) ;
	QObject::connect(notify,SIGNAL(hashingInfoChanged(const QString&)),w                   		,SLOT(updateHashingInfo(const QString&))) ;
	QObject::connect(notify,SIGNAL(diskFull(int,int))						,w                   		,SLOT(displayDiskSpaceWarning(int,int))) ;
	QObject::connect(notify,SIGNAL(filesPreModChanged(bool))          ,w->sharedfilesDialog		,SLOT(preModDirectories(bool)          )) ;
	QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w->sharedfilesDialog		,SLOT(postModDirectories(bool)         )) ;
	QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w                            ,SLOT(postModDirectories(bool)         )) ;
	QObject::connect(notify,SIGNAL(transfersChanged())                ,w->transfersDialog  		,SLOT(insertTransfers()                )) ;
	QObject::connect(notify,SIGNAL(friendsChanged())                  ,w->peersDialog      		,SLOT(insertPeers()                    )) ;
	QObject::connect(notify,SIGNAL(publicChatChanged(int))            ,w->peersDialog      		,SLOT(publicChatChanged(int)           ));
	QObject::connect(notify,SIGNAL(privateChatChanged(int))           ,w                   		,SLOT(privateChatChanged(int)          ));
	QObject::connect(notify,SIGNAL(neighborsChanged())                ,w->networkDialog    		,SLOT(insertConnect()                  )) ;
	QObject::connect(notify,SIGNAL(messagesChanged())                 ,w->messagesDialog   		,SLOT(insertMessages()                 )) ;
	QObject::connect(notify,SIGNAL(messagesTagsChanged())             ,w->messagesDialog   		,SLOT(messagesTagsChanged()            )) ;
	QObject::connect(notify,SIGNAL(messagesChanged())                 ,w                   		,SLOT(updateMessages()                 )) ;
	QObject::connect(notify,SIGNAL(forumsChanged())                   ,w                   		,SLOT(updateForums()                   ), Qt::QueuedConnection);

	QObject::connect(notify,SIGNAL(chatStatusChanged(const QString&,const QString&,bool)),w->peersDialog,SLOT(updatePeerStatusString(const QString&,const QString&,bool)));
	QObject::connect(notify,SIGNAL(peerHasNewAvatar(const QString&)),w->peersDialog,SLOT(updatePeersAvatar(const QString&)));
	QObject::connect(notify,SIGNAL(ownAvatarChanged()),w->peersDialog,SLOT(updateAvatar()));
	QObject::connect(notify,SIGNAL(ownStatusMessageChanged()),w->peersDialog,SLOT(loadmypersonalstatus()));

	QObject::connect(notify,SIGNAL(logInfoChanged(const QString&)),w->networkDialog,SLOT(setLogInfo(QString))) ;
	QObject::connect(notify,SIGNAL(errorOccurred(int,int,const QString&)),w,SLOT(displayErrorMessage(int,int,const QString&))) ;

	QObject::connect(ConfCertDialog::instance(),SIGNAL(configChanged()),w->networkDialog,SLOT(insertConnect())) ;
	QObject::connect(w->peersDialog,SIGNAL(friendsUpdated()),w->networkDialog,SLOT(insertConnect())) ;

	w->installGroupChatNotifier();

	/* only show window, if not startMinimized */
	if(!Settings->value(QString::fromUtf8("StartMinimized"), false).toBool())
	{
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

	/* cleanup */
	PopupChatDialog::cleanupChat();
#endif // MINIMAL_RSGUI

	rsicontrol->rsGlobalShutDown();

	Settings->sync();
	delete Settings;

	return ti ;
}
