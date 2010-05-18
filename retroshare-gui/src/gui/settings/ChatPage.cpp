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

#include "rsiface/rspeers.h" //for rsPeers variable
#include "rsiface/rsiface.h"

#include <QtGui>

#include <rshare.h>
#include "ChatPage.h"

#include "RSettings.h"

/** Constructor */
ChatPage::ChatPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void
ChatPage::closeEvent (QCloseEvent * event)
{
    QWidget::closeEvent(event);
}

/** Saves the changes on this page */
bool
ChatPage::save(QString &errmsg)
{
  RSettings settings(QString("Chat"));

  settings.setValue(QString::fromUtf8("Emoteicons_PrivatChat"), emotePrivatChat());
  settings.setValue(QString::fromUtf8("Emoteicons_GroupChat"), emoteGroupChat());
  settings.setValue(QString::fromUtf8("GroupChat_History"), groupchatHistory());
  settings.setValue(QString::fromUtf8("ChatScreenFont"), fontTempChat.toString());

  return true;
}

/** Loads the settings for this page */
void
ChatPage::load()
{
  RSettings settings(QString("Chat"));

  ui.checkBox_emoteprivchat->setChecked(settings.value(QString::fromUtf8("Emoteicons_PrivatChat"), true).toBool());
  ui.checkBox_emotegroupchat->setChecked(settings.value(QString::fromUtf8("Emoteicons_GroupChat"), true).toBool());
  ui.checkBox_groupchathistory->setChecked(settings.value(QString::fromUtf8("GroupChat_History"), true).toBool());

  fontTempChat.fromString(settings.value(QString::fromUtf8("ChatScreenFont")).toString());
  
  ui.labelChatFontPreview->setText(fontTempChat.rawName());
  ui.labelChatFontPreview->setFont(fontTempChat);
}

bool ChatPage::emotePrivatChat() const {
  if(ui.checkBox_emoteprivchat->isChecked()) return true;
  return ui.checkBox_emoteprivchat->isChecked();
}

bool ChatPage::emoteGroupChat() const {
  if(ui.checkBox_emotegroupchat->isChecked()) return true;
  return ui.checkBox_emotegroupchat->isChecked();
}

bool ChatPage::groupchatHistory() const {
  if(ui.checkBox_groupchathistory->isChecked()) return true;
  return ui.checkBox_groupchathistory->isChecked();
}

void ChatPage::on_pushButtonChangeChatFont_clicked()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok, fontTempChat, this);
	if (ok) {
		fontTempChat = font;
		ui.labelChatFontPreview->setText(fontTempChat.rawName());
		ui.labelChatFontPreview->setFont(fontTempChat);
	}
}
