/*******************************************************************************
 * gui/chat/ChatTabWidget.cpp                                                  *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QTabBar>

#include "ChatTabWidget.h"
#include "ui_ChatTabWidget.h"
#include "ChatDialog.h"
#include "gui/common/StatusDefs.h"
#include "rshare.h"

#define IMAGE_TYPING         ":/icons/png/typing.png"
#define IMAGE_CHAT           ":/images/chat.png"

ChatTabWidget::ChatTabWidget(QWidget *parent) :
	RSTabWidget(parent),
	ui(new Ui::ChatTabWidget)
{
	ui->setupUi(this);

	mEmptyIcon = NULL;

	connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClose(int)));
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

	connect(rApp, SIGNAL(blink(bool)), this, SLOT(blink(bool)));
}

ChatTabWidget::~ChatTabWidget()
{
	if (mEmptyIcon) {
		delete(mEmptyIcon);
	}

	delete ui;
}

void ChatTabWidget::addDialog(ChatDialog *dialog)
{
	addTab(dialog, dialog->getTitle());
	dialog->addToParent(this);

	QObject::connect(dialog, SIGNAL(infoChanged(ChatDialog*)), this, SLOT(tabInfoChanged(ChatDialog*)));
	QObject::connect(dialog, SIGNAL(dialogClose(ChatDialog*)), this, SLOT(dialogClose(ChatDialog*)));

	tabInfoChanged(dialog);
}

void ChatTabWidget::removeDialog(ChatDialog *dialog)
{
	QObject::disconnect(dialog, SIGNAL(infoChanged(ChatDialog*)), this, SLOT(tabInfoChanged(ChatDialog*)));
	QObject::disconnect(dialog, SIGNAL(dialogClose(ChatDialog*)), this, SLOT(dialogClose(ChatDialog*)));

	int tab = indexOf(dialog);
	if (tab >= 0) {
		dialog->removeFromParent(this);
		removeTab(tab);
		emit tabClosed(dialog);
	}
}

void ChatTabWidget::tabClose(int tab)
{
	ChatDialog *dialog = dynamic_cast<ChatDialog*>(widget(tab));

	if (dialog) {
		dialog->close();
	}
}

void ChatTabWidget::tabChanged(int tab)
{
	ChatDialog *dialog = dynamic_cast<ChatDialog*>(widget(tab));

	if (dialog) {
		emit tabChanged(dialog);
	}
}

void ChatTabWidget::tabInfoChanged(ChatDialog *dialog)
{
	int tab = indexOf(dialog);
	if (tab >= 0) {
		if (dialog->isTyping()) {
			setBlinking(tab, false);
			setTabIcon(tab, QIcon(IMAGE_TYPING));
		} else if (dialog->hasNewMessages()) {
			setTabIcon(tab, QIcon(IMAGE_CHAT));
			if (dialog->notifyBlink()) {
				setBlinking(tab, true);
			} else {
				setBlinking(tab, false);
			}
		} else if (dialog->hasPeerStatus()) {
			setBlinking(tab, false);
			setTabIcon(tab, QIcon(StatusDefs::imageIM(dialog->getPeerStatus())));
		} else {
			setBlinking(tab, false);
			setTabIcon(tab, QIcon());
		}
	}

	emit infoChanged();
}

void ChatTabWidget::dialogClose(ChatDialog *dialog)
{
	removeDialog(dialog);
}

void ChatTabWidget::getInfo(bool &isTyping, bool &hasNewMessage, QIcon *icon)
{
	isTyping = false;
	hasNewMessage = false;

	ChatDialog *cd;
	int tabCount = count();
	for (int i = 0; i < tabCount; i++) {
		cd = dynamic_cast<ChatDialog*>(widget(i));
		if (cd) {
			if (cd->isTyping()) {
				isTyping = true;
			}
			if (cd->hasNewMessages()) {
				hasNewMessage = true;
			}
		}
	}

	if (icon) {
		if (isTyping) {
			*icon = QIcon(IMAGE_TYPING);
		} else if (hasNewMessage) {
			*icon = QIcon(IMAGE_CHAT);
		} else {
			cd = dynamic_cast<ChatDialog*>(currentWidget());
			if (cd && cd->hasPeerStatus()) {
				*icon = QIcon(StatusDefs::imageIM(cd->getPeerStatus()));
			} else {
				*icon = QIcon();
			}
		}
	}
}

void ChatTabWidget::setBlinking(int tab, bool blink)
{
	QIcon icon = tabBar()->tabData(tab).value<QIcon>();

	if (blink) {
		/* save current icon */
		tabBar()->setTabData(tab, tabIcon(tab));
	} else {
		if (!icon.isNull()) {
			/* reset icon */
			setTabIcon(tab, icon);

			/* remove icon */
			tabBar()->setTabData(tab, QVariant());
		}
	}
}

void ChatTabWidget::blink(bool on)
{
	int tabCount = tabBar()->count();
	for (int tab = 0; tab < tabCount; ++tab) {
		QIcon icon = tabBar()->tabData(tab).value<QIcon>();

		if (!icon.isNull()) {
			if (mEmptyIcon == NULL) {
				/* create empty icon */
				QPixmap pixmap(16, 16);
				pixmap.fill(Qt::transparent);
				mEmptyIcon = new QIcon(pixmap);
			}
			tabBar()->setTabIcon(tab, on ? icon : *mEmptyIcon);
		}
	}
}
