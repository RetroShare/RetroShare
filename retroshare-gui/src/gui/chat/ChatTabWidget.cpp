/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, RetroShare Team
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

#include "ChatTabWidget.h"
#include "ui_ChatTabWidget.h"
#include "ChatDialog.h"
#include "gui/common/StatusDefs.h"

#define IMAGE_WINDOW         ":/images/rstray3.png"
#define IMAGE_TYPING         ":/images/typing.png"
#define IMAGE_CHAT           ":/images/chat.png"

ChatTabWidget::ChatTabWidget(QWidget *parent) :
	RSTabWidget(parent),
	ui(new Ui::ChatTabWidget)
{
	ui->setupUi(this);

    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClose(int)));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
}

ChatTabWidget::~ChatTabWidget()
{
	delete ui;
}

void ChatTabWidget::addDialog(ChatDialog *dialog)
{
	addTab(dialog, dialog->getTitle());
	dialog->addToParent(this);

	QObject::connect(dialog, SIGNAL(infoChanged(ChatDialog*)), this, SLOT(tabInfoChanged(ChatDialog*)));

	tabInfoChanged(dialog);
}

void ChatTabWidget::removeDialog(ChatDialog *dialog)
{
	QObject::disconnect(dialog, SIGNAL(infoChanged(ChatDialog*)), this, SLOT(tabInfoChanged(ChatDialog*)));

	int tab = indexOf(dialog);
	if (tab >= 0) {
		dialog->removeFromParent(this);
		removeTab(tab);
	}
}

void ChatTabWidget::tabClose(int tab)
{
	ChatDialog *dialog = dynamic_cast<ChatDialog*>(widget(tab));

	if (dialog) {
		if (dialog->canClose()) {
			dialog->deleteLater();
		}
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
			setTabIcon(tab, QIcon(IMAGE_TYPING));
		} else if (dialog->hasNewMessages()) {
			setTabIcon(tab, QIcon(IMAGE_CHAT));
		} else if (dialog->hasPeerStatus()) {
			setTabIcon(tab, QIcon(StatusDefs::imageIM(dialog->getPeerStatus())));
		} else {
			setTabIcon(tab, QIcon());
		}
	}

	emit infoChanged();
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
