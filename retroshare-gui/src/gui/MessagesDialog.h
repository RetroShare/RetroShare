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

#ifndef _MESSAGESDIALOG_H
#define _MESSAGESDIALOG_H

#include <QFileDialog>

#include "mainpage.h"
#include "ui_MessagesDialog.h"

class MessagesDialog : public MainPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  MessagesDialog(QWidget *parent = 0);
  /** Default Destructor */


 void insertMessages();
 void insertMsgTxtAndFiles();


private slots:

  /** Create the context popup menu and it's submenus */
  void messageslistWidgetCostumPopupMenu( QPoint point ); 
  void msgfilelistWidgetCostumPopupMenu(QPoint);  

void changeBox( int newrow );
void updateMessages ( QTreeWidgetItem * item, int column );

  void newmessage();

  void replytomessage();
  
  void removemessage();
  void markMsgAsRead();  
  
  void getcurrentrecommended();
  void getallrecommended();

  /* handle splitter */
  void togglefileview();

private:

  bool getCurrentMsg(std::string &cid, std::string &mid);

  std::string mCurrCertId;
  std::string mCurrMsgId;

  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
  
   /** Defines the actions for the context menu */
  QAction* newmsgAct;
  QAction* replytomsgAct;
  QAction* removemsgAct;

  QAction* getRecAct;
  QAction* getAllRecAct;
  
  /** Qt Designer generated object */
  Ui::MessagesDialog ui;
};

#endif

