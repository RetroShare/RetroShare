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
#include <gui/settings/rsharesettings.h>
#include <gui/common/rwindow.h>

class LogoBar;
class PeersDialog;
//class MainWindow;

class MessengerWindow : public RWindow
{
  Q_OBJECT

public:

  void setChatDialog(PeersDialog *cd);

  
  NetworkDialog *networkDialog2;

  QPixmap picture;

  static MessengerWindow* getInstance();
  static void releaseInstance();

public slots:
  void  insertPeers();
  /** Called when this dialog is to be displayed */
  void show();
  
  LogoBar & getLogoBar() const;

protected:
  void closeEvent (QCloseEvent * event);
  /** Default Constructor */
  MessengerWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default Destructor */


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

  void getAvatar();

  void openShareManager();

  void showMessagesPopup();

  //void showMessages(MainWindow::Page page = MainWindow::Messages);

  /** RsServer Friend Calls */
  void allowfriend2();
  void connectfriend2();
  void setaddressfriend2();
  void trustfriend2();
  
  void changeAvatarClicked();
  void updateAvatar();
	
	void loadmystatus();
  
private:

  static MessengerWindow *mv;
  /* Worker Functions */
  /* (1) Update Display */

  /* (2) Utility Fns */
  QTreeWidgetItem *getCurrentPeer(bool &isOnline);
  
  PeersDialog *chatDialog;

  //MainWindow* mainWindow;


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
  
  QFont itemFont;

  /** Qt Designer generated object */
  Ui::MessengerWindow ui;
};

#endif

