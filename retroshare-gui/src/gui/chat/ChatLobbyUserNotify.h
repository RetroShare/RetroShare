/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

struct ActionTag2 {
    RsPeerId cli;
    QString timeStamp;
    bool removeALL;
};
Q_DECLARE_METATYPE(ActionTag2)

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
	void makeSubMenu(QMenu* parentMenu, QIcon icoLobby, QString strLobbyName, ChatLobbyId id);
    void makeSubMenuForP2P(QMenu* parentMenu, QIcon icoLobby, QString strLobbyName, ChatId id);
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

    /* meiyousixin - add more for one2one chat */
    void chatP2PNewMessage(ChatId chatId, QDateTime time, QString senderName, QString msg);
    void chatP2PCleared(ChatId chatId, QString anchor, bool onlyUnread =false);

signals:
	void countChanged(ChatLobbyId id, unsigned int count);
    void countChangedFromP2P(ChatId id, unsigned int count);

private slots:

#ifdef WINDOWS_SYS
	void subMenuClicked(QAction* action);
    void subMenuClickedP2P(QAction* action);
	void subMenuHovered(QAction* action);
  //void subMenuHoveredP2P(QAction* action);
#endif
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

    typedef	std::map<ChatId, msg_map> p2pchat_map;
    p2pchat_map _listP2PMsg;

	bool _bCountUnRead;
	bool _bCheckForNickName;
	bool _bCountSpecificText;
	QStringList _textToNotify;
	bool _bTextCaseSensitive;
};

#endif // CHATLOBBYUSERNOTIFY_H
