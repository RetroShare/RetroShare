/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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

#include <QMessageBox>
#include <QString>
#include <QtDebug>
#include <QIcon>
#include <QPixmap>
#include <QColorDialog>
#include <QDesktopServices>
#include <QUrl>

#ifdef BLOGS
#include "gui/unfinished/blogs/BlogsDialog.h"
#endif 

#include <retroshare/rsplugin.h>
#include <retroshare/rsconfig.h>

#include "rshare.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "MessengerWindow.h"
#include "NetworkDialog.h"
#include "SearchDialog.h"
#include "gui/FileTransfer/TransfersDialog.h"
#include "MessagesDialog.h"
#include "SharedFilesDialog.h"
#include "PluginsPage.h"
#include "NewsFeed.h"
#include "ShareManager.h"
#include "NetworkView.h"
#include "ForumsDialog.h"
#include "FriendsDialog.h"
#include "ChatLobbyWidget.h"
#include "HelpDialog.h"
#include "AboutDialog.h"
#include "ChannelFeed.h"
#include "bwgraph/bwgraph.h"
#include "help/browser/helpbrowser.h"
#include "chat/ChatDialog.h"
#include "RetroShareLink.h"
#include "SoundManager.h"
#include "notifyqt.h"
#include "common/UserNotify.h"
#include "gui/ServicePermissionDialog.h"

#ifdef UNFINISHED
#include "unfinished/ApplicationWindow.h"
#endif

#define GETSTARTED_GUI  1
#ifdef GETSTARTED_GUI
#include "gui/GetStartedDialog.h"
#endif

#include "gui/FileTransfer/TurtleRouterDialog.h"
#include "idle/idle.h"

#include "statusbar/peerstatus.h"
#include "statusbar/natstatus.h"
#include "statusbar/ratesstatus.h"
#include "statusbar/dhtstatus.h"
#include "statusbar/hashingstatus.h"
#include "statusbar/discstatus.h"
#include "statusbar/OpModeStatus.h"
#include "statusbar/SoundStatus.h"
#include <retroshare/rsstatus.h>

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rsforums.h>
#include <retroshare/rschannels.h>
#include <retroshare/rsnotify.h>

#include "gui/connect/ConnectFriendWizard.h"
#include "util/rsguiversion.h"
#include "settings/rsettingswin.h"
#include "settings/rsharesettings.h"
#include "common/StatusDefs.h"

#include <iomanip>
#include <unistd.h>

/****
 *
 * #define USE_DHTWINDOW	1
 * #define USE_BWCTRLWINDOW	1
 *
 ****/
#define USE_DHTWINDOW	1
#define USE_BWCTRLWINDOW	1

#ifdef USE_DHTWINDOW
#include "dht/DhtWindow.h"
#endif

#ifdef USE_BWCTRLWINDOW
#include "bwctrl/BwCtrlWindow.h"
#endif


/* Images for toolbar icons */
//#define IMAGE_NETWORK2          ":/images/rs1.png"
#define IMAGE_PEERS         	":/images/groupchat.png"
#define IMAGE_TRANSFERS      	":/images/ktorrent32.png"
#define IMAGE_FILES   	        ":/images/fileshare32.png"
#define IMAGE_CHANNELS       	":/images/channels.png"
#define IMAGE_FORUMS            ":/images/konversation.png"
#define IMAGE_PREFERENCES       ":/images/kcmsystem24.png"
#define IMAGE_CHAT          	":/images/groupchat.png"
#define IMAGE_RETROSHARE        ":/images/rstray3.png"
#define IMAGE_ABOUT             ":/images/informations_24x24.png"
#define IMAGE_STATISTIC         ":/images/utilities-system-monitor.png"
#define IMAGE_MESSAGES          ":/images/evolution.png"
#define IMAGE_BWGRAPH           ":/images/ksysguard.png"
#define IMAGE_RSM32             ":/images/kdmconfig.png"
#define IMAGE_RSM16             ":/images/rsmessenger16.png"
#define IMAGE_CLOSE             ":/images/close_normal.png"
#define IMAGE_BLOCK         	":/images/blockdevice.png"
#define IMAGE_COLOR         	":/images/highlight.png"
#define IMAGE_GAMES             ":/images/kgames.png"
#define IMAGE_PHOTO             ":/images/lphoto.png"
#define IMAGE_ADDFRIEND         ":/images/add-friend24.png"
#define IMAGE_ADDSHARE          ":/images/directoryadd_24x24_shadow.png"
#define IMAGE_OPTIONS           ":/images/settings.png"
#define IMAGE_QUIT              ":/images/exit_24x24.png"
#define IMAGE_UNFINISHED        ":/images/underconstruction.png"
#define IMAGE_MINIMIZE          ":/images/window_nofullscreen.png"
#define IMAGE_MAXIMIZE          ":/images/window_fullscreen.png"
#define IMG_HELP                ":/images/help24.png"
#define IMAGE_NEWSFEED          ":/images/newsfeed128.png"
#define IMAGE_PLUGINS           ":/images/extension_32.png"
#define IMAGE_NOONLINE          ":/images/rstray0.png"
#define IMAGE_ONEONLINE         ":/images/rstray1.png"
#define IMAGE_TWOONLINE         ":/images/rstray2.png"
#define IMAGE_BLOGS             ":/images/kblogger.png"
#define IMAGE_DHT               ":/images/dht16.png"
#define IMAGE_CHATLOBBY			":/images/chat_32.png"

/*static*/ MainWindow *MainWindow::_instance = NULL;

/** create main window */
/*static*/ MainWindow *MainWindow::Create ()
{
    if (_instance == NULL) {
        /* _instance is set in constructor */
        new MainWindow ();
    }

    return _instance;
}

/*static*/ MainWindow *MainWindow::getInstance()
{
    return _instance;
}

/** Constructor */
MainWindow::MainWindow(QWidget* parent, Qt::WFlags flags)
    : RWindow("MainWindow", parent, flags), ui(new Ui::MainWindow)
{
    /* Invoke the Qt Designer generated QObject setup routine */
    ui->setupUi(this);

    _instance = this;

    m_bStatusLoadDone = false;
    isIdle = false;
    onlineCount = 0;

    notifyMenu = NULL;

    /* Calculate only once */
    RsPeerDetails pd;
    if (rsPeers->getPeerDetails(rsPeers->getOwnId(), pd)) {
        nameAndLocation = QString("%1 (%2)").arg(QString::fromUtf8(pd.name.c_str())).arg(QString::fromUtf8(pd.location.c_str()));
    }

    setWindowTitle(tr("RetroShare %1 a secure decentralized communication platform").arg(retroshareVersion()) + " - " + nameAndLocation);

    /* WORK OUT IF WE"RE IN ADVANCED MODE OR NOT */
    bool advancedMode = false;
    std::string advsetting;
    if (rsConfig->getConfigurationOption(RS_CONFIG_ADVANCED, advsetting) && (advsetting == "YES")) {
        advancedMode = true;
    }

    /* add url handler for RetroShare links */
    QDesktopServices::setUrlHandler(RSLINK_SCHEME, this, "retroshareLinkActivated");
    QDesktopServices::setUrlHandler("http", this, "externalLinkActivated");
    QDesktopServices::setUrlHandler("https", this, "externalLinkActivated");

    // Setting icons
    this->setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));

    /* Create all the dialogs of which we only want one instance */
    _bandwidthGraph = new BandwidthGraph();

    #ifdef UNFINISHED
    applicationWindow = new ApplicationWindow();
    applicationWindow->hide();
    #endif    

    /** Left Side ToolBar**/
    connect(ui->actionAdd_Friend, SIGNAL(triggered() ), this , SLOT( addFriend() ) );
    connect(ui->actionAdd_Share, SIGNAL(triggered() ), this , SLOT( openShareManager() ) );
    connect(ui->actionOptions, SIGNAL(triggered()), this, SLOT( showSettings()) );
//    connect(ui->actionMessenger, SIGNAL(triggered()), this, SLOT( showMessengerWindow()) );
    connect(ui->actionServicePermissions, SIGNAL(triggered()), this, SLOT(servicePermission()));

    ui->actionMessenger->setVisible(false);

    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT( showabout()) );
    //connect(ui->actionColor, SIGNAL(triggered()), this, SLOT( setStyle()) );

    /** adjusted quit behaviour: trigger a warning that can be switched off in the saved
        config file RetroShare.conf */
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(doQuit()));

    QList<QPair<MainPage*, QAction*> > notify;

    /* Create the Main pages and actions */
    QActionGroup *grp = new QActionGroup(this);
    QAction *action;

    ui->stackPages->add(newsFeed = new NewsFeed(ui->stackPages),
                       action = createPageAction(QIcon(IMAGE_NEWSFEED), tr("News feed"), grp));
    notify.push_back(QPair<MainPage*, QAction*>(newsFeed, action));

//    ui->stackPages->add(networkDialog = new NetworkDialog(ui->stackPages),
//                       createPageAction(QIcon(IMAGE_NETWORK2), tr("Network"), grp));

    ui->stackPages->add(friendsDialog = new FriendsDialog(ui->stackPages),
                       action = createPageAction(QIcon(IMAGE_PEERS), tr("Friends"), grp));
    notify.push_back(QPair<MainPage*, QAction*>(friendsDialog, action));

//    ui->stackPages->add(searchDialog = new SearchDialog(ui->stackPages),
//                       createPageAction(QIcon(IMAGE_SEARCH), tr("Search"), grp));

    ui->stackPages->add(transfersDialog = new TransfersDialog(ui->stackPages),
                      action = createPageAction(QIcon(IMAGE_TRANSFERS), tr("File sharing"), grp));
    notify.push_back(QPair<MainPage*, QAction*>(transfersDialog, action));

    ui->stackPages->add(chatLobbyDialog = new ChatLobbyWidget(ui->stackPages),
                      action = createPageAction(QIcon(IMAGE_CHATLOBBY), tr("Chat Lobbies"), grp));
    notify.push_back(QPair<MainPage*, QAction*>(chatLobbyDialog, action));

    ui->stackPages->add(messagesDialog = new MessagesDialog(ui->stackPages),
                      action = createPageAction(QIcon(IMAGE_MESSAGES), tr("Messages"), grp));
    notify.push_back(QPair<MainPage*, QAction*>(messagesDialog, action));

    ui->stackPages->add(channelFeed = new ChannelFeed(ui->stackPages),
                      action = createPageAction(QIcon(IMAGE_CHANNELS), tr("Channels"), grp));
    notify.push_back(QPair<MainPage*, QAction*>(channelFeed, action));

#ifdef BLOGS
     ui->stackPages->add(blogsFeed = new BlogsDialog(ui->stackPages), createPageAction(QIcon(IMAGE_BLOGS), tr("Blogs"), grp));
#endif
                      
    ui->stackPages->add(forumsDialog = new ForumsDialog(ui->stackPages),
                       action = createPageAction(QIcon(IMAGE_FORUMS), tr("Forums"), grp));
    notify.push_back(QPair<MainPage*, QAction*>(forumsDialog, action));

	 std::cerr << "Looking for interfaces in existing plugins:" << std::endl;
	 for(int i = 0;i<rsPlugins->nbPlugins();++i)
	 {
		 QIcon icon ;

		 if(rsPlugins->plugin(i) != NULL && rsPlugins->plugin(i)->qt_page() != NULL)
		 {
			 if(rsPlugins->plugin(i)->qt_icon() != NULL)
				 icon = *rsPlugins->plugin(i)->qt_icon() ;
			 else
				 icon = QIcon(":images/extension_48.png") ;

			 std::cerr << "  Addign widget page for plugin " << rsPlugins->plugin(i)->getPluginName() << std::endl;
			 MainPage *pluginPage = rsPlugins->plugin(i)->qt_page();
			 QAction *pluginAction = createPageAction(icon, QString::fromUtf8(rsPlugins->plugin(i)->getPluginName().c_str()), grp);
			 ui->stackPages->add(pluginPage, pluginAction);
			 notify.push_back(QPair<MainPage*, QAction*>(pluginPage, pluginAction));
		 }
		 else if(rsPlugins->plugin(i) == NULL)
			 std::cerr << "  No plugin object !" << std::endl;
		 else
			 std::cerr << "  No plugin page !" << std::endl;

	 }

#ifndef RS_RELEASE_VERSION
#ifdef PLUGINMGR
    ui->stackPages->add(pluginsPage = new PluginsPage(ui->stackPages),
                       createPageAction(QIcon(IMAGE_PLUGINS), tr("Plugins"), grp));
#endif
#endif

#ifdef GETSTARTED_GUI
    MainPage *getStartedPage = NULL;

    if (!advancedMode)
    {
        ui->stackPages->add(getStartedPage = new GetStartedDialog(ui->stackPages),
                       createPageAction(QIcon(IMG_HELP), tr("Getting Started"), grp));
    }
#endif

    /* Create the toolbar */
    ui->toolBar->addActions(grp->actions());

    connect(grp, SIGNAL(triggered(QAction *)), ui->stackPages, SLOT(showPage(QAction *)));

#ifdef UNFINISHED
    ui->toolBar->addSeparator();
    addAction(new QAction(QIcon(IMAGE_UNFINISHED), tr("Unfinished"), ui->toolBar), SLOT(showApplWindow()));
#endif

    ui->stackPages->setCurrentIndex(Settings->getLastPageInMainWindow());

    /** StatusBar section ********/
    /* initialize combobox in status bar */
    statusComboBox = new QComboBox(statusBar());
    statusComboBox->setFocusPolicy(Qt::ClickFocus);
    initializeStatusObject(statusComboBox, true);

    QWidget *widget = new QWidget();
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(6);
    hbox->addWidget(statusComboBox);
    widget->setLayout(hbox);
    statusBar()->addWidget(widget);

    peerstatus = new PeerStatus();
    statusBar()->addWidget(peerstatus);

    natstatus = new NATStatus();
    statusBar()->addWidget(natstatus);
    
    dhtstatus = new DHTStatus();
    statusBar()->addWidget(dhtstatus);

    hashingstatus = new HashingStatus();
    statusBar()->addPermanentWidget(hashingstatus);

    discstatus = new DiscStatus();
    statusBar()->addPermanentWidget(discstatus);

    ratesstatus = new RatesStatus();
    statusBar()->addPermanentWidget(ratesstatus);

    statusBar()->addPermanentWidget(new OpModeStatus());

    statusBar()->addPermanentWidget(new SoundStatus());
    /** Status Bar end ******/

    /* Creates a tray icon with a context menu and adds it to the system's * notification area. */
    createTrayIcon();

    QList<QPair<MainPage*, QAction*> >::iterator notifyIt;
    for (notifyIt = notify.begin(); notifyIt != notify.end(); ++notifyIt) {
        UserNotify *userNotify = notifyIt->first->getUserNotify(this);
        if (userNotify) {
            userNotify->initialize(ui->toolBar, notifyIt->second);
            connect(userNotify, SIGNAL(countChanged()), this, SLOT(updateTrayCombine()));
            userNotifyList.push_back(userNotify);
        }
    }

    createNotifyIcons();

    /* caclulate friend count */
    updateFriends();
    connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(QString,int)), this, SLOT(updateFriends()));
    connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(updateFriends()));

    loadOwnStatus();

    /* Set focus to the current page */
    ui->stackPages->currentWidget()->setFocus();

    idle = new Idle();
    idle->start();
    connect(idle, SIGNAL(secondsIdle(int)), this, SLOT(checkAndSetIdle(int)));

    QTimer *timer = new QTimer(this);
    timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    timer->start(1000);
}

/** Destructor. */
MainWindow::~MainWindow()
{
    Settings->setLastPageInMainWindow(ui->stackPages->currentIndex());

    delete peerstatus;
    delete natstatus;
    delete dhtstatus;
    delete ratesstatus;
    delete discstatus;
    MessengerWindow::releaseInstance();
#ifdef UNFINISHED
    delete applicationWindow;
#endif

    delete(ui);
}

void MainWindow::displayDiskSpaceWarning(int loc,int size_limit_mb)
{
	QString locString ;
	switch(loc)
	{
		case RS_PARTIALS_DIRECTORY: 	locString = "Partials" ;
												break ;

		case RS_CONFIG_DIRECTORY: 		locString = "Config" ;
												break ;

		case RS_DOWNLOAD_DIRECTORY: 	locString = "Download" ;
												break ;

		default:
												std::cerr << "Error: " << __PRETTY_FUNCTION__ << " was called with an unknown parameter loc=" << loc << std::endl ;
												return ;
	}
	QMessageBox::critical(NULL,tr("Low disk space warning"),
				tr("The disk space in your ")+locString +tr(" directory is running low (current limit is ")+QString::number(size_limit_mb)+tr("MB). \n\n RetroShare will now safely suspend any disk access to this directory. \n\n Please make some free space and click Ok.")) ;
}

/** Creates a tray icon with a context menu and adds it to the system
 * notification area. */
void MainWindow::createTrayIcon()
{
    /** Tray icon Menu **/
    QMenu *trayMenu = new QMenu(this);
    QObject::connect(trayMenu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
    toggleVisibilityAction = trayMenu->addAction(QIcon(IMAGE_RETROSHARE), tr("Show/Hide"), this, SLOT(toggleVisibilitycontextmenu()));

    /* Create status menu */
    QMenu *statusMenu = trayMenu->addMenu(tr("Status"));
    initializeStatusObject(statusMenu, true);

    /* Create notify menu */
    notifyMenu = trayMenu->addMenu(tr("Notify"));
    notifyMenu->menuAction()->setVisible(false);

    trayMenu->addSeparator();
    trayMenu->addAction(QIcon(IMAGE_RSM16), tr("Open Messenger"), this, SLOT(showMessengerWindow()));
    trayMenu->addAction(QIcon(IMAGE_MESSAGES), tr("Open Messages"), this, SLOT(showMess()));
    trayMenu->addAction(QIcon(IMAGE_BWGRAPH), tr("Bandwidth Graph"), _bandwidthGraph, SLOT(showWindow()));
#ifdef USE_DHTWINDOW
    trayMenu->addAction(QIcon(IMAGE_DHT), tr("DHT Details"), this, SLOT(showDhtWindow()));
#endif
#ifdef USE_BWCTRLWINDOW
    trayMenu->addAction(QIcon(IMAGE_DHT), tr("Bandwidth Details"), this, SLOT(showBwCtrlWindow()));
#endif

#ifdef UNFINISHED
    trayMenu->addAction(QIcon(IMAGE_UNFINISHED), tr("Applications"), this, SLOT(showApplWindow()));
#endif
    trayMenu->addAction(QIcon(IMAGE_PREFERENCES), tr("Options"), this, SLOT(showSettings()));
    trayMenu->addAction(QIcon(IMG_HELP), tr("Help"), this, SLOT(showHelpDialog()));
    trayMenu->addSeparator();
    trayMenu->addAction(QIcon(IMAGE_MINIMIZE), tr("Minimize"), this, SLOT(showMinimized()));
    trayMenu->addAction(QIcon(IMAGE_MAXIMIZE), tr("Maximize"), this, SLOT(showMaximized()));
    trayMenu->addSeparator();
    trayMenu->addAction(QIcon(IMAGE_CLOSE), tr("&Quit"), this, SLOT(doQuit()));
    /** End of Icon Menu **/

    // Create the tray icon
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setToolTip(tr("RetroShare"));
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setIcon(QIcon(IMAGE_NOONLINE));

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(toggleVisibility(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
}

void MainWindow::createNotifyIcons()
{
    /* create notify icons */
    QList<UserNotify*>::iterator it;
    for (it = userNotifyList.begin(); it != userNotifyList.end(); ++it) {
        UserNotify *userNotify = *it;
        userNotify->createIcons(notifyMenu);
        userNotify->updateIcon();
    }
    updateTrayCombine();
}

const QList<UserNotify*> &MainWindow::getUserNotifyList()
{
    return userNotifyList;
}

/*static*/ void MainWindow::displayLobbySystrayMsg(const QString& title,const QString& msg)
{
    if (_instance == NULL) 
        return;

    if(Settings->getDisplayTrayChatLobby()) 
		 _instance->displaySystrayMsg(title,msg) ;
}


/*static*/ void MainWindow::installGroupChatNotifier()
{
    if (_instance == NULL) {
        // nothing to do
        return;
    }

    if(Settings->getDisplayTrayGroupChat()) {
        QObject::connect(_instance->friendsDialog, SIGNAL(notifyGroupChat(const QString&,const QString&)), _instance, SLOT(displaySystrayMsg(const QString&,const QString&)), Qt::QueuedConnection);
    } else {
        QObject::disconnect(_instance->friendsDialog, SIGNAL(notifyGroupChat(const QString&,const QString&)), _instance, SLOT(displaySystrayMsg(const QString&,const QString&)));
    }
}

/*static*/ void MainWindow::installNotifyIcons()
{
   if (_instance == NULL) {
       // nothing to do
       return;
   }

   _instance->createNotifyIcons();
}

void MainWindow::displaySystrayMsg(const QString& title,const QString& msg)
{
    trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, 3000);
}

void MainWindow::updateTrayCombine()
{
    notifyToolTip.clear();

    bool visible = false;

    if (notifyMenu) {
        QList<QAction*> actions = notifyMenu->actions();
        int count = 0;
        QList<QAction*>::iterator actionIt;
        for (actionIt = actions.begin(); actionIt != actions.end(); actionIt++) {
            if ((*actionIt)->isVisible()) {
                visible = true;

                count += (*actionIt)->data().toInt();
// ToolTip is too long to show all services
//                if (notifyToolTip.isEmpty() == false) {
//                    notifyToolTip += "\r";
//                }
//                notifyToolTip += (*actionIt)->data().toString() + ":" + (*actionIt)->text();
            }
            if (visible) {
                notifyToolTip = ((count == 1) ? tr("%1 new message") : tr("%1 new messages")).arg(count);
            }

        }
    }
    notifyMenu->menuAction()->setVisible(visible);

    // update tray icon
    updateFriends();
}

void MainWindow::updateStatus()
{
    // This call is essential to remove locks due to QEventLoop re-entrance while asking gpg passwds. Dont' remove it!
    if(RsAutoUpdatePage::eventsLocked())
        return;

    float downKb = 0;
    float upKb = 0;
    rsConfig->GetCurrentDataRates(downKb, upKb);

    if (ratesstatus)
        ratesstatus->getRatesStatus(downKb, upKb);

    if (natstatus)
        natstatus->getNATStatus();
        
    if (dhtstatus)
        dhtstatus->getDHTStatus();

    if (discstatus) {
        discstatus->update();
    }

    QString tray = "RetroShare\n" + tr("Down: %1 (kB/s)").arg(downKb, 0, 'f', 2) + " | " + tr("Up: %1 (kB/s)").arg(upKb, 0, 'f', 2) + "\n";

    if (onlineCount == 1) {
        tray += tr("%1 friend connected").arg(onlineCount);
    } else {
        tray += tr("%1 friends connected").arg(onlineCount);
    }

    tray += "\n" + nameAndLocation;

    if (!notifyToolTip.isEmpty()) {
        tray += "\n";
        tray += notifyToolTip;
    }
    trayIcon->setToolTip(tray);
}

void MainWindow::updateFriends()
{
    unsigned int friendCount = 0;
    rsPeers->getPeerCount (&friendCount, &onlineCount, false);

    if (peerstatus)
        peerstatus->getPeerStatus(friendCount, onlineCount);

    QString trayIconResource;

    if (onlineCount == 0) {
        trayIconResource = IMAGE_NOONLINE;
    } else if (onlineCount < 2) {
        trayIconResource = IMAGE_ONEONLINE;
    } else if (onlineCount < 3) {
        trayIconResource = IMAGE_TWOONLINE;
    } else {
        trayIconResource = IMAGE_RETROSHARE;
    }

    QIcon icon;
    if (notifyMenu && notifyMenu->menuAction()->isVisible()) {
        QPixmap trayImage(trayIconResource);
        QPixmap overlayImage(":/images/rstray_star.png");

        QPainter painter(&trayImage);
        painter.drawPixmap(0, 0, overlayImage);

        icon.addPixmap(trayImage);
    } else {
        icon = QIcon(trayIconResource);
    }

    trayIcon->setIcon(icon);
}

void MainWindow::postModDirectories(bool update_local)
{
    RSettingsWin::postModDirectories(update_local);
    ShareManager::postModDirectories(update_local);

    QCoreApplication::flush();
}

/** Creates a new action associated with a config page. */
QAction *MainWindow::createPageAction(const QIcon &icon, const QString &text, QActionGroup *group)
{
    QFont font;
    QAction *action = new QAction(icon, text, group);
    font = action->font();
    font.setPointSize(9);
    action->setCheckable(true);
    action->setFont(font);
    return action;
}

/** Adds the given action to the toolbar and hooks its triggered() signal to
 * the specified slot (if given). */
void MainWindow::addAction(QAction *action, const char *slot)
{
    QFont font;
    font = action->font();
    font.setPointSize(9);
    action->setFont(font);
    ui->toolBar->addAction(action);
    connect(action, SIGNAL(triggered()), this, slot);
}

#ifdef WINDOWS_SYS
//void SetForegroundWindowInternal(HWND hWnd)
//{
//	if (!::IsWindow(hWnd)) return;

//	// relation time of SetForegroundWindow lock
//	DWORD lockTimeOut = 0;
//	HWND  hCurrWnd = ::GetForegroundWindow();
//	DWORD dwThisTID = ::GetCurrentThreadId(),
//		  dwCurrTID = ::GetWindowThreadProcessId(hCurrWnd,0);

//	// we need to bypass some limitations from Microsoft :)
//	if (dwThisTID != dwCurrTID) {
//		::AttachThreadInput(dwThisTID, dwCurrTID, TRUE);

//		::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT,0,&lockTimeOut,0);
//		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,0,0,SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);

//		::AllowSetForegroundWindow(ASFW_ANY);
//	}

//	::SetForegroundWindow(hWnd);

//	if(dwThisTID != dwCurrTID) {
//		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,0,(PVOID)lockTimeOut,SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
//		::AttachThreadInput(dwThisTID, dwCurrTID, FALSE);
//	}
//}

void SetForegroundWindowInternal(HWND hWnd)
{
	if (!::IsWindow(hWnd)) return;

	BYTE keyState[256] = {0};
	// to unlock SetForegroundWindow we need to imitate Alt pressing
	if (::GetKeyboardState((LPBYTE)&keyState)) {
		if(!(keyState[VK_MENU] & 0x80)) {
			::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
		}
	}

	::SetForegroundWindow(hWnd);

	if (::GetKeyboardState((LPBYTE)&keyState)) {
		if(!(keyState[VK_MENU] & 0x80)) {
			::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		}
	}
}
#endif

/*static*/ void MainWindow::raiseWindow()
{
    if (_instance == NULL) {
        return;
    }

    /* Show the dialog. */
    _instance->show();
    _instance->raise();

#ifdef WINDOWS_SYS
    SetForegroundWindowInternal(_instance->winId());
#endif
}

/** Shows the MainWindow with focus set to the given page. */
/*static*/ void MainWindow::showWindow(Page page)
{
    if (_instance == NULL) {
        return;
    }

    /* Show the dialog. */
    raiseWindow();
    /* Set the focus to the specified page. */
    activatePage (page);
}

/** Shows the MainWindow with focus set to the given page. */
/*static*/ void MainWindow::showWindow(MainPage *page)
{
    if (_instance == NULL) {
        return;
    }

    /* Show the dialog. */
    raiseWindow();
    /* Set the focus to the specified page. */
    _instance->ui->stackPages->setCurrentPage(page);
}

/** Set focus to the given page. */
/*static*/ bool MainWindow::activatePage(Page page)
{
    if (_instance == NULL) {
        return false;
    }

	 switch (page) {
		 case Search:
			 _instance->ui->stackPages->setCurrentPage( _instance->transfersDialog );
			 _instance->transfersDialog->activatePage(TransfersDialog::SearchTab) ;
			 break ;
		 case Network:
			 _instance->ui->stackPages->setCurrentPage( _instance->friendsDialog );
			 _instance->friendsDialog->activatePage(FriendsDialog::NetworkTab) ;
			 break;
		 case Friends:
			 _instance->ui->stackPages->setCurrentPage( _instance->friendsDialog );
			 break;
		 case ChatLobby:
			 _instance->ui->stackPages->setCurrentPage( _instance->chatLobbyDialog );
			 break;
		 case Transfers:
			 _instance->ui->stackPages->setCurrentPage( _instance->transfersDialog );
			 break;
		 case SharedDirectories:
			 _instance->ui->stackPages->setCurrentPage( _instance->transfersDialog );
			 _instance->transfersDialog->activatePage(TransfersDialog::LocalSharedFilesTab) ;
			 break;
		 case Messages:
			 _instance->ui->stackPages->setCurrentPage( _instance->messagesDialog );
			 break;
		 case Channels:
			 _instance->ui->stackPages->setCurrentPage( _instance->channelFeed );
			 return true ;
		 case Forums:
			 _instance->ui->stackPages->setCurrentPage( _instance->forumsDialog );
			 return true ;
#ifdef BLOGS
		 case Blogs:
			 Page = _instance->blogsFeed;
			 return true ;
#endif
		 default:
			 std::cerr << "Show page called on value that is not handled yet. Please code it! (value = " << page << ")" << std::endl;
	 }

	 return false ;
}

/** Get the active page. */
/*static*/ MainWindow::Page MainWindow::getActivatePage()
{
   if (_instance == NULL) {
       return Network;
   }

   QWidget *page = _instance->ui->stackPages->currentWidget();

//   if (page == _instance->networkDialog) {
//       return Network;
//   }
   if (page == _instance->friendsDialog) {
       return Friends;
   }
   if (page == _instance->chatLobbyDialog) {
       return ChatLobby;
   }
   if (page == _instance->transfersDialog) {
       return Transfers;
   }
//   if (page == _instance->sharedfilesDialog) {
//       return SharedDirectories;
 //  }
   if (page == _instance->messagesDialog) {
       return Messages;
   }
#ifdef RS_USE_LINKS
   if (page == _instance->linksDialog) {
       return Links;
   }
#endif
   if (page == _instance->channelFeed) {
       return Channels;
   }
   if (page == _instance->forumsDialog) {
       return Forums;
   }
#ifdef BLOGS
   if (page == _instance->blogsFeed) {
       return Blogs;
   }
#endif

   return Network;
}

/** get page */
/*static*/ MainPage *MainWindow::getPage (Page page)
{
   if (_instance == NULL) {
       return NULL;
   }

   switch (page) 
	{
		case Network:
			return _instance->friendsDialog->networkDialog;
		case Friends:
			return _instance->friendsDialog;
		case ChatLobby:
			return _instance->chatLobbyDialog;
		case Transfers:
			return _instance->transfersDialog;
		case SharedDirectories:
			return _instance->transfersDialog->localSharedFiles;
		case Search:
			return _instance->transfersDialog->searchDialog;
		case Messages:
			return _instance->messagesDialog;
#ifdef RS_USE_LINKS
		case Links:
			return _instance->linksDialog;
#endif
		case Channels:
			return _instance->channelFeed;
		case Forums:
			return _instance->forumsDialog;
#ifdef BLOGS
		case Blogs:
			return _instance->blogsFeed;
#endif
	}

   return NULL;
}

/***** TOOL BAR FUNCTIONS *****/

/** Add a Friend ShortCut */
void MainWindow::addFriend()
{
    ConnectFriendWizard connwiz (this);

    connwiz.exec ();
}

/** Shows Share Manager */
void MainWindow::openShareManager()
{
    ShareManager::showYourself();
}


/** Shows Messages Dialog */
void
MainWindow::showMess()
{
  showWindow(MainWindow::Messages);
}


/** Shows Options */
void MainWindow::showSettings()
{
    RSettingsWin::showYourself(this);
}

/** Shows Messenger window */
void MainWindow::showMessengerWindow()
{
    MessengerWindow::showYourself();
}

/** Shows Dht window */
void MainWindow::showDhtWindow()
{
#ifdef USE_DHTWINDOW
    DhtWindow::showYourself();
#endif
}


/** Shows Bandwitch Control window */
void MainWindow::showBwCtrlWindow()
{
#ifdef USE_BWCTRLWINDOW
    BwCtrlWindow::showYourself();
#endif
}


/** Shows Application window */
#ifdef UNFINISHED
void MainWindow::showApplWindow()
{
    applicationWindow->show();
}
#endif

/** If the user attempts to quit the app, a check-warning is issued. This warning can be
    turned off for future quit events.
*/
void MainWindow::doQuit()
{
	if(!Settings->value("doQuit", false).toBool())
	{
	  QString queryWrn;
	  queryWrn.clear();
	  queryWrn.append(tr("Do you really want to exit RetroShare ?"));

		if ((QMessageBox::question(this, tr("Really quit ? "),queryWrn,QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
		{
			qApp->quit();
		}
		else
			return;
	}

	rApp->quit();
}

void MainWindow::displayErrorMessage(int /*a*/,int /*b*/,const QString& error_msg)
{
	QMessageBox::critical(NULL, tr("Internal Error"),error_msg) ;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    e->ignore();

    if (Settings->getCloseToTray())
    {
        if (trayIcon->isVisible()) {
/*****
            static bool firstTime = true;

            if (firstTime)
            {
                QMessageBox::information(this, tr("RetroShare System tray"), tr("Application will continue running. Quit using context menu in the system tray"));
                firstTime = false;
            }
*****/
            hide();
        }
        return;
    }

    doQuit();
}

void MainWindow::updateMenu()
{
    toggleVisibilityAction->setText(isVisible() && !isMinimized() ? tr("Hide") : tr("Show"));
}

void MainWindow::toggleVisibility(QSystemTrayIcon::ActivationReason e)
{
    if (e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick) {
        if (isHidden() || isMinimized()) {
            show();
            if (isMinimized()) {
                if (isMaximized()) {
                    showMaximized();
                }else{
                    showNormal();
                }
            }
            raise();
            activateWindow();
        } else {
            hide();
        }
    }
}

void MainWindow::toggleVisibilitycontextmenu()
{
    if (isVisible() && !isMinimized())
        hide();
    else
        show();
}

void MainWindow::showabout()
{
    AboutDialog adlg(this);
    adlg.exec();
}

/** Displays the help browser and displays the most recently viewed help
 * topic. */
void MainWindow::showHelpDialog()
{
  showHelpDialog(QString());
}


/**< Shows the help browser and displays the given help <b>topic</b>. */
void MainWindow::showHelpDialog(const QString &topic)
{
  HelpBrowser::showWindow(topic);
}

/** Called when the user changes the UI translation. */
void
MainWindow::retranslateUi()
{
  ui->retranslateUi(this);
  foreach (MainPage *page, ui->stackPages->pages()) {
    page->retranslateUi();
  }
  foreach (QAction *action, ui->toolBar->actions()) {
    action->setText(tr(qPrintable(action->data().toString()), "MainWindow"));
  }
}

/* set status object to status value */
static void setStatusObject(QObject *pObject, int nStatus)
{
    QMenu *pMenu = dynamic_cast<QMenu*>(pObject);
    if (pMenu) {
        /* set action in menu */
        foreach(QObject *pObject, pMenu->children()) {
            QAction *pAction = qobject_cast<QAction*> (pObject);
            if (pAction == NULL) {
                continue;
            }

            if (pAction->data().toInt() == nStatus) {
                pAction->setChecked(true);
                break;
            }
        }
        return;
    }
    QComboBox *pComboBox = dynamic_cast<QComboBox*>(pObject);
    if (pComboBox) {
        /* set index of combobox */
        int nIndex = pComboBox->findData(nStatus, Qt::UserRole);
        if (nIndex != -1) {
            pComboBox->setCurrentIndex(nIndex);
        }
        return;
    }

    /* add more objects here */
}

/** Load own status Online,Away,Busy **/
void MainWindow::loadOwnStatus()
{
    m_bStatusLoadDone = true;

    StatusInfo statusInfo;
    if (rsStatus->getOwnStatus(statusInfo)) {
        /* send status to all added objects */
        for (std::set <QObject*>::iterator it = m_apStatusObjects.begin(); it != m_apStatusObjects.end(); it++) {
            setStatusObject(*it, statusInfo.status);
        }
    }
}

void MainWindow::checkAndSetIdle(int idleTime)
{
    int maxTimeBeforeIdle = Settings->getMaxTimeBeforeIdle();
    if ((idleTime >= maxTimeBeforeIdle) && !isIdle) {
        setIdle(true);
    } else if ((idleTime < maxTimeBeforeIdle) && isIdle) {
        setIdle(false);
    }

    return;
}

void MainWindow::setIdle(bool idle)
{
    isIdle = idle;

    StatusInfo statusInfo;
    if (rsStatus->getOwnStatus(statusInfo)) {
        setStatus(NULL, statusInfo.status);
    }
}

/* add and initialize status object */
void MainWindow::initializeStatusObject(QObject *pObject, bool bConnect)
{
    if (m_apStatusObjects.find(pObject) != m_apStatusObjects.end()) {
        /* already added */
        return;
    }

    m_apStatusObjects.insert(m_apStatusObjects.end(), pObject);

    std::string statusString;

    QMenu *pMenu = dynamic_cast<QMenu*>(pObject);
    if (pMenu) {
        /* initialize menu */
        QActionGroup *pGroup = new QActionGroup(pMenu);

        QAction *pAction = new QAction(QIcon(StatusDefs::imageIM(RS_STATUS_ONLINE)), StatusDefs::name(RS_STATUS_ONLINE), pMenu);
        pAction->setData(RS_STATUS_ONLINE);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);

        pAction = new QAction(QIcon(StatusDefs::imageIM(RS_STATUS_BUSY)), StatusDefs::name(RS_STATUS_BUSY), pMenu);
        pAction->setData(RS_STATUS_BUSY);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);

        pAction = new QAction(QIcon(StatusDefs::imageIM(RS_STATUS_AWAY)), StatusDefs::name(RS_STATUS_AWAY), pMenu);
        pAction->setData(RS_STATUS_AWAY);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);

        if (bConnect) {
            connect(pMenu, SIGNAL(triggered (QAction*)), this, SLOT(statusChangedMenu(QAction*)));
        }
    } else {
        /* initialize combobox */
        QComboBox *pComboBox = dynamic_cast<QComboBox*>(pObject);
        if (pComboBox) {
            pComboBox->addItem(QIcon(StatusDefs::imageIM(RS_STATUS_ONLINE)), StatusDefs::name(RS_STATUS_ONLINE), RS_STATUS_ONLINE);
            pComboBox->addItem(QIcon(StatusDefs::imageIM(RS_STATUS_BUSY)), StatusDefs::name(RS_STATUS_BUSY), RS_STATUS_BUSY);
            pComboBox->addItem(QIcon(StatusDefs::imageIM(RS_STATUS_AWAY)), StatusDefs::name(RS_STATUS_AWAY), RS_STATUS_AWAY);

            if (bConnect) {
                connect(pComboBox, SIGNAL(activated(int)), this, SLOT(statusChangedComboBox(int)));
            }
        }
        /* add more objects here */
    }

    if (m_bStatusLoadDone) {
        /* loadOwnStatus done, set own status directly */
        StatusInfo statusInfo;
        if (rsStatus->getOwnStatus(statusInfo)) {
            setStatusObject(pObject, statusInfo.status);
        }
    }
}

/* remove status object */
void MainWindow::removeStatusObject(QObject *pObject)
{
    m_apStatusObjects.erase(pObject);

    /* disconnect all signals between the object and MainWindow */
    disconnect(pObject, NULL, this, NULL);
}

/** Save own status Online,Away,Busy **/
void MainWindow::setStatus(QObject *pObject, int nStatus)
{
    if (isIdle && nStatus == (int) RS_STATUS_ONLINE) {
        /* set idle only when I am online */
        nStatus = RS_STATUS_INACTIVE;
    }

    rsStatus->sendStatus("", nStatus);

    /* set status in all status objects, but the calling one */
    for (std::set <QObject*>::iterator it = m_apStatusObjects.begin(); it != m_apStatusObjects.end(); it++) {
        if (*it != pObject) {
            setStatusObject(*it, nStatus);
        }
    }
}

/* new status from context menu */
void MainWindow::statusChangedMenu(QAction *pAction)
{
    if (pAction == NULL) {
        return;
    }

    setStatus(pAction->parent(), pAction->data().toInt());
}

/* new status from combobox in statusbar */
void MainWindow::statusChangedComboBox(int index)
{
    if (index < 0) {
        return;
    }

    /* no object known */
    setStatus(NULL, statusComboBox->itemData(index, Qt::UserRole).toInt());
}
void MainWindow::externalLinkActivated(const QUrl &url)
{
	static bool already_warned = false ;

	if(!already_warned)
	{
		QMessageBox mb(QObject::tr("Confirmation"), QObject::tr("Do you want this link to be handled by your system?")+"<br/><br/>"+ url.toString()+"<br/><br/>"+tr("Make sure this link has not been forged to drag you to a malicious website."), QMessageBox::Question, QMessageBox::Yes,QMessageBox::No, 0);
		mb.setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));

		QCheckBox *checkbox = new QCheckBox(tr("Don't ask me again")) ;
		mb.layout()->addWidget(checkbox) ;

		int res = mb.exec() ;

		if (res == QMessageBox::No) 
			return ;
		else if(checkbox->isChecked())
			already_warned = true ;
	}

	QDesktopServices::openUrl(url) ;
}
void MainWindow::retroshareLinkActivated(const QUrl &url)
{
    RetroShareLink link(url);

    if (link.valid() == false) {
        // QUrl can't handle the old RetroShare link format properly
        if (url.host().isEmpty()) {
            QMessageBox mb("RetroShare", tr("It seems to be an old RetroShare link. Please use copy instead."), QMessageBox::Critical, QMessageBox::Ok, 0, 0);
            mb.setWindowIcon(QIcon(":/images/rstray3.png"));
            mb.exec();
            return;
        }

        QMessageBox mb("RetroShare", tr("The file link is malformed."), QMessageBox::Critical, QMessageBox::Ok, 0, 0);
        mb.setWindowIcon(QIcon(":/images/rstray3.png"));
        mb.exec();
        return;
    }

    QList<RetroShareLink> links;
    links.append(link);
    RetroShareLink::process(links);
}

void MainWindow::servicePermission()
{
    ServicePermissionDialog::showYourself();
}
