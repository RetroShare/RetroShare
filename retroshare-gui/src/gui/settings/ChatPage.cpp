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

#include <QColorDialog>
#include <QFontDialog>
#include <QMenu>
#include <QMessageBox>
#include <time.h>

#include <retroshare/rsnotify.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include "ChatPage.h"
#include <gui/RetroShareLink.h>
#include "gui/chat/ChatStyle.h"
#include "gui/chat/ChatDialog.h"
#include "gui/notifyqt.h"
#include "rsharesettings.h"

#include <retroshare/rshistory.h>
#include <retroshare/rsmsgs.h>

#define VARIANT_STANDARD    "Standard"
#define IMAGE_CHAT_CREATE   ":/images/add_24x24.png"
#define IMAGE_CHAT_OPEN     ":/images/typing.png"
#define IMAGE_CHAT_DELETE   ":/images/deletemail24.png"
#define IMAGE_CHAT_COPY     ":/images/copyrslink.png"

static QString loadStyleInfo(ChatStyle::enumStyleType type, QListWidget *listWidget, QComboBox *comboBox, QString &styleVariant)
{
    QList<ChatStyleInfo> styles;
    QList<ChatStyleInfo>::iterator style;
    QListWidgetItem *item;
    QListWidgetItem *activeItem = NULL;

    QString stylePath;

    switch (type) {
    case ChatStyle::TYPE_PUBLIC:
        Settings->getPublicChatStyle(stylePath, styleVariant);
        break;
    case ChatStyle::TYPE_PRIVATE:
        Settings->getPrivateChatStyle(stylePath, styleVariant);
        break;
    case ChatStyle::TYPE_HISTORY:
        Settings->getHistoryChatStyle(stylePath, styleVariant);
        break;
    case ChatStyle::TYPE_UNKNOWN:
        return "";
    }

    ChatStyle::getAvailableStyles(type, styles);
    for (style = styles.begin(); style != styles.end(); ++style) {
        item = new QListWidgetItem(style->styleName);
        item->setData(Qt::UserRole, qVariantFromValue(*style));
        listWidget->addItem(item);

        if (style->stylePath == stylePath) {
            activeItem = item;
        }
    }

    listWidget->setCurrentItem(activeItem);

    /* now the combobox should be filled */

    int index = comboBox->findText(styleVariant);
    if (index != -1) {
        comboBox->setCurrentIndex(index);
    } else {
        if (comboBox->count()) {
            comboBox->setCurrentIndex(0);
        }
    }
    return stylePath;
}

/** Constructor */
ChatPage::ChatPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

#if QT_VERSION < 0x040600
    ui.minimumContrastLabel->hide();
    ui.minimumContrast->hide();
#endif
}

/** Saves the changes on this page */
bool
ChatPage::save(QString &/*errmsg*/)
{
    Settings->beginGroup(QString("Chat"));
    Settings->setValue("Emoteicons_PrivatChat", ui.checkBox_emoteprivchat->isChecked());
    Settings->setValue("Emoteicons_GroupChat", ui.checkBox_emotegroupchat->isChecked());
    Settings->setValue("EnableCustomFonts", ui.checkBox_enableCustomFonts->isChecked());
    Settings->setValue("EnableCustomFontSize", ui.checkBox_enableCustomFontSize->isChecked());
    Settings->setValue("EnableBold", ui.checkBox_enableBold->isChecked());
    Settings->setValue("EnableItalics", ui.checkBox_enableItalics->isChecked());
    Settings->setValue("MinimumContrast", ui.minimumContrast->value());
    Settings->endGroup();

    Settings->setChatScreenFont(fontTempChat.toString());
    NotifyQt::getInstance()->notifyChatFontChanged();

    Settings->setChatSendMessageWithCtrlReturn(ui.sendMessageWithCtrlReturn->isChecked());

		Settings->setChatSearchShowBarByDefault(ui.cbSearch_ShowBar->isChecked());
    Settings->setChatSearchCharToStartSearch(ui.sbSearch_CharToStart->value());
    Settings->setChatSearchCaseSensitively(ui.cbSearch_CaseSensitively->isChecked());
    Settings->setChatSearchWholeWords(ui.cbSearch_WholeWords->isChecked());
    Settings->setChatSearchMoveToCursor(ui.cbSearch_MoveToCursor->isChecked());
    Settings->setChatSearchSearchWithoutLimit(ui.cbSearch_WithoutLimit->isChecked());
    Settings->setChatSearchMaxSearchLimitColor(ui.sbSearch_MaxLimitColor->value());
    Settings->setChatSearchFoundColor(rgbChatSearchFoundColor);

    Settings->setPublicChatHistoryCount(ui.publicChatLoadCount->value());
    Settings->setPrivateChatHistoryCount(ui.privateChatLoadCount->value());
    Settings->setLobbyChatHistoryCount(ui.lobbyChatLoadCount->value());

    rsHistory->setEnable(RS_HISTORY_TYPE_PUBLIC , ui.publicChatEnable->isChecked());
    rsHistory->setEnable(RS_HISTORY_TYPE_PRIVATE, ui.privateChatEnable->isChecked());
    rsHistory->setEnable(RS_HISTORY_TYPE_LOBBY  , ui.lobbyChatEnable->isChecked());

    rsHistory->setSaveCount(RS_HISTORY_TYPE_PUBLIC , ui.publicChatSaveCount->value());
    rsHistory->setSaveCount(RS_HISTORY_TYPE_PRIVATE, ui.privateChatSaveCount->value());
    rsHistory->setSaveCount(RS_HISTORY_TYPE_LOBBY  , ui.lobbyChatSaveCount->value());

    RsGxsId chosen_id ;
    switch(ui.chatLobbyIdentity_IC->getChosenId(chosen_id))
    {
        case GxsIdChooser::KnowId:
        case GxsIdChooser::UnKnowId:
        rsMsgs->setDefaultIdentityForChatLobby(chosen_id) ;
        break ;

        default:;
    }

    ChatStyleInfo info;
    QListWidgetItem *item = ui.publicList->currentItem();
    if (item) {
        info = item->data(Qt::UserRole).value<ChatStyleInfo>();
        if (publicStylePath != info.stylePath || publicStyleVariant != ui.publicComboBoxVariant->currentText()) {
            Settings->setPublicChatStyle(info.stylePath, ui.publicComboBoxVariant->currentText());
            NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_PUBLIC);
        }
    }

    item = ui.privateList->currentItem();
    if (item) {
        info = item->data(Qt::UserRole).value<ChatStyleInfo>();
        if (privateStylePath != info.stylePath || privateStyleVariant != ui.privateComboBoxVariant->currentText()) {
            Settings->setPrivateChatStyle(info.stylePath, ui.privateComboBoxVariant->currentText());
            NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_PRIVATE);
        }
    }

    item = ui.historyList->currentItem();
    if (item) {
        info = item->data(Qt::UserRole).value<ChatStyleInfo>();
        if (historyStylePath != info.stylePath || historyStyleVariant != ui.historyComboBoxVariant->currentText()) {
            Settings->setHistoryChatStyle(info.stylePath, ui.historyComboBoxVariant->currentText());
            NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_HISTORY);
        }
    }

	 rsHistory->setMaxStorageDuration(ui.max_storage_period->value() * 86400) ;

    uint chatflags   = 0;

    if (ui.chat_NewWindow->isChecked())
        chatflags |= RS_CHAT_OPEN;
    if (ui.chat_Focus->isChecked())
        chatflags |= RS_CHAT_FOCUS;
    if (ui.chat_tabbedWindow->isChecked())
        chatflags |= RS_CHAT_TABBED_WINDOW;
    if (ui.chat_Blink->isChecked())
        chatflags |= RS_CHAT_BLINK;

    Settings->setChatFlags(chatflags);

    uint chatLobbyFlags = 0;

    if (ui.chatLobby_Blink->isChecked())
        chatLobbyFlags |= RS_CHATLOBBY_BLINK;

    Settings->setChatLobbyFlags(chatLobbyFlags);

    return true;
}

/** Loads the settings for this page */
void
ChatPage::load()
{
    Settings->beginGroup(QString("Chat"));
    ui.checkBox_emoteprivchat->setChecked(Settings->value("Emoteicons_PrivatChat", true).toBool());
    ui.checkBox_emotegroupchat->setChecked(Settings->value("Emoteicons_GroupChat", true).toBool());
    ui.checkBox_enableCustomFonts->setChecked(Settings->value("EnableCustomFonts", true).toBool());
    ui.checkBox_enableCustomFontSize->setChecked(Settings->value("EnableCustomFontSize", true).toBool());
    ui.checkBox_enableBold->setChecked(Settings->value("EnableBold", true).toBool());
    ui.checkBox_enableItalics->setChecked(Settings->value("EnableItalics", true).toBool());
    ui.minimumContrast->setValue(Settings->value("MinimumContrast", 4.5).toDouble());
    Settings->endGroup();

    fontTempChat.fromString(Settings->getChatScreenFont());

    ui.sendMessageWithCtrlReturn->setChecked(Settings->getChatSendMessageWithCtrlReturn());

		ui.cbSearch_ShowBar->setChecked(Settings->getChatSearchShowBarByDefault());
    ui.sbSearch_CharToStart->setValue(Settings->getChatSearchCharToStartSearch());
    ui.cbSearch_CaseSensitively->setChecked(Settings->getChatSearchCaseSensitively());
    ui.cbSearch_WholeWords->setChecked(Settings->getChatSearchWholeWords());
    ui.cbSearch_MoveToCursor->setChecked(Settings->getChatSearchMoveToCursor());
    ui.cbSearch_WithoutLimit->setChecked(Settings->getChatSearchSearchWithoutLimit());
    ui.sbSearch_MaxLimitColor->setValue(Settings->getChatSearchMaxSearchLimitColor());
    rgbChatSearchFoundColor=Settings->getChatSearchFoundColor();
    QPixmap pix(24, 24);
    pix.fill(rgbChatSearchFoundColor);
    ui.btSearch_FoundColor->setIcon(pix);

    ui.publicChatLoadCount->setValue(Settings->getPublicChatHistoryCount());
    ui.privateChatLoadCount->setValue(Settings->getPrivateChatHistoryCount());
    ui.lobbyChatLoadCount->setValue(Settings->getLobbyChatHistoryCount());

    ui.publicChatEnable->setChecked(rsHistory->getEnable(RS_HISTORY_TYPE_PUBLIC));
    ui.privateChatEnable->setChecked(rsHistory->getEnable(RS_HISTORY_TYPE_PRIVATE));
    ui.lobbyChatEnable->setChecked(rsHistory->getEnable(RS_HISTORY_TYPE_LOBBY));

    ui.publicChatSaveCount->setValue(rsHistory->getSaveCount(RS_HISTORY_TYPE_PUBLIC));
    ui.privateChatSaveCount->setValue(rsHistory->getSaveCount(RS_HISTORY_TYPE_PRIVATE));
    ui.lobbyChatSaveCount->setValue(rsHistory->getSaveCount(RS_HISTORY_TYPE_LOBBY));
    
    // using fontTempChat.rawname() does not always work!
    // see http://doc.qt.digia.com/qt-maemo/qfont.html#rawName
    QStringList fontname = fontTempChat.toString().split(",");
    ui.labelChatFontPreview->setText(fontname[0]);
    ui.labelChatFontPreview->setFont(fontTempChat);

	 ui.max_storage_period->setValue(rsHistory->getMaxStorageDuration()/86400) ;

    /* Load styles */
    publicStylePath = loadStyleInfo(ChatStyle::TYPE_PUBLIC, ui.publicList, ui.publicComboBoxVariant, publicStyleVariant);
    privateStylePath = loadStyleInfo(ChatStyle::TYPE_PRIVATE, ui.privateList, ui.privateComboBoxVariant, privateStyleVariant);
    historyStylePath = loadStyleInfo(ChatStyle::TYPE_HISTORY, ui.historyList, ui.historyComboBoxVariant, historyStyleVariant);

    RsGxsId gxs_id ;
    rsMsgs->getDefaultIdentityForChatLobby(gxs_id) ;

    ui.chatLobbyIdentity_IC->setFlags(IDCHOOSER_ID_REQUIRED) ;

    if(!gxs_id.isNull())
        ui.chatLobbyIdentity_IC->setChosenId(gxs_id);

    uint chatflags = Settings->getChatFlags();

    ui.chat_NewWindow->setChecked(chatflags & RS_CHAT_OPEN);
    ui.chat_Focus->setChecked(chatflags & RS_CHAT_FOCUS);
    ui.chat_tabbedWindow->setChecked(chatflags & RS_CHAT_TABBED_WINDOW);
    ui.chat_Blink->setChecked(chatflags & RS_CHAT_BLINK);

    uint chatLobbyFlags = Settings->getChatLobbyFlags();

    ui.chatLobby_Blink->setChecked(chatLobbyFlags & RS_CHATLOBBY_BLINK);

	 // load personal invites
	 //
#ifdef TO_BE_DONE
	 for()
	 {
		 QListWidgetItem *item = new QListWidgetItem;
		 item->setData(Qt::DisplayRole,tr("Private chat invite from")+" "+QString::fromUtf8(detail.name.c_str())) ;

		 QString tt ;
		 tt +=        tr("Name :")+" " + QString::fromUtf8(detail.name.c_str()) ;
		 tt += "\n" + tr("PGP id :")+" " + QString::fromStdString(invites[i].destination_pgp_id.toStdString()) ;
		 tt += "\n" + tr("Valid until :")+" " + QDateTime::fromTime_t(invites[i].time_of_validity).toString() ;

		 item->setData(Qt::UserRole,QString::fromStdString(invites[i].pid.toStdString())) ;
		 item->setToolTip(tt) ;

		 ui._collected_contacts_LW->insertItem(0,item) ;
	 }
#endif
}

void ChatPage::on_pushButtonChangeChatFont_clicked()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok, fontTempChat, this);
	if (ok) {
		fontTempChat = font;
		// using fontTempChat.rawname() does not always work!
		// see http://doc.qt.digia.com/qt-maemo/qfont.html#rawName
		QStringList fontname = fontTempChat.toString().split(",");
		ui.labelChatFontPreview->setText(fontname[0]);
		ui.labelChatFontPreview->setFont(fontTempChat);
	}
}

void ChatPage::setPreviewMessages(QString &stylePath, QString styleVariant, QTextBrowser *textBrowser)
{
    ChatStyle style;
    style.setStylePath(stylePath, styleVariant);

    textBrowser->clear();

    QString nameIncoming = tr("Incoming");
    QString nameOutgoing = tr("Outgoing");
    QDateTime timestmp = QDateTime::fromTime_t(time(NULL));

    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_HINCOMING, nameIncoming, timestmp, tr("Incoming message in history")));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_HOUTGOING, nameOutgoing, timestmp, tr("Outgoing message in history")));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_INCOMING,  nameIncoming, timestmp, tr("Incoming message")));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_OUTGOING,  nameOutgoing, timestmp, tr("Outgoing message")));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_OOUTGOING,  nameOutgoing, timestmp, tr("Outgoing offline message")));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_SYSTEM,  tr("System"), timestmp, tr("System message")));
}

void ChatPage::fillPreview(QListWidget *listWidget, QComboBox *comboBox, QTextBrowser *textBrowser)
{
    QListWidgetItem *item = listWidget->currentItem();
    if (item) {
        ChatStyleInfo info = item->data(Qt::UserRole).value<ChatStyleInfo>();

        setPreviewMessages(info.stylePath, comboBox->currentText(), textBrowser);
    } else {
        textBrowser->clear();
    }
}

void ChatPage::on_publicList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        QListWidgetItem *item = ui.publicList->item(currentRow);
        ChatStyleInfo info = item->data(Qt::UserRole).value<ChatStyleInfo>();

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        ui.publicAuthor->setText(author);
        ui.publicDescription->setText(info.styleDescription);

        QStringList variants;
        ChatStyle::getAvailableVariants(info.stylePath, variants);
        ui.publicComboBoxVariant->clear();
        ui.publicComboBoxVariant->setEnabled(variants.size() != 0);
        ui.publicComboBoxVariant->addItems(variants);

        /* try to find "Standard" */
        int index = ui.publicComboBoxVariant->findText(VARIANT_STANDARD);
        if (index != -1) {
            ui.publicComboBoxVariant->setCurrentIndex(index);
        } else {
            ui.publicComboBoxVariant->setCurrentIndex(0);
        }
    } else {
        ui.publicAuthor->clear();
        ui.publicDescription->clear();
        ui.publicComboBoxVariant->clear();
        ui.publicComboBoxVariant->setDisabled(true);
    }

    fillPreview(ui.publicList, ui.publicComboBoxVariant, ui.publicPreview);
}

void ChatPage::on_privateList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        QListWidgetItem *item = ui.privateList->item(currentRow);
        ChatStyleInfo info = item->data(Qt::UserRole).value<ChatStyleInfo>();

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        ui.privateAuthor->setText(author);
        ui.privateDescription->setText(info.styleDescription);

        QStringList variants;
        ChatStyle::getAvailableVariants(info.stylePath, variants);
        ui.privateComboBoxVariant->clear();
        ui.privateComboBoxVariant->setEnabled(variants.size() != 0);
        ui.privateComboBoxVariant->addItems(variants);

        /* try to find "Standard" */
        int index = ui.privateComboBoxVariant->findText(VARIANT_STANDARD);
        if (index != -1) {
            ui.privateComboBoxVariant->setCurrentIndex(index);
        } else {
            ui.privateComboBoxVariant->setCurrentIndex(0);
        }
    } else {
        ui.privateAuthor->clear();
        ui.privateDescription->clear();
        ui.privateComboBoxVariant->clear();
        ui.privateComboBoxVariant->setDisabled(true);
    }

    fillPreview(ui.privateList, ui.privateComboBoxVariant, ui.privatePreview);
}

void ChatPage::on_historyList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        QListWidgetItem *item = ui.historyList->item(currentRow);
        ChatStyleInfo info = item->data(Qt::UserRole).value<ChatStyleInfo>();

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        ui.historyAuthor->setText(author);
        ui.historyDescription->setText(info.styleDescription);

        QStringList variants;
        ChatStyle::getAvailableVariants(info.stylePath, variants);
        ui.historyComboBoxVariant->clear();
        ui.historyComboBoxVariant->setEnabled(variants.size() != 0);
        ui.historyComboBoxVariant->addItems(variants);

        /* try to find "Standard" */
        int index = ui.historyComboBoxVariant->findText(VARIANT_STANDARD);
        if (index != -1) {
            ui.historyComboBoxVariant->setCurrentIndex(index);
        } else {
            ui.historyComboBoxVariant->setCurrentIndex(0);
        }
    } else {
        ui.historyAuthor->clear();
        ui.historyDescription->clear();
        ui.historyComboBoxVariant->clear();
        ui.historyComboBoxVariant->setDisabled(true);
    }

    fillPreview(ui.historyList, ui.historyComboBoxVariant, ui.historyPreview);
}

void ChatPage::on_publicComboBoxVariant_currentIndexChanged(int /*index*/)
{
    fillPreview(ui.publicList, ui.publicComboBoxVariant, ui.publicPreview);
}

void ChatPage::on_privateComboBoxVariant_currentIndexChanged(int /*index*/)
{
    fillPreview(ui.privateList, ui.privateComboBoxVariant, ui.privatePreview);
}

void ChatPage::on_historyComboBoxVariant_currentIndexChanged(int /*index*/)
{
    fillPreview(ui.historyList, ui.historyComboBoxVariant, ui.historyPreview);
}

void ChatPage::on_cbSearch_WithoutLimit_toggled(bool checked)
{
	ui.sbSearch_MaxLimitColor->setEnabled(!checked);
	ui.lSearch_MaxLimitColor->setEnabled(!checked);
}

void ChatPage::on_btSearch_FoundColor_clicked()
{
	bool ok;
	QRgb color = QColorDialog::getRgba(rgbChatSearchFoundColor, &ok, window());
	if (ok) {
		rgbChatSearchFoundColor=color;
		QPixmap pix(24, 24);
		pix.fill(color);
		ui.btSearch_FoundColor->setIcon(pix);
	}
}
