/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007, RetroShare Team
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

#ifndef _MESSENGERWINDOW_H
#define _MESSENGERWINDOW_H
#include <QFileDialog>

#include "mainpage.h"
#include "ui_MessengerWindow.h"
#include "NetworkDialog.h"
#include <config/rsharesettings.h>

class LogoBar;
class ChatDialog;

class MessengerWindow : public QWidget
{
  Q_OBJECT

public:
  /** Default Constructor */
  MessengerWindow(QWidget *parent = 0);
  /** Default Destructor */

  void  insertPeers();
  void setChatDialog(ChatDialog *cd);
  
  NetworkDialog *networkDialog2;



public slots:
  /** Called when this dialog is to be displayed */
  void show();
  
  LogoBar & getLogoBar() const;

protected:
  void closeEvent (QCloseEvent * event);


private slots:

  /** Create the context popup menu and it's submenus */
  void messengertreeWidgetCostumPopupMenu( QPoint point );
  
  /** Export friend in Friends Dialog */
  void exportfriend2();
  /** Remove friend  */
  void removefriend2();
  /** start a chat with a friend **/
  void chatfriend2();
  
  void sendMessage();

  void configurefriend2();
  
  void addFriend2();

  /** RsServer Friend Calls */
  void allowfriend2();
  void connectfriend2();
  void setaddressfriend2();
  void trustfriend2();
  
  void changeAvatarClicked();
  void updateAvatar();

  
private:

  /* Worker Functions */
  /* (1) Update Display */

  /* (2) Utility Fns */
  QTreeWidgetItem *getCurrentPeer(bool &isOnline);
  
  ChatDialog *chatDialog;

  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
   /** Defines the actions for the context menu */
  QAction* chatAct;
  QAction* sendMessageAct;
  QAction* connectfriendAct;
  QAction* configurefriendAct;
  QAction* exportfriendAct;
  QAction* removefriend2Act;

  QTreeView *messengertreeWidget;

  LogoBar * _rsLogoBarmessenger;

  /** Qt Designer generated object */
  Ui::MessengerWindow ui;
};

#endif

