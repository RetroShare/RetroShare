/*******************************************************************************
 * retroshare-gui/src/: main.cpp                                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2006 by Crypton <retroshare@lunamutt.com>                         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "util/stacktrace.h"
#include "util/argstream.h"

CrashStackTrace gCrashStackTrace;

#include <QObject>
#include <QMessageBox>
#include <QSplashScreen>

#include <rshare.h>
#include "gui/common/FilesDefs.h"
#include "gui/FriendsDialog.h"
#include "gui/GenCertDialog.h"
#include "gui/MainWindow.h"
#include "gui/NetworkDialog.h"
#include "gui/NetworkView.h"
#include "gui/QuickStartWizard.h"
#include "gui/RetroShareLink.h"
#include "gui/SoundManager.h"
#include "gui/StartDialog.h"
#include "gui/chat/ChatDialog.h"
#include "gui/connect/ConfCertDialog.h"
#include "gui/common/Emoticons.h"
#include "gui/FileTransfer/SearchDialog.h"
#include "gui/FileTransfer/TransfersDialog.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/settings/rsharesettings.h"
#include "idle/idle.h"
#include "lang/languagesupport.h"
#include "util/RsGxsUpdateBroadcast.h"
#include "util/rsdir.h"
#include "util/rstime.h"
#include "retroshare/rsinit.h"

#ifdef MESSENGER_WINDOW
#include "gui/MessengerWindow.h"
#endif
#ifdef RS_WEBUI
#	include "gui/settings/WebuiPage.h"
#endif

#ifdef RS_JSONAPI
#	include "gui/settings/JsonApiPage.h"
#endif // RS_JSONAPI

#include "TorControl/TorManager.h"
#include "TorControl/TorControlWindow.h"

#include "retroshare/rsidentity.h"
#include "retroshare/rspeers.h"

#ifdef SIGFPE_DEBUG
#include <fenv.h>
#endif

#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
#ifdef WINDOWS_SYS
#include <QFileDialog>
#endif
#endif

#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK (5, 7, 0)
// see INSTALL.W32 in openssl
// Needed as workaround for gcc 5.3.0 because the call to GetProcAddress in cryptlib.c finds an function pointer
// to the not existing function _OPENSSL_isservice in the executable running on Windows 7.
extern "C" {
__declspec(dllexport) __cdecl BOOL _OPENSSL_isservice(void)
{
	DWORD sess;
	if (ProcessIdToSessionId(GetCurrentProcessId(),&sess))
		return sess==0;
	return FALSE;
}
}
#endif
#endif

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
	std::map<std::string,std::vector<std::string> > unsupported_keys;
	RsAccounts::GetUnsupportedKeys(unsupported_keys);
	
	if(unsupported_keys.empty())
		return ;

	QMessageBox msgBox;

	QString txt = QObject::tr("You appear to have nodes associated to DSA keys:");
	txt += "<UL>" ;
	for(std::map<std::string,std::vector<std::string> >::const_iterator it(unsupported_keys.begin());it!=unsupported_keys.end();++it)
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
	msgBox.setInformativeText(QObject::tr("DSA keys are not yet supported by this version of RetroShare. All these nodes will be unusable. We're very sorry for that."));
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setWindowIcon(FilesDefs::getIconFromQtResourcePath(":/icons/logo_128.png"));

	msgBox.exec();
}

#ifdef WINDOWS_SYS
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0) && QT_VERSION < QT_VERSION_CHECK (5, 3, 0)
QStringList filedialog_open_filenames_hook(QWidget * parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options)
{
	return QFileDialog::getOpenFileNames(parent, caption, dir, filter, selectedFilter, options | QFileDialog::DontUseNativeDialog);
}

QString filedialog_open_filename_hook(QWidget * parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options)
{
	return QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options | QFileDialog::DontUseNativeDialog);
}

QString filedialog_save_filename_hook(QWidget * parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options)
{
	return QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options | QFileDialog::DontUseNativeDialog);
}

QString filedialog_existing_directory_hook(QWidget *parent, const QString &caption, const QString &dir, QFileDialog::Options options)
{
	return QFileDialog::getExistingDirectory(parent, caption, dir, options | QFileDialog::DontUseNativeDialog);
}
#endif
#endif

int main(int argc, char *argv[])
{ 
#ifdef WINDOWS_SYS
	// The current directory of the application is changed when using the native dialog on Windows
	// This is a quick fix until libretroshare is using a absolute path in the portable Version
#if QT_VERSION >= QT_VERSION_CHECK (5, 3, 0)
	// Do we need a solution in v0.6?
#endif
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0) && QT_VERSION < QT_VERSION_CHECK (5, 3, 0)
	typedef QStringList (*_qt_filedialog_open_filenames_hook)(QWidget * parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options);
	typedef QString (*_qt_filedialog_open_filename_hook)     (QWidget * parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options);
	typedef QString (*_qt_filedialog_save_filename_hook)     (QWidget * parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options);
	typedef QString (*_qt_filedialog_existing_directory_hook)(QWidget *parent, const QString &caption, const QString &dir, QFileDialog::Options options);

	extern Q_GUI_EXPORT _qt_filedialog_open_filename_hook qt_filedialog_open_filename_hook;
	extern Q_GUI_EXPORT _qt_filedialog_open_filenames_hook qt_filedialog_open_filenames_hook;
	extern Q_GUI_EXPORT _qt_filedialog_save_filename_hook qt_filedialog_save_filename_hook;
	extern Q_GUI_EXPORT _qt_filedialog_existing_directory_hook qt_filedialog_existing_directory_hook;

	qt_filedialog_open_filename_hook = filedialog_open_filename_hook;
	qt_filedialog_open_filenames_hook = filedialog_open_filenames_hook;
	qt_filedialog_save_filename_hook = filedialog_save_filename_hook;
	qt_filedialog_existing_directory_hook = filedialog_existing_directory_hook;
#endif
#if QT_VERSION < QT_VERSION_CHECK (5, 0, 0)
	extern bool Q_GUI_EXPORT qt_use_native_dialogs;
	qt_use_native_dialogs = false;
#endif

	{
		/* Set the current directory to the application dir,
		   because the start dir with autostart from the registry run key is not the exe dir */
		QApplication app(argc, argv);
		QDir::setCurrent(QCoreApplication::applicationDirPath());
	}
#endif
#ifdef SIGFPE_DEBUG
feenableexcept(FE_INVALID | FE_DIVBYZERO);
#endif

	QStringList args = char_array_to_stringlist(argv+1, argc-1);

    Q_INIT_RESOURCE(images);
    Q_INIT_RESOURCE(icons);

	// Loop through all command-line args/values to get help wanted before RsInit exit.
	for (int i = 0; i < args.size(); ++i) {
		QString arg;
		/* Get the argument name and set a blank value */
		arg = args.at(i);
		if ((arg.toLower() == "-h") || (arg.toLower() == "--help")) {
			QApplication dummyApp (argc, argv); // needed for QMessageBox
			Rshare::showUsageMessageBox();
		}
	}

	// This is needed to allocate rsNotify, so that it can be used to ask for PGP passphrase
	//
	RsControl::earlyInitNotificationSystem() ;

	NotifyQt *notify = NotifyQt::Create();
	rsNotify->registerNotifyClient(notify);

	/* RetroShare Core Objects */
	RsInit::InitRsConfig();

    RsConfigOptions conf;

	argstream as(argc,argv);
	as      >> option('s',"stderr"           ,conf.outStderr      ,"output to stderr instead of log file."    )
	        >> option('u',"udp"              ,conf.udpListenerOnly,"Only listen to UDP."                      )
	        >> parameter('c',"base-dir"      ,conf.optBaseDir     ,"directory", "Set base directory."                                         ,false)
	        >> parameter('l',"log-file"      ,conf.logfname       ,"logfile"   ,"Set Log filename."                                           ,false)
	        >> parameter('d',"debug-level"   ,conf.debugLevel     ,"level"     ,"Set debug level."                                            ,false)
	        >> parameter('i',"ip-address"    ,conf.forcedInetAddress,"nnn.nnn.nnn.nnn", "Force IP address to use (if cannot be detected)."    ,false)
	        >> parameter('p',"port"          ,conf.forcedPort     ,"port"      ,"Set listenning port to use."                                 ,false)
	        >> parameter('o',"opmode"        ,conf.opModeStr      ,"opmode"    ,"Set Operating mode (Full, NoTurtle, Gaming, Minimal)."       ,false);
#ifdef RS_JSONAPI
	as      >> parameter('J', "jsonApiPort", conf.jsonApiPort, "jsonApiPort", "Enable JSON API on the specified port", false )
	        >> parameter('P', "jsonApiBindAddress", conf.jsonApiBindAddress, "jsonApiBindAddress", "JSON API Bind Address.", false);
#endif // ifdef RS_JSONAPI

#ifdef LOCALNET_TESTING
	as      >> parameter('R',"restrict-port" ,portRestrictions             ,"port1-port2","Apply port restriction"                   ,false);
#endif // ifdef LOCALNET_TESTING

#ifdef RS_AUTOLOGIN
	as      >> option('a',"auto-login"       ,conf.autoLogin      ,"AutoLogin (Windows Only) + StartMinimised");
#endif // ifdef RS_AUTOLOGIN

    conf.main_executable_path = argv[0];

	int initResult = RsInit::InitRetroShare(conf);

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
        msgBox.setWindowIcon(FilesDefs::getIconFromQtResourcePath(":/icons/logo_128.png"));

		int ret = msgBox.exec();

		if(ret == QMessageBox::Cancel)
			return 0 ;
		if(ret == QMessageBox::Ok)
		{
			if(!RsAccounts::CopyGnuPGKeyrings())
				return 0 ; 

			initResult = RsInit::InitRetroShare(conf);

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
        mb.setWindowIcon(FilesDefs::getIconFromQtResourcePath(":/icons/logo_128.png"));

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
	Rshare rshare(args, argc, argv,  QString::fromUtf8(RsAccounts::ConfigDirectory().c_str()));

	/* Start RetroShare */
	QString sDefaultGXSIdToCreate = "";
	switch (initResult) {
	case RS_INIT_OK:
		{
			/* Login Dialog */
			/* check for existing Certificate */
			bool genCert = false;
			std::list<RsPeerId> accountIds;
			if (RsAccounts::GetAccountIds(accountIds) && (accountIds.size() > 0))
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

				if (gd.exec () == QDialog::Rejected)
					return 1;

				sDefaultGXSIdToCreate = gd.getGXSNickname();
			}

			//splashScreen.show();
		}
		break;
	case RS_INIT_HAVE_ACCOUNT:
		{
			//splashScreen.show();
			//splashScreen.showMessage(rshare.translate("SplashScreen", "Load profile"), Qt::AlignHCenter | Qt::AlignBottom);

			RsPeerId preferredId;
			RsAccounts::GetPreferredAccountId(preferredId);

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

    bool is_hidden_node = false;
    bool is_auto_tor = false ;
    bool is_first_time = false ;

    RsAccounts::getCurrentAccountOptions(is_hidden_node,is_auto_tor,is_first_time);

    if(is_auto_tor)
	{
		// Now that we know the Tor service running, and we know the SSL id, we can make sure it provides a viable hidden service

		QString tor_hidden_service_dir = QString::fromStdString(RsAccounts::AccountDirectory()) + QString("/hidden_service/") ;

		Tor::TorManager *torManager = Tor::TorManager::instance();
		torManager->setTorDataDirectory(Rshare::dataDirectory() + QString("/tor/"));
		torManager->setHiddenServiceDirectory(tor_hidden_service_dir);	// re-set it, because now it's changed to the specific location that is run

		RsDirUtil::checkCreateDirectory(std::string(tor_hidden_service_dir.toUtf8())) ;

		torManager->setupHiddenService();

		if(! torManager->start() || torManager->hasError())
		{
			QMessageBox::critical(NULL,QObject::tr("Cannot start Tor Manager!"),QObject::tr("Tor cannot be started on your system: \n\n")+torManager->errorMessage()) ;
			return 1 ;
		}

		{
			TorControlDialog tcd(torManager) ;
			QString error_msg ;
			tcd.show();

			while(tcd.checkForTor(error_msg) != TorControlDialog::TOR_STATUS_OK || tcd.checkForHiddenService() != TorControlDialog::HIDDEN_SERVICE_STATUS_OK)	// runs until some status is reached: either tor works, or it fails.
			{
				QCoreApplication::processEvents();
				rstime::rs_usleep(0.2*1000*1000) ;

				if(!error_msg.isNull())
				{
					QMessageBox::critical(NULL,QObject::tr("Cannot start Tor"),QObject::tr("Sorry but Tor cannot be started on your system!\n\nThe error reported is:\"")+error_msg+"\"") ;
					return 1;
				}
			}

			tcd.hide();

			if(tcd.checkForHiddenService() != TorControlDialog::HIDDEN_SERVICE_STATUS_OK)
			{
				QMessageBox::critical(NULL,QObject::tr("Cannot start a hidden tor service!"),QObject::tr("It was not possible to start a hidden service.")) ;
				return 1 ;
			}
		}
	}

    QSplashScreen splashScreen(FilesDefs::getPixmapFromQtResourcePath(":/images/logo/logo_splash.png")/* , Qt::WindowStaysOnTopHint*/);

	splashScreen.show();
	splashScreen.showMessage(rshare.translate("SplashScreen", "Load configuration"), Qt::AlignHCenter | Qt::AlignBottom);

	QCoreApplication::processEvents();

	/* stop Retroshare if startup fails */
	if (!RsControl::instance()->StartupRetroShare())
	{
		std::cerr << "libretroshare failed to startup!" << std::endl;
		return 1;
	}

    if(is_auto_tor)
	{
		// Tor works with viable hidden service. Let's use it!

		QString service_id ;
		QString onion_address ;
		uint16_t service_port ;
		uint16_t service_target_port ;
		uint16_t proxy_server_port ;
		QHostAddress service_target_address ;
		QHostAddress proxy_server_address ;

		Tor::TorManager *torManager = Tor::TorManager::instance();
		torManager->getHiddenServiceInfo(service_id,onion_address,service_port,service_target_address,service_target_port);
		torManager->getProxyServerInfo(proxy_server_address,proxy_server_port) ;

		std::cerr << "Got hidden service info: " << std::endl;
		std::cerr << "  onion address  : " << onion_address.toStdString() << std::endl;
		std::cerr << "  service_id     : " << service_id.toStdString() << std::endl;
		std::cerr << "  service port   : " << service_port << std::endl;
		std::cerr << "  target port    : " << service_target_port << std::endl;
		std::cerr << "  target address : " << service_target_address.toString().toStdString() << std::endl;

		std::cerr << "Setting proxy server to " << service_target_address.toString().toStdString() << ":" << service_target_port << std::endl;

		rsPeers->setLocalAddress(rsPeers->getOwnId(), service_target_address.toString().toStdString(), service_target_port);
		rsPeers->setHiddenNode(rsPeers->getOwnId(), onion_address.toStdString(), service_port);
		rsPeers->setProxyServer(RS_HIDDEN_TYPE_TOR, proxy_server_address.toString().toStdString(),proxy_server_port) ;
	}

	Rshare::initPlugins();

	splashScreen.showMessage(rshare.translate("SplashScreen", "Create interface"), Qt::AlignHCenter | Qt::AlignBottom);
	QCoreApplication::processEvents();	// forces splashscreen to show up

	RsharePeerSettings::Create();

	Emoticons::load();

	if (Settings->value(QString::fromUtf8("FirstRun"), true).toBool()) {
		splashScreen.hide();

		Settings->setValue(QString::fromUtf8("FirstRun"), false);

		SoundManager::initDefault();

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

	w->processLastArgs();

	if (!sDefaultGXSIdToCreate.isEmpty()) {
		RsIdentityParameters params;
		params.nickname = sDefaultGXSIdToCreate.toUtf8().constData();
		params.isPgpLinked = true;
		params.mImage.clear();
		uint32_t token = 0;
		rsIdentity->createIdentity(token, params);
	}
	// I'm using a signal to transfer the hashing info to the mainwindow, because Qt schedules signals properly to
	// avoid clashes between infos from threads.
	//

	qRegisterMetaType<FileDetail>("FileDetail") ;
	qRegisterMetaType<RsPeerId>("RsPeerId") ;

	std::cerr << "connecting signals and slots" << std::endl ;
	QObject::connect(notify,SIGNAL(gotTurtleSearchResult(qulonglong,FileDetail)),w->transfersDialog->searchDialog	,SLOT(updateFiles(qulonglong,FileDetail))) ;
	QObject::connect(notify,SIGNAL(deferredSignatureHandlingRequested()),notify,SLOT(handleSignatureEvent()),Qt::QueuedConnection) ;
	QObject::connect(notify,SIGNAL(chatLobbyTimeShift(int)),notify,SLOT(handleChatLobbyTimeShift(int)),Qt::QueuedConnection) ;
	QObject::connect(notify,SIGNAL(diskFull(int,int))						,w                   		,SLOT(displayDiskSpaceWarning(int,int))) ;
    QObject::connect(notify,SIGNAL(filesPostModChanged(bool))         ,w                         ,SLOT(postModDirectories(bool)) ,Qt::QueuedConnection        ) ;
	QObject::connect(notify,SIGNAL(transfersChanged())                ,w->transfersDialog  		,SLOT(insertTransfers()                )) ;
	QObject::connect(notify,SIGNAL(publicChatChanged(int))            ,w->friendsDialog      		,SLOT(publicChatChanged(int)           ));
	QObject::connect(notify,SIGNAL(neighboursChanged())               ,w->friendsDialog->networkDialog    		,SLOT(securedUpdateDisplay())) ;

	QObject::connect(notify,SIGNAL(chatStatusChanged(const QString&,const QString&,bool)),w->friendsDialog,SLOT(updatePeerStatusString(const QString&,const QString&,bool)));
	QObject::connect(notify,SIGNAL(ownStatusMessageChanged()),w->friendsDialog,SLOT(loadmypersonalstatus()));

//	QObject::connect(notify,SIGNAL(logInfoChanged(const QString&))		,w->friendsDialog->networkDialog,SLOT(setLogInfo(QString))) ;
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

#ifdef RS_JSONAPI
	JsonApiPage::checkStartJsonApi();

#ifdef RS_WEBUI
    WebuiPage::checkStartWebui();	// normally we should rather save the UI flags internally to p3webui
#endif
#endif // RS_JSONAPI

	/* dive into the endless loop */
	int ti = rshare.exec();
	delete w ;

#ifdef RS_JSONAPI
	JsonApiPage::checkShutdownJsonApi();
#endif // RS_JSONAPI

	/* cleanup */
	ChatDialog::cleanupChat();
#ifdef RS_ENABLE_GXS
	RsGxsUpdateBroadcast::cleanup();
#endif

	RsControl::instance()->rsGlobalShutDown();

	delete(soundManager);
	soundManager = NULL;

	Settings->sync();
	delete(Settings);

	return ti ;
}
