/*******************************************************************************
 * gui/chat/ChatLobbyUserNotify.h                                              *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2014 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef CHATLOBBYUSERNOTIFY_H
#define CHATLOBBYUSERNOTIFY_H

#include "gui/common/UserNotify.h"
#include <retroshare/rsmsgs.h>
#include <QDateTime>
#include <QMetaType>

struct ActionTag {
	ChatLobbyId cli;
	QString timeStamp;
	bool removeALL;
};
Q_DECLARE_METATYPE(ActionTag)

struct MsgData {
	QString text;
	bool unread;
};
Q_DECLARE_METATYPE(MsgData)

class ChatLobbyUserNotify : public UserNotify
{
	Q_OBJECT

public:
	ChatLobbyUserNotify(QObject *parent = 0);

	virtual bool hasSetting(QString *name, QString *group);
	QMenu* createMenu();
	void makeSubMenu(QMenu* parentMenu, QIcon icoLobby, QString strLobbyName, ChatLobbyId id);
	void chatLobbyNewMessage(ChatLobbyId lobby_id, QDateTime time, QString senderName, QString msg);
	void chatLobbyCleared(ChatLobbyId lobby_id, QString anchor, bool onlyUnread=false);
	void setCheckForNickName(bool value);
	bool isCheckForNickName() { return _bCheckForNickName;}
	void setCountUnRead(bool value);
	bool isCountUnRead() { return _bCountUnRead;}
	void setCountSpecificText(bool value);
	bool isCountSpecificText() { return _bCountSpecificText;}
	void setTextToNotify(QStringList);
	void setTextToNotify(QString);
	QString textToNotify() { return _textToNotify.join("\n");}
	void setTextCaseSensitive(bool value);
	bool isTextCaseSensitive() {return _bTextCaseSensitive;}

signals:
	void countChanged(ChatLobbyId id, unsigned int count);

private slots:
	void subMenuClicked(QAction* action);
	void subMenuHovered(QAction* action);

private:
	virtual QIcon getIcon();
	virtual QIcon getMainIcon(bool hasNew);
	virtual unsigned int getNewCount();
	virtual QString getTrayMessage(bool plural);
	virtual QString getNotifyMessage(bool plural);
	virtual void iconClicked();
	virtual void iconHovered();
	bool checkWord(QString msg, QString word);

	QString _name;
	QString _group;

	typedef std::map<QString, MsgData> msg_map;
	typedef	std::map<ChatLobbyId, msg_map> lobby_map;
	lobby_map _listMsg;

	bool _bCountUnRead;
	bool _bCheckForNickName;
	bool _bCountSpecificText;
	QStringList _textToNotify;
	bool _bTextCaseSensitive;
};

#endif // CHATLOBBYUSERNOTIFY_H
