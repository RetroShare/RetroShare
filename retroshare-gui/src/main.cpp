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
#include "gui/MainWindow.h"
#include "gui/FriendsDialog.h"
#include "gui/SearchDialog.h"
#include "gui/TransfersDialog.h"
#include "gui/MessagesDialog.h"
#include "gui/SharedFilesDialog.h"
#include "gui/NetworkDialog.h"
#include "gui/chat/ChatDialog.h"
#include "gui/QuickStartWizard.h"
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
#include "gui/SoundManager.h"
#include "gui/NetworkView.h"
#include "lang/languagesupport.h"

/*** WINDOWS DON'T LIKE THIS - REDEFINES VER numbers.
#include <gui/qskinobject/qskinobject.h>
****/

#include <util/stringutil.h>
#include <retroshare/rsinit.h>
#include <retroshare/rsiface.h>
#include "gui/notifyqt.h"
#include <unistd.h>

static void displayWarningAboutDSAKeys()
{
	if(RsInit::unsupported_keys.empty())
		return ;

	QMessageBox msgBox;

	QString txt = QObject::tr("You appear to have locations associated to DSA keys:");
	txt += "<UL>" ;
	for(std::map<std::string,std::vector<std::string> >::const_iterator it(RsInit::unsupported_keys.begin());it!=RsInit::unsupported_keys.end();++it)
	{
		txt += "<LI>" + QString::fromStdString(it->first) ;
		txt += "<UL>" ;

		for(uint32_t i=0;i<it->second.size();++i)
			txt += "<li>" + QString::fromStdString(it->second[i]) + "</li>" ;

		txt += "</UL>" ;
		txt += "</li>" ;
	}
	txt += "</UL>" ;

	msgBox.setText(txt) ;
	msgBox.setInformativeText(QObject::tr("DSA keys are not yet supported by this version of RetroShare. All these locations will be unusable. We're very sorry for that."));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
	msgBox.setWindowIcon(QIcon(":/images/rstray3.png"));

	msgBox.exec();
}

int main(int argc, char *argv[])
{ 
#ifdef WINDOWS_SYS
	// The current directory of the application is changed when using the native dialog on Windows
	// This is a quick fix until libretroshare is using a absolute path in the portable Version
	extern bool Q_GUI_EXPORT qt_use_native_dialogs;
	qt_use_native_dialogs = false;

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

	if(initResult == RS_INIT_NO_KEYRING)	// happens when we already have accounts, but no pgp key. This is when switching to the openpgp-sdk version.
	{
		QApplication dummyApp (argc, argv); // needed for QMessageBox
		/* Translate into the desired language */
		LanguageSupport::translate(LanguageSupport::defaultLanguageCode());

		QMessageBox msgBox;
		msgBox.setText(QObject::tr("This version of RetroShare is using OpenPGP-SDK. As a side effect, it's not using the system shared PGP keyring, but has it's own keyring shared by all RetroShare instances. <br><br>You do not appear to have such a keyring, although PGP keys are mentioned by existing RetroShare accounts, probably because you just changed to this new version of the software."));
		msgBox.setInformativeText(QObject::tr("Choose between:<br><ul><li><b>Ok</b> to copy the existing keyring from gnupg (safest bet), or </li><li><b>Close without saving</b> to start fresh with an empty keyring (you will be asked to create a new PGP key to work with RetroShare, or import a previously saved pgp keypair). </li><li><b>Cancel</b> to quit and forge a keyring by yourself (needs some PGP skills)</li></ul>"));
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Discard | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.setWindowIcon(QIcon(":/images/rstray3.png"));

		int ret = msgBox.exec();

		if(ret == QMessageBox::Cancel)
			return 0 ;
		if(ret == QMessageBox::Ok)
		{
			if(!RsInit::copyGnuPGKeyrings())
				return 0 ; 

			initResult = RsInit::InitRetroShare(argc, argv);

			displayWarningAboutDSAKeys() ;

		}
		else
			initResult = RS_INIT_OK ;
	}

	if (initResult < 0) {
		/* Error occured */
		QApplication dummyApp (argc, argv); // needed for QMessageBox
		/* Translate into the desired language */
		LanguageSupport::translate(LanguageSupport::defaultLanguageCode());

		displayWarningAboutDSAKeys();

		QMessageBox mb(QMessageBox::Critical, QObject::tr("RetroShare"), "", QMessageBox::Ok);
		mb.setWindowIcon(QIcon(":/images/rstray3.png"));

		switch (initResult) 
		{
			case RS_INIT_AUTH_FAILED:
				std::cerr << "RsInit::InitRetroShare AuthGPG::InitAuth failed" << std::endl;
				mb.setText(QObject::tr("Initialization failed. Wrong or missing installation of PGP."));
				break;
			default:
				/* Unexpected return code */
				std::cerr << "RsInit::InitRetroShare unexpected return code " << initResult << std::endl;
				mb.setText(QObject::tr("An unexpected error occurred. Please report 'RsInit::InitRetroShare unexpected return code %1'.").arg(initResult));
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
			bool genCert = false;
			std::list<std::string> accountIds;
			if (RsInit::getAccountIds(accountIds) && (accountIds.size() > 0))
			{
				StartDialog sd;
				if (sd.exec() == QDialog::Rejected) {
					return 1;
				}

				/* if we're logged in */
				genCert = sd.requestedNewCert();
			}
			else
			{
				genCert = true;
			}

			if (genCert)
			{
				GenCertDialog gd(false);
				if (gd.exec () == QDialog::Rejected) {
					return 1;
				}
			}

			splashScreen.show();
		}
		break;
	case RS_INIT_HAVE_ACCOUNT:
		{
			splashScreen.show();
			splashScreen.showMessage(rshare.translate("SplashScreen", "Load profile"), Qt::AlignHCenter | Qt::AlignBottom);

			std::string preferredId;
			RsInit::getPreferedAccountId(preferredId);

			// true: note auto-login is active
			Rshare::loadCertificate(preferredId, true);
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

	SoundManager::create();

	splashScreen.showMessage(rshare.translate("SplashScreen", "Load configuration"), Qt::AlignHCenter | Qt::AlignBottom);

	/* stop Retroshare if startup fails */
	if (!rsicontrol->StartupRetroShare())
	{
		std::cerr << "libretroshare failed to startup!" << std::endl;
		return 1;
	}


	Rshare::initPlugins();

	splashScreen.showMessage(rshare.translate("SplashScreen", "Create interface"), Qt::AlignHCenter | Qt::AlignBottom);

	RsharePeerSettings::Create();

	Emoticons::load();

	if (Settings->value(QString::fromUtf8("FirstRun"), true).toBool()) {
		splashScreen.hide();

		Settings->setValue(QString::fromUtf8("FirstRun"), false);

#ifdef __APPLE__
		/* For OSX, we set the default to "cleanlooks", as the AQUA style hides some input boxes 
		 * only on the first run - as the user might want to change it ;)
		 */
		QString osx_style("cleanlooks");
		Rshare::setStyle(osx_style);
		Settings->setInterfaceStyle(osx_style);
#endif

// This is now disabled - as it doesn't add very much.
// Need to make sure that defaults are sensible!
#ifdef ENABLE_QUICKSTART_WIZARD
		QuickStartWizard qstartWizard;
		qstartWizard.exec();
#endif

	}

	MainWindow *w = MainWindow::Create ();
	splashScreen.finish(w);

	EventReceiver *eventReceiver = NULL;
	if (Settings->getRetroShareProtocol()) {
		/* Create event receiver */
		eventReceiver = new EventReceiver;
		if (eventReceiver->start()) {
			QObject::connect(eventReceiver, SIGNAL(linkReceived(const QUrl&)), w, SLOT(retroshareLinkActivated(const QUrl&)));
		}
	}

	if (!url.empty()) {
		/* Now use link from the command line, because no RetroShare was running */
		RetroShareLink link(QString::fromStdString(url));
		if (link.valid()) {
			w->retroshareLinkActivated(link.toUrl());
		}
	}

	// I'm using a signal to transfer the hashing info to the mainwindow, because Qt schedules signals properly to
	// avoid clashes between infos from threads.
	//

	qRegisterMetaType<FileDetail>("FileDetail") ;

	std::cerr << "connecting signals and slots" << std::endl ;
	QObject::connect(notify,SIGNAL(gotTurtleSearchResult(qulonglong,FileDetail)),w->transfersDialog->searchDialog	,SLOT(updateFiles(qulonglong,FileDetail))) ;
	QObject::connect(notify,SIGNAL(deferredSignatureHandlingRequested()),notify,SLOT(handleSignatureEvent()),Qt::QueuedConnection) ;
	QObject::connect(notify,SIGNAL(chatLobbyTimeShift(int)),notify,SLOT(handleChatLobbyTimeShift(int)),Qt::QueuedConnection) ;
	QObject::connect(notify,SIGNAL(diskFull(int,int))						,w                   		,SLOT(displayDiskSpaceWarning(int,int))) ;
	QObject::connect(notify,SIGNAL(filesPreModChanged(bool))          ,w->transfersDialog->localSharedFiles			,SLOT(preModDirectories(bool)          )) ;
	QObject::connect(notify,SIGNAL(filesPreModChanged(bool))          ,w->transfersDialog->remoteSharedFiles		,SLOT(preModDirectories(bool)          )) ;
	QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w->transfersDialog->localSharedFiles			,SLOT(postModDirectories(bool)         )) ;
	QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w->transfersDialog->remoteSharedFiles		,SLOT(postModDirectories(bool)         )) ;
	QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w                         ,SLOT(postModDirectories(bool)         )) ;
	QObject::connect(notify,SIGNAL(transfersChanged())                ,w->transfersDialog  		,SLOT(insertTransfers()                )) ;
	QObject::connect(notify,SIGNAL(publicChatChanged(int))            ,w->friendsDialog      		,SLOT(publicChatChanged(int)           ));
	QObject::connect(notify,SIGNAL(neighboursChanged())               ,w->friendsDialog->networkDialog    		,SLOT(insertConnect()                  )) ;
	QObject::connect(notify,SIGNAL(messagesChanged())                 ,w->messagesDialog   		,SLOT(insertMessages()                 )) ;
	QObject::connect(notify,SIGNAL(messagesTagsChanged())             ,w->messagesDialog   		,SLOT(messagesTagsChanged()            )) ;

	QObject::connect(notify,SIGNAL(chatStatusChanged(const QString&,const QString&,bool)),w->friendsDialog,SLOT(updatePeerStatusString(const QString&,const QString&,bool)));
	QObject::connect(notify,SIGNAL(ownStatusMessageChanged()),w->friendsDialog,SLOT(loadmypersonalstatus()));

	QObject::connect(notify,SIGNAL(logInfoChanged(const QString&))		,w->friendsDialog->networkDialog,SLOT(setLogInfo(QString))) ;
	QObject::connect(notify,SIGNAL(discInfoChanged())						,w->friendsDialog->networkView,SLOT(update()),Qt::QueuedConnection) ;
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

	notify->enable() ;	// enable notification system after GUI creation, to avoid data races in Qt.

	/* dive into the endless loop */
	int ti = rshare.exec();
	delete w ;

	if (eventReceiver) {
		/* Destroy event receiver */
		delete eventReceiver;
		eventReceiver = NULL;
	}

	/* cleanup */
	ChatDialog::cleanupChat();

	rsicontrol->rsGlobalShutDown();

	delete(soundManager);
	soundManager = NULL;

	Settings->sync();
	delete(Settings);

	return ti ;
}
