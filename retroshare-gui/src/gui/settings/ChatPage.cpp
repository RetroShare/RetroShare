/*******************************************************************************
 * gui/settings/ChatPage.cpp                                                   *
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

#include <QColorDialog>
#include <QMenu>
#include <QMessageBox>
#include <time.h>

#include <retroshare/rsnotify.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include "ChatPage.h"
#include <gui/RetroShareLink.h>
#include "gui/chat/ChatDialog.h"
#include "gui/notifyqt.h"
#include "rsharesettings.h"
#include "util/misc.h"
#include <retroshare/rsconfig.h>

#include <retroshare/rshistory.h>
#include <retroshare/rsmsgs.h>

#define VARIANT_STANDARD    "Standard"
#define IMAGE_CHAT_CREATE   ":/images/add_24x24.png"
#define IMAGE_CHAT_OPEN     ":/images/typing.png"
#define IMAGE_CHAT_DELETE   ":/images/deletemail24.png"
#define IMAGE_CHAT_COPY     ":/images/copyrslink.png"

QString ChatPage::loadStyleInfo(ChatStyle::enumStyleType type, QComboBox *style_CB, QComboBox *comboBox, QString &styleVariant)
{
    QList<ChatStyleInfo> styles;
    QList<ChatStyleInfo>::iterator style;
    int activeItem = 0;

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

    while(style_CB->count())
		whileBlocking(style_CB)->removeItem(0) ;

    int n=0;
    for (style = styles.begin(); style != styles.end(); ++style,++n)
    {
        whileBlocking(style_CB)->insertItem(n,style->styleName);

        style_CB->setItemData(n, qVariantFromValue(*style),Qt::UserRole);

        if (style->stylePath == stylePath) {
            activeItem = n;
        }
    }

    whileBlocking(style_CB)->setCurrentIndex(activeItem);

    switch(type)
    {
    case ChatStyle::TYPE_PUBLIC: on_publicList_currentRowChanged(activeItem);	break ;
    case ChatStyle::TYPE_PRIVATE: on_privateList_currentRowChanged(activeItem);	break ;
    case ChatStyle::TYPE_HISTORY: on_historyList_currentRowChanged(activeItem);	break ;
    default:
        break ;
    }

    /* now the combobox should be filled */

    int index = comboBox->findText(styleVariant);

    if (index != -1) {
        whileBlocking(comboBox)->setCurrentIndex(index);
    } else {
        if (comboBox->count()) {
            whileBlocking(comboBox)->setCurrentIndex(0);
        }
    }
    return stylePath;
}
void ChatPage::updateFontsAndEmotes()
{
    Settings->beginGroup(QString("Chat"));
    Settings->setValue("Emoteicons_PrivatChat", ui.checkBox_emoteprivchat->isChecked());
    Settings->setValue("Emoteicons_GroupChat", ui.checkBox_emotegroupchat->isChecked());
    Settings->setValue("EnableCustomFonts", ui.checkBox_enableCustomFonts->isChecked());
    Settings->setValue("EnableCustomFontSize", ui.checkBox_enableCustomFontSize->isChecked());
	Settings->setValue("MinimumFontSize", ui.minimumFontSize->value());
    Settings->setValue("EnableBold", ui.checkBox_enableBold->isChecked());
    Settings->setValue("EnableItalics", ui.checkBox_enableItalics->isChecked());
    Settings->setValue("MinimumContrast", ui.minimumContrast->value());
    Settings->endGroup();
}

/** Saves the changes on this page */
void ChatPage::updateChatParams()
{
	// state of distant Chat combobox
	Settings->setValue("DistantChat", ui.distantChatComboBox->currentIndex());

	Settings->setChatScreenFont(fontTempChat.toString());
	NotifyQt::getInstance()->notifyChatFontChanged();

	Settings->setChatSendMessageWithCtrlReturn(ui.sendMessageWithCtrlReturn->isChecked());
	Settings->setChatSendAsPlainTextByDef(ui.sendAsPlainTextByDef->isChecked());
	Settings->setChatLoadEmbeddedImages(ui.loadEmbeddedImages->isChecked());
	Settings->setChatDoNotSendIsTyping(ui.DontSendTyping->isChecked());
}

void ChatPage::updateChatSearchParams()
{
	Settings->setChatSearchCharToStartSearch(ui.sbSearch_CharToStart->value());
	Settings->setChatSearchCaseSensitively(ui.cbSearch_CaseSensitively->isChecked());
	Settings->setChatSearchWholeWords(ui.cbSearch_WholeWords->isChecked());
	Settings->setChatSearchMoveToCursor(ui.cbSearch_MoveToCursor->isChecked());
	Settings->setChatSearchSearchWithoutLimit(ui.cbSearch_WithoutLimit->isChecked());
	Settings->setChatSearchMaxSearchLimitColor(ui.sbSearch_MaxLimitColor->value());
	Settings->setChatSearchFoundColor(rgbChatSearchFoundColor);
}

void ChatPage::updateDefaultLobbyIdentity()
{
    RsGxsId chosen_id ;
	switch(ui.chatLobbyIdentity_IC->getChosenId(chosen_id))
	{
	case GxsIdChooser::KnowId:
	case GxsIdChooser::UnKnowId:
		rsMsgs->setDefaultIdentityForChatLobby(chosen_id) ;
		break ;

	default:;
	}
}


void ChatPage::updateHistoryParams()
{
	Settings->setPublicChatHistoryCount(ui.publicChatLoadCount->value());
	Settings->setPrivateChatHistoryCount(ui.privateChatLoadCount->value());
	Settings->setLobbyChatHistoryCount(ui.lobbyChatLoadCount->value());

	rsHistory->setEnable(RS_HISTORY_TYPE_PUBLIC , ui.publicChatEnable->isChecked());
	rsHistory->setEnable(RS_HISTORY_TYPE_PRIVATE, ui.privateChatEnable->isChecked());
	rsHistory->setEnable(RS_HISTORY_TYPE_LOBBY  , ui.lobbyChatEnable->isChecked());

	rsHistory->setSaveCount(RS_HISTORY_TYPE_PUBLIC , ui.publicChatSaveCount->value());
    rsHistory->setSaveCount(RS_HISTORY_TYPE_PRIVATE, ui.privateChatSaveCount->value());
    rsHistory->setSaveCount(RS_HISTORY_TYPE_LOBBY  , ui.lobbyChatSaveCount->value());
}

void ChatPage::updatePublicStyle()
{
	ChatStyleInfo info = ui.publicStyle->itemData(ui.historyStyle->currentIndex(),Qt::UserRole).value<ChatStyleInfo>();

	if (publicStylePath != info.stylePath || publicStyleVariant != ui.publicComboBoxVariant->currentText()) {
		Settings->setPublicChatStyle(info.stylePath, ui.publicComboBoxVariant->currentText());
		NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_PUBLIC);
	}
}

void ChatPage::updatePrivateStyle()
{
	ChatStyleInfo info = ui.privateStyle->itemData(ui.historyStyle->currentIndex(),Qt::UserRole).value<ChatStyleInfo>();

	if (privateStylePath != info.stylePath || privateStyleVariant != ui.privateComboBoxVariant->currentText()) {
		Settings->setPrivateChatStyle(info.stylePath, ui.privateComboBoxVariant->currentText());
		NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_PRIVATE);
	}
}

void ChatPage::updateHistoryStyle()
{
	ChatStyleInfo info = ui.historyStyle->itemData(ui.historyStyle->currentIndex(),Qt::UserRole).value<ChatStyleInfo>();

	if (historyStylePath != info.stylePath || historyStyleVariant != ui.historyComboBoxVariant->currentText()) {
		Settings->setHistoryChatStyle(info.stylePath, ui.historyComboBoxVariant->currentText());
		NotifyQt::getInstance()->notifyChatStyleChanged(ChatStyle::TYPE_HISTORY);
	}
}

void ChatPage::updateHistoryStorage() { rsHistory->setMaxStorageDuration(ui.max_storage_period->value() * 86400) ; }


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
	connect(ui.distantChatComboBox,        SIGNAL(currentIndexChanged(int)), this, SLOT(distantChatComboBoxChanged(int)));

	connect(ui.checkBox_emoteprivchat,     SIGNAL(toggled(bool)),            this, SLOT(updateFontsAndEmotes()));
	connect(ui.checkBox_emotegroupchat,    SIGNAL(toggled(bool)),            this, SLOT(updateFontsAndEmotes()));
	connect(ui.checkBox_enableCustomFonts, SIGNAL(toggled(bool)),            this, SLOT(updateFontsAndEmotes()));
	connect(ui.minimumFontSize,            SIGNAL(valueChanged(int)),        this, SLOT(updateFontsAndEmotes()));
	connect(ui.checkBox_enableBold,        SIGNAL(toggled(bool)),            this, SLOT(updateFontsAndEmotes()));
	connect(ui.checkBox_enableItalics,     SIGNAL(toggled(bool)),            this, SLOT(updateFontsAndEmotes()));
	connect(ui.minimumContrast,            SIGNAL(valueChanged(int)),        this, SLOT(updateFontsAndEmotes()));

	connect(ui.distantChatComboBox,        SIGNAL(currentIndexChanged(int)), this, SLOT(updateChatParams()));
	connect(ui.sendMessageWithCtrlReturn,  SIGNAL(toggled(bool)),            this, SLOT(updateChatParams()));
	connect(ui.sendAsPlainTextByDef,       SIGNAL(toggled(bool)),            this, SLOT(updateChatParams()));
	connect(ui.loadEmbeddedImages,         SIGNAL(toggled(bool)),            this, SLOT(updateChatParams()));
	connect(ui.DontSendTyping,             SIGNAL(toggled(bool)),            this, SLOT(updateChatParams()));

	connect(ui.sbSearch_CharToStart,       SIGNAL(valueChanged(int)),        this, SLOT(updateChatSearchParams()));
	connect(ui.cbSearch_CaseSensitively,   SIGNAL(toggled(bool)),            this, SLOT(updateChatSearchParams()));
	connect(ui.cbSearch_WholeWords,        SIGNAL(toggled(bool)),            this, SLOT(updateChatSearchParams()));
	connect(ui.cbSearch_MoveToCursor,      SIGNAL(toggled(bool)),            this, SLOT(updateChatSearchParams()));
	connect(ui.cbSearch_WithoutLimit,      SIGNAL(toggled(bool)),            this, SLOT(updateChatSearchParams()));
	connect(ui.sbSearch_MaxLimitColor,     SIGNAL(valueChanged(int)),        this, SLOT(updateChatSearchParams()));

	connect(ui.chatLobbyIdentity_IC,       SIGNAL(currentIndexChanged(int)), this, SLOT(updateDefaultLobbyIdentity()));

	connect(ui.publicChatLoadCount,        SIGNAL(valueChanged(int)),        this, SLOT(updateHistoryParams()));
	connect(ui.privateChatLoadCount,       SIGNAL(valueChanged(int)),        this, SLOT(updateHistoryParams()));
	connect(ui.lobbyChatLoadCount,         SIGNAL(valueChanged(int)),        this, SLOT(updateHistoryParams()));
	connect(ui.publicChatEnable,           SIGNAL(toggled(bool)),            this, SLOT(updateHistoryParams()));
	connect(ui.privateChatEnable,          SIGNAL(toggled(bool)),            this, SLOT(updateHistoryParams()));
	connect(ui.lobbyChatEnable,            SIGNAL(toggled(bool)),            this, SLOT(updateHistoryParams()));
	connect(ui.publicChatSaveCount,        SIGNAL(valueChanged(int)),        this, SLOT(updateHistoryParams()));
	connect(ui.privateChatSaveCount,       SIGNAL(valueChanged(int)),        this, SLOT(updateHistoryParams()));
	connect(ui.lobbyChatSaveCount,         SIGNAL(valueChanged(int)),        this, SLOT(updateHistoryParams()));

    connect(ui.publicStyle,                SIGNAL(currentIndexChanged(int)),   this, SLOT(updatePublicStyle())) ;
    connect(ui.publicComboBoxVariant,      SIGNAL(currentIndexChanged(int)), this, SLOT(updatePublicStyle())) ;

    connect(ui.privateStyle,                SIGNAL(currentIndexChanged(int)),   this, SLOT(updatePrivateStyle())) ;
    connect(ui.privateComboBoxVariant,     SIGNAL(currentIndexChanged(int)), this, SLOT(updatePrivateStyle())) ;

    connect(ui.historyStyle,                SIGNAL(currentIndexChanged(int)),   this, SLOT(updateHistoryStyle())) ;
    connect(ui.historyComboBoxVariant,     SIGNAL(currentIndexChanged(int)), this, SLOT(updateHistoryStyle())) ;

    connect(ui.max_storage_period,         SIGNAL(valueChanged(int)),        this, SLOT(updateHistoryStorage())) ;

	connect(ui.chat_NewWindow,             SIGNAL(toggled(bool)),            this, SLOT(updateChatFlags()));
	connect(ui.chat_Focus,                 SIGNAL(toggled(bool)),            this, SLOT(updateChatFlags()));
	connect(ui.chat_tabbedWindow,          SIGNAL(toggled(bool)),            this, SLOT(updateChatFlags()));
	connect(ui.chat_Blink,                 SIGNAL(toggled(bool)),            this, SLOT(updateChatFlags()));

	connect(ui.chatLobby_Blink,            SIGNAL(toggled(bool)),            this, SLOT(updateChatLobbyFlags()));

	connect(ui.publicStyle,                SIGNAL(currentIndexChanged(int)), this,SLOT(on_publicList_currentRowChanged(int)));
	connect(ui.privateStyle,               SIGNAL(currentIndexChanged(int)), this,SLOT(on_privateList_currentRowChanged(int)));
	connect(ui.historyStyle,               SIGNAL(currentIndexChanged(int)), this,SLOT(on_historyList_currentRowChanged(int)));
}
void ChatPage::updateChatFlags()
{
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
}

void ChatPage::updateChatLobbyFlags()
{
	uint chatLobbyFlags = 0;

	if (ui.chatLobby_Blink->isChecked())
		chatLobbyFlags |= RS_CHATLOBBY_BLINK;

	Settings->setChatLobbyFlags(chatLobbyFlags);
}

/** Loads the settings for this page */
void
ChatPage::load()
{
    Settings->beginGroup(QString("Chat"));
    whileBlocking(ui.checkBox_emoteprivchat)->setChecked(Settings->value("Emoteicons_PrivatChat", true).toBool());
    whileBlocking(ui.checkBox_emotegroupchat)->setChecked(Settings->value("Emoteicons_GroupChat", true).toBool());
    whileBlocking(ui.checkBox_enableCustomFonts)->setChecked(Settings->value("EnableCustomFonts", true).toBool());
    whileBlocking(ui.checkBox_enableCustomFontSize)->setChecked(Settings->value("EnableCustomFontSize", true).toBool());
	whileBlocking(ui.minimumFontSize)->setValue(Settings->value("MinimumFontSize", 10).toInt());
    whileBlocking(ui.checkBox_enableBold)->setChecked(Settings->value("EnableBold", true).toBool());
    whileBlocking(ui.checkBox_enableItalics)->setChecked(Settings->value("EnableItalics", true).toBool());
    whileBlocking(ui.minimumContrast)->setValue(Settings->value("MinimumContrast", 4.5).toDouble());
    Settings->endGroup();

	     // state of distant Chat combobox
    int index = Settings->value("DistantChat", 0).toInt();
    whileBlocking(ui.distantChatComboBox)->setCurrentIndex(index);

    fontTempChat.fromString(Settings->getChatScreenFont());

    whileBlocking(ui.sendMessageWithCtrlReturn)->setChecked(Settings->getChatSendMessageWithCtrlReturn());
    whileBlocking(ui.sendAsPlainTextByDef)->setChecked(Settings->getChatSendAsPlainTextByDef());
    whileBlocking(ui.loadEmbeddedImages)->setChecked(Settings->getChatLoadEmbeddedImages());
    whileBlocking(ui.DontSendTyping)->setChecked(Settings->getChatDoNotSendIsTyping());

	std::string advsetting;
	if(rsConfig->getConfigurationOption(RS_CONFIG_ADVANCED, advsetting) && (advsetting == "YES"))
	{ }
	else
		ui.DontSendTyping->hide();

    whileBlocking(ui.sbSearch_CharToStart)->setValue(Settings->getChatSearchCharToStartSearch());
    whileBlocking(ui.cbSearch_CaseSensitively)->setChecked(Settings->getChatSearchCaseSensitively());
    whileBlocking(ui.cbSearch_WholeWords)->setChecked(Settings->getChatSearchWholeWords());
    whileBlocking(ui.cbSearch_MoveToCursor)->setChecked(Settings->getChatSearchMoveToCursor());
    whileBlocking(ui.cbSearch_WithoutLimit)->setChecked(Settings->getChatSearchSearchWithoutLimit());
    whileBlocking(ui.sbSearch_MaxLimitColor)->setValue(Settings->getChatSearchMaxSearchLimitColor());
    rgbChatSearchFoundColor=Settings->getChatSearchFoundColor();
    QPixmap pix(24, 24);
    pix.fill(rgbChatSearchFoundColor);
    ui.btSearch_FoundColor->setIcon(pix);

    whileBlocking(ui.publicChatLoadCount)->setValue(Settings->getPublicChatHistoryCount());
    whileBlocking(ui.privateChatLoadCount)->setValue(Settings->getPrivateChatHistoryCount());
    whileBlocking(ui.lobbyChatLoadCount)->setValue(Settings->getLobbyChatHistoryCount());

    whileBlocking(ui.publicChatEnable)->setChecked(rsHistory->getEnable(RS_HISTORY_TYPE_PUBLIC));
    whileBlocking(ui.privateChatEnable)->setChecked(rsHistory->getEnable(RS_HISTORY_TYPE_PRIVATE));
    whileBlocking(ui.lobbyChatEnable)->setChecked(rsHistory->getEnable(RS_HISTORY_TYPE_LOBBY));

    whileBlocking(ui.publicChatSaveCount)->setValue(rsHistory->getSaveCount(RS_HISTORY_TYPE_PUBLIC));
    whileBlocking(ui.privateChatSaveCount)->setValue(rsHistory->getSaveCount(RS_HISTORY_TYPE_PRIVATE));
    whileBlocking(ui.lobbyChatSaveCount)->setValue(rsHistory->getSaveCount(RS_HISTORY_TYPE_LOBBY));
    
    // using fontTempChat.rawname() does not always work!
    // see http://doc.qt.digia.com/qt-maemo/qfont.html#rawName
    QStringList fontname = fontTempChat.toString().split(",");
    whileBlocking(ui.labelChatFontPreview)->setText(fontname[0]);
    whileBlocking(ui.labelChatFontPreview)->setFont(fontTempChat);

	whileBlocking(ui.max_storage_period)->setValue(rsHistory->getMaxStorageDuration()/86400) ;

    /* Load styles */
    publicStylePath = loadStyleInfo(ChatStyle::TYPE_PUBLIC, ui.publicStyle, ui.publicComboBoxVariant, publicStyleVariant);
    privateStylePath = loadStyleInfo(ChatStyle::TYPE_PRIVATE, ui.privateStyle, ui.privateComboBoxVariant, privateStyleVariant);
    historyStylePath = loadStyleInfo(ChatStyle::TYPE_HISTORY, ui.historyStyle, ui.historyComboBoxVariant, historyStyleVariant);

    RsGxsId gxs_id ;
    rsMsgs->getDefaultIdentityForChatLobby(gxs_id) ;

    ui.chatLobbyIdentity_IC->setFlags(IDCHOOSER_ID_REQUIRED) ;

    if(!gxs_id.isNull())
        ui.chatLobbyIdentity_IC->setChosenId(gxs_id);

    uint chatflags = Settings->getChatFlags();

    whileBlocking(ui.chat_NewWindow)->setChecked(chatflags & RS_CHAT_OPEN);
    whileBlocking(ui.chat_Focus)->setChecked(chatflags & RS_CHAT_FOCUS);
    whileBlocking(ui.chat_tabbedWindow)->setChecked(chatflags & RS_CHAT_TABBED_WINDOW);
    whileBlocking(ui.chat_Blink)->setChecked(chatflags & RS_CHAT_BLINK);

    uint chatLobbyFlags = Settings->getChatLobbyFlags();

    whileBlocking(ui.chatLobby_Blink)->setChecked(chatLobbyFlags & RS_CHATLOBBY_BLINK);

	 // load personal invites
	 //
#ifdef TO_BE_DONE
	 for(;;)
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
	QFont font = misc::getFont(&ok, fontTempChat, this, tr("Choose your default font for Chat."));

	if (ok) {
		fontTempChat = font;
		// using fontTempChat.rawname() does not always work!
		// see http://doc.qt.digia.com/qt-maemo/qfont.html#rawName
		QStringList fontname = fontTempChat.toString().split(",");
		ui.labelChatFontPreview->setText(fontname[0]);
		ui.labelChatFontPreview->setFont(fontTempChat);
		updateChatParams();
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
    QColor backgroundColor = textBrowser->palette().base().color();

    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_HINCOMING, nameIncoming, timestmp, tr("Incoming message in history"), 0, backgroundColor));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_HOUTGOING, nameOutgoing, timestmp, tr("Outgoing message in history"), 0, backgroundColor));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_INCOMING,  nameIncoming, timestmp, tr("Incoming message"), 0, backgroundColor));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_OUTGOING,  nameOutgoing, timestmp, tr("Outgoing message"), 0, backgroundColor));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_OOUTGOING,  nameOutgoing, timestmp, tr("Outgoing offline message"), 0, backgroundColor));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_SYSTEM,  tr("System"), timestmp, tr("System message"), 0, backgroundColor));
    textBrowser->append(style.formatMessage(ChatStyle::FORMATMSG_OUTGOING,  tr("UserName"), timestmp, tr("/me is sending a message with /me"), 0, backgroundColor));
}

void ChatPage::fillPreview(QComboBox *listWidget, QComboBox *comboBox, QTextBrowser *textBrowser)
{
	ChatStyleInfo info = listWidget->itemData(listWidget->currentIndex(),Qt::UserRole).value<ChatStyleInfo>();

	setPreviewMessages(info.stylePath, comboBox->currentText(), textBrowser);
}

void ChatPage::on_publicList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        ChatStyleInfo info = ui.publicStyle->itemData(currentRow,Qt::UserRole).value<ChatStyleInfo>();

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        ui.publicAuthor->setText(author);
        ui.publicDescription->setText(info.styleDescription);

        QStringList variants;
        ChatStyle::getAvailableVariants(info.stylePath, variants);
        whileBlocking(ui.publicComboBoxVariant)->clear();
        whileBlocking(ui.publicComboBoxVariant)->setEnabled(variants.size() != 0);
        whileBlocking(ui.publicComboBoxVariant)->addItems(variants);

        /* try to find "Standard" */
        int index = ui.publicComboBoxVariant->findText(VARIANT_STANDARD);
        if (index != -1) {
            whileBlocking(ui.publicComboBoxVariant)->setCurrentIndex(index);
        } else {
            whileBlocking(ui.publicComboBoxVariant)->setCurrentIndex(0);
        }
    } else {
        whileBlocking(ui.publicAuthor)->clear();
        whileBlocking(ui.publicDescription)->clear();
        whileBlocking(ui.publicComboBoxVariant)->clear();
        whileBlocking(ui.publicComboBoxVariant)->setDisabled(true);
    }

    fillPreview(ui.publicStyle, ui.publicComboBoxVariant, ui.publicPreview);
}

void ChatPage::on_privateList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        ChatStyleInfo info = ui.privateStyle->itemData(ui.privateStyle->currentIndex(),Qt::UserRole).value<ChatStyleInfo>();

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        whileBlocking(ui.privateAuthor)->setText(author);
        whileBlocking(ui.privateDescription)->setText(info.styleDescription);

        QStringList variants;
        ChatStyle::getAvailableVariants(info.stylePath, variants);
        whileBlocking(ui.privateComboBoxVariant)->clear();
        whileBlocking(ui.privateComboBoxVariant)->setEnabled(variants.size() != 0);
        whileBlocking(ui.privateComboBoxVariant)->addItems(variants);

        /* try to find "Standard" */
        int index = ui.privateComboBoxVariant->findText(VARIANT_STANDARD);
        if (index != -1) {
            whileBlocking(ui.privateComboBoxVariant)->setCurrentIndex(index);
        } else {
            whileBlocking(ui.privateComboBoxVariant)->setCurrentIndex(0);
        }
    } else {
        whileBlocking(ui.privateAuthor)->clear();
        whileBlocking(ui.privateDescription)->clear();
        whileBlocking(ui.privateComboBoxVariant)->clear();
        whileBlocking(ui.privateComboBoxVariant)->setDisabled(true);
    }

    fillPreview(ui.privateStyle, ui.privateComboBoxVariant, ui.privatePreview);
}

void ChatPage::on_historyList_currentRowChanged(int currentRow)
{
    if (currentRow != -1) {
        ChatStyleInfo info = ui.historyStyle->itemData(ui.historyStyle->currentIndex(),Qt::UserRole).value<ChatStyleInfo>();

        QString author = info.authorName;
        if (info.authorEmail.isEmpty() == false) {
            author += " (" + info.authorEmail + ")";
        }
        whileBlocking(ui.historyAuthor)->setText(author);
        whileBlocking(ui.historyDescription)->setText(info.styleDescription);

        QStringList variants;
        ChatStyle::getAvailableVariants(info.stylePath, variants);
        whileBlocking(ui.historyComboBoxVariant)->clear();
        whileBlocking(ui.historyComboBoxVariant)->setEnabled(variants.size() != 0);
        whileBlocking(ui.historyComboBoxVariant)->addItems(variants);

        /* try to find "Standard" */
        int index = ui.historyComboBoxVariant->findText(VARIANT_STANDARD);
        if (index != -1) {
            whileBlocking(ui.historyComboBoxVariant)->setCurrentIndex(index);
        } else {
            whileBlocking(ui.historyComboBoxVariant)->setCurrentIndex(0);
        }
    } else {
        whileBlocking(ui.historyAuthor)->clear();
        whileBlocking(ui.historyDescription)->clear();
        whileBlocking(ui.historyComboBoxVariant)->clear();
        whileBlocking(ui.historyComboBoxVariant)->setDisabled(true);
    }

    fillPreview(ui.historyStyle, ui.historyComboBoxVariant, ui.historyPreview);
}

void ChatPage::on_publicComboBoxVariant_currentIndexChanged(int /*index*/)
{
    fillPreview(ui.publicStyle, ui.publicComboBoxVariant, ui.publicPreview);
}

void ChatPage::on_privateComboBoxVariant_currentIndexChanged(int /*index*/)
{
    fillPreview(ui.privateStyle, ui.privateComboBoxVariant, ui.privatePreview);
}

void ChatPage::on_historyComboBoxVariant_currentIndexChanged(int /*index*/)
{
    fillPreview(ui.historyStyle, ui.historyComboBoxVariant, ui.historyPreview);
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

void ChatPage::distantChatComboBoxChanged(int i)
{
	switch(i)
	{
		default: 
		case 0: rsMsgs->setDistantChatPermissionFlags(RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_NONE) ;           
				  break ;
				  
		case 1:  rsMsgs->setDistantChatPermissionFlags(RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_NON_CONTACTS) ;
				  break ;

		case 2:  rsMsgs->setDistantChatPermissionFlags(RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_EVERYBODY) ;
				  break ;
	}

}

