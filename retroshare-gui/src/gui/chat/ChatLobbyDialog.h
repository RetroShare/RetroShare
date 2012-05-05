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


#ifndef _CHATLOBBYDIALOG_H
#define _CHATLOBBYDIALOG_H

#include "ui_ChatLobbyDialog.h"
#include "ChatDialog.h"

class ChatLobbyDialog: public ChatDialog
{
	Q_OBJECT 

	friend class ChatDialog;

public:
	void displayLobbyEvent(int event_type, const QString& nickname, const QString& str);

	virtual void showDialog(uint chatflags);
	virtual ChatWidget *getChatWidget();
	virtual bool hasPeerStatus() { return false; }
	void setNickname(const QString &nickname);

private slots:
	void showParticipantsFrame(bool show);

protected:
	/** Default constructor */
	ChatLobbyDialog(const ChatLobbyId& lid, QWidget *parent = 0, Qt::WFlags flags = 0);

	/** Default destructor */
	virtual ~ChatLobbyDialog();

	void processSettings(bool load);

	virtual void init(const std::string &peerId, const QString &title);
	virtual bool canClose();
	virtual void addIncomingChatMsg(const ChatInfo& info);

protected slots:
	void changeNickname();
	void changePartipationState(QListWidgetItem *item);
	
private:
	void updateParticipantsList();

	bool isParticipantMuted(QString &participant);

	ChatLobbyId lobbyId;
	time_t lastUpdateListTime;

	/** Qt Designer generated object */
	Ui::ChatLobbyDialog ui;
	
	/** Ignored Users in Chatlobby */
	QStringList *mutedParticipants;

	
};

#endif
