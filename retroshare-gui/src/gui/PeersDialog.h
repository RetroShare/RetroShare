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

//#include <config/rsharesettings.h>

#include "mainpage.h"
#include "ui_PeersDialog.h"


class ChatDialog;

class PeersDialog : public MainPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  PeersDialog(QWidget *parent = 0);
  /** Default Destructor */

  void  insertPeers();

  void setChatDialog(ChatDialog *cd);




private slots:

  /** Create the context popup menu and it's submenus */
  void peertreeWidgetCostumPopupMenu( QPoint point );
  
  /** Export friend in Friends Dialog */
  void exportfriend();
  /** Remove friend  */
  void removefriend();
  /** start a chat with a friend **/
  void chatfriend();
  void msgfriend();

  void configurefriend();

  /** RsServer Friend Calls */
  void allowfriend();
  void connectfriend();
  void setaddressfriend();
  void trustfriend();

  
private:

  /* Worker Functions */
  /* (1) Update Display */

  /* (2) Utility Fns */
  QTreeWidgetItem *getCurrentPeer();

  ChatDialog *chatDialog;


  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
    /** Defines the actions for the context menu */
  QAction* chatAct;
  QAction* msgAct;
  QAction* connectfriendAct;
  QAction* configurefriendAct;
  QAction* exportfriendAct;
  QAction* removefriendAct;

  QTreeWidget *peertreeWidget;

  /** Qt Designer generated object */
  Ui::PeersDialog ui;
};

#endif

