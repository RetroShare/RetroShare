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

#include "gui/common/rwindow.h"

namespace Ui {
class MainWindow;
}

class QComboBox;
class QLabel;
class QActionGroup;
class QListWidgetItem;
class Idle;
class PeerStatus;
class NATStatus;
class DHTStatus;
class HashingStatus;
class DiscStatus;
class RatesStatus;
class OpModeStatus;
class SoundStatus;
class ToasterDisable;
class SysTrayStatus;
//class ForumsDialog;
class GxsChannelDialog ;
class GxsForumsDialog ;
class PostedDialog;
class FriendsDialog;
class IdDialog;
class ChatLobbyWidget;
class SettingsPage ;
class ChatDialog;
class NetworkDialog;
class SearchDialog;
class TransfersDialog;
class MessagesDialog;
class SharedFilesDialog;
class MessengerWindow;
class PluginsPage;
class HomePage;
//class ChannelFeed;
class BandwidthGraph;
class MainPage;
class NewsFeed;
class UserNotify;

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
        Friends            = 1,  /** Friends page. */
        ChatLobby          = 2,  /** Chat Lobby page. */
        Transfers          = 3,  /** Transfers page. */
        SharedDirectories  = 4,  /** Shared Directories page. */
        Messages           = 5,  /** Messages page. */
        Channels           = 6,  /** Channels page. */
        Forums             = 7,  /** Forums page. */
        Search             = 8,  /** Search page. */
#ifdef BLOGS
        Blogs              = 9,  /** Blogs page. */
#endif
#ifdef RS_USE_LINKS
        Links              = 10,  /** Links page. */
#endif
        Posted             = 11,  /** Posted links */
        People             = 12,   /** People page. */
        Options            = 13   /** People page. */
    };


    enum StatusElement {
        StatusGrpStatus    = 0x01,
        StatusCompactMode  = 0x02,
        StatusShowToolTip  = 0x03,
        StatusShowStatus   = 0x04,
        StatusShowPeer     = 0x05,
        StatusShowNAT      = 0x06,
        StatusShowDHT      = 0x07,
        StatusShowHashing  = 0x08,
        StatusShowDisc     = 0x09,
        StatusShowRate     = 0x0a,
        StatusShowOpMode   = 0x0b,
        StatusShowSound    = 0x0c,
        StatusShowToaster  = 0x0d,
        StatusShowSystray  = 0x0e,
    };

    /** Create main window */
    static MainWindow *Create ();
    static MainWindow *getInstance();

    /** Destructor. */
    ~MainWindow();

    static void raiseWindow();
    /** Shows the MainWindow dialog with focus set to the given page. */
    static void showWindow(Page page);
    static void showWindow(MainPage *page);
    /** Set focus to the given page. */
    static bool activatePage (Page page);
    static Page getActivatePage ();

    /** get page */
    static MainPage *getPage (Page page);

    const QList<UserNotify*> &getUserNotifyList();

    /* A Bit of a Hack... but public variables for
    * the dialogs, so we can add them to the
    * Notify Class...
    */

//    NetworkDialog     *networkDialog;
//    SearchDialog      *searchDialog;

	 HomePage          *homePage;
	 NewsFeed          *newsFeed;
	 FriendsDialog     *friendsDialog;
	 TransfersDialog   *transfersDialog;
 	 IdDialog          *idDialog;
	 ChatLobbyWidget   *chatLobbyDialog;
	 MessagesDialog    *messagesDialog;
	 SettingsPage      *settingsDialog;
	 SharedFilesDialog *sharedfilesDialog;
	 GxsChannelDialog  *gxschannelDialog ;
	 GxsForumsDialog   *gxsforumDialog ;
	 PostedDialog      *postedDialog;

//    ForumsDialog      *forumsDialog;
//    ChannelFeed       *channelFeed;
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
    static void installNotifyIcons();
    static void displayLobbySystrayMsg(const QString&,const QString&);

    static void switchVisibilityStatus(MainWindow::StatusElement e,bool b);

    /* initialize widget with status informations, status constant stored in data or in Qt::UserRole */
    void initializeStatusObject(QObject *pObject, bool bConnect);
    void removeStatusObject(QObject *pObject);
    void setStatus(QObject *pObject, int nStatus);

    QComboBox *statusComboBoxInstance();
    PeerStatus *peerstatusInstance();
    NATStatus *natstatusInstance();
    DHTStatus *dhtstatusInstance();
    HashingStatus *hashingstatusInstance();
    DiscStatus *discstatusInstance();
    RatesStatus *ratesstatusInstance();
    OpModeStatus *opModeStatusInstance();
    SoundStatus *soundStatusInstance();
    ToasterDisable *toasterDisableInstance();
    SysTrayStatus *sysTrayStatusInstance();

public slots:
    void receiveNewArgs(QStringList args);
    void displayErrorMessage(int,int,const QString&) ;
    void postModDirectories(bool update_local);
    void displayDiskSpaceWarning(int loc,int size_limit_mb) ;
    void checkAndSetIdle(int idleTime);

    void externalLinkActivated(const QUrl &url);
    void retroshareLinkActivated(const QUrl &url);
    void openRsCollection(const QString &filename);
    void processLastArgs();
    //! Go to a specific part of the control panel.
    void setNewPage(int page);
    void setCompactStatusMode(bool compact);

    void toggleStatusToolTip(bool toggle);
protected:
    /** Default Constructor */
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);

    void closeEvent(QCloseEvent *);
    
    /** Called when the user changes the UI translation. */
    virtual void retranslateUi();

private slots:

    void updateMenu();
    void updateStatus();
    void updateFriends();

    void toggleVisibility(QSystemTrayIcon::ActivationReason e);
    void toggleVisibilitycontextmenu();

    /** Toolbar fns. */
    void addFriend();
    void newRsCollection();
    void showMessengerWindow();
    void showStatisticsWindow();
#ifdef ENABLE_WEBUI
    void showWebinterface();
#endif
    //void servicePermission();

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
    void statusChangedMenu(QAction *pAction);
    void statusChangedComboBox(int index);
    void settingsChanged();

    /** Called when user attempts to quit via quit button*/
    void doQuit();
    
    void updateTrayCombine();

private:
    void initStackedPage();
    void addPage(MainPage *page, QActionGroup *grp, 	QList<QPair<MainPage *, QPair<QAction *, QListWidgetItem *> > > *notify);
    void createTrayIcon();
    void createNotifyIcons();
    static MainWindow *_instance;

    /** A BandwidthGraph object which handles monitoring RetroShare bandwidth usage */
    BandwidthGraph* _bandwidthGraph;

    typedef void (MainWindow::*FunctionType)();

    /** Creates a new action for a Main page. */
    QAction* createPageAction(const QIcon &icon, const QString &text, QActionGroup *group);
    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, FunctionType actionFunction, const char *slot = 0);
    QMap<QString, FunctionType> _functionList;

    QString nameAndLocation;

    QSystemTrayIcon *trayIcon;
    QMenu *notifyMenu;
    QString notifyToolTip;
    QAction *toggleVisibilityAction, *toolAct;
    QList<UserNotify*> userNotifyList;

    QComboBox *statusComboBox;
    PeerStatus *peerstatus;
    NATStatus *natstatus;
    DHTStatus *dhtstatus;
    HashingStatus *hashingstatus;
    DiscStatus *discstatus;
    RatesStatus *ratesstatus;
    OpModeStatus *opModeStatus;
    SoundStatus *soundStatus;
    ToasterDisable *toasterDisable;
    SysTrayStatus *sysTrayStatus;

    /* Status */
    std::set <QObject*> m_apStatusObjects; // added objects for status
    bool m_bStatusLoadDone;
    unsigned int onlineCount;

    void loadOwnStatus();

    // idle function
    void setIdle(bool Idle);
    bool isIdle;

	 Ui::MainWindow *ui ;
};

#endif
