/****************************************************************
 *  RetroShare is distributed under the following license:
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

#ifndef _CHATDIALOG_H
#define _CHATDIALOG_H

#include "mainpage.h"
#include "ui_ChatDialog.h"

#include "chat/PopupChatDialog.h"

class QAction;
class QTextEdit;
class QTextCharFormat;

class ChatDialog : public MainPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  ChatDialog(QWidget *parent = 0);
  /** Default Destructor */

  void insertChat();
  PopupChatDialog *getPrivateChat(std::string id, std::string name, bool show);
  void clearOldChats();
  
  void loadEmoticonsgroupchat();

        

public slots:

  void setChatInfo(QString info, QColor color=QApplication::palette().color(QPalette::WindowText));
  
  void smileyWidgetgroupchat();
  void addSmileys();

private slots:

void toggleSendItem( QTreeWidgetItem *item, int col );

  /** Create the context popup menu and it's submenus */
  void msgSendListCostumPopupMenu( QPoint point );

  void setColor();      
  void insertSendList();
  void checkChat();
  void sendMsg();
  
  void privchat();
  
  void setFont();
  void getFont();
  
  
  void on_actionClearChat_triggered();
  void displayInfoChatMenu(const QPoint& pos);
  

private:
  
   QAction     *actionTextBold;
   QAction     *actionTextUnderline;
   QAction     *actionTextItalic;
   
     /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
  /** Defines the actions for the context menu */
  QAction* privchatAct;

  QTreeView *msgSendList;
  	
  QColor textColor;
  QColor _currentColor;

  QHash<QString, QString> smileys;

  std::map<std::string, PopupChatDialog *> chatDialogs;



  /** Qt Designer generated object */
  Ui::ChatDialog ui;
};

#endif

