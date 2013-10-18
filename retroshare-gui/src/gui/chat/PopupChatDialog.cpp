/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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
	connect(NotifyQt::getInstance(), SIGNAL(chatStatusChanged(const QString&, const QString&, bool)), this, SLOT(chatStatusChanged(const QString&, const QString&, bool)));
}

void PopupChatDialog::init(const std::string &peerId, const QString &title)
{
	connect(ui.chatWidget, SIGNAL(statusChanged(int)), this, SLOT(statusChanged(int)));

	ChatDialog::init(peerId, title);

	/* Hide or show the avatar frames */
	showAvatarFrame(PeerSettings->getShowAvatarFrame(peerId));

	ui.avatarWidget->setFrameType(AvatarWidget::STATUS_FRAME);
	ui.avatarWidget->setId(peerId, false);

	ui.ownAvatarWidget->setFrameType(AvatarWidget::STATUS_FRAME);
	ui.ownAvatarWidget->setOwnId();

	ui.chatWidget->addToolsAction(ui.actionClearOfflineMessages);

	// add offline chat messages
	onChatChanged(NOTIFY_LIST_PRIVATE_OUTGOING_CHAT, NOTIFY_TYPE_ADD);

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
void PopupChatDialog::chatStatusChanged(const QString &peerId, const QString& statusString, bool isPrivateChat)
{
	if (isPrivateChat && this->peerId == peerId.toStdString()) {
		ui.chatWidget->updateStatusString(getPeerName(peerId.toStdString()) + " %1", statusString);
	}
}

void PopupChatDialog::addIncomingChatMsg(const ChatInfo& info)
{
	ChatWidget *cw = getChatWidget();
	if (cw) {
		QDateTime sendTime = QDateTime::fromTime_t(info.sendTime);
		QDateTime recvTime = QDateTime::fromTime_t(info.recvTime);
		QString message = QString::fromStdWString(info.msg);
		QString name = getPeerName(info.rsid) ;

		cw->addChatMsg(true, name, sendTime, recvTime, message, ChatWidget::TYPE_NORMAL);
	}
}

void PopupChatDialog::addChatBarWidget(QWidget *w)
{
	getChatWidget()->addChatBarWidget(w) ;
}

void PopupChatDialog::onChatChanged(int list, int type)
{
	if (list == NOTIFY_LIST_PRIVATE_OUTGOING_CHAT) {
		switch (type) {
		case NOTIFY_TYPE_ADD:
			{
				std::list<ChatInfo> savedOfflineChatNew;

				QString name = getPeerName(rsPeers->getOwnId()) ;

				std::list<ChatInfo> offlineChat;
				if (rsMsgs->getPrivateChatQueueCount(false) && rsMsgs->getPrivateChatQueue(false, peerId, offlineChat)) {
					ui.actionClearOfflineMessages->setEnabled(true);

					std::list<ChatInfo>::iterator it;
					for(it = offlineChat.begin(); it != offlineChat.end(); it++) {
						/* are they public? */
						if ((it->chatflags & RS_CHAT_PRIVATE) == 0) {
							/* this should not happen */
							continue;
						}

						savedOfflineChatNew.push_back(*it);

						if (std::find(savedOfflineChat.begin(), savedOfflineChat.end(), *it) != savedOfflineChat.end()) {
							continue;
						}

						QDateTime sendTime = QDateTime::fromTime_t(it->sendTime);
						QDateTime recvTime = QDateTime::fromTime_t(it->recvTime);
						QString message = QString::fromStdWString(it->msg);

						ui.chatWidget->addChatMsg(false, name, sendTime, recvTime, message, ChatWidget::TYPE_OFFLINE);
					}
				}

				savedOfflineChat = savedOfflineChatNew;
			}
			break;
		case NOTIFY_TYPE_DEL:
			{
				if (manualDelete == false) {
					QString name = getPeerName(rsPeers->getOwnId()) ;

					// now show saved offline chat messages as sent
					std::list<ChatInfo>::iterator it;
					for(it = savedOfflineChat.begin(); it != savedOfflineChat.end(); ++it) {
						QDateTime sendTime = QDateTime::fromTime_t(it->sendTime);
						QDateTime recvTime = QDateTime::fromTime_t(it->recvTime);
						QString message = QString::fromStdWString(it->msg);

						ui.chatWidget->addChatMsg(false, name, sendTime, recvTime, message, ChatWidget::TYPE_NORMAL);
					}
				}

				savedOfflineChat.clear();
			}
			break;
		}

		ui.actionClearOfflineMessages->setEnabled(!savedOfflineChat.empty());
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

	PeerSettings->setShowAvatarFrame(getPeerId(), show);
}

void PopupChatDialog::clearOfflineMessages()
{
	manualDelete = true;
	rsMsgs->clearPrivateChatQueue(false, peerId);
	manualDelete = false;
}

void PopupChatDialog::statusChanged(int status)
{
	updateStatus(status);
}
