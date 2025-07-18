/*******************************************************************************
 * gui/chat/PopupChatDialog.h                                                  *
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

#ifndef _POPUPCHATDIALOG_H
#define _POPUPCHATDIALOG_H

#include "ui_PopupChatDialog.h"
#include "ChatDialog.h"

#include <retroshare/rsmsgs.h>

class PopupChatDialog : public ChatDialog
{
	Q_OBJECT

	friend class ChatDialog;

protected slots:
    void showAvatarFrame(bool show);
private slots:
	void clearOfflineMessages();
    void chatStatusChanged(const ChatId &chat_id, const QString &statusString);

protected:
	/** Default constructor */
	PopupChatDialog(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	/** Default destructor */
	virtual ~PopupChatDialog();

	virtual void init(const ChatId &chat_id, const QString &title);
	virtual void showDialog(uint chatflags);
	virtual ChatWidget *getChatWidget();
	virtual bool hasPeerStatus() { return true; }
	virtual bool notifyBlink();

	virtual void updateStatus(int /*status*/) {}

	void processSettings(bool load);

protected:
    virtual void addChatMsg(const ChatMessage& msg);
    //virtual void onChatChanged(int list, int type);

protected:
	bool manualDelete;

	/** Qt Designer generated object */
	Ui::PopupChatDialog ui;
};

#endif
