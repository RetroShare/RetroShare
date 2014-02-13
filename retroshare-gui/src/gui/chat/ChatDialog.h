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

#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QWidget>
#include <retroshare/rsmsgs.h>

class ChatWidget;
class RSStyle;

class ChatDialog : public QWidget
{
	Q_OBJECT

public:
    static ChatDialog *getExistingChat(const RsPeerId &peerId);
    static ChatDialog *getChat(const RsPeerId &peerId, uint chatflags);
	static void cleanupChat();
    static void chatFriend(const RsPeerId &peerId, bool forceFocus = true);
    static void closeChat(const RsPeerId &peerId);
	static void chatChanged(int list, int type);

	virtual void showDialog(uint /*chatflags*/) {}

	virtual ChatWidget *getChatWidget() = 0;
	virtual bool hasPeerStatus() = 0;
	virtual bool notifyBlink() = 0;

	void addToParent(QWidget *newParent);
	void removeFromParent(QWidget *oldParent);

    RsPeerId getPeerId() { return peerId; }
	QString getTitle();
	bool hasNewMessages();
	bool isTyping();

	bool setStyle();
	const RSStyle *getStyle();

	void insertChatMsgs();
	int getPeerStatus();
	void setPeerStatus(uint32_t state);

	void focusDialog();

signals:
	void infoChanged(ChatDialog *dialog);
	void newMessage(ChatDialog *dialog);
	void dialogClose(ChatDialog *dialog);

private slots:
	void chatInfoChanged(ChatWidget*);
	void chatNewMessage(ChatWidget*);

protected:
	explicit ChatDialog(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	virtual ~ChatDialog();

	void closeEvent(QCloseEvent *event);
	virtual bool canClose() { return true; }

    virtual QString getPeerName(const RsPeerId &sslid) const ;	// can be overloaded for chat dialogs that have specific peers

    virtual void init(const RsPeerId &peerId, const QString &title);
	virtual void onChatChanged(int /*list*/, int /*type*/) {}

	virtual void addIncomingChatMsg(const ChatInfo& info) = 0;

    RsPeerId peerId;
};

#endif // CHATDIALOG_H
