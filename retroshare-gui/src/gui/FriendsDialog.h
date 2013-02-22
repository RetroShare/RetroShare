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
#include "retroshare-gui/RsAutoUpdatePage.h"

#include "ui_FriendsDialog.h"

class QFont;
class QAction;
class QTextEdit;
class QTextCharFormat;
class ChatTabWidget;
class NetworkDialog;
class NetworkView;

class FriendsDialog : public RsAutoUpdatePage
{
    Q_OBJECT

public:
    /** Default Constructor */
    FriendsDialog(QWidget *parent = 0);
    /** Default Destructor */
    ~FriendsDialog ();

    virtual UserNotify *getUserNotify(QObject *parent);

    virtual void updateDisplay() ;	// overloaded from RsAutoUpdatePage

    static bool isGroupChatActive();
    static void groupChatActivate();

	 NetworkDialog *networkDialog ;
	 NetworkView *networkView ;
public slots:

    void publicChatChanged(int type);
//    void toggleSendItem( QTreeWidgetItem *item, int col );

    void insertChat();
    void setChatInfo(QString info, QColor color=QApplication::palette().color(QPalette::WindowText));
    void resetStatusBar() ;

    void fileHashingFinished(QList<HashedFile> hashedFiles);

    void smileyWidgetgroupchat();
    void addSmileys();

    // called by notifyQt when another peer is typing (in group chant and private chat)
    void updatePeerStatusString(const QString& peer_id,const QString& status_string,bool is_private_chat) ;

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void showEvent (QShowEvent *event);

private slots:
    void pasteLink() ;
    void contextMenu(QPoint) ;
    void contextMenuMsgText(QPoint);
    void chatCharFormatChanged();

    void on_actionClear_Chat_History_triggered();
    void on_actionDelete_Chat_History_triggered();
    void on_actionMessageHistory_triggered();

    void updateStatusString(const QString& peer_id, const QString& statusString) ;	// called when a peer is typing in group chat
    void updateStatusTyping() ;										// called each time a key is hit

    //void updatePeerStatusString(const QString& peer_id,const QString& chat_status) ;

    void addFriend();

    void chooseColor();
    void insertSendList();
    void sendMsg();

    void statusmessage();

    void setFont();
    void chooseFont();

    void getAvatar();

    void on_actionAdd_Group_activated();

    void loadmypersonalstatus();

    void addExtraFile();

    bool fileSave();
    bool fileSaveAs();

    void setCurrentFileName(const QString &fileName);

    //void newsFeedChanged(int count);

    void recommendFriends();
    void servicePermission();

signals:
    void notifyGroupChat(const QString&,const QString&) ;

private:
    void processSettings(bool bLoad);
    void addChatMsg(bool incoming, bool history, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message);

    void colorChanged();
    void fontChanged();
    void setColorAndFont();

    QString fileName;

    ChatStyle style;

    QColor mCurrentColor;
    time_t last_status_send_time ;

    QFont mCurrentFont; /* how the text will come out */

    //QWidget *newsFeed;
    //QColor newsFeedTabColor;
    //QString newsFeedText;
    bool inChatCharFormatChanged;

    /** Qt Designer generated object */
    Ui::FriendsDialog ui;
};

#endif
