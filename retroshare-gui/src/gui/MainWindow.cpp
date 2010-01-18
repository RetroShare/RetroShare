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

#include <QtGui>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QString>
#include <QtDebug>
#include <QIcon>
#include <QPixmap>

#include "ChannelFeed.h"
#include "ShareManager.h"
#include "NetworkView.h"
#include "LinksDialog.h"
#include "ForumsDialog.h"
#include "NewsFeed.h"

#include "rshare.h"
#include "MainWindow.h"
#include "MessengerWindow.h"
#include "HelpDialog.h"
#include "AboutDialog.h"
#include "QuickStartWizard.h"

#include "gui/TurtleRouterDialog.h"

#include "statusbar/peerstatus.h"
#include "statusbar/natstatus.h"
#include "statusbar/ratesstatus.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"

#include "gui/connect/ConnectFriendWizard.h"
#include "util/rsversion.h"
#include "settings/rsettingswin.h"

#include <sstream>
#include <iomanip>
#include <unistd.h>

#define FONT        QFont(tr("Arial"), 9)

/* Images for toolbar icons */
#define IMAGE_NETWORK           ":/images/retrosharelogo1.png"
#define IMAGE_NETWORK2          ":/images/rs1.png"
#define IMAGE_PEERS         	  ":/images/groupchat.png"
#define IMAGE_SEARCH    		    ":/images/filefind.png"
#define IMAGE_TRANSFERS      	  ":/images/ktorrent32.png"
#define IMAGE_LINKS             ":/images/irkick.png"
#define IMAGE_FILES   	        ":/images/fileshare24.png"
#define IMAGE_CHANNELS       	  ":/images/channels.png"
#define IMAGE_FORUMS            ":/images/konversation.png"
#define IMAGE_TURTLE            ":/images/turtle.png"
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
#define IMAGE_SMPLAYER			    ":/images/smplayer_icon32.png"
#define IMAGE_BLOCK         	  ":/images/blockdevice.png"
#define IMAGE_COLOR         	  ":/images/highlight.png"
#define IMAGE_GAMES             ":/images/kgames.png"
#define IMAGE_PHOTO             ":/images/lphoto.png"
#define IMAGE_SMPLAYER          ":/images/smplayer_icon32.png"
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


/** Constructor */
MainWindow::MainWindow(QWidget* parent, Qt::WFlags flags)
    : RWindow("MainWindow", parent, flags)
{
    /* Invoke the Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    /* Create RshareSettings object */
    _settings = new RshareSettings();
    
    if (!_settings->value(QString::fromUtf8("FirstRun"), false).toBool())
    {
    _settings->setValue(QString::fromUtf8("FirstRun"), false);
		QuickStartWizard *qstartWizard = new QuickStartWizard(this);
		qstartWizard->exec();
    }

    setWindowTitle(tr("RetroShare %1 a secure decentralised commmunication platform").arg(retroshareVersion()));

    // Setting icons
    this->setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));

    /* Create all the dialogs of which we only want one instance */
    _bandwidthGraph = new BandwidthGraph();

    /*messengerWindow instance is created statically so that RsAutoUpdatePage can access it*/ 
    messengerWindow = MessengerWindow::getInstance();
#ifdef UNFINISHED
    applicationWindow = new ApplicationWindow();
    applicationWindow->hide();
#endif    

    /** Left Side ToolBar**/
    connect(ui.actionAdd_Friend, SIGNAL(triggered() ), this , SLOT( addFriend() ) );
    connect(ui.actionAdd_Share, SIGNAL(triggered() ), this , SLOT( openShareManager() ) );
    connect(ui.actionOptions, SIGNAL(triggered()), this, SLOT( showSettings()) );
    connect(ui.actionMessenger, SIGNAL(triggered()), this, SLOT( showMessengerWindow()) );
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
                      createPageAction(QIcon(IMAGE_MESSAGES), tr("Messages"), grp));

    LinksDialog *linksDialog = NULL;

    ChannelFeed *channelFeed = NULL;
    ui.stackPages->add(channelFeed = new ChannelFeed(ui.stackPages),
                      createPageAction(QIcon(IMAGE_CHANNELS), tr("Channels"), grp));

    ui.stackPages->add(linksDialog = new LinksDialog(ui.stackPages),
			createPageAction(QIcon(IMAGE_LINKS), tr("Links Cloud"), grp));

    ForumsDialog *forumsDialog = NULL;
    ui.stackPages->add(forumsDialog = new ForumsDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_FORUMS), tr("Forums"), grp));

    NewsFeed *newsFeed = NULL;
    ui.stackPages->add(newsFeed = new NewsFeed(ui.stackPages),
		createPageAction(QIcon(IMAGE_NEWSFEED), tr("News Feed"), grp));

#ifdef PLUGINMGR
    ui.stackPages->add(pluginsPage = new PluginsPage(ui.stackPages),
                       createPageAction(QIcon(IMAGE_PLUGINS), tr("Plugins"), grp));
#endif


    /* Create the toolbar */
    ui.toolBar->addActions(grp->actions());
    ui.toolBar->addSeparator();
    connect(grp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *)));

#ifdef UNFINISHED
    addAction(new QAction(QIcon(IMAGE_UNFINISHED), tr("Unfinished"), ui.toolBar), SLOT(showApplWindow()));
#endif

    /* Select the first action */
    grp->actions()[0]->setChecked(true);

    /* also an empty list of chat windows */
    messengerWindow->setChatDialog(peersDialog);

    /** StatusBar section ********/
    peerstatus = new PeerStatus();
    statusBar()->addWidget(peerstatus);

    natstatus = new NATStatus();
    statusBar()->addWidget(natstatus);

	  QWidget *widget = new QWidget();
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
    delete _settings;
#ifdef UNFINISHED   
    delete applicationWindow;
#endif    
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

void MainWindow::displaySystrayMsg(const QString& title,const QString& msg)
{
    trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, 3000);
}

void MainWindow::updateStatus()
{

      if (ratesstatus)
      ratesstatus->getRatesStatus();

      if (peerstatus)
      peerstatus->getPeerStatus();

      if (natstatus)
      natstatus->getNATStatus();

    std::list<std::string> ids;
    rsPeers->getOnlineList(ids);
    int online = ids.size();

    if (online == 0)
    {
        trayIcon->setIcon(QIcon(IMAGE_NOONLINE));
    }
    else if (online < 2)
    {
        trayIcon->setIcon(QIcon(IMAGE_ONEONLINE));
    }
    else if (online < 3)
    {
        trayIcon->setIcon(QIcon(IMAGE_TWOONLINE));
    }
    else
    {
        trayIcon->setIcon(QIcon(IMAGE_RETROSHARE));
    }

}

void MainWindow::updateHashingInfo(const QString& s)
{
	if(s == "")
		_hashing_info_label->hide() ;
	else
	{
		_hashing_info_label->setText("Hashing file " + s) ;
		_hashing_info_label->show() ;
	}
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
void MainWindow::showWindow(Page page)
{
    /* Show the dialog. */
    //show();

    /* Show the dialog. */
    RWindow::showWindow();
    /* Set the focus to the specified page. */
    ui.stackPages->setCurrentIndex((int)page);
}



/***** TOOL BAR FUNCTIONS *****/

/** Add a Friend ShortCut */
void MainWindow::addFriend()
{
    ConnectFriendWizard* connwiz = new ConnectFriendWizard(this);

    // set widget to be deleted after close
    connwiz->setAttribute( Qt::WA_DeleteOnClose, true);


    connwiz->show();
}

/** Shows Share Manager */
void MainWindow::openShareManager()
{
	 ShareManager::showYourself();
}


/** Shows Messages Dialog */
void
MainWindow::showMess(MainWindow::Page page)
{
  showWindow(page);
}


/** Shows Options */
void MainWindow::showSettings()
{
    static RSettingsWin *win = new RSettingsWin(this);
    if (win->isHidden())
        win->setNewPage(0);
    win->show();
    win->activateWindow();
}

/** Shows Messenger window */
void MainWindow::showMessengerWindow()
{
    messengerWindow->show();
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
    connect(_bandwidthAct, SIGNAL(triggered()),
            _bandwidthGraph, SLOT(showWindow()));

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

	if(!_settings->value(QString::fromUtf8("doQuit"), false).toBool())
	{
	  QString queryWrn;
	  queryWrn.clear();
	  queryWrn.append(tr("Do you really want to exit RetroShare ?"));

		if ((QMessageBox::question(this, tr("Really quit ? "),queryWrn,QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
		{
			rsicontrol->rsGlobalShutDown();
			qApp->quit();
		}
		else
		return;
	}
	else
	rsicontrol->rsGlobalShutDown();
	rApp->quit();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    static bool firstTime = true;

   if(!_settings->value(QString::fromUtf8("ClosetoTray"), false).toBool())
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
    static AboutDialog *adlg = new AboutDialog(this);
    adlg->exec();
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
    QuickStartWizard *qstartwizard = new QuickStartWizard(this);
    qstartwizard->exec();
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
