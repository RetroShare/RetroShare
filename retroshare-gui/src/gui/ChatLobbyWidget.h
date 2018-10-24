#pragma once

#include "ui_ChatLobbyWidget.h"

#include "RsAutoUpdatePage.h"
#include "chat/ChatLobbyUserNotify.h"
#include "gui/gxs/GxsIdChooser.h"
#include "chat/PopupChatDialog.h"

#include <retroshare/rsmsgs.h>

#include <QAbstractButton>
#include <QTreeWidget>

#define IMAGE_CHATLOBBY			    ":/home/img/face_icon/un_chat_icon_x_128.png"

#define CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC  1
#define CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE 2
#define CHAT_LOBBY_ONE2ONE_LEVEL	 3

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

struct ChatOne2OneInfoStruct
{
	QIcon default_icon ;
	PopupChatDialog *dialog;
	time_t last_typing_event;
};

class ChatLobbyWidget : public RsAutoUpdatePage
{
	Q_OBJECT

public:
	/** Default constructor */
	ChatLobbyWidget(QWidget *parent = 0, Qt::WindowFlags flags = 0);

	/** Default destructor */
	~ChatLobbyWidget();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_CHATLOBBY) ; } //MainPage
	virtual QString pageName() const { return tr("Chats") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	virtual UserNotify *getUserNotify(QObject *parent); //MainPage

	virtual void updateDisplay();

	void setCurrentChatPage(ChatLobbyDialog *) ;	// used by ChatLobbyDialog to raise.
	void addChatPage(ChatLobbyDialog *) ;
	bool showLobbyAnchor(ChatLobbyId id, QString anchor) ;

	uint unreadCount();

	void openOne2OneChat(std::string rsId, std::string nickname);
	void addOne2OneChatPage(PopupChatDialog *d);
	void setCurrentOne2OneChatPage(PopupChatDialog *d);
    void updateContactItem(QTreeWidget *treeWidget, QTreeWidgetItem *item, const std::string &nickname, const ChatId& chatId, const std::string &rsId, uint current_time, bool unread);
	void fromGpgIdToChatId(const RsPgpId &gpgId,  ChatId &chatId);

    bool showContactAnchor(RsPeerId id, QString anchor);
signals:
	void unreadCountChanged(uint unreadCount);

protected slots:
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
	void showBlankPage(ChatLobbyId id) ;
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
    void updateRecentTime(const ChatId&, uint);
    //void updateP2PMessageChanged(bool incoming, const ChatId& chatId, QDateTime time, QString senderName, QString msg);
    void updateP2PMessageChanged(ChatMessage);

private slots:
	void filterColumnChanged(int);
	void filterItems(const QString &text);
	
	void setShowUserCountColumn(bool show);
	void setShowTopicColumn(bool show);
	void setShowSubscribeColumn(bool show);

	void updateNotify(ChatLobbyId id, unsigned int count) ;
    void updateNotifyFromP2P(ChatId id, unsigned int count);
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
	QTreeWidgetItem *chatContactItem; //21 Sep 2018 - meiyousixin - add this 'contact' tree for one2one chat
	QTreeWidgetItem *getTreeWidgetItem(ChatLobbyId);
	QTreeWidgetItem *getTreeWidgetItemForChatId(ChatId);

	ChatTabWidget *tabWidget ;

	std::map<ChatLobbyId,ChatLobbyInfoStruct> _lobby_infos ;
	std::map<std::string,ChatOne2OneInfoStruct> _chatOne2One_infos ; // 22 Sep 2018 - meiyousixin - add this for containing all one2one chat widget

	std::map<QTreeWidgetItem*,time_t> _icon_changed_map ;

    bool m_bProcessSettings;
    void processSettings(bool bLoad);

    /** Defines the actions for the header context menu */
    QAction* showUserCountAct;
	QAction* showTopicAct;
	QAction* showSubscribeAct;
	int getNumColVisible();

	ChatLobbyUserNotify* myChatLobbyUserNotify;

	QAbstractButton* myInviteYesButton;
	GxsIdChooser* myInviteIdChooser;

	/* UI - from Designer */
	Ui::ChatLobbyWidget ui;

	void showContactChat(QTreeWidgetItem *item);
    void getHistoryForRecentList();
    void resetAvatarForContactItem(const ChatId &chatId);

    std::set<ChatId> recentUnreadListOfChatId;
};

