/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 The RetroShare Team
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

#ifndef _MainWindow_H
#define _MainWindow_H

#include <QSystemTrayIcon>
#include <set>

#include "ui_MainWindow.h"
#include "gui/common/rwindow.h"

class QComboBox;
class QLabel;
class Idle;
class PeerStatus;
class NATStatus;
class RatesStatus;
class DHTStatus;
class HashingStatus;
class ForumsDialog;
class PeersDialog;
class ChatDialog;
class NetworkDialog;
class SearchDialog;
class TransfersDialog;
class MessagesDialog;
class SharedFilesDialog;
class MessengerWindow;
class PluginsPage;
class ChannelFeed;
class BandwidthGraph;

#ifdef RS_USE_LINKS
class LinksDialog;
#endif

#ifdef BLOGS
class BlogsDialog;
#endif

#ifdef UNFINISHED
class ApplicationWindow;
#endif

class MainWindow : public RWindow
{
  Q_OBJECT

public:
    /** Main dialog pages. */
    enum Page {
        /* Fixed numbers for load and save the last page */
        Network            = 0,  /** Network page. */
        Friends            = 1,  /** Peers page. */
        Search             = 2,  /** Search page. */
        Transfers          = 3,  /** Transfers page. */
        SharedDirectories  = 4,  /** Shared Directories page. */
        Messages           = 5,  /** Messages page. */
        Channels           = 6,  /** Channels page. */
        Forums             = 7,  /** Forums page. */
#ifdef BLOGS
        Blogs              = 8,  /** Blogs page. */
#endif
#ifdef RS_USE_LINKS
        Links              = 9,  /** Links page. */
#endif        
    };

    /** Create main window */
    static MainWindow *Create ();
    static MainWindow *getInstance();

    /** Destructor. */
    ~MainWindow();

    /** Shows the MainWindow dialog with focus set to the given page. */
    static void showWindow(Page page);
    /** Set focus to the given page. */
    static bool activatePage (Page page);
    static Page getActivatePage ();

    /** get page */
    static MainPage *getPage (Page page);

    /* A Bit of a Hack... but public variables for
    * the dialogs, so we can add them to the
    * Notify Class...
    */

    NetworkDialog     *networkDialog;
    PeersDialog       *peersDialog;
    SearchDialog      *searchDialog;
    TransfersDialog   *transfersDialog;
    ChatDialog        *chatDialog;
    MessagesDialog    *messagesDialog;
    SharedFilesDialog *sharedfilesDialog;
    ForumsDialog      *forumsDialog;
    ChannelFeed       *channelFeed;
    Idle              *idle;

#ifdef RS_USE_LINKS
    LinksDialog       *linksDialog;
#endif

#ifdef BLOGS
    BlogsDialog       *blogsFeed;
#endif

#ifdef UNFINISHED
    ApplicationWindow   *applicationWindow;
#endif
    PluginsPage*   pluginsPage ;

    static void installGroupChatNotifier();

    /* initialize widget with status informations, status constant stored in data or in Qt::UserRole */
    void initializeStatusObject(QObject *pObject, bool bConnect);
    void removeStatusObject(QObject *pObject);
    void setStatus(QObject *pObject, int nStatus);

public slots:
    void displayErrorMessage(int,int,const QString&) ;
    void postModDirectories(bool update_local);
    void displayDiskSpaceWarning(int loc,int size_limit_mb) ;
    void checkAndSetIdle(int idleTime);
    void updateMessages();
    void updateForums();
    void updateChannels(int type);
    void privateChatChanged(int list, int type);

protected:
    /** Default Constructor */
    MainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);

    void closeEvent(QCloseEvent *);
    
    /** Called when the user changes the UI translation. */
    virtual void retranslateUi();

private slots:

    void updateMenu();
    void updateStatus();

    void toggleVisibility(QSystemTrayIcon::ActivationReason e);
    void toggleVisibilitycontextmenu();
    void trayIconMessagesClicked(QSystemTrayIcon::ActivationReason e);
    void trayIconForumsClicked(QSystemTrayIcon::ActivationReason e);
    void trayIconChannelsClicked(QSystemTrayIcon::ActivationReason e);
    void trayIconChatClicked(QSystemTrayIcon::ActivationReason e);

    /** Toolbar fns. */
    void addFriend();
    void showMessengerWindow();
#ifdef UNFINISHED    
    void showApplWindow();
#endif

    void showabout();
    void openShareManager();
    void displaySystrayMsg(const QString&,const QString&) ;

    /** Displays the help browser and displays the most recently viewed help
    * topic. */
    void showHelpDialog();
    /** Called when a child window requests the given help <b>topic</b>. */
    void showHelpDialog(const QString &topic);

    void showMess();
    void showSettings();
    void setStyle();
    void statusChangedMenu(QAction *pAction);
    void statusChangedComboBox(int index);

    /** Called when user attempts to quit via quit button*/
    void doQuit();
    
    void on_actionQuick_Start_Wizard_activated();

    void linkActivated(const QUrl &url);

private:

    /** Create the actions on the tray menu or menubar */
    void createActions();
    
    void createTrayIcon();

    static MainWindow *_instance;

    /** Defines the actions for the tray menu */
    QAction* _settingsAct;
    QAction* _bandwidthAct;
    QAction* _messengerwindowAct;
    QAction* _messagesAct;
    QAction* _smplayerAct;
    QAction* _helpAct;
#ifdef UNFINISHED   
    QAction* _appAct;
#endif    

    /** A BandwidthGraph object which handles monitoring RetroShare bandwidth usage */
    BandwidthGraph* _bandwidthGraph;

    /** Creates a new action for a Main page. */
    QAction* createPageAction(QIcon img, QString text, QActionGroup *group);
    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, const char *slot = 0);

    void loadStyleSheet(const QString &sheetName);

    QSystemTrayIcon *trayIcon;
    QSystemTrayIcon *trayIconMessages;
    QSystemTrayIcon *trayIconForums;
    QSystemTrayIcon *trayIconChannels;
    QSystemTrayIcon *trayIconChat;
    QAction *toggleVisibilityAction, *toolAct;
    QMenu *trayMenu;

    PeerStatus *peerstatus;
    NATStatus *natstatus;
    DHTStatus *dhtstatus;
    RatesStatus *ratesstatus;
    HashingStatus *hashingstatus;
    QComboBox *statusComboBox;

    QAction *messageAction;
    QAction *forumAction;
    QAction *channelAction;

    /* Status */
    std::set <QObject*> m_apStatusObjects; // added objects for status
    bool m_bStatusLoadDone;

    void loadOwnStatus();

    // idle function
    void setIdle(bool Idle);
    bool isIdle;

    /** Qt Designer generated object */
    Ui::MainWindow ui;
};

#endif
