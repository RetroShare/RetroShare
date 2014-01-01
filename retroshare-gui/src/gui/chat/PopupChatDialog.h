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

// a Container for the logic behind buttons in a PopupChatDialog
// Plugins can implement this interface to provide their own buttons
class PopupChatDialog_WidgetsHolder{
public:
    virtual ~PopupChatDialog_WidgetsHolder(){}
    virtual void init(const std::string &peerId, const QString &title, ChatWidget* chatWidget) = 0;
    virtual std::vector<QWidget*> getWidgets() = 0;

    // status comes from notifyPeerStatusChanged
    // see rststaus.h for possible values
    virtual void updateStatus(int status) = 0;
};

class PopupChatDialog : public ChatDialog
{
	Q_OBJECT

	friend class ChatDialog;

public:
    virtual void addWidgets(PopupChatDialog_WidgetsHolder *wh);
    virtual std::vector<PopupChatDialog_WidgetsHolder*> getWidgets();

private slots:
	void showAvatarFrame(bool show);
	void clearOfflineMessages();
	void chatStatusChanged(const QString &peerId, const QString &statusString, bool isPrivateChat);
	void statusChanged(int);

protected:
	/** Default constructor */
	PopupChatDialog(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	/** Default destructor */
	virtual ~PopupChatDialog();

	virtual void init(const std::string &peerId, const QString &title);
	virtual void showDialog(uint chatflags);
	virtual ChatWidget *getChatWidget();
	virtual bool hasPeerStatus() { return true; }
	virtual bool notifyBlink();

	virtual void updateStatus(int /*status*/) {}

	void processSettings(bool load);

	// used by plugins 
	void addChatBarWidget(QWidget *w) ;

protected:
	virtual void addIncomingChatMsg(const ChatInfo& info);
	virtual void onChatChanged(int list, int type);

private:
	bool manualDelete;
	std::list<ChatInfo> savedOfflineChat;
    std::vector<PopupChatDialog_WidgetsHolder*> widgetsHolders;

	/** Qt Designer generated object */
	Ui::PopupChatDialog ui;
};

#endif
