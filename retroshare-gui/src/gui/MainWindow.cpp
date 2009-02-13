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


#include "NetworkView.h"
#include "LinksDialog.h"
#include "PhotoDialog.h"
#include "ForumsDialog.h"
#include "NewsFeed.h"
#include "PeersFeed.h"
#include "TransferFeed.h"
#include "MsgFeed.h"
#include "ChannelFeed.h"
#include "ShareManager.h"


#include <rshare.h>
#include "MainWindow.h"
#include "MessengerWindow.h"
#include "HelpDialog.h"

#include "games/qbackgammon/bgwindow.h"
//#include "smplayer.h"

#include "statusbar/peerstatus.h"
#include "Preferences/PreferencesWindow.h"
#include "Settings/gsettingswin.h"
#include "util/rsversion.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"

#include "gui/connect/InviteDialog.h"
#include "gui/connect/AddFriendDialog.h"

#include <sstream>
#include <iomanip>
#include <unistd.h>

#define FONT        QFont(tr("Arial"), 9)

/* Images for toolbar icons */
#define IMAGE_NETWORK           ":/images/retrosharelogo1.png"
#define IMAGE_PEERS         	":/images/groupchat.png"
#define IMAGE_SEARCH    	":/images/filefind.png"
#define IMAGE_TRANSFERS      	":/images/ktorrent32.png"
#define IMAGE_LINKS             ":/images/knewsticker24.png"
#define IMAGE_FILES   	        ":/images/fileshare24.png"
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
#define IMAGE_SMPLAYER		":/images/smplayer_icon32.png"
#define IMAGE_BLOCK         	":/images/blockdevice.png"
#define IMAGE_COLOR         	":/images/highlight.png"
#define IMAGE_GAMES             ":/images/kgames.png"
#define IMAGE_PHOTO             ":/images/lphoto.png"
#define IMAGE_SMPLAYER          ":/images/smplayer_icon32.png"
#define IMAGE_ADDFRIEND         ":/images/add-friend24.png"
#define IMAGE_INVITEFRIEND      ":/images/invite-friend24.png"
#define IMAGE_ADDSHARE          ":/images/directoryadd_24x24_shadow.png"
#define IMAGE_OPTIONS           ":/images/settings.png"
#define IMAGE_QUIT              ":/images/exit_24x24.png"
#define IMAGE_UNFINISHED        ":/images/underconstruction.png"
#define IMAGE_MINIMIZE          ":/images/window_nofullscreen.png"
#define IMAGE_MAXIMIZE          ":/images/window_fullscreen.png"
#define IMG_HELP                ":/images/help.png"
#define IMAGE_NEWSFEED          ":/images/konqsidebar_news24.png"
#define IMAGE_PLUGINS           ":/images/extension_32.png"


/* Keys for UI Preferences */
#define UI_PREF_PROMPT_ON_QUIT  "UIOptions/ConfirmOnQuit"
/* uncomment this for release version */

/*****
 * #define RS_RELEASE_VERSION    1
 ****/

//#define RS_RELEASE_VERSION    1

/** Constructor */
MainWindow::MainWindow(QWidget* parent, Qt::WFlags flags)
    : RWindow("MainWindow", parent, flags)
{
    /* Invoke the Qt Designer generated QObject setup routine */
    ui.setupUi(this);
    
    /* Create RshareSettings object */
    _settings = new RshareSettings();
    
    setWindowTitle(tr("RetroShare %1 RetroShare a private and secure decentralised commmunication platform").arg(retroshareVersion())); 

    mSMPlayer = NULL;
  
    ui.toolBarservice->hide();
	
    // Setting icons
    this->setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));
    
    /* Create all the dialogs of which we only want one instance */
    _bandwidthGraph = new BandwidthGraph();
    messengerWindow = new MessengerWindow();
    _preferencesWindow = new PreferencesWindow();
    messengerWindow->hide();
    //messengerWindow->show();
    applicationWindow = new ApplicationWindow();
    applicationWindow->hide();
	
	/** Left Side ToolBar**/
    connect(ui.actionAdd_Friend, SIGNAL(triggered() ), this , SLOT( addFriend() ) );
    connect(ui.actionInvite_Friend, SIGNAL(triggered() ), this , SLOT( inviteFriend() ) );
    connect(ui.actionAdd_Share, SIGNAL(triggered() ), this , SLOT( openShareManager() ) );
    connect(ui.actionOptions, SIGNAL(triggered()), this, SLOT( showPreferencesWindow()) );
    connect(ui.actionMessenger, SIGNAL(triggered()), this, SLOT( showMessengerWindow()) );
    connect(ui.actionSMPlayer, SIGNAL(triggered()), this, SLOT( showsmplayer()) );
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT( showabout()) );
    connect(ui.actionColor, SIGNAL(triggered()), this, SLOT( setStyle()) );
    //connect(ui.actionSettings, SIGNAL(triggered()), this, SLOT( showSettings()) );

   	 
       
    /** adjusted quit behaviour: trigger a warning that can be switched off in the saved
        config file RetroShare.conf */
   connect(ui.actionQuit, SIGNAL(triggered()), this, SLOT(doQuit()));

    /* load the StyleSheet*/
    loadStyleSheet(Rshare::stylesheet()); 



    /* Create the Main pages and actions */
    QActionGroup *grp = new QActionGroup(this);


    NewsFeed *newsFeed = NULL;
    ui.stackPages->add(newsFeed = new NewsFeed(ui.stackPages),
		createPageAction(QIcon(IMAGE_NEWSFEED), tr("News Feed"), grp));

  
    ui.stackPages->add(peersDialog = new PeersDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_PEERS), tr("Friends"), grp));

    //PeersFeed *peersFeed = NULL;
    //ui.stackPages->add(peersFeed = new PeersFeed(ui.stackPages),
    //		createPageAction(QIcon(IMAGE_PEERS), tr("Peers"), grp));
	
    ui.stackPages->add(searchDialog = new SearchDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_SEARCH), tr("Search"), grp));
                     
    ui.stackPages->add(transfersDialog = new TransfersDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_TRANSFERS), tr("Transfers"), grp));
                     
    //TransferFeed *transferFeed = NULL;
    //ui.stackPages->add(transferFeed = new TransferFeed(ui.stackPages),
    //		createPageAction(QIcon(IMAGE_LINKS), tr("Transfers"), grp));
	
    ui.stackPages->add(sharedfilesDialog = new SharedFilesDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_FILES), tr("Files"), grp));
                     

    //MsgFeed *msgFeed = NULL;
    //ui.stackPages->add(msgFeed = new MsgFeed(ui.stackPages),
    //		createPageAction(QIcon(IMAGE_MESSAGES), tr("Messages"), grp));

    ui.stackPages->add(messagesDialog = new MessagesDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_MESSAGES), tr("Messages"), grp));
                       
    LinksDialog *linksDialog = NULL;


#ifdef RS_RELEASE_VERSION    
    channelsDialog = NULL;
    ui.stackPages->add(linksDialog = new LinksDialog(ui.stackPages),
			createPageAction(QIcon(IMAGE_LINKS), tr("Links Cloud"), grp));

    ChannelFeed *channelFeed = NULL;
    ui.stackPages->add(channelFeed = new ChannelFeed(ui.stackPages),
                      createPageAction(QIcon(IMAGE_CHANNELS), tr("Channel Feed"), grp));

    ForumsDialog *forumsDialog = NULL;
    ui.stackPages->add(forumsDialog = new ForumsDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_FORUMS), tr("Forums"), grp));

	
#else
    channelsDialog = NULL;
    ui.stackPages->add(linksDialog = new LinksDialog(ui.stackPages),
			createPageAction(QIcon(IMAGE_LINKS), tr("Links Cloud"), grp));

    ChannelFeed *channelFeed = NULL;
    ui.stackPages->add(channelFeed = new ChannelFeed(ui.stackPages),
                      createPageAction(QIcon(IMAGE_CHANNELS), tr("Channels"), grp));

    ForumsDialog *forumsDialog = NULL;
    ui.stackPages->add(forumsDialog = new ForumsDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_FORUMS), tr("Forums"), grp));

#endif

    ui.stackPages->add(networkDialog = new NetworkDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_NETWORK), tr("Network"), grp));

    ui.stackPages->add(pluginsPage = new PluginsPage(ui.stackPages),
                       createPageAction(QIcon(IMAGE_PLUGINS), tr("Plugins"), grp));

                     
  //ui.stackPages->add(groupsDialog = new GroupsDialog(ui.stackPages),
  //                   createPageAction(QIcon(), tr("Groups"), grp));
                                                              
  //ui.stackPages->add(new StatisticDialog(ui.stackPages),
  //                   createPageAction(QIcon(IMAGE_STATISTIC), tr("Statistics"), grp));

    /* also an empty list of chat windows */
    messengerWindow->setChatDialog(peersDialog);

    /* Create the toolbar */
    ui.toolBar->addActions(grp->actions());
    ui.toolBar->addSeparator();
    connect(grp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *)));
       
    /* Select the first action */
    grp->actions()[0]->setChecked(true);

    // Allow to play files from SharedFilesDialog.
    connect(sharedfilesDialog, SIGNAL(playFiles( QStringList )), this, SLOT(playFiles( QStringList )));
    connect(transfersDialog, SIGNAL(playFiles( QStringList )), this, SLOT(playFiles( QStringList )));

#ifdef RS_RELEASE_VERSION    
    //addAction(new QAction(QIcon(IMAGE_UNFINISHED), tr("Unfinished"), ui.toolBar), SLOT(showApplWindow()));


#else
    addAction(new QAction(QIcon(IMAGE_UNFINISHED), tr("Unfinished"), ui.toolBar), SLOT(showApplWindow()));

    toolAct = ui.toolBarservice->toggleViewAction();
    toolAct->setText("Service");
    toolAct->setShortcut(tr("Ctrl+T"));
    toolAct->setIcon(QIcon(":/images/blockdevice2.png"));
    //ui.toolBar->addAction(toolAct);
 
        /* Create the Service pages and actions */
    QActionGroup *servicegrp = new QActionGroup(this);
   
                           
#if 0
    LinksDialog *linksDialog = NULL;
    ui.stackPages->add(linksDialog = new LinksDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_LINKS), tr("Links Cloud"), servicegrp));

    ChannelsDialog *channelsDialog = NULL;
    ui.stackPages->add(channelsDialog = new ChannelsDialog(ui.stackPages),
                           createPageAction(QIcon(IMAGE_CHANNELS), tr("Channels"), servicegrp));

    GamesDialog *gamesDialog = NULL;
    ui.stackPages->add(gamesDialog = new GamesDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_GAMES), tr("Games"), servicegrp));
                     
    PhotoDialog *photoDialog = NULL;
    ui.stackPages->add(photoDialog = new PhotoDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_PHOTO), tr("Photo View"), servicegrp)); 
                         
#endif
                                      
    
    /* Create the toolbarservice */
    ui.toolBarservice->addActions(servicegrp->actions());
    ui.toolBarservice->addSeparator();
    connect(servicegrp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *))); 
        
    ui.toolBarservice->addSeparator();

#endif
    peerstatus = new PeerStatus();
    statusBar()->addWidget(peerstatus);

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

    statusBar()->addPermanentWidget(statusRates = new QLabel(tr("<strong>Down:</strong> 0.00 (kB/s) | <strong>Up:</strong> 0.00 (kB/s) ")));

    //servicegrp->actions()[0]->setChecked(true);
  
    /* Create the actions that will go in the tray menu */
    createActions();
             
/******  
    * This is an annoying warning I get all the time...
    * (no help!)
    *
    *
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    QMessageBox::warning(0, tr("System tray is unavailable"),
    tr("System tray unavailable"));
******/

    // Tray icon Menu
    menu = new QMenu(this);
    QObject::connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
    toggleVisibilityAction = 
            menu->addAction(QIcon(IMAGE_RETROSHARE), tr("Show/Hide"), this, SLOT(toggleVisibilitycontextmenu()));
    menu->addSeparator();
    menu->addAction(_messengerwindowAct);
    menu->addAction(_messagesAct);
    menu->addAction(_bandwidthAct);

    /* bandwidth only in development version */
#ifdef RS_RELEASE_VERSION    
#else
    menu->addAction(_appAct);
#endif
    menu->addAction(_prefsAct);
    //menu->addAction(_smplayerAct);
    menu->addAction(_helpAct);
    menu->addSeparator();
    menu->addAction(QIcon(IMAGE_MINIMIZE), tr("Minimize"), this, SLOT(showMinimized()));
    menu->addAction(QIcon(IMAGE_MAXIMIZE), tr("Maximize"), this, SLOT(showMaximized()));
    menu->addSeparator();
    menu->addAction(QIcon(IMAGE_CLOSE), tr("&Quit"), this, SLOT(doQuit()));
    // End of Icon Menu
    
    // Create the tray icon
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setToolTip(tr("RetroShare"));
    trayIcon->setContextMenu(menu);
    trayIcon->setIcon(QIcon(IMAGE_RETROSHARE));
    
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, 
            SLOT(toggleVisibility(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();

    QTimer *timer = new QTimer(this);
    timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    timer->start(1000);
}

void MainWindow::updateStatus()
{
	/* set users/friends/network */
	float downKb = 0;
	float upKb = 0;
	rsicontrol -> ConfigGetDataRates(downKb, upKb);

	std::ostringstream out;
	out << "<strong>Down:</strong> " << std::setprecision(2) << std::fixed << downKb << " (kB/s) |  <strong>Up:</strong> " << std::setprecision(2) << std::fixed <<  upKb << " (kB/s) ";

	/* set uploads/download rates */
	if (statusRates)
    		statusRates -> setText(QString::fromStdString(out.str()));

    	if (peerstatus)
		peerstatus->setPeerStatus();

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

/** Creates a new action associated with a config page. */
QAction* MainWindow::createPageActionservice(QIcon img, QString text, QActionGroup *groupservice)
{
    QAction *actionservice = new QAction(img, text, groupservice);
    actionservice->setCheckable(true);
    actionservice->setFont(FONT);
    return actionservice;
}


/** Adds the given action to the toolbar and hooks its triggered() signal to
 * the specified slot (if given). */
void MainWindow::addAction(QAction *action, const char *slot)
{
    action->setFont(FONT);
    ui.toolBar->addAction(action);
    connect(action, SIGNAL(triggered()), this, slot);
}

/** Adds the given action to the toolbar and hooks its triggered() signal to
 * the specified slot (if given). */
void MainWindow::addActionservice(QAction *actionservice, const char *slot)
{
    actionservice->setFont(FONT);
    ui.toolBarservice->addAction(actionservice);
    connect(actionservice, SIGNAL(triggered()), this, slot);
}

/** Overloads the default show so we can load settings */
/*void MainWindow::show()
{
  
    if (!this->isVisible()) {
        QMainWindow::show();
    } else {
        QMainWindow::activateWindow();
        setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
        QMainWindow::raise();
    }
}*/


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
    /* call load Certificate */
#if 0
    std::string id;
    if (connectionsDialog)
    {
        id = connectionsDialog->loadneighbour();
    }

    /* call make Friend */
    if (id != "")
    {
        connectionsDialog->showpeerdetails(id);
    }
    virtual int NeighLoadPEMString(std::string pem, std::string &id)  = 0;
#else

    static  AddFriendDialog *addDialog = 
    new AddFriendDialog(networkDialog, this);

    std::string invite = "";
    addDialog->setInfo(invite);
    addDialog->show();
#endif
}


/** Add a Friend ShortCut */
void MainWindow::inviteFriend()
{
    static  InviteDialog *inviteDialog = new InviteDialog(this);

    std::string invite = rsPeers->GetRetroshareInvite();
    inviteDialog->setInfo(invite);
    inviteDialog->show();


}

/** Add a Share */
void MainWindow::addSharedDirectory()
{
    /* Same Code as in Preferences Window (add Share) */

    QString qdir = QFileDialog::getExistingDirectory(this, tr("Add Shared Directory"), "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
						   
    /* add it to the server */
    std::string dir = qdir.toStdString();
    if (dir != "")
    {
        rsFiles -> addSharedDirectory(dir);
    }

}

/** Shows Share Manager */
void MainWindow::openShareManager()
{
    static ShareManager* sharemanager = new ShareManager(this);
    sharemanager->show();

}

/** Creates and displays the Configuration dialog with the current page set to
 * <b>page</b>. */
void
MainWindow::showPreferencesWindow(PreferencesWindow::Page page)
{
  _preferencesWindow->showWindow(page);
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
    static GSettingsWin *win = new GSettingsWin(this);
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
void MainWindow::showApplWindow()
{
    applicationWindow->show();
}

/** Destructor. */
MainWindow::~MainWindow()
{
    delete _bandwidthGraph;
    delete _messengerwindowAct;
    //delete _smplayerAct;
    delete _preferencesWindow;
}

/** Create and bind actions to events. Setup for initial
 * tray menu configuration. */
void MainWindow::createActions()
{

    _prefsAct = new QAction(QIcon(IMAGE_PREFERENCES), tr("Options"), this);
    connect(_prefsAct, SIGNAL(triggered()), this, SLOT(showPreferencesWindow()));
    
    _bandwidthAct = new QAction(QIcon(IMAGE_BWGRAPH), tr("Bandwidth Graph"), this);
    connect(_bandwidthAct, SIGNAL(triggered()), 
            _bandwidthGraph, SLOT(showWindow()));
          
    _messengerwindowAct = new QAction(QIcon(IMAGE_RSM16), tr("Open Messenger"), this);
    connect(_messengerwindowAct, SIGNAL(triggered()),this, SLOT(showMessengerWindow()));

    _messagesAct = new QAction(QIcon(IMAGE_MESSAGES), tr("Open Messages"), this);
    connect(_messagesAct, SIGNAL(triggered()),this, SLOT(showMess()));
    
    _appAct = new QAction(QIcon(IMAGE_UNFINISHED), tr("Applications"), this);
    connect(_appAct, SIGNAL(triggered()),this, SLOT(showApplWindow()));
    
    //_smplayerAct = new QAction(QIcon(IMAGE_SMPLAYER), tr("SMPlayer"), this);
    //connect(_smplayerAct, SIGNAL(triggered()),this, SLOT(showsmplayer()));
    
    _helpAct = new QAction(QIcon(IMG_HELP), tr("Help"), this);
    connect(_helpAct, SIGNAL(triggered()), this, SLOT(showHelpDialog()));
         
            
}

/** If the user attempts to quit the app, a check-warning is issued. This warning can be 
    turned off for future quit events. 
*/
void MainWindow::doQuit()
{
    RshareSettings rsharesettings;
    QString key (UI_PREF_PROMPT_ON_QUIT);
    bool doConfirm = rsharesettings.value(key, QVariant(true)).toBool();
    if (doConfirm)
    {
        ConfirmQuitDialog * confirm = new ConfirmQuitDialog;
        confirm->exec();
        // save configuration setting
        if (confirm->reminderCheckBox->checkState() == Qt::Checked) 
        {
            rsharesettings.setValue(key, QVariant(false));
        }
        
        if (confirm->result() == QDialog::Accepted) 
        {
	    rsicontrol->rsGlobalShutDown(); 
            qApp->quit();
        } else {
            delete confirm;
        }
        
    } else {
	rsicontrol->rsGlobalShutDown(); 
        qApp->quit();
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    static bool firstTime = true;

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

void MainWindow::startgammon()
{
	BgWindow *bgWindow = new BgWindow(this); 
	bgWindow->show(); 


}

void MainWindow::startqcheckers()
{
#if 0
    myTopLevel* top = new myTopLevel();
    top->show();
#endif
}


/** Shows smplayer */
void MainWindow::showsmplayer()
{    
    return;

#if 0
    if (mSMPlayer == 0) 
    {
    	mSMPlayer = new SMPlayer(QString::null, this);
    	mSMPlayer->gui()->hide();
    }
    else
    {
    	mSMPlayer->gui()->show();
    }
#endif
}

void MainWindow::playFiles(QStringList files)
{
	std::cerr << "MainWindow::playFiles() Can only play first currently" << std::endl;
	QStringList::iterator it;
	it = files.begin();
	if (it == files.end())
	{
		return;
	}
	std::string path = (*it).toStdString();
	std::cerr << "MainWindow::playFiles() opening: " << path << std::endl;

	openFile(path);
	return;

#if 0
	showsmplayer();

	std::cerr << "MainWindow::playFiles() showsmplayer() done" << std::endl;

	if (mSMPlayer)
		mSMPlayer->gui()->openFiles(files);

	std::cerr << "MainWindow::playFiles() done" << std::endl;
#endif
}


void MainWindow::showabout()
{
    static HelpDialog *helpdlg = new HelpDialog(this); 
	helpdlg->show(); 
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


void openFile(std::string path)
{
	bool isAbs = true;
	QString surl("file://");

#if defined(Q_OS_WIN)
	/* check that it is an absolute path */
	if (path.size() < 4)
	{
		std::cerr << "[WIN] openPath() Very Small path ignoring: " << path;
		std::cerr << std::endl;
		return;
	}

	if ((path[1] == ':') && ((path[2] == '\\') || (path[2] == '/')))
	{
		isAbs = true;
	}
#else

	/* check that it is an absolute path */
	if (path.size() < 1)
	{
		std::cerr << "[UNIX] openPath() Very Small path ignoring: " << path;
		std::cerr << std::endl;
		return;
	}

	if (path[0] == '/')
	{
		isAbs = true;
	}
#endif

	if (!isAbs)
	{

#define ROOT_PATH_SIZE 1024

		char rootdir[ROOT_PATH_SIZE];
		if (NULL == getcwd(rootdir, ROOT_PATH_SIZE))
		{
			std::cerr << "openPath() get Abs Failed: " << path;
			std::cerr << std::endl;
			return;
		}

		std::string rdir(rootdir);
		surl += QString::fromStdString(rdir);
		surl += '/';
	}

	surl += QString::fromStdString(path);
	std::cerr << "openPath() opening AbsPath Url: " << surl.toStdString();
	std::cerr << std::endl;

	QUrl url(surl);
	QDesktopServices::openUrl(url);

	return;
}


