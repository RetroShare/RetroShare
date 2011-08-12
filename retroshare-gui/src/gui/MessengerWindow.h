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
class FriendsDialog;
class PopupChatDialog;
class RSTreeWidgetItemCompareRole;

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
#ifndef MINIMAL_RSGUI
    void updateAvatar();
    void loadmystatusmessage();
#endif // MINIMAL_RSGUI

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

#ifndef MINIMAL_RSGUI
    /** Add a new friend */
    void addFriend();
    /** Export friend  */
    void exportfriend();
    /** Remove friend  */
    void removefriend();
#endif // MINIMAL_RSGUI
    /** start to connect to a friend **/
    void connectfriend();
#ifndef MINIMAL_RSGUI
    /** start a chat with a friend **/
    void chatfriend(QTreeWidgetItem *pPeer);
    void chatfriendproxy();
    /** start Messages Composer **/
    void sendMessage();
    /** show peers details for each friend **/
    void configurefriend();

    void recommendfriend();
    void pastePerson();

    /** Open Shared Manager **/
    void openShareManager();

    /** get own last stored Avatar**/
    void getAvatar();

    void updateOwnStatus(const QString &peer_id, int status);

    void savestatusmessage();
#endif // MINIMAL_RSGUI

    void on_actionSort_Peers_Descending_Order_activated();
    void on_actionSort_Peers_Ascending_Order_activated();
    void on_actionRoot_is_decorated_activated();

    void filterRegExpChanged();
    void clearFilter();

signals:
    void friendsUpdated() ;

private:
    static MessengerWindow *_instance;

    void processSettings(bool bLoad);

    void displayMenu();

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
    QString m_nickName;

    RSTreeWidgetItemCompareRole *m_compareRole;

    /** Qt Designer generated object */
    Ui::MessengerWindow ui;
};

#endif