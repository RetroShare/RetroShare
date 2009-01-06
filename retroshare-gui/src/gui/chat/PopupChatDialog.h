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


#ifndef _POPUPCHATDIALOG_H
#define _POPUPCHATDIALOG_H

#include <QtGui>
#include <QDialog>

#include "ui_PopupChatDialog.h"
#include <gui/Preferences/rsharesettings.h>

#include "rsiface/rsiface.h"



class QAction;
class QTextEdit;
class QTextCharFormat;

class ChatInfo;

class PopupChatDialog : public QMainWindow
{
  Q_OBJECT

public:
  /** Default constructor */
  PopupChatDialog(std::string id, std::string name, 
  		QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default destructor */
  ~PopupChatDialog();

  void updateChat();
  void addChatMsg(ChatInfo *ci);
  
  void loadEmoticons();
  void loadEmoticons2();


  QString loadEmptyStyle(); 


   QPixmap picture;

   
public slots:
  /** Overloaded QWidget.show */
  void show(); 

  void getfocus();
  void flash(); 
  	
  void smileyWidget();
  void addSmiley();
  
  void changeStyle();

protected:
  void closeEvent (QCloseEvent * event);
  
private slots:

  void showAvatarFrame(bool show);

  void setColor();    
  void getFont();
  void setFont();
 
  void checkChat();
  void sendChat();

  void getAvatar();

  
  
private:

  void colorChanged(const QColor &c);
  
   QAction     *actionTextBold;
   QAction     *actionTextUnderline;
   QAction     *actionTextItalic;
   
   std::string dialogId, dialogName;
   unsigned int lastChatTime;
   std::string  lastChatName;
   
   QHash<QString, QString> smileys;
   QColor mCurrentColor;
   QFont  mCurrentFont;
   
   QString styleHtm;
   QString emptyStyle;
   QStringList history;
   QString wholeChat;


  
  /** Qt Designer generated object */
  Ui::PopupChatDialog ui;
};

#endif




