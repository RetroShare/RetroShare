/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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
	PopupChatDialog(QWidget *parent = 0, Qt::WindowFlags flags = 0);
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
