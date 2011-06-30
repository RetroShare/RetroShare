/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2011 RetroShare Team
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

#ifndef _FRIENDSDIALOG_H
#define _FRIENDSDIALOG_H

#include "chat/ChatStyle.h"
#include "RsAutoUpdatePage.h"

#include "mainpage.h"

#include "im_history/IMHistoryKeeper.h"

// states for sorting (equal values are possible)
// used in BuildSortString - state + name
#define PEER_STATE_ONLINE       1
#define PEER_STATE_BUSY         2
#define PEER_STATE_AWAY         3
#define PEER_STATE_AVAILABLE    4
#define PEER_STATE_INACTIVE     5
#define PEER_STATE_OFFLINE      6

#define BuildStateSortString(bEnabled,sName,nState) bEnabled ? (QString ("%1").arg(nState) + " " + sName) : sName

#ifndef MINIMAL_RSGUI
#include "ui_FriendsDialog.h"

class QFont;
class QAction;
class QTextEdit;
class QTextCharFormat;
class ChatDialog;
class AttachFileItem;
class RSTreeWidgetItemCompareRole;

class FriendsDialog : public RsAutoUpdatePage 
{
    Q_OBJECT

public:
    /** Default Constructor */
    FriendsDialog(QWidget *parent = 0);
    /** Default Destructor */
    ~FriendsDialog ();

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

    void on_actionClear_Chat_History_triggered();
    void on_actionDelete_Chat_History_triggered();
    void on_actionMessageHistory_triggered();

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
    void copyLink();
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

    void changeAvatarClicked();
    void getAvatar();
	void updateOwnStatus(const QString &peer_id, int status);

    void on_actionAdd_Group_activated();
    void on_actionCreate_New_Forum_activated();
    void on_actionCreate_New_Channel_activated();

    void loadmypersonalstatus();

    void addExtraFile();
    void addAttachment(std::string);

    bool fileSave();
    bool fileSaveAs();

    void setCurrentFileName(const QString &fileName);

    void setStateColumn();
    void sortPeersAscendingOrder();
    void sortPeersDescendingOrder();
    void peerSortIndicatorChanged(int,Qt::SortOrder);

    void newsFeedChanged(int count);

signals:
    void friendsUpdated() ;
    void notifyGroupChat(const QString&,const QString&) ;

private:
    void processSettings(bool bLoad);
    void addChatMsg(bool incoming, bool history, QString &name, QDateTime &recvTime, QString &message);

    void colorChanged(const QColor &c);
    void fontChanged(const QFont &font);

    class QLabel *iconLabel, *textLabel;
    class QWidget *widget;
    class QWidgetAction *widgetAction;
    class QSpacerItem *spacerItem;

    RSTreeWidgetItemCompareRole *m_compareRole;

    void displayMenu();
    ///play the sound when recv a message
    void playsound();

    QString fileName;
    bool groupsHasChanged;
    std::list<std::string> openGroups;

    /* Worker Functions */
    /* (1) Update Display */

    /* (2) Utility Fns */
    QTreeWidgetItem *getCurrentPeer();

    IMHistoryKeeper historyKeeper;
    ChatStyle style;

    QColor mCurrentColor;
    time_t last_status_send_time ;

    QFont mCurrentFont; /* how the text will come out */

    int newsFeedTabIndex;
    QColor newsFeedTabColor;
    QString newsFeedText;
    bool wasStatusColumnHidden;
    bool correctColumnStatusSize;

    /** Qt Designer generated object */
    Ui::FriendsDialog ui;
};

#endif // MINIMAL_RSGUI

#endif
