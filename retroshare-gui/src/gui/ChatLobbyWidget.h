#pragma once

#include <QTreeWidget>
#include <retroshare/rsmsgs.h>
#include "ui_ChatLobbyWidget.h"
#include "RsAutoUpdatePage.h"

class RSTreeWidgetItemCompareRole;
class ChatTabWidget ;
class ChatLobbyDialog ;
class QTextBrowser ;

class ChatLobbyWidget : public RsAutoUpdatePage, Ui::ChatLobbyWidget
{
	Q_OBJECT

public:
	/** Default constructor */
	ChatLobbyWidget(QWidget *parent = 0, Qt::WFlags flags = 0);

	/** Default destructor */
	~ChatLobbyWidget();

	virtual void updateDisplay();

	void setCurrentChatPage(ChatLobbyDialog *) ;	// used by ChatLobbyDialog to raise.
	void addChatPage(ChatLobbyDialog *) ;

protected slots:
	void lobbyChanged();
	void lobbyTreeWidgetCustomPopupMenu(QPoint);
	void createChatLobby();
	void subscribeItem();
	void unsubscribeItem();
	void itemDoubleClicked(QTreeWidgetItem *item, int column);
	void updateCurrentLobby() ;
	void displayChatLobbyEvent(qulonglong lobby_id, int event_type, const QString& nickname, const QString& str);
	void readChatLobbyInvites();
	void showLobby(QTreeWidgetItem *lobby_item) ;
	void showBlankPage(ChatLobbyId id) ;
	void unsubscribeChatLobby(ChatLobbyId id) ;
	void updateTypingStatus(ChatLobbyId id) ;
	void resetLobbyTreeIcons() ;

private:
	RSTreeWidgetItemCompareRole *compareRole;
	QTreeWidgetItem *privateLobbyItem;
	QTreeWidgetItem *publicLobbyItem;
	QTreeWidgetItem *getTreeWidgetItem(ChatLobbyId);

	ChatTabWidget *tabWidget ;

	std::map<ChatLobbyId,ChatLobbyDialog*> _lobby_dialogs ;
	QTextBrowser *_lobby_blank_page ;

	std::map<QTreeWidgetItem*,time_t> _icon_changed_map ;
};

