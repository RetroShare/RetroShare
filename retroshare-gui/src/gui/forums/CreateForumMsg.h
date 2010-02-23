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


#ifndef _CREATE_FORUM_MSG_DIALOG_H
#define _CREATE_FORUM_MSG_DIALOG_H

#include <QtGui>
#include <QWidget>
#include <string>

#include "gui/feeds/AttachFileItem.h"

#include "ui_CreateForumMsg.h"

class CreateForumMsg : public QMainWindow
{
  Q_OBJECT

public:
  CreateForumMsg(std::string fId, std::string pId);

  void  newMsg(); /* cleanup */
  
  void loadEmoticonsForums();
  
private slots:
  /** Create the context popup menu and it's submenus */
  void forumMessageCostumPopupMenu( QPoint point );

  void fileHashingFinished(AttachFileItem* file);
	/* actions to take.... */
  void  createMsg();
  void  cancelMsg();
  void pasteLink();


  void smileyWidgetForums();
	void addSmileys();
	void addFile();
	void addAttachment(std::string);
	
  
private:
   /** Define the popup menus for the Context menu */
   QMenu* contextMnu;
    
   /** Defines the actions for the context menu */
   QAction* pasteLinkAct;

  std::string mForumId;
  std::string mParentId;
  
  QHash<QString, QString> smileys;

  /** Qt Designer generated object */
  Ui::CreateForumMsg ui;
};

#endif

