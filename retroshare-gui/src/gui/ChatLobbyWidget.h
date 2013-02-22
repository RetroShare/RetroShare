#pragma once

#include <QTreeWidget>
#include "ui_ChatLobbyWidget.h"
#include "RsAutoUpdatePage.h"

class RSTreeWidgetItemCompareRole;
class ChatTabWidget ;

class ChatLobbyWidget : public RsAutoUpdatePage, Ui::ChatLobbyWidget
{
	Q_OBJECT

public:
	/** Default constructor */
	ChatLobbyWidget(QWidget *parent = 0, Qt::WFlags flags = 0);

	/** Default destructor */
	~ChatLobbyWidget();

	virtual void updateDisplay();

	static ChatTabWidget *getTabWidget() ;
protected slots:
	void lobbyChanged();
	void lobbyTreeWidgetCostumPopupMenu();
	void createChatLobby();
	void subscribeItem();
	void unsubscribeItem();
	void itemDoubleClicked(QTreeWidgetItem *item, int column);
	void displayChatLobbyEvent(qulonglong lobby_id, int event_type, const QString& nickname, const QString& str);
	void readChatLobbyInvites();

private:
	RSTreeWidgetItemCompareRole *compareRole;
	QTreeWidgetItem *privateLobbyItem;
	QTreeWidgetItem *publicLobbyItem;

	ChatTabWidget *tabWidget ;
	QTreeWidget *lobbyTreeWidget ;
};
