/*******************************************************************************
 * gui/chat/ChatDialog.h                                                       *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2011 RetroShare Team <retroshare.project@gmail.com>           *
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
    static ChatDialog *getExistingChat(ChatId id);
    static ChatDialog *getChat(ChatId id, uint chatflags = 0);
	static void cleanupChat();
    static void chatFriend(const ChatId &peerId, bool forceFocus = true);
	static void chatFriend(const RsPgpId &gpgId, bool forceFocus = true);
    static void closeChat(const ChatId &chat_id);
    static void chatMessageReceived(ChatMessage msg);

	virtual void showDialog(uint /*chatflags*/) {}

	virtual ChatWidget *getChatWidget() = 0;
	virtual bool hasPeerStatus() = 0;
	virtual bool notifyBlink() = 0;

	void addToParent(QWidget *newParent);
	void removeFromParent(QWidget *oldParent);

	QString getTitle();
	bool hasNewMessages();
	bool isTyping();

	bool setStyle();
	const RSStyle *getStyle();

	int getPeerStatus();
	void setPeerStatus(uint32_t state);

	void focusDialog();

    ChatId getChatId(){ return mChatId; }

signals:
	void infoChanged(ChatDialog *dialog);
	void newMessage(ChatDialog *dialog);
	void dialogClose(ChatDialog *dialog);

private slots:
	void chatInfoChanged(ChatWidget*);
	void chatNewMessage(ChatWidget*);

protected:
	explicit ChatDialog(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	virtual ~ChatDialog();

	void closeEvent(QCloseEvent *event);
	virtual bool canClose() { return true; }

    virtual QString getPeerName(const ChatId &sslid,QString& additional_info) const ;	// can be overloaded for chat dialogs that have specific peers
    virtual QString getOwnName() const;

    virtual void init(const ChatId &id, const QString &title);
    virtual void addChatMsg(const ChatMessage& msg) = 0;

    ChatId mChatId;
};

class ChatFriendMethod: public QObject
{
    Q_OBJECT
public:
    ChatFriendMethod(QObject* parent, RsPeerId peerId): QObject(parent), mPeerId(peerId){}
public slots:
    void chatFriend(){ChatDialog::chatFriend(ChatId(mPeerId));}
private:
    RsPeerId mPeerId;
};

#endif // CHATDIALOG_H
