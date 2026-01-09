/****************************************************************
 *  RetroShareGui is distributed under the following license:
 *
 *  Copyright (C) 2007-2008 Robert Fernie
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

#ifndef _GAMESDIALOG_H
#define _GAMESDIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_GamesDialog.h"

class GamesDialog : public MainPage
{
  Q_OBJECT

public:
  /** Default Constructor */
  GamesDialog(QWidget *parent = 0);
  /** Default Destructor */

  void  insertExample();

private slots:
  /** Create the context popup menu and it's submenus */
  void gameListPopupMenu( QPoint point );
  void gamePeersPopupMenu( QPoint point );

  void createGame();
  void deleteGame();
  void inviteGame();
  void playGame();

  void invitePeer();
  void uninvitePeer();
  void confirmPeer();
  void unconfirmPeer();

  void interested();
  void uninterested();

  void updateGameList();
  void updateGameDetails();
  /***
   *
   */

private:


  /***** UTILS *******/
QTreeWidgetItem *getCurrentGame();
QTreeWidgetItem *getCurrentPeer();

 /* Data */
std::string mCurrentGame;
std::string mCurrentGameStatus;
private:

  /** Qt Designer generated object */
  Ui::GamesDialog ui;
};

#endif

