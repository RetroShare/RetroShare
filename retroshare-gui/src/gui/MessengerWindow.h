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

#include <gui/settings/rsharesettings.h>
#include <gui/common/rwindow.h>
#include "chat/PopupChatDialog.h"


class LogoBar;
class PeersDialog;

class MessengerWindow : public RWindow
{
  Q_OBJECT

public:

	PopupChatDialog *getPrivateChat(std::string id, std::string name, uint chatflags);

  QPixmap picture;

  static MessengerWindow* getInstance();
  static void releaseInstance();
  
  virtual void updateMessengerDisplay() ;	// overloaded from RsAutoUpdatePage

public slots:
  void  insertPeers();
  /** Called when this dialog is to be displayed */
  void show();
  
  void updatePeersAvatar(const QString& peer_id);
  void updateAvatar();
  void loadmystatusmessage();
  void loadstatus();
  
  LogoBar & getLogoBar() const;

protected:
  void closeEvent (QCloseEvent * event);
  /** Default Constructor */
  MessengerWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default Destructor */


private slots:

  /** Create the context popup menu and it's submenus */
  void messengertreeWidgetCostumPopupMenu( QPoint point );
  
  /** Add a new friend */
  void addFriend(); 
  /** Export friend  */
  void exportfriend();  
  /** Remove friend  */
  void removefriend();  
  /** start a chat with a friend **/
  void chatfriend();  
  /** start Messages Composer **/
  void sendMessage();  
  /** start to connect to a friend **/
  void connectfriend();
  /** show peers details for each friend **/
  void configurefriend();

  /** Open Shared Manager **/
  void openShareManager();
  
  /** get own last stored Avatar**/
  void getAvatar();  
  
  void changeAvatarClicked();
	
	void savestatusmessage();
	
	void savestatus();
	
	void on_actionSort_Peers_Descending_Order_activated();
  void on_actionSort_Peers_Ascending_Order_activated();
  void on_actionRoot_is_decorated_activated();
  void on_actionRoot_isnot_decorated_activated();
  
  void displayMenu();

signals:
		void friendsUpdated() ;
  
private:

  static MessengerWindow *mv;
  /* Worker Functions */
  /* (1) Update Display */

  /* (2) Utility Fns */
  QTreeWidgetItem *getCurrentPeer();
  
  std::map<std::string, PopupChatDialog *> chatDialogs;

		class QLabel *iconLabel, *textLabel;
		class QWidget *widget;
		class QWidgetAction *widgetAction;
		class QSpacerItem *spacerItem; 

  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
   /** Defines the actions for the context menu */
  QAction* chatAct;
  QAction* sendMessageAct;
  QAction* connectfriendAct;
  QAction* configurefriendAct;
  QAction* exportfriendAct;
  QAction* removefriendAct;

  QTreeView *messengertreeWidget;

  LogoBar * _rsLogoBarmessenger;
  
  QFont itemFont;
  
  /** A RshareSettings object used for saving/loading settings */
  RshareSettings* _settings;

  /** Qt Designer generated object */
  Ui::MessengerWindow ui;
};

#endif

