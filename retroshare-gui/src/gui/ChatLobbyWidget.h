#pragma once

#include <QTreeWidget>
#include <retroshare/rsmsgs.h>
#include "ui_ChatLobbyWidget.h"
#include "RsAutoUpdatePage.h"

class RSTreeWidgetItemCompareRole;
class ChatTabWidget ;
class ChatLobbyDialog ;
class QTextBrowser ;

struct ChatLobbyInfoStruct
{
	QIcon default_icon ;
	ChatLobbyDialog *dialog ;
	time_t last_typing_event ;
};

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
	void updateMessageChanged(ChatLobbyId);
	void updatePeerEntering(ChatLobbyId);
	void updatePeerLeaving(ChatLobbyId);
	void autoSubscribeItem();

private slots:
	void filterColumnChanged(int);
	void filterItems(const QString &text);
	

    void setShowUserCountColumn(bool show);
    void setShowTopicColumn(bool show);
    void setShowSubscribeColumn(bool show);

private:
	void autoSubscribeLobby(QTreeWidgetItem *item);

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
    QAction* showSubscribeAct;
    int getNumColVisible();
};

