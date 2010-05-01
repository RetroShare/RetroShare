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
  
  void loadForumEmoticons();
  
  
private slots:

  void anchorClicked (const QUrl &);
  void insertThreads();
  /** Create the context popup menu and it's submenus */
  void forumListCustomPopupMenu( QPoint point ); 
  void threadListCustomPopupMenu( QPoint point );  

  void newforum();

  void checkUpdate();

  void changedForum( QTreeWidgetItem *curr, QTreeWidgetItem *prev );
  void changedThread();


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

  void previousMessage ();
  void nextMessage ();

private:

  void forumSubscribe(bool subscribe);
  bool getCurrentMsg(std::string &cid, std::string &mid);
  void FillForums(QTreeWidgetItem *Forum, QList<QTreeWidgetItem *> &ChildList);
  void FillThreads(QList<QTreeWidgetItem *> &ThreadList);
  void FillChildren(QTreeWidgetItem *Parent, QTreeWidgetItem *NewParent);

  QTreeWidgetItem *YourForums;
  QTreeWidgetItem *SubscribedForums;
  QTreeWidgetItem *PopularForums;
  QTreeWidgetItem *OtherForums;

  std::string mCurrForumId;
  std::string mCurrThreadId;
  std::string mCurrPostId;
  
  QFont m_ForumNameFont;
  QFont m_ItemFont;
  int m_LastViewType;
  std::string m_LastForumID;
  
  QHash<QString, QString> smileys;
  
  std::string fId;
  std::string pId;

  /** Qt Designer generated object */
  Ui::ForumsDialog ui;
};

#endif

