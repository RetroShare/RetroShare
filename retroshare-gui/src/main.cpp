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

#include <QtGui>
#include <QObject>
#include <rshare.h>
#include <gui/MainWindow.h>
#include <gui/StartDialog.h>
#include <gui/GenCertDialog.h>
#include <gui/settings/rsharesettings.h>
#include <gui/connect/ConfCertDialog.h>

/*** WINDOWS DON'T LIKE THIS - REDEFINES VER numbers.
#include <gui/qskinobject/qskinobject.h>
****/

#include <util/stringutil.h>
#include "rsiface/rsinit.h"
#include "rsiface/rsiface.h"
#include "gui/notifyqt.h"

int main(int argc, char *argv[])
{ 

  QStringList args = char_array_to_stringlist(argv+1, argc-1);
  
  Q_INIT_RESOURCE(images);

	rsiface = NULL;

	NotifyQt   *notify = new NotifyQt();
	createRsIface(*notify);
	createRsControl(*rsiface, *notify);

        /* RetroShare Core Objects */
	RsInit::InitRsConfig();
	bool okStart = RsInit::InitRetroShare(argc, argv);
	
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
		}
		else
		{
			genCert = true;
		}

		if (genCert)
		{
			GenCertDialog *gd = new GenCertDialog();

			gd->show();

			while(gd -> isVisible())
			{
				rshare.processEvents();
#ifdef WIN32
				Sleep(10);
#else // __LINUX__
				usleep(10000);
#endif
			}
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
            RsInit::LoadCertificates(true);
	}

	rsicontrol->StartupRetroShare();

	MainWindow *w = new MainWindow;

	// I'm using a signal to transfer the hashing info to the mainwindow, because Qt schedules signals properly to
	// avoid clashes between infos from threads.
	//

	qRegisterMetaType<FileDetail>("FileDetail") ;

	std::cerr << "connecting signals and slots" << std::endl ;
	QObject::connect(notify,SIGNAL(gotTurtleSearchResult(qulonglong,FileDetail)),w->searchDialog	,SLOT(updateFiles(qulonglong,FileDetail))) ;
	QObject::connect(notify,SIGNAL(hashingInfoChanged(const QString&)),w                   		,SLOT(updateHashingInfo(const QString&))) ;
	QObject::connect(notify,SIGNAL(filesPreModChanged(bool))          ,w->sharedfilesDialog		,SLOT(preModDirectories(bool)          )) ;
	QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w->sharedfilesDialog		,SLOT(postModDirectories(bool)         )) ;
	QObject::connect(notify,SIGNAL(transfersChanged())                ,w->transfersDialog  		,SLOT(insertTransfers()                )) ;
	QObject::connect(notify,SIGNAL(friendsChanged())                  ,w->messengerWindow  		,SLOT(insertPeers()                    )) ;
	QObject::connect(notify,SIGNAL(friendsChanged())                  ,w->peersDialog      		,SLOT(insertPeers()                    )) ;
	QObject::connect(notify,SIGNAL(neighborsChanged())                ,w->networkDialog    		,SLOT(insertConnect()                  )) ;
	QObject::connect(notify,SIGNAL(messagesChanged())                 ,w->messagesDialog   		,SLOT(insertMessages()                 )) ;
	QObject::connect(notify,SIGNAL(configChanged())                   ,w->messagesDialog   		,SLOT(displayConfig()                  )) ;

	QObject::connect(notify,SIGNAL(chatStatusChanged(const QString&,const QString&,bool)),w->peersDialog,SLOT(updatePeerStatusString(const QString&,const QString&,bool)));
	QObject::connect(notify,SIGNAL(peerHasNewCustomStateString(const QString&)),w->peersDialog,SLOT(updatePeersCustomStateString(const QString&)));
	QObject::connect(notify,SIGNAL(peerHasNewAvatar(const QString&)),w->peersDialog,SLOT(updatePeersAvatar(const QString&)));
	QObject::connect(notify,SIGNAL(ownAvatarChanged()),w->peersDialog,SLOT(updateAvatar()));
	QObject::connect(notify,SIGNAL(ownAvatarChanged()),w->messengerWindow,SLOT(updateAvatar()));
	QObject::connect(notify,SIGNAL(ownStatusMessageChanged()),w->peersDialog,SLOT(loadmypersonalstatus()));
	QObject::connect(notify,SIGNAL(ownStatusMessageChanged()),w->messengerWindow,SLOT(loadmystatusmessage()));

	QObject::connect(notify,SIGNAL(logInfoChanged(const QString&)),w->networkDialog,SLOT(setLogInfo(QString))) ;
	QObject::connect(notify,SIGNAL(errorOccurred(int,int,const QString&)),w,SLOT(displayErrorMessage(int,int,const QString&))) ;

	QObject::connect(ConfCertDialog::instance(),SIGNAL(configChanged()),w->networkDialog,SLOT(insertConnect())) ;
	QObject::connect(w->peersDialog,SIGNAL(friendsUpdated()),w->networkDialog,SLOT(insertConnect())) ;
	QObject::connect(w->peersDialog,SIGNAL(notifyGroupChat(const QString&,const QString&)),w,SLOT(displaySystrayMsg(const QString&,const QString&))) ;

	/* only show window, if not startMinimized */
	RshareSettings  *_settings = new RshareSettings();

  if(!_settings->value(QString::fromUtf8("StartMinimized"), false).toBool()) 
	{

		w->show();
	}

	/* Startup a Timer to keep the gui's updated */
	QTimer *timer = new QTimer(w);
	timer -> connect(timer, SIGNAL(timeout()), notify, SLOT(UpdateGUI()));
	timer->start(1000);

	/* dive into the endless loop */
	// return ret;
    int ti = rshare.exec();
    delete w ;
	return ti ;

}





