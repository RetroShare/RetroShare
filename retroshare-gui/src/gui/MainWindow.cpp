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
#ifdef RS_USE_LINKS
#include "LinksDialog.h"
#endif
#include "ForumsDialog.h"
#include "PeersDialog.h"
#include "HelpDialog.h"
#include "AboutDialog.h"
#include "QuickStartWizard.h"
#include "ChannelFeed.h"
#include "bwgraph/bwgraph.h"
#include "help/browser/helpbrowser.h"
#include "chat/PopupChatDialog.h"
#include "RetroShareLink.h"

#ifdef UNFINISHED
#include "unfinished/ApplicationWindow.h"
#endif

#include "gui/TurtleRouterDialog.h"
#include "idle/idle.h"

#include "statusbar/peerstatus.h"
#include "statusbar/natstatus.h"
#include "statusbar/ratesstatus.h"
#include "statusbar/dhtstatus.h"
#include "statusbar/hashingstatus.h"
#include <retroshare/rsstatus.h>

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rsforums.h>
#include <retroshare/rschannels.h>
#include <retroshare/rsnotify.h>

#include "gui/connect/ConnectFriendWizard.h"
#include "util/rsversion.h"
#include "settings/rsettingswin.h"
#include "settings/rsharesettings.h"
#include "common/StatusDefs.h"

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
    : RWindow("MainWindow", parent, flags)
{
    /* Invoke the Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    _instance = this;

    m_bStatusLoadDone = false;
    isIdle = false;

    trayIconCombined = NULL;
    trayIconMessages = NULL;
    trayIconForums = NULL;
    trayIconChannels = NULL;
    trayIconChat = NULL;
    trayIconTransfers = NULL;
    trayActionMessages = NULL;
    trayActionForums = NULL;
    trayActionChannels = NULL;
    trayActionChat = NULL;
    trayActionTransfers = NULL;

    setWindowTitle(tr("RetroShare %1 a secure decentralised communication platform").arg(retroshareVersion()));

    /* add url handler for RetroShare links */
    QDesktopServices::setUrlHandler(RSLINK_SCHEME, this, "linkActivated");

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
                      transferAction = createPageAction(QIcon(IMAGE_TRANSFERS), tr("Transfers"), grp));


    ui.stackPages->add(sharedfilesDialog = new SharedFilesDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_FILES), tr("Files"), grp));


    ui.stackPages->add(messagesDialog = new MessagesDialog(ui.stackPages),
                      messageAction = createPageAction(QIcon(IMAGE_MESSAGES), tr("Messages"), grp));   

    ui.stackPages->add(channelFeed = new ChannelFeed(ui.stackPages),
                      channelAction = createPageAction(QIcon(IMAGE_CHANNELS), tr("Channels"), grp));

#ifdef BLOGS
    ui.stackPages->add(blogsFeed = new BlogsDialog(ui.stackPages),
		createPageAction(QIcon(IMAGE_BLOGS), tr("Blogs"), grp));
#endif
                      
    ui.stackPages->add(forumsDialog = new ForumsDialog(ui.stackPages),
                       forumAction = createPageAction(QIcon(IMAGE_FORUMS), tr("Forums"), grp));

#ifdef RS_USE_LINKS
    ui.stackPages->add(linksDialog = new LinksDialog(ui.stackPages),
			createPageAction(QIcon(IMAGE_LINKS), tr("Links Cloud"), grp));
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

    if (activatePage((Page) Settings->getLastPageInMainWindow()) == false) {
        /* Select the first action */
        grp->actions()[0]->setChecked(true);
    }

    /** StatusBar section ********/
    /* initialize combobox in status bar */
    statusComboBox = new QComboBox(statusBar());
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

    ratesstatus = new RatesStatus();
    statusBar()->addPermanentWidget(ratesstatus);
    /** Status Bar end ******/

    /* Create the actions that will go in the tray menu */
    createActions();
    /* Creates a tray icon with a context menu and adds it to the system's * notification area. */
    createTrayIcon();

    loadOwnStatus();

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
    Settings->setLastPageInMainWindow(getActivatePage());

    delete _bandwidthGraph;
    delete _messengerwindowAct;
    delete peerstatus;
    delete natstatus;
    delete dhtstatus;
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
    QMenu *trayMenu = new QMenu(this);
    QObject::connect(trayMenu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));
    toggleVisibilityAction = trayMenu->addAction(QIcon(IMAGE_RETROSHARE), tr("Show/Hide"), this, SLOT(toggleVisibilitycontextmenu()));

    QMenu *pStatusMenu = trayMenu->addMenu(tr("Status"));
    initializeStatusObject(pStatusMenu, true);

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

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(toggleVisibility(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();

    createNotifyIcons();
}

void MainWindow::createNotifyIcons()
{
#define DELETE_ICON(x) if (x) { delete(x); x = NULL; }

    int notifyFlag = Settings->getTrayNotifyFlags();

    QMenu *trayMenu = NULL;

    /* Delete combined systray icon and rebuild it */
    trayActionMessages = NULL;
    trayActionForums = NULL;
    trayActionChannels = NULL;
    trayActionChat = NULL;
    trayActionTransfers = NULL;
    DELETE_ICON(trayIconCombined);

    if (notifyFlag & TRAYNOTIFY_COMBINEDICON) {
        /* Delete single systray icons */
        DELETE_ICON(trayIconMessages);
        DELETE_ICON(trayIconForums);
        DELETE_ICON(trayIconChannels);
        DELETE_ICON(trayIconChat);
        DELETE_ICON(trayIconTransfers);

        /* Create combined systray icon */
        trayIconCombined = new QSystemTrayIcon(this);
        trayIconCombined->setIcon(QIcon(":/images/rstray_new.png"));

        trayMenu = new QMenu(this);
        trayIconCombined->setContextMenu(trayMenu);
    }

    /* Create systray icons or actions */
    if (notifyFlag & TRAYNOTIFY_MESSAGES) {
        if (trayMenu) {
            DELETE_ICON(trayIconMessages);

            trayActionMessages = trayMenu->addAction(QIcon(":/images/newmsg.png"), "", this, SLOT(trayIconMessagesClicked()));
            trayActionMessages->setVisible(false);
            trayActionMessages->setData(tr("Messages"));
        } else if (trayIconMessages == NULL) {
            // Create the tray icon for messages
            trayIconMessages = new QSystemTrayIcon(this);
            trayIconMessages->setIcon(QIcon(":/images/newmsg.png"));
            connect(trayIconMessages, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconMessagesClicked(QSystemTrayIcon::ActivationReason)));
        }
    } else {
        DELETE_ICON(trayIconMessages);
    }

    if (notifyFlag & TRAYNOTIFY_FORUMS) {
        if (trayMenu) {
            DELETE_ICON(trayIconForums);

            trayActionForums = trayMenu->addAction(QIcon(":/images/konversation16.png"), "", this, SLOT(trayIconForumsClicked()));
            trayActionForums->setVisible(false);
            trayActionForums->setData(tr("Forums"));
        } else if (trayIconForums == NULL) {
            // Create the tray icon for forums
            trayIconForums = new QSystemTrayIcon(this);
            trayIconForums->setIcon(QIcon(":/images/konversation16.png"));
            connect(trayIconForums, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconForumsClicked(QSystemTrayIcon::ActivationReason)));
        }
    } else {
        DELETE_ICON(trayIconForums);
    }

    if (notifyFlag & TRAYNOTIFY_CHANNELS) {
        if (trayMenu) {
            DELETE_ICON(trayIconChannels);

            trayActionChannels = trayMenu->addAction(QIcon(":/images/channels16.png"), "", this, SLOT(trayIconChannelsClicked()));
            trayActionChannels->setVisible(false);
            trayActionChannels->setData(tr("Channels"));
        } else if (trayIconChannels == NULL) {
            // Create the tray icon for channels
            trayIconChannels = new QSystemTrayIcon(this);
            trayIconChannels->setIcon(QIcon(":/images/channels16.png"));
            connect(trayIconChannels, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconChannelsClicked(QSystemTrayIcon::ActivationReason)));
        }
    } else {
        DELETE_ICON(trayIconChannels);
    }

    if (notifyFlag & TRAYNOTIFY_PRIVATECHAT) {
        if (trayMenu) {
            DELETE_ICON(trayIconChat);

            trayActionChat = trayMenu->addAction(QIcon(":/images/chat.png"), "", this, SLOT(trayIconChatClicked()));
            trayActionChat->setVisible(false);
            trayActionChat->setData(tr("Chat"));
        } else if (trayIconChat == NULL) {
            // Create the tray icon for chat
            trayIconChat = new QSystemTrayIcon(this);
            trayIconChat->setIcon(QIcon(":/images/chat.png"));
            connect(trayIconChat, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconChatClicked(QSystemTrayIcon::ActivationReason)));
        }
    } else {
        DELETE_ICON(trayIconChat);
    }

    if (notifyFlag & TRAYNOTIFY_TRANSFERS) {
        if (trayMenu) {
            DELETE_ICON(trayIconTransfers);

            trayActionTransfers = trayMenu->addAction(QIcon(":/images/ktorrent32.png"), "", this, SLOT(trayIconTransfersClicked()));
            trayActionTransfers->setVisible(false);
            trayActionTransfers->setData(tr("Transfers"));
        } else if (trayIconTransfers == NULL) {
            // Create the tray icon for transfers
            trayIconTransfers = new QSystemTrayIcon(this);
            trayIconTransfers->setIcon(QIcon(":/images/ktorrent32.png"));
            connect(trayIconTransfers, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconTransfersClicked(QSystemTrayIcon::ActivationReason)));
        }
    } else {
        DELETE_ICON(trayIconTransfers);
    }

    /* call once */
    updateMessages();
    updateForums();
    updateChannels(NOTIFY_TYPE_ADD);
    privateChatChanged(NOTIFY_LIST_PRIVATE_INCOMING_CHAT, NOTIFY_TYPE_ADD);
    // transfer

#undef DELETE_ICON
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

void MainWindow::updateMessages()
{
    unsigned int newInboxCount = 0;
    rsMsgs->getMessageCount (NULL, &newInboxCount, NULL, NULL, NULL, NULL);

    if(newInboxCount) {
        messageAction->setIcon(QIcon(QPixmap(":/images/messages_new.png"))) ;
    } else {
        messageAction->setIcon(QIcon(QPixmap(":/images/evolution.png"))) ;
    }

    if (trayIconMessages) {
        if (newInboxCount) {
            if (newInboxCount > 1) {
                trayIconMessages->setToolTip("RetroShare\n" + tr("You have %1 new messages").arg(newInboxCount));
            } else {
                trayIconMessages->setToolTip("RetroShare\n" + tr("You have %1 new message").arg(newInboxCount));
            }
            trayIconMessages->show();
        } else {
            trayIconMessages->hide();
        }
    }

    if (trayActionMessages) {
        if (newInboxCount) {
            if (newInboxCount > 1) {
                trayActionMessages->setText(tr("%1 new messages").arg(newInboxCount));
            } else {
                trayActionMessages->setText(tr("%1 new message").arg(newInboxCount));
            }
            trayActionMessages->setVisible(true);
        } else {
            trayActionMessages->setVisible(false);
        }
    }

    updateTrayCombine();
}

void MainWindow::updateForums()
{
    unsigned int newMessageCount = 0;
    unsigned int unreadMessageCount = 0;
    rsForums->getMessageCount("", newMessageCount, unreadMessageCount);

    if (newMessageCount) {
        forumAction->setIcon(QIcon(":/images/konversation_new.png")) ;
    } else {
        forumAction->setIcon(QIcon(IMAGE_FORUMS)) ;
    }

    if (trayIconForums) {
        if (newMessageCount) {
            if (newMessageCount > 1) {
                trayIconForums->setToolTip("RetroShare\n" + tr("You have %1 new messages").arg(newMessageCount));
            } else {
                trayIconForums->setToolTip("RetroShare\n" + tr("You have %1 new message").arg(newMessageCount));
            }
            trayIconForums->show();
        } else {
            trayIconForums->hide();
        }
    }

    if (trayActionForums) {
        if (newMessageCount) {
            if (newMessageCount > 1) {
                trayActionForums->setText(tr("%1 new messages").arg(newMessageCount));
            } else {
                trayActionForums->setText(tr("%1 new message").arg(newMessageCount));
            }
            trayActionForums->setVisible(true);
        } else {
            trayActionForums->setVisible(false);
        }
    }

    updateTrayCombine();
}

void MainWindow::updateChannels(int type)
{
    unsigned int newMessageCount = 0;
    unsigned int unreadMessageCount = 0;
    rsChannels->getMessageCount("", newMessageCount, unreadMessageCount);

    if (newMessageCount) {
        channelAction->setIcon(QIcon(":/images/channels_new32.png")) ;
    } else {
        channelAction->setIcon(QIcon(IMAGE_CHANNELS)) ;
    }

    if (trayIconChannels) {
        if (newMessageCount) {
            if (newMessageCount > 1) {
                trayIconChannels->setToolTip("RetroShare\n" + tr("You have %1 new messages").arg(newMessageCount));
            } else {
                trayIconChannels->setToolTip("RetroShare\n" + tr("You have %1 new message").arg(newMessageCount));
            }
            trayIconChannels->show();
        } else {
            trayIconChannels->hide();
        }
    }

    if (trayActionChannels) {
        if (newMessageCount) {
            if (newMessageCount > 1) {
                trayActionChannels->setText(tr("%1 new messages").arg(newMessageCount));
            } else {
                trayActionChannels->setText(tr("%1 new message").arg(newMessageCount));
            }
            trayActionChannels->setVisible(true);
        } else {
            trayActionChannels->setVisible(false);
        }
    }

    updateTrayCombine();
}

void MainWindow::updateTransfers(int count)
{
    if (count) {
        transferAction->setIcon(QIcon(":/images/transfers_new32.png")) ;
    } else {
        transferAction->setIcon(QIcon(IMAGE_TRANSFERS)) ;
    }

    if (trayIconTransfers) {
        if (count) {
            if (count > 1) {
                trayIconTransfers->setToolTip("RetroShare\n" + tr("You have %1 completed downloads").arg(count));
            } else {
                trayIconTransfers->setToolTip("RetroShare\n" + tr("You have %1 completed download").arg(count));
            }
            trayIconTransfers->show();
        } else {
            trayIconTransfers->hide();
        }
    }

    if (trayActionTransfers) {
        if (count) {
            if (count > 1) {
                trayActionTransfers->setText(tr("%1 completed downloads").arg(count));
            } else {
                trayActionTransfers->setText(tr("%1 completed download").arg(count));
            }
            trayActionTransfers->setVisible(true);
        } else {
            trayActionTransfers->setVisible(false);
        }
    }

    updateTrayCombine();
}

void MainWindow::updateTrayCombine()
{
    if (trayIconCombined) {
        QMenu *trayMenu = trayIconCombined->contextMenu();
        QList<QAction*> actions = trayMenu->actions();

        bool visible = false;
        QString toolTip;

        QList<QAction*>::iterator actionIt;
        for (actionIt = actions.begin(); actionIt != actions.end(); actionIt++) {
            if ((*actionIt)->isVisible()) {
                visible = true;
                if (toolTip.isEmpty() == false) {
                    toolTip += "\r";
                }
                toolTip += (*actionIt)->data().toString() + ":" + (*actionIt)->text();
            }
        }

        trayIconCombined->setToolTip(toolTip);
        trayIconCombined->setVisible(visible);
    }
}

void MainWindow::updateStatus()
{
    // This call is essential to remove locks due to QEventLoop re-entrance while asking gpg passwds. Dont' remove it!
    if(RsAutoUpdatePage::eventsLocked())
        return;

    unsigned int nFriendCount = 0;
    unsigned int nOnlineCount = 0;
    rsPeers->getPeerCount (&nFriendCount, &nOnlineCount, false);
    
    float downKb = 0;
    float upKb = 0;
    rsicontrol -> ConfigGetDataRates(downKb, upKb);

    if (ratesstatus)
        ratesstatus->getRatesStatus(downKb, upKb);

    if (peerstatus)
        peerstatus->getPeerStatus(nFriendCount, nOnlineCount);

    if (natstatus)
        natstatus->getNATStatus();
        
    if (dhtstatus)
        dhtstatus->getDHTStatus();

    if (nOnlineCount == 0)
    {
        trayIcon->setIcon(QIcon(IMAGE_NOONLINE));
    }
    else if (nOnlineCount < 2)
    {
        trayIcon->setIcon(QIcon(IMAGE_ONEONLINE));
    }
    else if (nOnlineCount < 3)
    {
        trayIcon->setIcon(QIcon(IMAGE_TWOONLINE));
    }
    else
    {
        trayIcon->setIcon(QIcon(IMAGE_RETROSHARE));
    }

    QString tray = "RetroShare\n" + tr("Down: %1 (kB/s)").arg(downKb, 0, 'f', 2) + " | " + tr("Up: %1 (kB/s)").arg(upKb, 0, 'f', 2) + "\n";

    if (nOnlineCount == 1) {
        tray += tr("%1 friend connected").arg(nOnlineCount);
    } else {
        tray += tr("%1 friends connected").arg(nOnlineCount);
    }

    trayIcon->setToolTip(tray);
}

void MainWindow::privateChatChanged(int list, int type)
{
    /* first process the chat messages */
    PopupChatDialog::privateChatChanged(list, type);

    if (list == NOTIFY_LIST_PRIVATE_INCOMING_CHAT) {
        /* than count the chat messages */
        int chatCount = rsMsgs->getPrivateChatQueueCount(true);

        if (trayIconChat) {
            if (chatCount) {
                if (chatCount > 1) {
                    trayIconChat->setToolTip("RetroShare\n" + tr("You have %1 new messages").arg(chatCount));
                } else {
                    trayIconChat->setToolTip("RetroShare\n" + tr("You have %1 new message").arg(chatCount));
                }
                trayIconChat->show();
            } else {
                trayIconChat->hide();
            }
        }

        if (trayActionChat) {
            if (chatCount) {
                if (chatCount > 1) {
                    trayActionChat->setText(tr("%1 new messages").arg(chatCount));
                } else {
                    trayActionChat->setText(tr("%1 new message").arg(chatCount));
                }
                trayActionChat->setVisible(true);
            } else {
                trayActionChat->setVisible(false);;
            }
        }

        updateTrayCombine();
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
/*static*/ bool MainWindow::activatePage(Page page)
{
    if (_instance == NULL) {
        return false;
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
#ifdef RS_USE_LINKS
    case Links:
        Page = _instance->linksDialog;
        break;
#endif
    case Channels:
        Page = _instance->channelFeed;
        break;
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
        return true;
    }

    return false;
}

/** Get the active page. */
/*static*/ MainWindow::Page MainWindow::getActivatePage()
{
   if (_instance == NULL) {
       return Network;
   }

   QWidget *page = _instance->ui.stackPages->currentWidget();

   if (page == _instance->networkDialog) {
       return Network;
   }
   if (page == _instance->peersDialog) {
       return Friends;
   }
   if (page == _instance->searchDialog) {
       return Search;
   }
   if (page == _instance->transfersDialog) {
       return Transfers;
   }
   if (page == _instance->sharedfilesDialog) {
       return SharedDirectories;
   }
   if (page == _instance->messagesDialog) {
       return Messages;
   }
#ifdef RS_USE_LINKS
   if (page == _instance->linksDialog) {
       return Links;
   }
   if (page == _instance->channelFeed) {
       return Channels;
   }
#endif
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

void MainWindow::trayIconMessagesClicked(QSystemTrayIcon::ActivationReason e)
{
    if(e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick) {
        showMess();
    }
}

void MainWindow::trayIconForumsClicked(QSystemTrayIcon::ActivationReason e)
{
    if(e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick) {
        showWindow(MainWindow::Forums);
    }
}

void MainWindow::trayIconChannelsClicked(QSystemTrayIcon::ActivationReason e)
{
    if(e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick) {
        showWindow(MainWindow::Channels);
    }
}

void MainWindow::trayIconChatClicked(QSystemTrayIcon::ActivationReason e)
{
    if(e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick) {
        PopupChatDialog *pcd = NULL;
        std::list<std::string> ids;
        if (rsMsgs->getPrivateChatQueueIds(true, ids) && ids.size()) {
            pcd = PopupChatDialog::getPrivateChat(ids.front(), RS_CHAT_OPEN | RS_CHAT_FOCUS);
        }

        if (pcd == NULL) {
            showWindow(MainWindow::Friends);
        }
    }
}

void MainWindow::trayIconTransfersClicked(QSystemTrayIcon::ActivationReason e)
{
    if(e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick) {
        showWindow(MainWindow::Transfers);
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

void MainWindow::linkActivated(const QUrl &url)
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

    link.process(RSLINK_PROCESS_NOTIFY_ERROR | RSLINK_PROCESS_NOTIFY_SUCCESS);
}
