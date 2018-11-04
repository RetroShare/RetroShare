/*******************************************************************************
 * gui/chat/PopupChatDialog.cpp                                                *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2006, Crypton <retroshare.project@gmail.com>                  *
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

#include "PopupChatDialog.h"
#include "PopupChatWindow.h"

#include "gui/settings/rsharesettings.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/notifyqt.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsiface.h>
#include <retroshare/rsnotify.h>

#include <algorithm>

#define appDir QApplication::applicationDirPath()

#define WINDOW(This) dynamic_cast<PopupChatWindow*>(This->window())

/** Default constructor */
PopupChatDialog::PopupChatDialog(QWidget *parent, Qt::WindowFlags flags)
  : ChatDialog(parent, flags)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	manualDelete = false;

	connect(ui.avatarFrameButton, SIGNAL(toggled(bool)), this, SLOT(showAvatarFrame(bool)));
	connect(ui.actionClearOfflineMessages, SIGNAL(triggered()), this, SLOT(clearOfflineMessages()));
    connect(NotifyQt::getInstance(), SIGNAL(chatStatusChanged(ChatId,QString)), this, SLOT(chatStatusChanged(ChatId,QString)));
}

void PopupChatDialog::init(const ChatId &chat_id, const QString &title)
{
    ChatDialog::init(chat_id, title);

	/* Hide or show the avatar frames */
    showAvatarFrame(PeerSettings->getShowAvatarFrame(chat_id));

    ui.avatarWidget->setId(chat_id);
    ui.avatarWidget->setFrameType(AvatarWidget::STATUS_FRAME);

	ui.ownAvatarWidget->setFrameType(AvatarWidget::STATUS_FRAME);
	ui.ownAvatarWidget->setOwnId();

	ui.chatWidget->addToolsAction(ui.actionClearOfflineMessages);

	// add to window
	PopupChatWindow *window = PopupChatWindow::getWindow(false);
	if (window) {
		window->addDialog(this);
	}

	// load settings
	processSettings(true);
}

/** Destructor. */
PopupChatDialog::~PopupChatDialog()
{
	// save settings
	processSettings(false);
}

ChatWidget *PopupChatDialog::getChatWidget()
{
	return ui.chatWidget;
}

bool PopupChatDialog::notifyBlink()
{
	return (Settings->getChatFlags() & RS_CHAT_BLINK);
}

void PopupChatDialog::processSettings(bool load)
{
	Settings->beginGroup(QString("PopupChatDialog"));

	if (load) {
		// load settings
	} else {
		// save settings
	}

	Settings->endGroup();
}

void PopupChatDialog::showDialog(uint chatflags)
{
	PopupChatWindow *window = WINDOW(this);
	if (window) {
		window->showDialog(this, chatflags);
	}
}

// Called by libretroshare through notifyQt to display the peer's status
//
void PopupChatDialog::chatStatusChanged(const ChatId &chat_id, const QString& statusString)
{
	QString additional_info ;

    if (mChatId.isSameEndpoint(chat_id)) {
        ui.chatWidget->updateStatusString(getPeerName(chat_id,additional_info) + " %1", statusString);
	}
}

void PopupChatDialog::addChatMsg(const ChatMessage &msg)
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
        QDateTime sendTime = QDateTime::fromTime_t(msg.sendTime);
        QDateTime recvTime = QDateTime::fromTime_t(msg.recvTime);
		QString additional_info ;
        QString message = QString::fromUtf8(msg.msg.c_str());
        QString name = msg.incoming? getPeerName(msg.chat_id,additional_info): getOwnName();

        cw->addChatMsg(msg.incoming, name, sendTime, recvTime, message, ChatWidget::MSGTYPE_NORMAL);
	}
}

/**
 Toggles the ToolBox on and off, changes toggle button text
 */
void PopupChatDialog::showAvatarFrame(bool show)
{
	ui.avatarframe->setVisible(show);
	ui.avatarFrameButton->setChecked(show);

	if (show) {
		ui.avatarFrameButton->setToolTip(tr("Hide Avatar"));
		ui.avatarFrameButton->setIcon(QIcon(":images/hide_toolbox_frame.png"));
	} else {
		ui.avatarFrameButton->setToolTip(tr("Show Avatar"));
		ui.avatarFrameButton->setIcon(QIcon(":images/show_toolbox_frame.png"));
	}

    PeerSettings->setShowAvatarFrame(mChatId, show);
}

void PopupChatDialog::clearOfflineMessages()
{
	manualDelete = true;
    // TODO
#ifdef REMOVE
	rsMsgs->clearPrivateChatQueue(false, peerId);
#endif
	manualDelete = false;
}
