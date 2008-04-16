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

#include "LinksDialog.h"
#include "GamesDialog.h"
#include "PhotoDialog.h"
#include "channels/channelsDialog.h"

#include <rshare.h>
#include "MainWindow.h"
#include "MessengerWindow.h"
#include "HelpDialog.h"

#include "games/qbackgammon/bgwindow.h"
#include "smplayer.h"

#include "Preferences/PreferencesWindow.h"
#include "Settings/gsettingswin.h"
#include "util/rsversion.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"

#include "gui/connect/InviteDialog.h"
#include "gui/connect/AddFriendDialog.h"


#define FONT        QFont(tr("Arial"), 8)

/* Images for toolbar icons */
#define IMAGE_NETWORK           ":/images/network32.png"
#define IMAGE_PEERS         	":/images/peers_24x24.png"
#define IMAGE_SEARCH    	    ":/images/filefind.png"
#define IMAGE_TRANSFERS      	":/images/ktorrent32.png"
#define IMAGE_FILES   	        ":/images/folder_green.png"
#define IMAGE_CHANNELS       	":/images/channels.png"
#define IMAGE_PREFERENCES       ":/images/settings16.png"
#define IMAGE_CHAT          	":/images/groupchat.png"
#define IMAGE_RETROSHARE        ":/images/rstray3.png"
#define IMAGE_ABOUT             ":/images/informations_24x24.png"
#define IMAGE_STATISTIC         ":/images/utilities-system-monitor.png"
#define IMAGE_MESSAGES          ":/images/evolution.png"
#define IMAGE_BWGRAPH           ":/images/ksysguard.png"
#define IMAGE_RSM32             ":/images/kdmconfig.png"
#define IMAGE_RSM16             ":/images/rsmessenger16.png"
#define IMAGE_CLOSE             ":/images/close_normal.png"
#define IMAGE_SMPLAYER		    ":/images/smplayer_icon32.png"
#define IMAGE_BLOCK         	":/images/blockdevice.png"
#define IMAGE_COLOR         	":/images/highlight.png"
#define IMAGE_GAMES             ":/images/kgames.png"
#define IMAGE_PHOTO             ":/images/lphoto.png"
#define IMAGE_SMPLAYER          ":/images/smplayer_icon32.png"
#define IMAGE_LINKS         	":/images/ktorrent.png"
#define IMAGE_ADDFRIEND         ":/images/add-friend24.png"
#define IMAGE_INVITEFRIEND      ":/images/invite-friend24.png"
#define IMAGE_ADDSHARE          ":/images/directoryadd_24x24_shadow.png"
#define IMAGE_OPTIONS           ":/images/settings.png"
#define IMAGE_QUIT              ":/images/exit_24x24.png"

/* Keys for UI Preferences */
#define UI_PREF_PROMPT_ON_QUIT  "UIOptions/ConfirmOnQuit"
/* uncomment this for release version */

/*****
 * #define RS_RELEASE_VERSION    1
 ****/

/** Constructor */
MainWindow::MainWindow(QWidget* parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
{
    /* Invoke the Qt Designer generated QObject setup routine */
    ui.setupUi(this);
    
    setWindowTitle(tr("RetroShare %1").arg(retroshareVersion())); 

    mSMPlayer = NULL;
  
    /* Hide Console frame */
    //showConsoleFrame(false);
    //connect(ui.btnToggleConsole, SIGNAL(toggled(bool)), this, SLOT(showConsoleFrame(bool)));	

	ui.toolBarservice->hide();
	
    // Setting icons
    this->setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));
	
    /* Create all the dialogs of which we only want one instance */
    _bandwidthGraph = new BandwidthGraph();
    messengerWindow = new MessengerWindow();
    messengerWindow->hide();
    applicationWindow = new ApplicationWindow();
    applicationWindow->hide();
	
	/** Left Side ToolBar**/
    connect(ui.actionAdd_Friend, SIGNAL(triggered() ), this , SLOT( addFriend() ) );
    connect(ui.actionInvite_Friend, SIGNAL(triggered() ), this , SLOT( inviteFriend() ) );
    connect(ui.actionAdd_Share, SIGNAL(triggered() ), this , SLOT( addSharedDirectory() ) );
    connect(ui.actionOptions, SIGNAL(triggered()), this, SLOT( showPreferencesWindow()) );
    connect(ui.actionMessenger, SIGNAL(triggered()), this, SLOT( showMessengerWindow()) );
    connect(ui.actionSMPlayer, SIGNAL(triggered()), this, SLOT( showsmplayer()) );
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT( showabout()) );
    connect(ui.actionColor, SIGNAL(triggered()), this, SLOT( setStyle()) );

    
    /** Games ToolBox*/
	//connect(ui.qbackgammonButton, SIGNAL(clicked( bool )), this, SLOT( startgammon()) );
	//connect(ui.qcheckersButton, SIGNAL(clicked( bool )), this, SLOT( startqcheckers()) );
	 
        
    /** adjusted quit behaviour: trigger a warning that can be switched off in the saved
        config file RetroShare.conf */
   connect(ui.actionQuit, SIGNAL(triggered()), this, SLOT(doQuit()));

    /* load the StyleSheet*/
    loadStyleSheet(Rshare::stylesheet()); 



    /* Create the Main pages and actions */
    QActionGroup *grp = new QActionGroup(this);


    ui.stackPages->add(networkDialog = new NetworkDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_NETWORK), tr("Network"), grp));
  
    ui.stackPages->add(peersDialog = new PeersDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_PEERS), tr("Friends"), grp));
                                        
    ui.stackPages->add(searchDialog = new SearchDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_SEARCH), tr("Search"), grp));
                     
    ui.stackPages->add(transfersDialog = new TransfersDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_TRANSFERS), tr("Transfers"), grp));
                     
    ui.stackPages->add(sharedfilesDialog = new SharedFilesDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_FILES), tr("Files"), grp));
                     
    ui.stackPages->add(chatDialog = new ChatDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_CHAT), tr("Chat"), grp));

    ui.stackPages->add(messagesDialog = new MessagesDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_MESSAGES), tr("Messages"), grp));
                       


#ifdef RS_RELEASE_VERSION    
    channelsDialog = NULL;
#else
    channelsDialog = NULL;
//    ui.stackPages->add(channelsDialog = new ChannelsDialog(ui.stackPages),
//                       createPageAction(QIcon(IMAGE_CHANNELS), tr("Channels"), grp));
#endif

    //ui.stackPages->add(new HelpDialog(ui.stackPages),
    //                   createPageAction(QIcon(IMAGE_ABOUT), tr("About/Help"), grp));
                     
  //ui.stackPages->add(groupsDialog = new GroupsDialog(ui.stackPages),
  //                   createPageAction(QIcon(), tr("Groups"), grp));
                                                              
  //ui.stackPages->add(new StatisticDialog(ui.stackPages),
  //                   createPageAction(QIcon(IMAGE_STATISTIC), tr("Statistics"), grp));

    /* also an empty list of chat windows */
    peersDialog->setChatDialog(chatDialog);
    messengerWindow->setChatDialog(chatDialog);

    /* Create the toolbar */
    ui.toolBar->addActions(grp->actions());
    ui.toolBar->addSeparator();
    connect(grp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *)));
       

    // Allow to play files from SharedFilesDialog.
    connect(sharedfilesDialog, SIGNAL(playFiles( QStringList )), this, SLOT(playFiles( QStringList )));
    connect(transfersDialog, SIGNAL(playFiles( QStringList )), this, SLOT(playFiles( QStringList )));

#ifdef RS_RELEASE_VERSION    
    //addAction(new QAction(QIcon(IMAGE_BLOCK), tr("Unfinished"), ui.toolBar), SLOT(showApplWindow()));

#else
    addAction(new QAction(QIcon(IMAGE_BLOCK), tr("Unfinished"), ui.toolBar), SLOT(showApplWindow()));

    toolAct = ui.toolBarservice->toggleViewAction();
    toolAct->setText("Service");
    toolAct->setShortcut(tr("Ctrl+T"));
    toolAct->setIcon(QIcon(":/images/blockdevice2.png"));
    ui.toolBar->addAction(toolAct);
 

    /* Select the first action */
    grp->actions()[0]->setChecked(true);
    /* Select the first action */
        /* Create the Service pages and actions */
    QActionGroup *servicegrp = new QActionGroup(this);
   
                           
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
                         
                                      
    
    /* Create the toolbarservice */
    ui.toolBarservice->addActions(servicegrp->actions());
    ui.toolBarservice->addSeparator();
    connect(servicegrp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *))); 
        
    ui.toolBarservice->addSeparator();

    statusBar()->addWidget(new QLabel(tr("Users: 0  Files: 0 ")));
    statusBar()->addPermanentWidget(new QLabel(tr("Down: 0.0  Up: 0.0 ")));
    statusBar()->addPermanentWidget(new QLabel(tr("Connections: 0/45 ")));

#endif

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

    /* bandwidth only in development version */
#ifdef RS_RELEASE_VERSION    
#else
    menu->addAction(_bandwidthAct);
#endif
    menu->addAction(_prefsAct);
    menu->addAction(_smplayerAct);
    menu->addSeparator();
    menu->addAction(tr("Minimize"), this, SLOT(showMinimized()));
    menu->addAction(tr("Maximize"), this, SLOT(showMaximized()));
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
void MainWindow::show()
{
  
    if (!this->isVisible()) {
        QMainWindow::show();
    } else {
        QMainWindow::activateWindow();
        setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
        QMainWindow::raise();
    }
}


/** Shows the config dialog with focus set to the given page. */
void MainWindow::show(Page page)
{
    /* Show the dialog. */
    show();

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

/** Shows Preferences */
void MainWindow::addSharedDirectory()
{
    /* Same Code as in Preferences Window (add Share) */

    QString qdir = QFileDialog::getExistingDirectory(this, tr("Add Shared Directory"), "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
						   
    /* add it to the server */
    std::string dir = qdir.toStdString();
    if (dir != "")
    {
        rsicontrol -> ConfigAddSharedDir(dir);
        //rsicontrol -> ConfigSave();
    }

}

/** Shows Preferences */
void MainWindow::showPreferencesWindow()
{
    static PreferencesWindow* preferencesWindow = new PreferencesWindow(this);
    preferencesWindow->show();
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
    delete _prefsAct;
    delete _bandwidthGraph;
    delete _messengerwindowAct;
    delete _smplayerAct;
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
    
    _smplayerAct = new QAction(QIcon(IMAGE_SMPLAYER), tr("SMPlayer"), this);
    connect(_smplayerAct, SIGNAL(triggered()),this, SLOT(showsmplayer()));
         
          
    //connect(ui.btntoggletoolbox, SIGNAL(toggled(bool)), this, SLOT(showToolboxFrame(bool)));
  
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


/**
Toggles the Console pane on and off, changes toggle button text
 */
/*void MainWindow::showConsoleFrame(bool show)
{
    if (show) {
        ui.frmConsole->setVisible(true);
        ui.btnToggleConsole->setChecked(true);
        ui.btnToggleConsole->setToolTip(tr("Hide Console"));
    } else {
        ui.frmConsole->setVisible(false);
        ui.btnToggleConsole->setChecked(false);
        ui.btnToggleConsole->setToolTip(tr("Show Console"));
    }
}*/

/**
 Toggles the ToolBox on and off, changes toggle button text
 */
/*void MainWindow::showToolboxFrame(bool show)
{
    if (show) {
        ui.toolboxframe->setVisible(true);
        ui.btntoggletoolbox->setChecked(true);
        ui.btntoggletoolbox->setToolTip(tr("Hide ToolBox"));
        ui.btntoggletoolbox->setIcon(QIcon(tr(":images/hide_toolbox_frame.png")));
    } else {
        ui.toolboxframe->setVisible(false);
        ui.btntoggletoolbox->setChecked(false);
        ui.btntoggletoolbox->setToolTip(tr("Show ToolBox"));
        ui.btntoggletoolbox->setIcon(QIcon(tr(":images/show_toolbox_frame.png")));
    }
}*/



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
    myTopLevel* top = new myTopLevel();
    top->show();
    
}


/** Shows smplayer */
void MainWindow::showsmplayer()
{    
    static SMPlayer * smplayer = 0;
    
    if (mSMPlayer == 0) 
    {
    	mSMPlayer = new SMPlayer(QString::null, this);
    }
    mSMPlayer->gui()->show();

}

void MainWindow::playFiles(QStringList files)
{
	showsmplayer();
	mSMPlayer->gui()->openFiles(files);
}


void MainWindow::showabout()
{
    static HelpDialog *helpdlg = new HelpDialog(this); 
	helpdlg->show(); 
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



