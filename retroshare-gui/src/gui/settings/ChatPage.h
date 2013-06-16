/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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

#ifndef _CHATPAGE_H
#define _CHATPAGE_H

#include <retroshare-gui/configpage.h>
#include "ui_ChatPage.h"

class ChatPage : public ConfigPage
{
  Q_OBJECT

  public:
      /** Default Constructor */
      ChatPage(QWidget * parent = 0, Qt::WFlags flags = 0);
      /** Default Destructor */
      ~ChatPage() {}

      /** Saves the changes on this page */
      virtual bool save(QString &errmsg);
      /** Loads the settings for this page */
      virtual void load();

		virtual QPixmap iconPixmap() const { return QPixmap(":/images/chat_24.png") ; }
		virtual QString pageName() const { return tr("Chat") ; }

  private slots:
      void on_historyComboBoxVariant_currentIndexChanged(int index);
      void on_privateComboBoxVariant_currentIndexChanged(int index);
      void on_publicComboBoxVariant_currentIndexChanged(int index);
      void on_pushButtonChangeChatFont_clicked();
      void on_publicList_currentRowChanged(int currentRow);
      void on_privateList_currentRowChanged(int currentRow);
      void on_historyList_currentRowChanged(int currentRow);

		void personalInvites_customPopupMenu(QPoint) ;
		void collectedContacts_customPopupMenu(QPoint) ;

		void personalInvites_copyLink() ;
		void personalInvites_delete() ;
		void personalInvites_create() ;

		void collectedInvite_openDistantChat() ;
		void collectedInvite_delete() ;

  private:
      void setPreviewMessages(QString &stylePath, QString styleVariant, QTextBrowser *textBrowser);
      void fillPreview(QListWidget *listWidget, QComboBox *comboBox, QTextBrowser *textBrowser);

      QFont fontTempChat;

      QString publicStylePath;
      QString publicStyleVariant;
      QString privateStylePath;
      QString privateStyleVariant;
      QString historyStylePath;
      QString historyStyleVariant;

      /** Qt Designer generated object */
      Ui::ChatPage ui;
};

#endif

