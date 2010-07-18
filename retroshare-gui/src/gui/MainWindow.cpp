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

#ifdef BLOGS
#include "gui/unfinished/blogs/BlogsDialog.h"
#endif 

#include "rshare.h"
#include "MainWindow.h"
#include "MessengerWindow.h"
#include "NetworkDialog.h"
#include "SearchDialog.h"
#include "TransfersDialog.h"
#include "MessagesDialog.h"
#include "SharedFilesDialog.h"
#include "PluginsPage.h"
#include "ShareManager.h"
#include "NetworkView.h"
#include "LinksDialog.h"
#include "ForumsDialog.h"
#include "NewsFeed.h"
#include "PeersDialog.h"
#include "HelpDialog.h"
#include "AboutDialog.h"
#include "QuickStartWizard.h"

#include "gui/TurtleRouterDialog.h"
#include "idle/idle.h"

#include "statusbar/peerstatus.h"
#include "statusbar/natstatus.h"
#include "statusbar/ratesstatus.h"
#include "rsiface/rsstatus.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"

#include "gui/connect/ConnectFriendWizard.h"
#include "util/rsversion.h"
#include "settings/rsettingswin.h"
#include "settings/rsharesettings.h"

#include <sstream>
#include <iomanip>
#include <unistd.h>

#define FONT        QFont("Arial", 9)

/* Images for toolbar icons */
#define IMAGE_NETWORK2          ":/images/rs1.png"
#define IMAGE_PEERS         	  ":/images/groupchat.png"
#define IMAGE_SEARCH    		    ":/images/filefind.png"
#define IMAGE_TRANSFERS      	  ":/images/ktorrent32.png"
#define IMAGE_LINKS             ":/images/irkick.png"
#define IMAGE_FILES   	        ":/images/fileshare24.png"
#define IMAGE_CHANNELS       	  ":/images/channels.png"
#define IMAGE_FORUMS            ":/images/konversation.png"
#define IMAGE_PREFERENCES       ":/images/kcmsystem24.png"
#define IMAGE_CHAT          	  ":/images/groupchat.png"
#define IMAGE_RETROSHARE        ":/images/rstray3.png"
#define IMAGE_ABOUT             ":/images/informations_24x24.png"
#define IMAGE_STATISTIC         ":/images/utilities-system-monitor.png"
#define IMAGE_MESSAGES          ":/images/evolution.png"
#define IMAGE_BWGRAPH           ":/images/ksysguard.png"
#define IMAGE_RSM32             ":/images/kdmconfig.png"
#define IMAGE_RSM16             ":/images/rsmessenger16.png"
#define IMAGE_CLOSE             ":/images/close_normal.png"
#define IMAGE_BLOCK         	  ":/images/blockdevice.png"
#define IMAGE_COLOR         	  ":/images/highlight.png"
#define IMAGE_GAMES             ":/images/kgames.png"
#define IMAGE_PHOTO             ":/images/lphoto.png"
#define IMAGE_ADDFRIEND         ":/images/add-friend24.png"
#define IMAGE_ADDSHARE          ":/images/directoryadd_24x24_shadow.png"
#define IMAGE_OPTIONS           ":/images/settings.png"
#define IMAGE_QUIT              ":/images/exit_24x24.png"
#define IMAGE_UNFINISHED        ":/images/underconstruction.png"
#define IMAGE_MINIMIZE          ":/images/window_nofullscreen.png"
#define IMAGE_MAXIMIZE          ":/images/window_fullscreen.png"
#define IMG_HELP                ":/images/help.png"
#define IMAGE_NEWSFEED          ":/images/konqsidebar_news24.png"
#define IMAGE_PLUGINS           ":/images/extension_32.png"
#define IMAGE_NOONLINE          ":/images/rstray0.png"
#define IMAGE_ONEONLINE         ":/images/rstray1.png"
#define IMAGE_TWOONLINE         ":/images/rstray2.png"
#define IMAGE_BLOGS             ":/images/kblogger.png"

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
    : RWindow("MainWindow", parent, flags), maxTimeBeforeIdle(30)
{
    /* Invoke the Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    _instance = this;

    m_bStatusLoadDone = false;
    isIdle = false;

    if (Settings->value(QString::fromUtf8("FirstRun"), true).toBool())
    {
		Settings->setValue(QString::fromUtf8("FirstRun"), false);
		QuickStartWizard *qstartWizard = new QuickStartWizard(this);
		qstartWizard->exec();
    }

    setWindowTitle(tr("RetroShare %1 a secure decentralised communication platform").arg(retroshareVersion()));

    // Setting icons
    this->setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));

    /* Create all the dialogs of which we only want one instance */
    _bandwidthGraph = new BandwidthGraph();

    #ifdef UNFINISHED
    applicationWindow = new ApplicationWindow();
    applicationWindow->hide();
    #endif    

    /** Left Side ToolBar**/
    connect(ui.actionAdd_Friend, SIGNAL(triggered() ), this , SLOT( addFriend() ) );
    connect(ui.actionAdd_Share, SIGNAL(triggered() ), this , SLOT( openShareManager() ) );
    connect(ui.actionOptions, SIGNAL(triggered()), this, SLOT( showSettings()) );
    connect(ui.actionMessenger, SIGNAL(triggered()), this, SLOT( showMessengerWindow()) );

    ui.actionMessenger->setVisible(true);

    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT( showabout()) );
    //connect(ui.actionColor, SIGNAL(triggered()), this, SLOT( setStyle()) );


    /** adjusted quit behaviour: trigger a warning that can be switched off in the saved
        config file RetroShare.conf */
    connect(ui.actionQuit, SIGNAL(triggered()), this, SLOT(doQuit()));

    /* load the StyleSheet*/
    loadStyleSheet(Rshare::stylesheet());


    /* Create the Main pages and actions */
    QActionGroup *grp = new QActionGroup(this);


    ui.stackPages->add(networkDialog = new NetworkDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_NETWORK2), tr("Network"), grp));


    ui.stackPages->add(peersDialog = new PeersDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_PEERS), tr("Friends"), grp));


    ui.stackPages->add(searchDialog = new SearchDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_SEARCH), tr("Search"), grp));


    ui.stackPages->add(transfersDialog = new TransfersDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_TRANSFERS), tr("Transfers"), grp));


    ui.stackPages->add(sharedfilesDialog = new SharedFilesDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_FILES), tr("Files"), grp));


    ui.stackPages->add(messagesDialog = new MessagesDialog(ui.stackPages),
                      messageAction = createPageAction(QIcon(IMAGE_MESSAGES), tr("Messages"), grp));   

    ui.stackPages->add(channelFeed = new ChannelFeed(ui.stackPages),
                      createPageAction(QIcon(IMAGE_CHANNELS), tr("Channels"), grp));

    #ifdef BLOGS
    ui.stackPages->add(blogsFeed = new BlogsDialog(ui.stackPages),
		createPageAction(QIcon(IMAGE_BLOGS), tr("Blogs"), grp));
    #endif
                      
    ui.stackPages->add(forumsDialog = new ForumsDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_FORUMS), tr("Forums"), grp));  

    #ifndef RS_RELEASE_VERSION
    ui.stackPages->add(linksDialog = new LinksDialog(ui.stackPages),
			createPageAction(QIcon(IMAGE_LINKS), tr("Links Cloud"), grp));
    #endif

    #ifndef RS_RELEASE_VERSION
    ui.stackPages->add(newsFeed = new NewsFeed(ui.stackPages),
		createPageAction(QIcon(IMAGE_NEWSFEED), tr("News Feed"), grp));
    #endif

	#ifndef RS_RELEASE_VERSION
    #ifdef PLUGINMGR
    ui.stackPages->add(pluginsPage = new PluginsPage(ui.stackPages),
                       createPageAction(QIcon(IMAGE_PLUGINS), tr("Plugins"), grp));
    #endif
	#endif

    /* Create the toolbar */
    ui.toolBar->addActions(grp->actions());

    connect(grp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *)));

#ifdef UNFINISHED
    ui.toolBar->addSeparator();
    addAction(new QAction(QIcon(IMAGE_UNFINISHED), tr("Unfinished"), ui.toolBar), SLOT(showApplWindow()));
#endif

    /* Select the first action */
    grp->actions()[0]->setChecked(true);

    /** StatusBar section ********/
    /* initialize combobox in status bar */
    statusComboBox = new QComboBox(statusBar());
    initializeStatusObject(statusComboBox);
    connect(statusComboBox, SIGNAL(activated(int)), this, SLOT(statusChangedComboBox(int)));

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

    widget = new QWidget();
    QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
    widget->setSizePolicy(sizePolicy);
    QHBoxLayout *horizontalLayout = new QHBoxLayout(widget);
	  horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
	  _hashing_info_label = new QLabel(widget) ;
    _hashing_info_label->setObjectName(QString::fromUtf8("label"));

    horizontalLayout->addWidget(_hashing_info_label);
    QSpacerItem *horizontalSpacer = new QSpacerItem(1000, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);

    statusBar()->addPermanentWidget(widget);
	  _hashing_info_label->hide() ;

    ratesstatus = new RatesStatus();
    statusBar()->addPermanentWidget(ratesstatus);
    /** Status Bar end ******/

    /* Create the actions that will go in the tray menu */
    createActions();
    /* Creates a tray icon with a context menu and adds it to the system's * notification area. */
    createTrayIcon();

    loadOwnStatus(); // hack; placed in constructor to preempt sendstatus, so status loaded from file

    /* Set focus to the current page */
    ui.stackPages->currentWidget()->setFocus();

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
    delete _bandwidthGraph;
    delete _messengerwindowAct;
    delete peerstatus;
    delete natstatus;
    delete ratesstatus;
    MessengerWindow::releaseInstance();
#ifdef UNFINISHED
    delete applicationWindow;
#endif
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
    trayMenu = new QMenu(this);
    QObject::connect(trayMenu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
    toggleVisibilityAction =
    trayMenu->addAction(QIcon(IMAGE_RETROSHARE), tr("Show/Hide"), this, SLOT(toggleVisibilitycontextmenu()));

    QMenu *pStatusMenu = trayMenu->addMenu(tr("Status"));
    initializeStatusObject(pStatusMenu);
    connect(pStatusMenu, SIGNAL(triggered (QAction*)), this, SLOT(statusChanged(QAction*)));

    trayMenu->addSeparator();
    trayMenu->addAction(_messengerwindowAct);
    trayMenu->addAction(_messagesAct);
    trayMenu->addAction(_bandwidthAct);

#ifdef UNFINISHED
    trayMenu->addAction(_appAct);
#endif
    trayMenu->addAction(_settingsAct);
    trayMenu->addAction(_helpAct);
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

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
            SLOT(toggleVisibility(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
}

/*static*/ void MainWindow::installGroupChatNotifier()
{
    if (_instance == NULL) {
        // nothing to do
        return;
    }

    if(Settings->getDisplayTrayGroupChat()) {
        QObject::connect(_instance->peersDialog, SIGNAL(notifyGroupChat(const QString&,const QString&)), _instance, SLOT(displaySystrayMsg(const QString&,const QString&)), Qt::QueuedConnection);
    } else {
        QObject::disconnect(_instance->peersDialog, SIGNAL(notifyGroupChat(const QString&,const QString&)), _instance, SLOT(displaySystrayMsg(const QString&,const QString&)));
    }
}

void MainWindow::displaySystrayMsg(const QString& title,const QString& msg)
{
    trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, 3000);
}

void MainWindow::updateStatus()
{
	// This call is essential to remove locks due to QEventLoop re-entrance while asking gpg passwds. Dont' remove it!
	if(RsAutoUpdatePage::eventsLocked())
		return ;

	unsigned int nFriendCount = 0;
	unsigned int nOnlineCount = 0;
	rsPeers->getPeerCount (&nFriendCount, &nOnlineCount);

	if (ratesstatus)
		ratesstatus->getRatesStatus();

	if (peerstatus)
		peerstatus->getPeerStatus(nFriendCount, nOnlineCount);

	if (natstatus)
		natstatus->getNATStatus();

	unsigned int newInboxCount = 0;
	rsMsgs->getMessageCount (NULL, &newInboxCount, NULL, NULL, NULL, NULL);

        if(newInboxCount)
                messageAction->setIcon(QIcon(QPixmap(":/images/messages_new.png"))) ;
        else
                messageAction->setIcon(QIcon(QPixmap(":/images/evolution.png"))) ;

	if(newInboxCount)
	{
		trayIcon->setIcon(QIcon(":/images/newmsg.png"));
		if (newInboxCount > 1) {
			trayIcon->setToolTip(tr("RetroShare") + "\n" + tr("You have %1 new messages").arg(newInboxCount));
		} else {
			trayIcon->setToolTip(tr("RetroShare") + "\n" + tr("You have %1 new message").arg(newInboxCount));
		}
	}
	else if (nOnlineCount == 0)
	{
		trayIcon->setIcon(QIcon(IMAGE_NOONLINE));
		trayIcon->setToolTip(tr("RetroShare"));
	}
	else if (nOnlineCount < 2)
	{
		trayIcon->setIcon(QIcon(IMAGE_ONEONLINE));
		trayIcon->setToolTip(tr("RetroShare"));
	}
	else if (nOnlineCount < 3)
	{
		trayIcon->setIcon(QIcon(IMAGE_TWOONLINE));
		trayIcon->setToolTip(tr("RetroShare"));
	}
	else
	{
		trayIcon->setIcon(QIcon(IMAGE_RETROSHARE));
		trayIcon->setToolTip(tr("RetroShare"));
	}
}

void MainWindow::updateHashingInfo(const QString& s)
{
	if(s == "")
		_hashing_info_label->hide() ;
	else
	{
		_hashing_info_label->setText(s) ;
		_hashing_info_label->show() ;
	}
}

void MainWindow::postModDirectories(bool update_local)
{
    RSettingsWin::postModDirectories(update_local);
    ShareManager::postModDirectories(update_local);

    QCoreApplication::flush();
}

/** Creates a new action associated with a config page. */
QAction* MainWindow::createPageAction(QIcon img, QString text, QActionGroup *group)
{
    QAction *action = new QAction(img, text, group);
    action->setCheckable(true);
    action->setFont(FONT);
    return action;
}

/** Adds the given action to the toolbar and hooks its triggered() signal to
 * the specified slot (if given). */
void MainWindow::addAction(QAction *action, const char *slot)
{
    action->setFont(FONT);
    ui.toolBar->addAction(action);
    connect(action, SIGNAL(triggered()), this, slot);
}

/** Shows the MainWindow with focus set to the given page. */
/*static*/ void MainWindow::showWindow(Page page)
{
    if (_instance == NULL) {
        return;
    }

    /* Show the dialog. */
    _instance->show();
    /* Set the focus to the specified page. */
    activatePage (page);
}

/** Set focus to the given page. */
/*static*/ void MainWindow::activatePage(Page page)
{
    if (_instance == NULL) {
        return;
    }

    MainPage *Page = NULL;

    switch (page) {
    case Network:
        Page = _instance->networkDialog;
        break;
    case Friends:
        Page = _instance->peersDialog;
        break;
    case Search:
        Page = _instance->searchDialog;
        break;
    case Transfers:
        Page = _instance->transfersDialog;
        break;
    case SharedDirectories:
        Page = _instance->sharedfilesDialog;
        break;
    case Messages:
        Page = _instance->messagesDialog;
        break;
#ifndef RS_RELEASE_VERSION
    case Links:
        Page = _instance->linksDialog;
        break;
    case Channels:
        Page = _instance->channelFeed;
        break;
#endif
    case Forums:
        Page = _instance->forumsDialog;
        break;
#ifdef BLOGS
    case Blogs:
        Page = _instance->blogsFeed;
        break;
#endif
    }

    if (Page) {
        /* Set the focus to the specified page. */
        _instance->ui.stackPages->setCurrentPage(Page);
    }
}

/** get page */
/*static*/ MainPage *MainWindow::getPage (Page page)
{
   if (_instance == NULL) {
       return NULL;
   }

   switch (page) {
   case Network:
       return _instance->networkDialog;
   case Friends:
       return _instance->peersDialog;
   case Search:
       return _instance->searchDialog;
   case Transfers:
       return _instance->transfersDialog;
   case SharedDirectories:
       return _instance->sharedfilesDialog;
   case Messages:
       return _instance->messagesDialog;
#ifndef RS_RELEASE_VERSION
   case Links:
       return _instance->linksDialog;
   case Channels:
       return _instance->channelFeed;
#endif
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


/** Shows Application window */
#ifdef UNFINISHED
void MainWindow::showApplWindow()
{
    applicationWindow->show();
}
#endif

/** Create and bind actions to events. Setup for initial
 * tray menu configuration. */
void MainWindow::createActions()
{
    _settingsAct = new QAction(QIcon(IMAGE_PREFERENCES), tr("Options"), this);
    connect(_settingsAct, SIGNAL(triggered()), this, SLOT(showSettings()));

    _bandwidthAct = new QAction(QIcon(IMAGE_BWGRAPH), tr("Bandwidth Graph"), this);
    connect(_bandwidthAct, SIGNAL(triggered()), _bandwidthGraph, SLOT(showWindow()));

    _messengerwindowAct = new QAction(QIcon(IMAGE_RSM16), tr("Open Messenger"), this);
    connect(_messengerwindowAct, SIGNAL(triggered()),this, SLOT(showMessengerWindow()));

    _messagesAct = new QAction(QIcon(IMAGE_MESSAGES), tr("Open Messages"), this);
    connect(_messagesAct, SIGNAL(triggered()),this, SLOT(showMess()));
#ifdef UNFINISHED
    _appAct = new QAction(QIcon(IMAGE_UNFINISHED), tr("Applications"), this);
    connect(_appAct, SIGNAL(triggered()),this, SLOT(showApplWindow()));
#endif
    _helpAct = new QAction(QIcon(IMG_HELP), tr("Help"), this);
    connect(_helpAct, SIGNAL(triggered()), this, SLOT(showHelpDialog()));
}

/** If the user attempts to quit the app, a check-warning is issued. This warning can be
    turned off for future quit events.
*/
void MainWindow::doQuit()
{
	if(!Settings->value(QString::fromUtf8("doQuit"), false).toBool())
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

void MainWindow::displayErrorMessage(int a,int b,const QString& error_msg)
{
	QMessageBox::critical(NULL, tr("Internal Error"),error_msg) ;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    static bool firstTime = true;

   if(!Settings->value(QString::fromUtf8("ClosetoTray"), false).toBool())
   {
      if (trayIcon->isVisible()) {
          if (firstTime)
          {
/*****
            QMessageBox::information(this, tr("RetroShare System tray"),
            tr("Application will continue running. Quit using context menu in the system tray"));
*****/
            firstTime = false;
          }
          hide();
          e->ignore();
      }
   }
   else   
   {
       rsicontrol->rsGlobalShutDown();
       rApp->quit();
   }
   

}


void MainWindow::updateMenu()
{
    toggleVisibilityAction->setText(isVisible() ? tr("Hide") : tr("Show"));
}

void MainWindow::toggleVisibility(QSystemTrayIcon::ActivationReason e)
{
    if(e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick){
        if(isHidden()){
            show();
            if(isMinimized()){
                if(isMaximized()){
                    showMaximized();
                }else{
                    showNormal();
                }
            }
            raise();
            activateWindow();
        }else{
            hide();
        }
    }
}

void MainWindow::toggleVisibilitycontextmenu()
{
    if (isVisible())
        hide();
    else
        show();
}

void MainWindow::loadStyleSheet(const QString &sheetName)
{
    /** internal Stylesheets **/
    //QFile file(":/qss/" + sheetName.toLower() + ".qss");

    /** extern Stylesheets **/
    QFile file(QApplication::applicationDirPath() + "/qss/" + sheetName.toLower() + ".qss");

    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());

    qApp->setStyleSheet(styleSheet);

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
  static HelpBrowser *helpBrowser = 0;
  if (!helpBrowser)
    helpBrowser = new HelpBrowser(this);
  helpBrowser->showWindow(topic);
}

void MainWindow::on_actionQuick_Start_Wizard_activated()
{
    QuickStartWizard qstartwizard(this);
    qstartwizard.exec();
}

/** Called when the user changes the UI translation. */
void
MainWindow::retranslateUi()
{
  ui.retranslateUi(this);
  foreach (MainPage *page, ui.stackPages->pages()) {
    page->retranslateUi();
  }
  foreach (QAction *action, ui.toolBar->actions()) {
    action->setText(tr(qPrintable(action->data().toString()), "MainWindow"));
  }
}

void MainWindow::setStyle()
{
 QString standardSheet = "{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 <color1>, stop:1 <color2>);}";
 QColor stop1 = QColorDialog::getColor(Qt::white);
 QColor stop2 = QColorDialog::getColor(Qt::black);
 //QString widgetSheet = ".QWidget" + standardSheet.replace("<color1>", stop1.name()).replace("<color2>", stop2.name());
 QString toolSheet = "QToolBar" + standardSheet.replace("<color1>", stop1.name()).replace("<color2>", stop2.name());
 QString menuSheet = "QMenuBar" + standardSheet.replace("<color1>", stop1.name()).replace("<color2>", stop2.name());
 qApp->setStyleSheet(/*widgetSheet + */toolSheet + menuSheet);

}

/* get own status */
static int getOwnStatus()
{
    std::string ownId = rsPeers->getOwnId();

    StatusInfo si;
    std::list<StatusInfo> statusList;
    std::list<StatusInfo>::iterator it;

    if (!rsStatus->getStatus(statusList)) {
        return -1;
    }

    for (it = statusList.begin(); it != statusList.end(); it++){
        if (it->id == ownId)
            return it->status;
    }

    return -1;
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

    int nStatus = getOwnStatus();

    for (std::set <QObject*>::iterator it = m_apStatusObjects.begin(); it != m_apStatusObjects.end(); it++) {
        setStatusObject(*it, nStatus);
    }
}

void MainWindow::checkAndSetIdle(int idleTime)
{
    if ((idleTime >= (int) maxTimeBeforeIdle) && !isIdle) {
        setIdle(true);
    }else
        if((idleTime < (int) maxTimeBeforeIdle) && isIdle) {
        setIdle(false);
    }

    return;
}

void MainWindow::setIdle(bool idle)
{
    isIdle = idle;
    setStatus(NULL, getOwnStatus());
}

/* add and initialize status object */
void MainWindow::initializeStatusObject(QObject *pObject)
{
    if (m_apStatusObjects.find(pObject) != m_apStatusObjects.end()) {
        return;
    }

    m_apStatusObjects.insert(m_apStatusObjects.end(), pObject);

    QMenu *pMenu = dynamic_cast<QMenu*>(pObject);
    if (pMenu) {
        /* initialize menu */
        QActionGroup *pGroup = new QActionGroup(pMenu);

        QAction *pAction = new QAction(QIcon(":/images/im-user.png"), tr("Online"), pMenu);
        pAction->setData(RS_STATUS_ONLINE);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);

        pAction = new QAction(QIcon(":/images/im-user-busy.png"), tr("Busy"), pMenu);
        pAction->setData(RS_STATUS_BUSY);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);

        pAction = new QAction(QIcon(":/images/im-user-away.png"), tr("Away"), pMenu);
        pAction->setData(RS_STATUS_AWAY);
        pAction->setCheckable(true);
        pMenu->addAction(pAction);
        pGroup->addAction(pAction);
    } else {
        /* initialize combobox */
        QComboBox *pComboBox = dynamic_cast<QComboBox*>(pObject);
        if (pComboBox) {
            pComboBox->addItem(QIcon(":/images/im-user.png"), tr("Online"), RS_STATUS_ONLINE);
            pComboBox->addItem(QIcon(":/images/im-user-busy.png"), tr("Busy"), RS_STATUS_BUSY);
            pComboBox->addItem(QIcon(":/images/im-user-away.png"), tr("Away"), RS_STATUS_AWAY);
        }
        /* add more objects here */
    }

    if (m_bStatusLoadDone) {
        /* loadOwnStatus done, set own status directly */
        int nStatus = getOwnStatus();
        if (nStatus != -1) {
            setStatusObject(pObject, nStatus);
        }
    }
}

/* remove status object */
void MainWindow::removeStatusObject(QObject *pObject)
{
    m_apStatusObjects.erase(pObject);
}

/** Save own status Online,Away,Busy **/
void MainWindow::setStatus(QObject *pObject, int nStatus)
{
    RsPeerDetails detail;
    std::string ownId = rsPeers->getOwnId();

    if (!rsPeers->getPeerDetails(ownId, detail)) {
        return;
    }

    StatusInfo si;

    if (isIdle && nStatus == (int) RS_STATUS_ONLINE) {
        /* set idle state only when I am in online state */
        nStatus = RS_STATUS_INACTIVE;
    }

    si.id = ownId;
    si.status = nStatus;

    rsStatus->sendStatus(si);

    /* set status in all status objects, but the calling one */
    for (std::set <QObject*>::iterator it = m_apStatusObjects.begin(); it != m_apStatusObjects.end(); it++) {
        if (*it != pObject) {
            setStatusObject(*it, nStatus);
        }
    }
}

/* new status from context menu */
void MainWindow::statusChanged(QAction *pAction)
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

    setStatus(statusComboBox, statusComboBox->itemData(index, Qt::UserRole).toInt());
}
