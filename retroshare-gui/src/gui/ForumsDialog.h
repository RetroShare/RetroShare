/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#ifndef _FORUMSDIALOG_H
#define _FORUMSDIALOG_H

#include "mainpage.h"
#include "ui_ForumsDialog.h"

class ForumsDialog : public MainPage 
{
  Q_OBJECT

public:
  ForumsDialog(QWidget *parent = 0);

 void insertForums();
 void insertPost();

private slots:

 void insertThreads();
  /** Create the context popup menu and it's submenus */
  void forumListCustomPopupMenu( QPoint point ); 
  void threadListCustomPopupMenu( QPoint point );  

  void newforum();

void checkUpdate();

void changedForum( QTreeWidgetItem *curr, QTreeWidgetItem *prev );
void changedThread( QTreeWidgetItem *curr, QTreeWidgetItem *prev );
void changedThread2();

void changeBox( int newrow );
void updateMessages ( QTreeWidgetItem * item, int column );

  void newmessage();

  void replytomessage();
  //void print();
  //void printpreview();
  
  void removemessage();
  void markMsgAsRead();  
  
  /* handle splitter */
  void togglefileview();

  void showthread();
  void createmessage();

  void subscribeToForum();
  void unsubscribeToForum();

  void showForumDetails();

private:

  void forumSubscribe(bool subscribe);
  bool getCurrentMsg(std::string &cid, std::string &mid);

  std::string mCurrForumId;
  std::string mCurrThreadId;
  std::string mCurrPostId;
  
  QFont mForumNameFont;

  /** Qt Designer generated object */
  Ui::ForumsDialog ui;
};

#endif

