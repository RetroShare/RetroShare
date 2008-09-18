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
#include <gui/Preferences/rsharesettings.h>

/*** WINDOWS DON'T LIKE THIS - REDEFINES VER numbers.
#include <gui/qskinobject/qskinobject.h>
****/

//#include <util/process.h>
#include <util/stringutil.h>
#include "rsiface/rsiface.h"

#include "rsiface/notifyqt.h"

RsIface   *rsiface    = NULL;
RsControl *rsicontrol = NULL;

int main(int argc, char *argv[])
{ 

  QStringList args = char_array_to_stringlist(argv+1, argc-1);
  
  Q_INIT_RESOURCE(images);

	rsiface = NULL;

        /* RetroShare Core Objects */
        RsInit *config = InitRsConfig();
        bool okStart = InitRetroShare(argc, argv, config);
	
	/*
	Function RsConfigMinimised is not available in SVN, so I commented it out.
        bool startMinimised = RsConfigStartMinimised(config);
	*/

	bool startMinimised = false;

	/* Setup The GUI Stuff */
	Rshare rshare(args, argc, argv, 
		QString(RsConfigDirectory(config)));

	/* Login Dialog */
  	if (!okStart)
	{
                /* check for existing Certificate */
                std::string userName;
   
   		StartDialog *sd = NULL;
		bool genCert = false;
                if (ValidateCertificate(config, userName))
		{
		  sd = new StartDialog(config);
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
		  GenCertDialog *gd = new GenCertDialog(config);

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
		/* don't save auto login details */
        	LoadCertificates(config, false);
	}

        NotifyQt   *notify = new NotifyQt();
        RsIface *iface = createRsIface(*notify);
        RsControl *rsServer = createRsControl(*iface, *notify);

        notify->setRsIface(iface);

	/* save to the global variables */
	rsiface = iface;
	rsicontrol = rsServer;

        rsServer -> StartupRetroShare(config);
        CleanupRsConfig(config);

  	MainWindow *w = new MainWindow;
	//QMainWindow *skinWindow = new QMainWindow();
	
	//skinWindow->resize(w->size().width()+15,w->size().width()+15);
	//skinWindow->setWindowTitle(w->windowTitle());
	//skinWindow->setCentralWidget(w);
	
	/* Attach the Dialogs, to the Notify Class */
        notify->setNetworkDialog(w->networkDialog);
        notify->setPeersDialog(w->peersDialog);
        notify->setDirDialog(w->sharedfilesDialog);
        notify->setTransfersDialog(w->transfersDialog);
        notify->setChatDialog(w->chatDialog);
        notify->setMessagesDialog(w->messagesDialog);
        notify->setChannelsDialog(w->channelsDialog);
        notify->setMessengerWindow(w->messengerWindow);

	/* only show window, if not startMinimized */
  	/*if (!startMinimised)
	{
		
		w->show();
		//skinWindow->show();
		
	}*/
   
    /* Run Retroshare */
   //int ret = rshare.run();

	/* Startup a Timer to keep the gui's updated */
	QTimer *timer = new QTimer(w);
	timer -> connect(timer, SIGNAL(timeout()), notify, SLOT(UpdateGUI()));
        timer->start(1000);
	
  /* dive into the endless loop */
  // return ret;
  return rshare.exec();

}





