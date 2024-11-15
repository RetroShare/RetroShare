/*******************************************************************************
 * gui/ChatLobbyWidget.h                                                       *
 *                                                                             *
 * Copyright (C) 2012 Retroshare Team <retroshare.project@gmail.com>           *
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
#pragma once

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "ui_ChatLobbyWidget.h"

#include "chat/ChatLobbyUserNotify.h"
#include "gui/gxs/GxsIdChooser.h"


#include <retroshare/rsmsgs.h>

#include <QAbstractButton>
#include <QTreeWidget>

#define IMAGE_CHATLOBBY			    ":/icons/png/chat-lobbies.png"

#define CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC  1
#define CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE 2

class RSTreeWidgetItemCompareRole;
class ChatTabWidget ;
class ChatDialog ;
class ChatLobbyDialog ;
class QTextBrowser ;

struct ChatLobbyInfoStruct
{
	QIcon default_icon ;
	ChatLobbyDialog *dialog ;
	time_t last_typing_event ;
};

class ChatLobbyWidget : public RsAutoUpdatePage
{
	Q_OBJECT

public:
	/** Default constructor */
	ChatLobbyWidget(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());

	/** Default destructor */
	~ChatLobbyWidget();

	virtual QIcon iconPixmap() const override { return QIcon(IMAGE_CHATLOBBY) ; } //MainPage
	virtual QString pageName() const override { return tr("Chats") ; } //MainPage
	virtual QString helpText() const override { return ""; } //MainPage

	virtual UserNotify *createUserNotify(QObject *parent) override; //MainPage

	virtual void updateDisplay() override; //RsAutoUpdatePage

	void setCurrentChatPage(ChatLobbyDialog *) ;	// used by ChatLobbyDialog to raise.
	void addChatPage(ChatLobbyDialog *) ;
	void removeChatPage(ChatLobbyDialog *) ;
	bool showLobbyAnchor(ChatLobbyId id, QString anchor) ;

	uint unreadCount();

signals:
	void unreadCountChanged(uint unreadCount);

protected slots:
	void dialogClose(ChatDialog*);
	void lobbyChanged();
	void lobbyTreeWidgetCustomPopupMenu(QPoint);
	void createChatLobby();
	void subscribeItem();
	void unsubscribeItem();
	void itemDoubleClicked(QTreeWidgetItem *item, int column);
	void updateCurrentLobby() ;
    void displayChatLobbyEvent(qulonglong lobby_id, int event_type, const RsGxsId& gxs_id, const QString& str);
	void readChatLobbyInvites();
	void showLobby(QTreeWidgetItem *lobby_item) ;
	void showBlankPage(ChatLobbyId id, bool subscribed = false) ;
    void unsubscribeChatLobby(ChatLobbyId id) ;
    void createIdentityAndSubscribe();
    void subscribeChatLobbyAs() ;
    void updateTypingStatus(ChatLobbyId id) ;
	void resetLobbyTreeIcons() ;
	void updateMessageChanged(bool incoming, ChatLobbyId, QDateTime time, QString senderName, QString msg);
	void updatePeerEntering(ChatLobbyId);
	void updatePeerLeaving(ChatLobbyId);
	void autoSubscribeItem();
	void copyItemLink();

private slots:
	void filterColumnChanged(int);
	void filterItems(const QString &text);
	
    void setShowUserCountColumn(bool show);
    void setShowTopicColumn(bool show);

	void updateNotify(ChatLobbyId id, unsigned int count) ;
	void idChooserCurrentIndexChanged(int index);

private:
	void autoSubscribeLobby(QTreeWidgetItem *item);
    void subscribeChatLobby(ChatLobbyId id) ;
    void subscribeChatLobbyAtItem(QTreeWidgetItem *item) ;

	bool filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn);

	RSTreeWidgetItemCompareRole *compareRole;
	QTreeWidgetItem *privateLobbyItem;
	QTreeWidgetItem *publicLobbyItem;
    QTreeWidgetItem *privateSubLobbyItem;
    QTreeWidgetItem *publicSubLobbyItem;
	QTreeWidgetItem *getTreeWidgetItem(ChatLobbyId);

	ChatTabWidget *tabWidget ;

	std::map<ChatLobbyId,ChatLobbyInfoStruct> _lobby_infos ;

	std::map<QTreeWidgetItem*,time_t> _icon_changed_map ;

    bool m_bProcessSettings;
    void processSettings(bool bLoad);

    /** Defines the actions for the header context menu */
    QAction* showUserCountAct;
	QAction* showTopicAct;
	int getNumColVisible();

	ChatLobbyUserNotify* myChatLobbyUserNotify; // local copy that avoids dynamic casts

	QAbstractButton* myInviteYesButton;
	GxsIdChooser* myInviteIdChooser;

	/* UI - from Designer */
	Ui::ChatLobbyWidget ui;
};

