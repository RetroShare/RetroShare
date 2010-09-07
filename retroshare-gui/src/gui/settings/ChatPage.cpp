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

#include <QFontDialog>
#include <time.h>

#include "ChatPage.h"
#include "gui/chat/ChatStyle.h"
#include "gui/notifyqt.h"
#include "rsharesettings.h"

static QString loadStyleInfo(ChatStyle::enumStyleType type, QListWidget *listWidget)
{
    QList<ChatStyleInfo> styles;
    QList<ChatStyleInfo>::iterator style;
    QListWidgetItem *item;
    QListWidgetItem *activeItem = NULL;

    QString stylePath;

    switch (type) {
    case ChatStyle::TYPE_PUBLIC:
        Settings->getPublicChatStyle(stylePath);
        break;
    case ChatStyle::TYPE_PRIVATE:
        Settings->getPrivateChatStyle(stylePath);
        break;
    case ChatStyle::TYPE_HISTORY:
        Settings->getHistoryChatStyle(stylePath);
        break;
    case ChatStyle::TYPE_UNKNOWN:
        return "";
    }

    ChatStyle::getAvailableStyles(type, styles);
    for (style = styles.begin(); style != styles.end(); style++) {
        item = new QListWidgetItem(style->styleName);
        item->setData(Qt::UserRole, qVariantFromValue(*style));
        listWidget->addItem(item);

        if (style->stylePath == stylePath) {
            activeItem = item;
        }
    }

    listWidget->setCurrentItem(activeItem);

    return stylePath;
}

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
    Settings->beginGroup(QString("Chat"));

    Settings->setValue(QString::fromUtf8("Emoteicons_PrivatChat"), emotePrivatChat());
    Settings->setValue(QString::fromUtf8("Emoteicons_GroupChat"), emoteGroupChat());
    Settings->setValue(QString::fromUtf8("GroupChat_History"), groupchatHistory());
    Settings->setValue(QString::fromUtf8("ChatScreenFont"), fontTempChat.toString());

    Settings->endGroup();

    Settings->setChatSendMessageWithCtrlReturn(ui.sendMessageWithCtrlReturn->isChecked());

    ChatStyleInfo info;
    QListWidgetItem *item = ui.publicList->currentItem();
    if (item) {
        info = qVariantValue<ChatStyleInfo>(item->data(Qt::UserRole));
        if (publicStylePath != info.stylePath) {
            Settings->setPublicChatStyle(info.stylePath);
            NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_PUBLIC);
        }
    }

    item = ui.privateList->currentItem();
    if (item) {
        info = qVariantValue<ChatStyleInfo>(item->data(Qt::UserRole));
        if (privateStylePath != info.stylePath) {
            Settings->setPrivateChatStyle(info.stylePath);
            NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_PRIVATE);
        }
    }

    item = ui.historyList->currentItem();
    if (item) {
        info = qVariantValue<ChatStyleInfo>(item->data(Qt::UserRole));
        if (historyStylePath != info.stylePath) {
            Settings->setHistoryChatStyle(info.stylePath);
            NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_HISTORY);
        }
    }

    return true;
}

/** Loads the settings for this page */
void
ChatPage::load()
{
    Settings->beginGroup(QString("Chat"));

    ui.checkBox_emoteprivchat->setChecked(Settings->value(QString::fromUtf8("Emoteicons_PrivatChat"), true).toBool());
    ui.checkBox_emotegroupchat->setChecked(Settings->value(QString::fromUtf8("Emoteicons_GroupChat"), true).toBool());
    ui.checkBox_groupchathistory->setChecked(Settings->value(QString::fromUtf8("GroupChat_History"), true).toBool());

    fontTempChat.fromString(Settings->value(QString::fromUtf8("ChatScreenFont")).toString());

    Settings->endGroup();

    ui.sendMessageWithCtrlReturn->setChecked(Settings->getChatSendMessageWithCtrlReturn());

    ui.labelChatFontPreview->setText(fontTempChat.rawName());
    ui.labelChatFontPreview->setFont(fontTempChat);

    /* Load styles */
    publicStylePath = loadStyleInfo(ChatStyle::TYPE_PUBLIC, ui.publicList);
    privateStylePath = loadStyleInfo(ChatStyle::TYPE_PRIVATE, ui.privateList);
    historyStylePath = loadStyleInfo(ChatStyle::TYPE_HISTORY, ui.historyList);
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

void ChatPage::setPreviewMessages(QString &stylePath, QTextBrowser *textBrowser)
{
    ChatStyle style;
    style.setStylePath(stylePath);
    style.loadEmoticons();

    textBrowser->clear();

    QString nameIncoming = "Incoming";
    QString nameOutgoing = "Outgoing";
    QDateTime timestmp = QDateTime::fromTime_t(time(NULL));
    QTextEdit textEdit;
    QString message;

    textEdit.setText(tr("Incoming message in history"));
    message = textEdit.toHtml();
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_HINCOMING, nameIncoming, timestmp, message, CHAT_FORMATTEXT_EMBED_SMILEYS));
    textEdit.setText(tr("Outgoing message in history"));
    message = textEdit.toHtml();
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_HOUTGOING, nameOutgoing, timestmp, message, CHAT_FORMATTEXT_EMBED_SMILEYS));
    textEdit.setText(tr("Incoming message"));
    message = textEdit.toHtml();
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_INCOMING,  nameIncoming, timestmp, message, CHAT_FORMATTEXT_EMBED_SMILEYS));
    textEdit.setText(tr("Outgoing message"));
    message = textEdit.toHtml();
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_OUTGOING,  nameOutgoing, timestmp, message, CHAT_FORMATTEXT_EMBED_SMILEYS));
}

void ChatPage::on_publicList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        QListWidgetItem *item = ui.publicList->item(currentRow);
        ChatStyleInfo info = qVariantValue<ChatStyleInfo>(item->data(Qt::UserRole));
        setPreviewMessages(info.stylePath, ui.publicPreview);

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        ui.publicAuthor->setText(author);
        ui.publicDescription->setText(info.styleDescription);
    } else {
        ui.publicPreview->clear();
        ui.publicAuthor->clear();
        ui.publicDescription->clear();
    }
}

void ChatPage::on_privateList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        QListWidgetItem *item = ui.privateList->item(currentRow);
        ChatStyleInfo info = qVariantValue<ChatStyleInfo>(item->data(Qt::UserRole));
        setPreviewMessages(info.stylePath, ui.privatePreview);

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        ui.privateAuthor->setText(author);
        ui.privateDescription->setText(info.styleDescription);
    } else {
        ui.privatePreview->clear();
        ui.privateAuthor->clear();
        ui.privateDescription->clear();
    }
}

void ChatPage::on_historyList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        QListWidgetItem *item = ui.historyList->item(currentRow);
        ChatStyleInfo info = qVariantValue<ChatStyleInfo>(item->data(Qt::UserRole));
        setPreviewMessages(info.stylePath, ui.historyPreview);

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        ui.historyAuthor->setText(author);
        ui.historyDescription->setText(info.styleDescription);
    } else {
        ui.historyPreview->clear();
        ui.historyAuthor->clear();
        ui.historyDescription->clear();
    }
}
