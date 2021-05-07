/*******************************************************************************
 * gui/chat/PopupChatWindow.cpp                                                *
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

#include <QPixmap>
#include <QCloseEvent>
#include <QMenu>

#include "gui/common/FilesDefs.h"
#include "PopupChatWindow.h"
#include "ChatDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/common/StatusDefs.h"
#include "gui/style/RSStyle.h"
#include "util/misc.h"
#include "rshare.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rsnotify.h>


#define IMAGE_TYPING         ":/images/white-bubble-64.png"
#define IMAGE_CHAT           ":/images/orange-bubble-64.png"

static PopupChatWindow *instance = NULL;

/*static*/ PopupChatWindow *PopupChatWindow::getWindow(bool needSingleWindow)
{
	if (needSingleWindow == false && (Settings->getChatFlags() & RS_CHAT_TABBED_WINDOW)) {
		if (instance == NULL) {
			instance = new PopupChatWindow(true);
		}

		return instance;
	}

	return new PopupChatWindow(false);
}

/*static*/ void PopupChatWindow::cleanup()
{
	if (instance) {
	   delete(instance);
	   instance = NULL;
	}
}

/** Default constructor */
PopupChatWindow::PopupChatWindow(bool tabbed, QWidget *parent, Qt::WindowFlags flags)
  : QMainWindow(parent, flags),tabbedWindow(tabbed),firstShow(true)
  , chatDialog(nullptr),mEmptyIcon(nullptr)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	ui.tabWidget->setVisible(tabbedWindow);

//	if (Settings->getChatFlags() & RS_CHAT_TABBED_WINDOW) {
//		ui.actionDockTab->setVisible(tabbedWindow == false);
//		ui.actionUndockTab->setVisible(tabbedWindow);
//	} else {
//		ui.actionDockTab->setVisible(false);
//		ui.actionUndockTab->setVisible(false);
//
//	}

//    ui.actionAvatar->setVisible(false);	// removed because it is already handled by clicking on your own avatar.
//    ui.actionSetOnTop->setVisible(false);// removed, because the window manager should handle this already.
//    ui.actionColor->setVisible(false);// moved to the context menu

	ui.headerToolBar->hide();	// no widgets left!

	setAttribute(Qt::WA_DeleteOnClose, true);

	// connect(ui.actionAvatar, SIGNAL(triggered()),this, SLOT(getAvatar()));
	// connect(ui.actionColor, SIGNAL(triggered()), this, SLOT(setStyle()));
	// connect(ui.actionDockTab, SIGNAL(triggered()), this, SLOT(dockTab()));
	// connect(ui.actionUndockTab, SIGNAL(triggered()), this, SLOT(undockTab()));
	// connect(ui.actionSetOnTop, SIGNAL(toggled(bool)), this, SLOT(setOnTop()));

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

	connect(ui.tabWidget, SIGNAL(tabChanged(ChatDialog*)), this, SLOT(tabChanged(ChatDialog*)));
	connect(ui.tabWidget, SIGNAL(tabClosed(ChatDialog*)), this, SLOT(tabClosed(ChatDialog*)));

	connect(rApp, SIGNAL(blink(bool)), this, SLOT(blink(bool)));

	if (tabbedWindow) {
		/* signal toggled is called */
		ui.actionSetOnTop->setChecked(Settings->valueFromGroup("ChatWindow", "OnTop", false).toBool());
	}
}

void PopupChatWindow::showContextMenu(QPoint)
{
	QMenu contextMnu(this);
    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/highlight.png"),tr("Choose window color..."),this,SLOT(setStyle()));

	if (Settings->getChatFlags() & RS_CHAT_TABBED_WINDOW)
    {
        if(tabbedWindow)
            contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/tab-dock.png"),tr("Dock window"),this,SLOT(docTab()));

        contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/tab-undock.png"),tr("Dock window"),this,SLOT(undockTab()));
    }
    contextMnu.exec(QCursor::pos());
}

/** Destructor. */
PopupChatWindow::~PopupChatWindow()
{
	saveSettings();

	if (mEmptyIcon) {
		delete(mEmptyIcon);
	}

	if (this == instance) {
		instance = NULL;
	}
}

void PopupChatWindow::saveSettings()
{
	if (tabbedWindow) {
		Settings->saveWidgetInformation(this);

		Settings->setValueToGroup("ChatWindow", "OnTop", ui.actionSetOnTop->isChecked());
	} else {
        if (!chatId.isNotSet()) {
            PeerSettings->saveWidgetInformation(chatId, this);
            PeerSettings->setPrivateChatOnTop(chatId, ui.actionSetOnTop->isChecked());
        }
	}
}

void PopupChatWindow::showEvent(QShowEvent */*event*/)
{
	if (firstShow) {
		firstShow = false;

		if (tabbedWindow) {
			Settings->loadWidgetInformation(this);
		} else {
			this->move(qrand()%100, qrand()%100); //avoid to stack multiple popup chat windows on the same position
            PeerSettings->loadWidgetInformation(chatId, this);
		}
	}
}

void PopupChatWindow::closeEvent(QCloseEvent *event)
{
	if (tabbedWindow) {
		for (int index = ui.tabWidget->count() - 1; index >= 0; --index) {
			ChatDialog *dialog = dynamic_cast<ChatDialog*>(ui.tabWidget->widget(0));
			if (dialog == NULL) {
				continue;
			}
		
			if (!dialog->close()) {
				event->ignore();
				break;
			}
		}
	} else {
		if (chatDialog && !chatDialog->close()) {
			event->ignore();
		}
	}
}

ChatDialog *PopupChatWindow::getCurrentDialog()
{
	if (tabbedWindow) {
		return dynamic_cast<ChatDialog*>(ui.tabWidget->currentWidget());
	}

	return chatDialog;
}

void PopupChatWindow::addDialog(ChatDialog *dialog)
{
	if (tabbedWindow) {
		ui.tabWidget->addDialog(dialog);
	} else {
		ui.chatcentralLayout->addWidget(dialog);
		dialog->addToParent(this);
		ui.chatcentralLayout->setContentsMargins(0, 0, 0, 0);
		chatId = dialog->getChatId();
		chatDialog = dialog;
		calculateStyle(dialog);
		calculateTitle(dialog);

		/* signal toggled is called */
		ui.actionSetOnTop->setChecked(PeerSettings->getPrivateChatOnTop(chatId));

		QObject::connect(dialog, SIGNAL(dialogClose(ChatDialog*)), this, SLOT(dialogClose(ChatDialog*)));
	}

	QObject::connect(dialog, SIGNAL(infoChanged(ChatDialog*)), this, SLOT(tabInfoChanged(ChatDialog*)));
	QObject::connect(dialog, SIGNAL(newMessage(ChatDialog*)), this, SLOT(tabNewMessage(ChatDialog*)));
}

void PopupChatWindow::removeDialog(ChatDialog *dialog)
{
	QObject::disconnect(dialog, SIGNAL(infoChanged(ChatDialog*)), this, SLOT(tabInfoChanged(ChatDialog*)));
	QObject::disconnect(dialog, SIGNAL(newMessage(ChatDialog*)), this, SLOT(tabNewMessage(ChatDialog*)));

	if (tabbedWindow) {
		ui.tabWidget->removeDialog(dialog);

		if (ui.tabWidget->count() == 0) {
			deleteLater();
		}
	} else {

		if (chatDialog == dialog) {
			QObject::disconnect(dialog, SIGNAL(dialogClose(ChatDialog*)), this, SLOT(dialogClose(ChatDialog*)));
			saveSettings();
			dialog->removeFromParent(this);
			ui.chatcentralLayout->removeWidget(dialog);
			chatDialog = nullptr;
			chatId = ChatId();
			close();
			deleteLater();
		}
	}
}

void PopupChatWindow::showDialog(ChatDialog *dialog, uint chatflags)
{
	if (chatflags & RS_CHAT_FOCUS) {
		if (tabbedWindow) {
			ui.tabWidget->setCurrentWidget(dialog);
		}
		show();
		activateWindow();
		setWindowState((windowState() & (~Qt::WindowMinimized)) | Qt::WindowActive);
		raise();
		dialog->focusDialog();
	} else {
		if (isVisible() == false) {
			showMinimized();
		}
		alertDialog(dialog);
	}
}

void PopupChatWindow::alertDialog(ChatDialog */*dialog*/)
{
	QApplication::alert(this);
}

void PopupChatWindow::calculateTitle(ChatDialog *dialog)
{
	bool hasNewMessages = false;
	ChatDialog *cd;

	/* is typing */
	bool isTyping = false;
	if (ui.tabWidget->isVisible()) {
		ui.tabWidget->getInfo(isTyping, hasNewMessages, NULL);
	} else {
		if (dialog) {
			isTyping = dialog->isTyping();
			hasNewMessages = dialog->hasNewMessages();
		}
	}

	if (ui.tabWidget->isVisible()) {
		cd = dynamic_cast<ChatDialog*>(ui.tabWidget->currentWidget());
	} else {
		cd = dialog;
	}

	QIcon icon;
	if (isTyping) {
		mBlinkIcon = QIcon();
        icon = FilesDefs::getIconFromQtResourcePath(IMAGE_TYPING);
	} else if (hasNewMessages) {
        icon = FilesDefs::getIconFromQtResourcePath(IMAGE_CHAT);
		if (Settings->getChatFlags() & RS_CHAT_BLINK) {
			mBlinkIcon = icon;
		} else {
			mBlinkIcon = QIcon();
		}
	} else {
		mBlinkIcon = QIcon();
		if (cd && cd->hasPeerStatus()) {
			icon = QIcon(StatusDefs::imageIM(cd->getPeerStatus()));
		} else {
			icon = qApp->windowIcon();
		}
	}

	setWindowIcon(icon);

	if (cd) {
		QString title = cd->getTitle();
		if (cd->hasPeerStatus()) {
			title += " (" + StatusDefs::name(cd->getPeerStatus()) + ")";
		}
		setWindowTitle(title);
	} else {
		setWindowTitle("RetroShare");
	}
}

void PopupChatWindow::getAvatar()
{
	QByteArray ba;
	if (misc::getOpenAvatarPicture(this, ba)) {
		std::cerr << "Avatar image size = " << ba.size() << std::endl ;

		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()), ba.size());	// last char 0 included.
	}
}

void PopupChatWindow::tabClosed(ChatDialog */*dialog*/)
{
	if (tabbedWindow) {
		if (ui.tabWidget->count() == 0) {
			deleteLater();
		}
	}
}

void PopupChatWindow::dialogClose(ChatDialog *dialog)
{
	if (!tabbedWindow) {
		removeDialog(dialog);
	}
}

void PopupChatWindow::tabChanged(ChatDialog *dialog)
{
	calculateStyle(dialog);
	calculateTitle(dialog);
}

void PopupChatWindow::tabInfoChanged(ChatDialog *dialog)
{
	calculateTitle(dialog);
}

void PopupChatWindow::tabNewMessage(ChatDialog *dialog)
{
	alertDialog(dialog);
}

void PopupChatWindow::dockTab()
{
	if ((Settings->getChatFlags() & RS_CHAT_TABBED_WINDOW) && chatDialog) {
		PopupChatWindow *pcw = getWindow(false);
		if (pcw) {
			ChatDialog *pcd = chatDialog;
			removeDialog(pcd);
			pcw->addDialog(pcd);
			pcw->show();
			pcw->calculateTitle(pcd);
		}
	}
}

void PopupChatWindow::undockTab()
{
	ChatDialog *cd = dynamic_cast<ChatDialog*>(ui.tabWidget->currentWidget());

	if (cd) {
		PopupChatWindow *pcw = getWindow(true);
		if (pcw) {
			removeDialog(cd);
			pcw->addDialog(cd);
			cd->show();
			pcw->show();
			pcw->calculateTitle(cd);
		}
	}
}

void PopupChatWindow::setStyle()
{
	ChatDialog *cd = getCurrentDialog();

	if (cd && cd->setStyle()) {
		calculateStyle(cd);
	}
}

void PopupChatWindow::setOnTop()
{
	Qt::WindowFlags flags = windowFlags();
	if (ui.actionSetOnTop->isChecked()) {
		flags |= Qt::WindowStaysOnTopHint;
	} else {
		flags &= ~Qt::WindowStaysOnTopHint;
	}
	setWindowFlags(flags);

	/* Show window again */
	show();
}

void PopupChatWindow::calculateStyle(ChatDialog *dialog)
{
	QString toolSheet;
	QString statusSheet;
	QString widgetSheet;

	if (dialog) {
		const RSStyle *style = dialog->getStyle();
		if (style) {
			QString styleSheet = style->getStyleSheet();

			if (styleSheet.isEmpty() == false) {
				toolSheet = QString("QToolBar{%1}").arg(styleSheet);
				statusSheet = QString(".QStatusBar{%1}").arg(styleSheet);
				widgetSheet = QString(".QWidget{%1}").arg(styleSheet);
			}
		}
	}

	ui.headerToolBar->setStyleSheet(toolSheet);
	ui.chatstatusbar->setStyleSheet(statusSheet);
	ui.chatcentralwidget->setStyleSheet(widgetSheet);
}

void PopupChatWindow::blink(bool on)
{
	if (!mBlinkIcon.isNull()) {
		if (mEmptyIcon == NULL) {
			/* create empty icon */
			QPixmap pixmap(16, 16);
			pixmap.fill(Qt::transparent);
			mEmptyIcon = new QIcon(pixmap);
		}
		setWindowIcon(on ? mBlinkIcon : *mEmptyIcon);
	}
}
