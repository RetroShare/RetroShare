/*******************************************************************************
 * gui/settings/ChatPage.h                                                     *
 *                                                                             *
 * Copyright 2006, Retroshare Team <retroshare.project@gmail.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _CHATPAGE_H
#define _CHATPAGE_H

#include "retroshare-gui/configpage.h"
#include "gui/chat/ChatStyle.h"
#include "gui/chat/ChatLobbyUserNotify.h"
#include "ui_ChatPage.h"
#include "gui/common/FilesDefs.h"

class ChatPage : public ConfigPage
{
  Q_OBJECT

  public:
      /** Default Constructor */
      ChatPage(QWidget * parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
      /** Default Destructor */
      ~ChatPage() {}

      /** Loads the settings for this page */
      virtual void load();

        virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/settings/chat.svg") ; }
		virtual QString pageName() const { return tr("Chats") ; }
		virtual QString helpText() const { return ""; }

  private slots:
      void updateChatLobbyUserNotify();
      void on_historyComboBoxVariant_currentIndexChanged(int index);
      void on_privateComboBoxVariant_currentIndexChanged(int index);
      void on_publicComboBoxVariant_currentIndexChanged(int index);
      void on_pushButtonChangeChatFont_clicked();
      void on_publicList_currentRowChanged(int currentRow);
      void on_privateList_currentRowChanged(int currentRow);
      void on_historyList_currentRowChanged(int currentRow);

    void on_cbSearch_WithoutLimit_toggled(bool);
    void on_btSearch_FoundColor_clicked();
 
    void distantChatComboBoxChanged(int);
  
    void updateFontsAndEmotes();
    void updateChatParams();
    void updateChatSearchParams();
    void updateDefaultLobbyIdentity() ;
    void updateHistoryParams();
    void updatePublicStyle() ;
    void updatePrivateStyle() ;
    void updateHistoryStyle() ;
    void updateHistoryStorage();
    void updateChatFlags();
    void updateChatLobbyFlags();

  private:
      void setPreviewMessages(QString &stylePath, QString styleVariant, QTextBrowser *textBrowser);
      void fillPreview(QComboBox *listWidget, QComboBox *comboBox, QTextBrowser *textBrowser);
	  QString loadStyleInfo(ChatStyle::enumStyleType type, QComboBox *style_CB, QComboBox *var_CB, QString &styleVariant);

      QFont fontTempChat;

      QString publicStylePath;
      QString publicStyleVariant;
      QString privateStylePath;
      QString privateStyleVariant;
      QString historyStylePath;
      QString historyStyleVariant;

	QRgb rgbChatSearchFoundColor;

      /** Qt Designer generated object */
      Ui::ChatPage ui;

      ChatLobbyUserNotify* mChatLobbyUserNotify;
};

#endif
