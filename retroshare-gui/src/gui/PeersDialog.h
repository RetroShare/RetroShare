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

#include <QFileDialog>

#include "chat/PopupChatDialog.h"

#include "mainpage.h"
#include "ui_PeersDialog.h"

#include "im_history/IMHistoryKeeper.h"

class QFont;
class QAction;
class QTextEdit;
class QTextCharFormat;
class ChatDialog;

class PeersDialog : public MainPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  PeersDialog(QWidget *parent = 0);
  /** Default Destructor */
  
  PopupChatDialog *getPrivateChat(std::string id, std::string name, uint chatflags);
  void clearOldChats();
  
  void loadEmoticonsgroupchat();
//  void setChatDialog(ChatDialog *cd);

public slots:

  void  insertPeers();
	void toggleSendItem( QTreeWidgetItem *item, int col );

  void insertChat();
  void setChatInfo(QString info, QColor color=QApplication::palette().color(QPalette::WindowText));
  
  void smileyWidgetgroupchat();
  void addSmileys();
  
    void on_actionClearChat_triggered();
  void displayInfoChatMenu(const QPoint& pos);


private slots:

  /** Create the context popup menu and it's submenus */
  void peertreeWidgetCostumPopupMenu( QPoint point );
  
  void updatePeerStatusString(const QString& peer_id,const QString& chat_status) ;

  /** Export friend in Friends Dialog */
  void exportfriend();
  /** Remove friend  */
  void removefriend();
  /** start a chat with a friend **/
  void chatfriend();
  void msgfriend();

  void configurefriend();
  void viewprofile();

  /** RsServer Friend Calls */
  void allowfriend();
  void connectfriend();
  void setaddressfriend();
  void trustfriend();
  
  void setColor();      
  void insertSendList();
  void checkChat();
  void sendMsg();
  
  //void privchat();
  
  void setFont();
  void getFont();
  void underline(); 

  
private:

  /* Worker Functions */
  /* (1) Update Display */

  /* (2) Utility Fns */
  QTreeWidgetItem *getCurrentPeer();

 // ChatDialog *chatDialog;


  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
    /** Defines the actions for the context menu */
  QAction* chatAct;
  QAction* msgAct;
  QAction* connectfriendAct;
  QAction* profileviewAct;
  QAction* configurefriendAct;
  QAction* exportfriendAct;
  QAction* removefriendAct;

  QTreeWidget *peertreeWidget;

  IMHistoryKeeper historyKeeper;
  
  QColor _currentColor;
  bool _underline;

  QHash<QString, QString> smileys;

  std::map<std::string, PopupChatDialog *> chatDialogs;

  QFont mCurrentFont; /* how the text will come out */

  /** Qt Designer generated object */
  Ui::PeersDialog ui;
};

#endif

