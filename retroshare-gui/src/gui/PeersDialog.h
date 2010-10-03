/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#ifndef _PEERSDIALOG_H
#define _PEERSDIALOG_H

#include "chat/ChatStyle.h"
#include "RsAutoUpdatePage.h"

#include "mainpage.h"

#include "im_history/IMHistoryKeeper.h"

// states for sorting (equal values are possible)
// used in BuildSortString - state + name
#define PEER_STATE_ONLINE       1
#define PEER_STATE_AWAY         2
#define PEER_STATE_BUSY         3
#define PEER_STATE_AVAILABLE    4
#define PEER_STATE_INACTIVE     5
#define PEER_STATE_OFFLINE      6

#define BuildStateSortString(bEnabled,sName,nState) bEnabled ? (QString ("%1").arg(nState) + " " + sName) : sName

#ifndef MINIMAL_RSGUI
#include "ui_PeersDialog.h"

class QFont;
class QAction;
class QTextEdit;
class QTextCharFormat;
class ChatDialog;
class AttachFileItem;
class RSTreeWidgetItemCompareRole;

class PeersDialog : public RsAutoUpdatePage 
{
    Q_OBJECT

public:
    /** Default Constructor */
    PeersDialog(QWidget *parent = 0);
    /** Default Destructor */
    ~PeersDialog ();

    //  void setChatDialog(ChatDialog *cd);

    virtual void updateDisplay() ;	// overloaded from RsAutoUpdatePage
    // replaced by shortcut
    //		virtual void keyPressEvent(QKeyEvent *) ;

public slots:

    void  insertPeers();
    void publicChatChanged(int type);
//    void toggleSendItem( QTreeWidgetItem *item, int col );

    void insertChat();
    void setChatInfo(QString info, QColor color=QApplication::palette().color(QPalette::WindowText));
    void resetStatusBar() ;

    void fileHashingFinished(AttachFileItem* file);

    void smileyWidgetgroupchat();
    void addSmileys();

    void on_actionClearChat_triggered();
    void on_actionMessageHistory_triggered();

    void displayInfoChatMenu(const QPoint& pos);

    // called by notifyQt when another peer is typing (in group chant and private chat)
    void updatePeerStatusString(const QString& peer_id,const QString& status_string,bool is_private_chat) ;

    void updatePeersAvatar(const QString& peer_id);
    void updateAvatar();	// called by notifyQt to update the avatar when it gets changed by another component

    void groupsChanged(int type);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    bool eventFilter(QObject *obj, QEvent *ev);
    void showEvent (QShowEvent *event);

private slots:
    void pasteLink() ;
    void contextMenu(QPoint) ;

    /** Create the context popup menu and it's submenus */
    void peertreeWidgetCostumPopupMenu( QPoint point );

    void updateStatusString(const QString& peer_id, const QString& statusString) ;	// called when a peer is typing in group chat
    void updateStatusTyping() ;										// called each time a key is hit

    //void updatePeerStatusString(const QString& peer_id,const QString& chat_status) ;

    /** Export friend in Friends Dialog */
    void exportfriend();
    /** Remove friend  */
    void removefriend();
    /** start a chat with a friend **/
    void addFriend();
    void chatfriend(QTreeWidgetItem* );
    void chatfriendproxy();
    void msgfriend();
    void recommendfriend();
    void pastePerson();
    void addToGroup();
    void moveToGroup();
    void removeFromGroup();
    void editGroup();
    void removeGroup();

    void configurefriend();
    void viewprofile();

    /** RsServer Friend Calls */
    void connectfriend();

    void setColor();
    void insertSendList();
    void sendMsg();

    void statusmessage();

    void setFont();
    void getFont();
    void underline();

    void changeAvatarClicked();
    void getAvatar();

    void on_actionAdd_Group_activated();
    void on_actionCreate_New_Forum_activated();
    void on_actionCreate_New_Channel_activated();

    void loadmypersonalstatus();

    void addExtraFile();
    void anchorClicked (const QUrl &);
    void addAttachment(std::string);

    bool fileSave();
    bool fileSaveAs();

    void setCurrentFileName(const QString &fileName);

    void displayMenu();
    void statusColumn();

signals:
    void friendsUpdated() ;
    void notifyGroupChat(const QString&,const QString&) ;

private:
    void processSettings(bool bLoad);
    void addChatMsg(bool incoming, bool history, QString &name, QDateTime &recvTime, QString &message);

    class QLabel *iconLabel, *textLabel;
    class QWidget *widget;
    class QWidgetAction *widgetAction;
    class QSpacerItem *spacerItem;

    RSTreeWidgetItemCompareRole *m_compareRole;

    ///play the sound when recv a message
    void playsound();

    QString fileName;
    bool groupsHasChanged;
    std::list<std::string> openGroups;

    /* Worker Functions */
    /* (1) Update Display */

    /* (2) Utility Fns */
    QTreeWidgetItem *getCurrentPeer();

    /** Defines the actions for the context menu */
    QAction* pasteLinkAct;

    IMHistoryKeeper historyKeeper;
    ChatStyle style;

    QColor _currentColor;
    bool _underline;
    time_t last_status_send_time ;

    QFont mCurrentFont; /* how the text will come out */

    /** Qt Designer generated object */
    Ui::PeersDialog ui;
};

#endif // MINIMAL_RSGUI

#endif
