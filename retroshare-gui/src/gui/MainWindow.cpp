/*******************************************************************************
 * gui/MainWindow.cpp                                                          *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
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

#include <QColorDialog>
#include <QDesktopServices>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QStatusBar>
#include <QString>
#include <QUrl>
#include <QtDebug>

#include <retroshare/rsplugin.h>
#include <retroshare/rsconfig.h>

#ifdef MESSENGER_WINDOW
#include "MessengerWindow.h"
#endif

#include "rshare.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "HomePage.h"
#include "NetworkDialog.h"
#include "gui/FileTransfer/SearchDialog.h"
#include "gui/FileTransfer/SharedFilesDialog.h"
#include "gui/FileTransfer/TransfersDialog.h"
#include "gui/msgs/MessagesDialog.h"
#include "PluginsPage.h"
#include "NewsFeed.h"
#include "ShareManager.h"
#include "NetworkView.h"
#include "FriendsDialog.h"
#include "ChatLobbyWidget.h"
#include "HelpDialog.h"
#include "AboutDialog.h"
#include "gui/statistics/BandwidthGraphWindow.h"
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

#ifdef RS_USE_CIRCLES
#include "gui/People/PeopleDialog.h"
#endif
#include "idle/idle.h"

#include "statusbar/peerstatus.h"
#include "statusbar/natstatus.h"
#include "statusbar/ratesstatus.h"
#include "statusbar/dhtstatus.h"
#include "statusbar/hashingstatus.h"
#include "statusbar/discstatus.h"
#include "statusbar/OpModeStatus.h"
#include "statusbar/SoundStatus.h"
#include "statusbar/ToasterDisable.h"
#include "statusbar/SysTrayStatus.h"
#include "statusbar/torstatus.h"

#include "retroshare/rsstatus.h"
#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsfiles.h"
#include "retroshare/rsnotify.h"
#include "retroshare/rsinit.h"

#include "gui/gxschannels/GxsChannelDialog.h"
#include "gui/gxsforums/GxsForumsDialog.h"
#include "gui/Identity/IdDialog.h"
//#ifdef RS_USE_CIRCLES
//#include "gui/Circles/CirclesDialog.h"
//#endif
#ifdef RS_USE_WIKI
#include "gui/WikiPoos/WikiDialog.h"
#endif
#ifdef RS_USE_WIRE
#include "gui/TheWire/WireDialog.h"
#endif
#ifdef RS_USE_PHOTO
#include "gui/PhotoShare/PhotoShare.h"
#endif
#include "gui/Posted/PostedDialog.h"
#include "gui/statistics/StatisticsWindow.h"

#include "gui/connect/ConnectFriendWizard.h"
#include "gui/common/RsCollection.h"
#include "settings/rsettingswin.h"
#include "settings/rsharesettings.h"
#include "common/StatusDefs.h"
#include "gui/notifyqt.h"

#ifdef RS_WEBUI
#	include "settings/WebuiPage.h"
#endif

#include <iomanip>
#include <unistd.h>

#define IMAGE_QUIT              ":/icons/png/exit.png"
#define IMAGE_PREFERENCES       ":/icons/png/options2.png"
#define IMAGE_ABOUT             ":/icons/png/info.png"
#define IMAGE_STATS             ":/icons/png/netgraph2.png"
#define IMAGE_CLOSE             ":/icons/png/exit2.png"

#define IMAGE_ADDFRIEND         ":/icons/png/invite.png"
#define IMAGE_RETROSHARE        ":/icons/logo_128.png"
#define IMAGE_NOONLINE          ":/icons/logo_0_connected_128.png"
#define IMAGE_ONEONLINE         ":/icons/logo_1_connected_128.png"
#define IMAGE_TWOONLINE         ":/icons/logo_2_connected_128.png"
#define IMAGE_OVERLAY           ":/icons/star_overlay_128.png"

#define IMAGE_BWGRAPH           ":/icons/png/bandwidth.png"
#define IMAGE_COLOR         	":/images/highlight.png"
#define IMAGE_NEWRSCOLLECTION   ":/images/library.png"
#define IMAGE_ADDSHARE          ":/images/directoryadd_24x24_shadow.png"
#define IMAGE_OPTIONS           ":/images/settings.png"
#define IMAGE_MINIMIZE          ":/icons/fullscreen.png"
#define IMAGE_MAXIMIZE          ":/icons/fullscreen-exit.png"

#define IMAGE_PLUGINS           ":/images/extension_32.png"

/*static*/ bool MainWindow::hiddenmode = false;

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
MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
    : RWindow("MainWindow", parent, flags)
{
    ui = new Ui::MainWindow;
    trayIcon = NULL;

	friendsDialog=NULL;
	idDialog=NULL;
	chatLobbyDialog=NULL;
	settingsDialog=NULL;
	transfersDialog=NULL;
	messagesDialog=NULL;
	gxschannelDialog=NULL;
	gxsforumDialog=NULL;
	postedDialog=NULL;

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
        if(pd.netMode == RS_NETMODE_HIDDEN)
            hiddenmode = true;
    }

    setWindowTitle(tr("RetroShare %1 a secure decentralized communication platform").arg(Rshare::retroshareVersion(true)) + " - " + nameAndLocation);
    connect(rApp, SIGNAL(newArgsReceived(QStringList)), this, SLOT(receiveNewArgs(QStringList)));

    /* add url handler for RetroShare links */
    QDesktopServices::setUrlHandler(RSLINK_SCHEME, this, "retroshareLinkActivated");
    QDesktopServices::setUrlHandler("http", this, "externalLinkActivated");
    QDesktopServices::setUrlHandler("https", this, "externalLinkActivated");

    // Setting icons
    this->setWindowIcon(QIcon(QString::fromUtf8(":/icons/logo_128.png")));

    /* Create all the dialogs of which we only want one instance */
    _bandwidthGraph = NULL ;

    #ifdef UNFINISHED
    applicationWindow = new ApplicationWindow();
    applicationWindow->hide();
    #endif

    initStackedPage();
    connect(ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(setNewPage(int)));
    connect(ui->stackPages, SIGNAL(currentChanged(int)), this, SLOT(setNewPage(int)));

	int lastpageindex = Settings->getLastPageInMainWindow();
	if(lastpageindex < ui->stackPages->count())	//Do not crash when a page was removed after last run
		setNewPage(lastpageindex);

    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);

    /* Load listWidget postion */
    QByteArray state = Settings->valueFromGroup("MainWindow", "SplitterState", QByteArray()).toByteArray();
    if (!state.isEmpty()) ui->splitter->restoreState(state);
    state = Settings->valueFromGroup("MainWindow", "State", QByteArray()).toByteArray();
    if (!state.isEmpty()) restoreState(state);

    /** StatusBar section ********/
    statusBar()->setVisible(Settings->valueFromGroup("StatusBar", "ShowStatusBar", QVariant(true)).toBool());

    /* initialize combobox in status bar */
    statusComboBox = new RSComboBox(statusBar());
    statusComboBox->setVisible(Settings->valueFromGroup("StatusBar", "ShowStatus", QVariant(true)).toBool());
    statusComboBox->setFocusPolicy(Qt::ClickFocus);
    initializeStatusObject(statusComboBox, true);

    QWidget *widget = new QWidget();
    widget->setObjectName("trans_statusComboBoxFrame");
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(6);
    hbox->addWidget(statusComboBox);
    widget->setLayout(hbox);
    statusBar()->addWidget(widget);

    peerstatus = new PeerStatus();
    peerstatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowPeer", QVariant(true)).toBool());
    statusBar()->addWidget(peerstatus);

	if(hiddenmode)
	{
        if(RsAccounts::isHiddenNode())
		{
			torstatus = new TorStatus();
			torstatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowTor", QVariant(true)).toBool());
			statusBar()->addWidget(torstatus);
			torstatus->getTorStatus();
		}
        else
			torstatus = NULL ;

		natstatus = NULL ;
		dhtstatus = NULL ;
	}
	else
	{
		torstatus = NULL ;

		natstatus = new NATStatus();
		if(hiddenmode) natstatus->setVisible(false);
		else natstatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowNAT", QVariant(true)).toBool());
		statusBar()->addWidget(natstatus);
		natstatus->getNATStatus();

		dhtstatus = new DHTStatus();
		if(hiddenmode) dhtstatus->setVisible(false);
		else dhtstatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowDHT", QVariant(true)).toBool());
		statusBar()->addWidget(dhtstatus);
		dhtstatus->getDHTStatus();
	}

    hashingstatus = new HashingStatus();
    hashingstatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowHashing", QVariant(true)).toBool());
    statusBar()->addPermanentWidget(hashingstatus, 1);

    discstatus = new DiscStatus();
    discstatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowDisc", QVariant(true)).toBool());
    statusBar()->addPermanentWidget(discstatus);

    ratesstatus = new RatesStatus();
    ratesstatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowRate", QVariant(true)).toBool());
    statusBar()->addPermanentWidget(ratesstatus);

    opModeStatus = new OpModeStatus();
    opModeStatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowOpMode", QVariant(false)).toBool());
    statusBar()->addPermanentWidget(opModeStatus);

    soundStatus = new SoundStatus();
    soundStatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowSound", QVariant(true)).toBool());
    statusBar()->addPermanentWidget(soundStatus);

    toasterDisable = new ToasterDisable();
    toasterDisable->setVisible(Settings->valueFromGroup("StatusBar", "ShowToaster", QVariant(true)).toBool());
    statusBar()->addPermanentWidget(toasterDisable);

    sysTrayStatus = new SysTrayStatus();
    sysTrayStatus->setVisible(Settings->valueFromGroup("StatusBar", "ShowSysTrayOnStatusBar", QVariant(false)).toBool());
    statusBar()->addPermanentWidget(sysTrayStatus);


    setCompactStatusMode(Settings->valueFromGroup("StatusBar", "CompactMode", QVariant(false)).toBool());

    /** Status Bar end ******/

    /* Creates a tray icon with a context menu and adds it to the system's * notification area. */
    createTrayIcon();

    createNotifyIcons();

    /* calculate friend count */
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

    connect(NotifyQt::getInstance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
    settingsChanged();
}

/** Destructor. */
MainWindow::~MainWindow()
{
    Settings->setLastPageInMainWindow(ui->stackPages->currentIndex());
    /* Save listWidget position */
    Settings->setValueToGroup("MainWindow", "SplitterState", ui->splitter->saveState());
    Settings->setValueToGroup("MainWindow", "State", saveState());

    delete statusComboBox;
    delete peerstatus;
    delete natstatus;
    delete dhtstatus;
    delete hashingstatus;
    delete discstatus;
    delete ratesstatus;
    delete opModeStatus;
    delete soundStatus;
    delete toasterDisable;
    delete sysTrayStatus;
    delete trayIcon;
    delete trayMenu;
//  delete notifyMenu; // already deleted by the deletion of trayMenu
    StatisticsWindow::releaseInstance();

#ifdef MESSENGER_WINDOW
    MessengerWindow::releaseInstance();
#endif
#ifdef UNFINISHED
    delete applicationWindow;
#endif

    delete(ui);
}

/** Initialyse Stacked Page*/
void MainWindow::initStackedPage()
{
  /* WORK OUT IF WE"RE IN ADVANCED MODE OR NOT */
  //bool advancedMode = false;
  //std::string advsetting;
  //if (rsConfig->getConfigurationOption(RS_CONFIG_ADVANCED, advsetting) && (advsetting == "YES")) {
  //    advancedMode = true;
  //}

  QList<QPair<MainPage*, QPair<QAction*, QListWidgetItem*> > > notify;

  /* Create the Main pages and actions */
  QActionGroup *grp = new QActionGroup(this);

  addPage(homePage = new HomePage(ui->stackPages), grp, NULL);

//#ifdef RS_USE_CIRCLES
//  CirclesDialog *circlesDialog = NULL;
//  addPage(circlesDialog = new CirclesDialog(ui->stackPages), grp, &notify);
//#endif
  addPage(friendsDialog = new FriendsDialog(ui->stackPages), grp, &notify);
  addPage(idDialog = new IdDialog(ui->stackPages), grp, &notify);
  addPage(chatLobbyDialog = new ChatLobbyWidget(ui->stackPages), grp, &notify);
  addPage(messagesDialog = new MessagesDialog(ui->stackPages), grp, &notify);
  addPage(transfersDialog = new TransfersDialog(ui->stackPages), grp, &notify);
  addPage(gxschannelDialog = new GxsChannelDialog(ui->stackPages), grp, &notify);
  addPage(gxsforumDialog = new GxsForumsDialog(ui->stackPages), grp, &notify);
  addPage(postedDialog = new PostedDialog(ui->stackPages), grp, &notify);

  #ifdef RS_USE_NEW_PEOPLE_DIALOG
  PeopleDialog *peopleDialog = NULL;
  addPage(peopleDialog = new PeopleDialog(ui->stackPages), grp, &notify);
  #endif
#ifdef RS_USE_WIKI
  WikiDialog *wikiDialog = NULL;
  addPage(wikiDialog = new WikiDialog(ui->stackPages), grp, &notify);
#endif

#ifdef RS_USE_WIRE
  WireDialog *wireDialog = NULL;
  addPage(wireDialog = new WireDialog(ui->stackPages), grp, &notify);
#endif

#ifdef RS_USE_PHOTO
  PhotoShare *photoDialog = NULL;
  addPage(photoDialog = new PhotoShare(ui->stackPages), grp, &notify);
#endif


 std::cerr << "Looking for interfaces in existing plugins:" << std::endl;
 for(int i = 0;i<rsPlugins->nbPlugins();++i)
 {
	 MainPage *pluginPage = NULL;
	 QIcon icon, *pIcon = NULL;

	 if(rsPlugins->plugin(i) != NULL && (pluginPage = rsPlugins->plugin(i)->qt_page()) != NULL)
	 {
		 if((pIcon = rsPlugins->plugin(i)->qt_icon()) != NULL)
			 icon = *pIcon ;
		 else
			 icon = QIcon(":images/extension_48.png") ;

		 std::cerr << "  Adding widget page for plugin " << rsPlugins->plugin(i)->getPluginName() << std::endl;
		 pluginPage->setIconPixmap(icon);
		 pluginPage->setPageName(QString::fromUtf8(rsPlugins->plugin(i)->getPluginName().c_str()));
		 addPage(pluginPage, grp, &notify);
	 }
	 else if(rsPlugins->plugin(i) == NULL)
		 std::cerr << "  No plugin object !" << std::endl;
	 else
		 std::cerr << "  No plugin page !" << std::endl;

 }

#ifndef RS_RELEASE_VERSION
  addPage(pluginsPage = new gui::PluginsPage(ui->stackPages), grp, NULL);
#endif

#undef GETSTARTED_GUI
#ifdef GETSTARTED_GUI
  MainPage *getStartedPage = NULL;

  if (!advancedMode)
  {
      //ui->stackPages->add(getStartedPage = new GetStartedDialog(ui->stackPages),
      //               createPageAction(QIcon(IMG_HELP), tr("Getting Started"), grp));
      addPage(getStartedPage = new GetStartedDialog(ui->stackPages), grp, NULL);
  }
#endif


  addPage(newsFeed = new NewsFeed(ui->stackPages), grp, &notify);

  //List All notify before Setting was created
  for (auto notifyIt:notify)
  {
      UserNotify *userNotify = notifyIt.first->getUserNotify();
      if (userNotify) {
          userNotify->initialize(ui->toolBarPage, notifyIt.second.first, notifyIt.second.second);
          connect(userNotify, SIGNAL(countChanged()), this, SLOT(updateTrayCombine()));
          userNotifyList.push_back(userNotify);
      }
  }

  addPage(settingsDialog = new SettingsPage(ui->stackPages),grp,&notify);

  /* Create the toolbar */
  ui->toolBarPage->addActions(grp->actions());
  connect(grp, SIGNAL(triggered(QAction *)), ui->stackPages, SLOT(showPage(QAction *)));


#ifdef UNFINISHED
  addAction(new QAction(QIcon(), tr("Unfinished"), ui->toolBar), &MainWindow::showApplWindow, SLOT(showApplWindow()));
  ui->toolBarAction->addSeparator();
  notify += applicationWindow->getNotify();
#endif

  /** Add icon on Action bar */
  // I remove add a friend because it's in HOME ghibli
  //addAction(new QAction(QIcon(IMAGE_ADDFRIEND), tr("Add"), ui->toolBarAction), &MainWindow::addFriend, SLOT(addFriend()));
  //addAction(new QAction(QIcon(IMAGE_NEWRSCOLLECTION), tr("New"), ui->toolBarAction), &MainWindow::newRsCollection, SLOT(newRsCollection()));
  //addAction(new QAction(QIcon(IMAGE_PREFERENCES), tr("Options"), ui->toolBarAction), &MainWindow::showSettings, SLOT(showSettings()));

  // Removed About because it's now in options.
  //addAction(new QAction(QIcon(IMAGE_ABOUT), tr("About"), ui->toolBarAction), &MainWindow::showabout, SLOT(showabout()));

  addAction(new QAction(QIcon(IMAGE_QUIT), tr("Quit"), ui->toolBarAction), &MainWindow::doQuit, SLOT(doQuit()));

}

/** Creates a new action associated with a config page. */
QAction *MainWindow::createPageAction(const QIcon &icon, const QString &text, QActionGroup *group)
{
    QAction *action = new QAction(icon, text, group);
    action->setCheckable(true);
    return action;
}

/** Adds the given action to the toolbar and hooks its triggered() signal to
 * the specified slot (if given).
 * Have to pass function pointer and slot, because we can't make slot of function pointer */
void MainWindow::addAction(QAction *action, FunctionType actionFunction, const char *slot)
{
    ui->toolBarAction->addAction(action);
    if (slot) connect(action, SIGNAL(triggered()), this, slot);

    QListWidgetItem *item = new QListWidgetItem(action->icon(),action->text()) ;
    item->setData(Qt::UserRole,QString(slot));
    ui->listWidget->addItem(item) ;

    if (slot) _functionList[slot] = actionFunction;
}

/** Add the given page to the stackPage and list. */
void MainWindow::addPage(MainPage *page, QActionGroup *grp, QList<QPair<MainPage*, QPair<QAction*, QListWidgetItem*> > > *notify)
{
	QAction *action;
	ui->stackPages->add(page, action = createPageAction(page->iconPixmap(), page->pageName(), grp));

	QListWidgetItem *item = new QListWidgetItem(QIcon(page->iconPixmap()),page->pageName()) ;
	ui->listWidget->addItem(item) ;

	if (notify)
	{
		QPair<QAction*, QListWidgetItem*> pair = QPair<QAction*, QListWidgetItem*>( action, item);
		notify->push_back(QPair<MainPage*, QPair<QAction*, QListWidgetItem*> >(page, pair));
	}
}

/** Selection page. */
void MainWindow::setNewPage(int page)
{
	MainPage *pagew = dynamic_cast<MainPage*>(ui->stackPages->widget(page)) ;

	if(pagew)
	{
		ui->stackPages->setCurrentIndex(page);
		ui->listWidget->setCurrentRow(page);
	} else {
		QString procName = ui->listWidget->item(page)->data(Qt::UserRole).toString();
		FunctionType function = _functionList[procName];
		if (function) (this->*function)();

		ui->listWidget->setCurrentRow(ui->stackPages->currentIndex());
	}
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
				tr("The disk space in your")+" "+locString +" "+tr("directory is running low (current limit is")+" "+QString::number(size_limit_mb)+tr("MB). \n\n RetroShare will now safely suspend any disk access to this directory. \n\n Please make some free space and click Ok.")) ;
}

/** Creates a tray icon with a context menu and adds it to the system
 * notification area. */
void MainWindow::createTrayIcon()
{
    /** Tray icon Menu **/
    trayMenu = new QMenu(this);
    if (sysTrayStatus) sysTrayStatus->trayMenu = trayMenu;
    QObject::connect(trayMenu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
    toggleVisibilityAction = trayMenu->addAction(QIcon(IMAGE_RETROSHARE), tr("Show/Hide"), this, SLOT(toggleVisibilitycontextmenu()));
    if (sysTrayStatus) sysTrayStatus->toggleVisibilityAction = toggleVisibilityAction;

    /* Create status menu */
    QMenu *statusMenu = trayMenu->addMenu(tr("Status"));
    initializeStatusObject(statusMenu, true);

    /* Create notify menu */
    notifyMenu = trayMenu->addMenu(tr("Notify"));
    notifyMenu->menuAction()->setVisible(false);

    trayMenu->addSeparator();
#ifdef MESSENGER_WINDOW
    trayMenu->addAction(QIcon(), tr("Open Messenger"), this, SLOT(showMessengerWindow()));
#endif
    trayMenu->addAction(QIcon(IMAGE_MESSAGES), tr("Open Messages"), this, SLOT(showMess()));
#ifdef RS_JSONAPI
#ifdef RS_WEBUI
    trayMenu->addAction(QIcon(":/images/emblem-web.png"), tr("Show web interface"), this, SLOT(showWebinterface()));
#endif
#endif
    trayMenu->addAction(QIcon(IMAGE_BWGRAPH), tr("Bandwidth Graph"), this, SLOT(showBandwidthGraph()));
    trayMenu->addAction(QIcon(IMAGE_STATS), tr("Statistics"), this, SLOT(showStatisticsWindow()));


#ifdef UNFINISHED
    trayMenu->addAction(QIcon(), tr("Applications"), this, SLOT(showApplWindow()));
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

void MainWindow::showBandwidthGraph()
{
	if(_bandwidthGraph == NULL)
		_bandwidthGraph = new BandwidthGraph();

	_bandwidthGraph->showWindow();
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
        for (actionIt = actions.begin(); actionIt != actions.end(); ++actionIt) {
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
    if (notifyMenu)
        notifyMenu->menuAction()->setVisible(visible);

    // update tray icon
    updateFriends();
}

void MainWindow::toggleStatusToolTip(bool toggle){
    if(!toggle)return;
    QString tray = "RetroShare\n";
    tray += "\n" + nameAndLocation;
    trayIcon->setToolTip(tray);
}

void MainWindow::updateStatus()
{
    // This call is essential to remove locks due to QEventLoop re-entrance while asking gpg passwds. Dont' remove it!
    if(RsAutoUpdatePage::eventsLocked())
        return;
    float downKb = 0;
    float upKb = 0;
    rsConfig->GetCurrentDataRates(downKb, upKb);
	uint64_t down = 0;
	uint64_t up = 0;
	rsConfig->GetTrafficSum(down, up);

    if (ratesstatus)
        ratesstatus->getRatesStatus(downKb, down, upKb, up);

    if(torstatus)
        torstatus->getTorStatus();

    if(!hiddenmode)
    {
        if (natstatus)
            natstatus->getNATStatus();

        if (dhtstatus)
            dhtstatus->getDHTStatus();
    }

    if (discstatus) {
        discstatus->update();
    }

    if(!Settings->valueFromGroup("StatusBar", "DisableSysTrayToolTip", QVariant(false)).toBool()) {

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
}

void MainWindow::updateFriends()
{
	if(RsAutoUpdatePage::eventsLocked())
		return ;

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
        QPixmap overlayImage(IMAGE_OVERLAY);

        QPainter painter(&trayImage);
        painter.drawPixmap(0, 0, overlayImage);

        icon.addPixmap(trayImage);
    } else {
        icon = QIcon(trayIconResource);
    }

    if (trayIcon) trayIcon->setIcon(icon);
    if (sysTrayStatus) sysTrayStatus->setIcon(icon);
}

void MainWindow::postModDirectories(bool /*update_local*/)
{
    //RSettingsPage::postModDirectories(update_local);

    QCoreApplication::flush();
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
    SetForegroundWindowInternal((HWND) _instance->winId());
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
		 case People:
			 _instance->ui->stackPages->setCurrentPage( _instance->idDialog );
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
		 case Options:
			 _instance->ui->stackPages->setCurrentPage( _instance->settingsDialog );
			 break;
		 case Messages:
			 _instance->ui->stackPages->setCurrentPage( _instance->messagesDialog );
			 break;
		 case Channels:
                         _instance->ui->stackPages->setCurrentPage( _instance->gxschannelDialog );
			 return true ;
		 case Forums:
                         _instance->ui->stackPages->setCurrentPage( _instance->gxsforumDialog );
                         return true ;
		case Posted:
			_instance->ui->stackPages->setCurrentPage( _instance->postedDialog );
			return true ;

#ifdef RS_USE_WIRE
        case Wire:
            _instance->ui->stackPages->setCurrentPage( _instance->wireDialog );
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
   if (page == _instance->settingsDialog) {
       return Options;
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
#if 0
   if (page == _instance->channelFeed) {
       return Channels;
   }
   if (page == _instance->forumsDialog) {
       return Forums;
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
		case People:
			return _instance->idDialog;
		case ChatLobby:
			return _instance->chatLobbyDialog;
		case Options:
			return _instance->settingsDialog;
		case Transfers:
			return _instance->transfersDialog;
		case SharedDirectories:
			return _instance->transfersDialog->localSharedFiles;
		case Search:
			return _instance->transfersDialog->searchDialog;
		case Messages:
			return _instance->messagesDialog;
		case Channels:
			return _instance->gxschannelDialog;
		case Forums:
			return _instance->gxsforumDialog;
		case Posted:
			return _instance->postedDialog;
        case Home:
            return _instance->homePage;

#ifdef RS_USE_WIRE
        case Wire:
            return _instance->wireDialog;
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

/** New RSCollection ShortCut */
// void MainWindow::newRsCollection()
// {
//     std::vector <DirDetails> dirVec;
//
//     RsCollection(dirVec).openNewColl(this);
// }

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
    showWindow(MainWindow::Options);
}

#ifdef MESSENGER_WINDOW
/** Shows Messenger window */
void MainWindow::showMessengerWindow()
{
    MessengerWindow::showYourself();
}
#endif

/** Shows Statistics window */
void MainWindow::showStatisticsWindow()
{
    StatisticsWindow::showYourself();
}

#ifdef RS_JSONAPI
#ifdef RS_WEBUI
void MainWindow::showWebinterface()
{
    WebuiPage::showWebui();
}
#endif // ENABLE_WEBUI
#endif 

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

		if ((QMessageBox::question(this, tr("Really quit ?"),queryWrn,QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
		{
			qApp->quit();
		}
		else
			return;
	}

	rApp->quit();
}

void MainWindow::receiveNewArgs(QStringList args)
{
	Rshare::parseArguments(args, false);
	processLastArgs();
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
            if(torstatus)
                torstatus->reset();
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
  //retranslateUi();
  foreach (MainPage *page, ui->stackPages->pages()) {
    page->retranslateUi();
  }
  foreach (QAction *action, ui->toolBarPage->actions()) {
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
    RSComboBox *pComboBox = dynamic_cast<RSComboBox*>(pObject);
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
        for (std::set <QObject*>::iterator it = m_apStatusObjects.begin(); it != m_apStatusObjects.end(); ++it) {
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

    //std::string statusString;

    QMenu *pMenu = dynamic_cast<QMenu*>(pObject);
    if (pMenu) {
        /* initialize menu */
        QActionGroup *pGroup = new QActionGroup(pMenu);

        QAction *pAction = new QAction(QIcon(StatusDefs::imageStatus(RS_STATUS_ONLINE)), StatusDefs::name(RS_STATUS_ONLINE), pMenu);
        pAction->setData(RS_STATUS_ONLINE);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);

        pAction = new QAction(QIcon(StatusDefs::imageStatus(RS_STATUS_BUSY)), StatusDefs::name(RS_STATUS_BUSY), pMenu);
        pAction->setData(RS_STATUS_BUSY);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);

        pAction = new QAction(QIcon(StatusDefs::imageStatus(RS_STATUS_AWAY)), StatusDefs::name(RS_STATUS_AWAY), pMenu);
        pAction->setData(RS_STATUS_AWAY);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);

        if (bConnect) {
            connect(pMenu, SIGNAL(triggered (QAction*)), this, SLOT(statusChangedMenu(QAction*)));
        }
    } else {
        /* initialize combobox */
        RSComboBox *pComboBox = dynamic_cast<RSComboBox*>(pObject);
        if (pComboBox) {
            pComboBox->addItem(QIcon(StatusDefs::imageStatus(RS_STATUS_ONLINE)), StatusDefs::name(RS_STATUS_ONLINE), RS_STATUS_ONLINE);
            pComboBox->addItem(QIcon(StatusDefs::imageStatus(RS_STATUS_BUSY)), StatusDefs::name(RS_STATUS_BUSY), RS_STATUS_BUSY);
            pComboBox->addItem(QIcon(StatusDefs::imageStatus(RS_STATUS_AWAY)), StatusDefs::name(RS_STATUS_AWAY), RS_STATUS_AWAY);

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

    rsStatus->sendStatus(RsPeerId(), nStatus);

    /* set status in all status objects, but the calling one */
    for (std::set <QObject*>::iterator it = m_apStatusObjects.begin(); it != m_apStatusObjects.end(); ++it) {
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

/*new setting*/
void MainWindow::settingsChanged()
{
	ui->toolBarPage->setVisible(Settings->getPageButtonLoc());
	ui->toolBarAction->setVisible(Settings->getActionButtonLoc());
	ui->listWidget->setVisible(!Settings->getPageButtonLoc() || !Settings->getActionButtonLoc());
	for(int i = 0; i < ui->listWidget->count(); ++i) {
		if (ui->listWidget->item(i)->data(Qt::UserRole).toString() == "") {
			ui->listWidget->item(i)->setHidden(Settings->getPageButtonLoc());
		} else {
			ui->listWidget->item(i)->setHidden(Settings->getActionButtonLoc());
		}
	}
	int toolSize = Settings->getToolButtonSize();
	ui->toolBarPage->setToolButtonStyle(Settings->getToolButtonStyle());
	ui->toolBarPage->setIconSize(QSize(toolSize,toolSize));
	ui->toolBarAction->setToolButtonStyle(Settings->getToolButtonStyle());
	ui->toolBarAction->setIconSize(QSize(toolSize,toolSize));
	int itemSize = Settings->getListItemIconSize();
	ui->listWidget->setIconSize(QSize(itemSize,itemSize));
}

void MainWindow::externalLinkActivated(const QUrl &url)
{
	static bool already_warned = false ;
	bool never_ask_me = Settings->value("NeverAskMeForExternalLinkActivated",false).toBool();

	if(!already_warned && !never_ask_me)
	{
		QMessageBox mb(QObject::tr("Confirmation"), QObject::tr("Do you want this link to be handled by your system?")+"<br/><br/>"+ url.toString()+"<br/><br/>"+tr("Make sure this link has not been forged to drag you to a malicious website."), QMessageBox::Question, QMessageBox::Yes,QMessageBox::No, 0);

		QCheckBox *dontAsk_CB = new QCheckBox(tr("Don't ask me again"));
		QCheckBox *neverAsk_CB = new QCheckBox(tr("Never ask me again"));
		dontAsk_CB->setToolTip(tr("This will be saved only for this session."));
		neverAsk_CB->setToolTip(tr("This will be saved permanently. You'll need to clean RetroShare.conf to revert."));
		QGridLayout* layout = qobject_cast<QGridLayout*>(mb.layout());
		if (layout)
		{
			layout->addWidget(dontAsk_CB,layout->rowCount(),0,1, layout->columnCount(), Qt::AlignLeft);
			layout->addWidget(neverAsk_CB,layout->rowCount(),0,1, layout->columnCount(), Qt::AlignLeft);
		} else {
			//Not QGridLayout so add at end
			mb.layout()->addWidget(dontAsk_CB);
			mb.layout()->addWidget(neverAsk_CB);
		}

		int res = mb.exec() ;

		if (res == QMessageBox::No)
			return ;

		if(dontAsk_CB->isChecked())
			already_warned = true ;

		if(neverAsk_CB->isChecked())
			Settings->setValue("NeverAskMeForExternalLinkActivated",true);
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
            mb.exec();
            return;
        }

        QMessageBox mb("RetroShare", tr("The file link is malformed."), QMessageBox::Critical, QMessageBox::Ok, 0, 0);
        mb.exec();
        return;
    }

    QList<RetroShareLink> links;
    links.append(link);
    RetroShareLink::process(links);
}

void MainWindow::openRsCollection(const QString &filename)
{
	QFileInfo qinfo(filename);
	if (qinfo.exists()) {
		if (qinfo.absoluteFilePath().endsWith(RsCollection::ExtensionString)) {
			RsCollection collection;
			collection.openColl(qinfo.absoluteFilePath());
		}
	}
}

void MainWindow::processLastArgs()
{
	while (!Rshare::links()->isEmpty()) {
		std::cerr << "MainWindow::processLastArgs() : " << Rshare::links()->count() << std::endl;
		/* Now use links from the command line, because no RetroShare was running */
		RetroShareLink link(Rshare::links()->takeFirst());
		if (link.valid()) {
			retroshareLinkActivated(link.toUrl());
		}
	}
	while (!Rshare::files()->isEmpty()) {
		/* Now use files from the command line, because no RetroShare was running */
		openRsCollection(Rshare::files()->takeFirst());
	}
	/* Handle the -opmode options. */
	if (opModeStatus) {
		QString opmode = Rshare::opmode().toLower();
		if (opmode == "noturtle") {
			opModeStatus->setCurrentIndex(static_cast<typename std::underlying_type<RsOpMode>::type>(RsOpMode::NOTURTLE) - 1);
		} else if (opmode == "gaming") {
			opModeStatus->setCurrentIndex(static_cast<typename std::underlying_type<RsOpMode>::type>(RsOpMode::GAMING) - 1);
		} else if (opmode == "minimal") {
			opModeStatus->setCurrentIndex(static_cast<typename std::underlying_type<RsOpMode>::type>(RsOpMode::MINIMAL) - 1);
		} else if (opmode != "") {
			opModeStatus->setCurrentIndex(static_cast<typename std::underlying_type<RsOpMode>::type>(RsOpMode::FULL) - 1);
		}
		opModeStatus->setOpMode();
	} else {
		std::cerr << "ERR: MainWindow::processLastArgs opModeStatus is not initialized.";
	}

}

void MainWindow::switchVisibilityStatus(StatusElement e,bool b)
{
    switch(e)
    {
        case 	StatusGrpStatus   : getInstance()->statusBar()               ->setVisible(b); break ;
        case 	StatusCompactMode : getInstance()->setCompactStatusMode(b) ; break ;
        case 	StatusShowToolTip : getInstance()->toggleStatusToolTip(b) ; break ;
        case 	StatusShowStatus  : getInstance()->statusComboBoxInstance()  ->setVisible(b); break ;
        case 	StatusShowPeer    : getInstance()->peerstatusInstance()      ->setVisible(b); break ;
        case 	StatusShowNAT     : getInstance()->natstatusInstance()       ->setVisible(b); break ;
        case 	StatusShowDHT     : getInstance()->dhtstatusInstance()       ->setVisible(b); break ;
        case 	StatusShowHashing : getInstance()->hashingstatusInstance()   ->setVisible(b); break ;
        case 	StatusShowDisc    : getInstance()->discstatusInstance()      ->setVisible(b); break ;
        case 	StatusShowRate    : getInstance()->ratesstatusInstance()     ->setVisible(b); break ;
        case 	StatusShowOpMode  : getInstance()->opModeStatusInstance()    ->setVisible(b); break ;
        case 	StatusShowSound   : getInstance()->soundStatusInstance()     ->setVisible(b); break ;
        case 	StatusShowToaster : getInstance()->toasterDisableInstance()  ->setVisible(b); break ;
        case 	StatusShowSystray : getInstance()->sysTrayStatusInstance()   ->setVisible(b); break ;

    default:
        std::cerr << "(EE) Unknown object to change visibility of: " << (int)e << std::endl;
    }
}

//void MainWindow::servicePermission()
//{
//    ServicePermissionDialog::showYourself();
//}
RSComboBox *MainWindow::statusComboBoxInstance()
{
	return statusComboBox;
}

PeerStatus *MainWindow::peerstatusInstance()
{
	return peerstatus;
}

NATStatus *MainWindow::natstatusInstance()
{
	return natstatus;
}

void MainWindow::torstatusReset()
{
	if(torstatus != nullptr)
		torstatus->reset();
}
DHTStatus *MainWindow::dhtstatusInstance()
{
	return dhtstatus;
}

HashingStatus *MainWindow::hashingstatusInstance()
{
	return hashingstatus;
}

DiscStatus *MainWindow::discstatusInstance()
{
	return discstatus;
}

RatesStatus *MainWindow::ratesstatusInstance()
{
	return ratesstatus;
}

OpModeStatus *MainWindow::opModeStatusInstance()
{
	return opModeStatus;
}

SoundStatus *MainWindow::soundStatusInstance()
{
	return soundStatus;
}

ToasterDisable *MainWindow::toasterDisableInstance()
{
	return toasterDisable;
}

SysTrayStatus *MainWindow::sysTrayStatusInstance()
{
	return sysTrayStatus;
}

void MainWindow::setCompactStatusMode(bool compact)
{
	//statusComboBox: TODO Show only icon
	peerstatus->setCompactMode(compact);
	updateFriends();
    if(!hiddenmode)
    {
	natstatus->setCompactMode(compact);
	natstatus->getNATStatus();
	dhtstatus->setCompactMode(compact);
	dhtstatus->getDHTStatus();
    }

	if(torstatus)
		torstatus->setCompactMode(compact) ;

	hashingstatus->setCompactMode(compact);
	ratesstatus->setCompactMode(compact);
	//opModeStatus: TODO Show only ???
}

Gui_InputDialogReturn MainWindow::guiInputDialog(const QString& windowTitle, const QString& labelText, QLineEdit::EchoMode textEchoMode, bool modal)
{

	QInputDialog dialog(this);
	dialog.setWindowTitle(windowTitle);
	dialog.setLabelText(labelText);
	dialog.setTextEchoMode(textEchoMode);
	dialog.setModal(modal);

	Gui_InputDialogReturn ret;
	ret.execReturn = dialog.exec();
	ret.textValue = dialog.textValue();

	return ret;
}
