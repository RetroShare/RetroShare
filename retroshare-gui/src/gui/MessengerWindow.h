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

#include "ui_MessengerWindow.h"

#include <gui/common/rwindow.h>

class LogoBar;
class PeersDialog;
class PopupChatDialog;

class MessengerWindow : public RWindow
{
    Q_OBJECT

 public:
    QPixmap picture;

    static void showYourself ();
    static MessengerWindow* getInstance();
    static void releaseInstance();

public slots:
    void updateMessengerDisplay() ;
    void updateAvatar();
    void loadmystatusmessage();

    LogoBar & getLogoBar() const;

protected:
    /** Default Constructor */
    MessengerWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
    /** Default Destructor */
    ~MessengerWindow();

    void closeEvent (QCloseEvent * event);

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
    void chatfriend(QTreeWidgetItem *pPeer);
    void chatfriendproxy();
    /** start Messages Composer **/
    void sendMessage();
    /** start to connect to a friend **/
    void connectfriend();
    /** show peers details for each friend **/
    void configurefriend();

    void recommendfriend();
    void pastePerson();

    /** Open Shared Manager **/
    void openShareManager();

    /** get own last stored Avatar**/
    void getAvatar();

    void changeAvatarClicked();

    void savestatusmessage();

    void on_actionSort_Peers_Descending_Order_activated();
    void on_actionSort_Peers_Ascending_Order_activated();
    void on_actionRoot_is_decorated_activated();

    void displayMenu();

    void filterRegExpChanged();
    void clearFilter();

signals:
    void friendsUpdated() ;
  
private:
    static MessengerWindow *_instance;

    void processSettings(bool bLoad);

    /* Worker Functions */
    /* (1) Update Display */
    QTimer *timer;

    /* (2) Utility Fns */
    QTreeWidgetItem *getCurrentPeer();
    void  insertPeers();

    void FilterItems();
    bool FilterItem(QTreeWidgetItem *pItem, QString &sPattern);

    QTreeView *messengertreeWidget;

    LogoBar * _rsLogoBarmessenger;

    QFont itemFont;

    /** Qt Designer generated object */
    Ui::MessengerWindow ui;
};

#endif

