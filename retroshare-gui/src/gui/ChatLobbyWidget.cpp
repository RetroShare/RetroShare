#include <QTreeWidget>
#include <QMenu>
#include <QMessageBox>
#include "ChatLobbyWidget.h"
#include "chat/CreateLobbyDialog.h"
#include "common/RSTreeWidgetItem.h"
#include "notifyqt.h"
#include "chat/ChatLobbyDialog.h"

#include "retroshare/rsmsgs.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsnotify.h"

#define COLUMN_NAME       0
#define COLUMN_USER_COUNT 1
#define COLUMN_TOPIC      2
#define COLUMN_COUNT      3

#define COLUMN_DATA       0
#define ROLE_SORT         Qt::UserRole
#define ROLE_ID           Qt::UserRole + 1
#define ROLE_SUBSCRIBED   Qt::UserRole + 2
#define ROLE_PRIVACYLEVEL Qt::UserRole + 3

#define TYPE_FOLDER       0
#define TYPE_LOBBY        1

#define IMAGE_CREATE      ""
#define IMAGE_PUBLIC      ""
#define IMAGE_PRIVATE     ""
#define IMAGE_SUBSCRIBE   ""
#define IMAGE_UNSUBSCRIBE ""

ChatLobbyWidget::ChatLobbyWidget(QWidget *parent, Qt::WFlags flags)
	: RsAutoUpdatePage(5000, parent, flags)
{
	setupUi(this);

	QObject::connect(NotifyQt::getInstance(), SIGNAL(lobbyListChanged()), SLOT(lobbyChanged()));
	QObject::connect(NotifyQt::getInstance(), SIGNAL(chatLobbyEvent(qulonglong,int,const QString&,const QString&)), this, SLOT(displayChatLobbyEvent(qulonglong,int,const QString&,const QString&)));
	QObject::connect(NotifyQt::getInstance(), SIGNAL(chatLobbyInviteReceived()), this, SLOT(readChatLobbyInvites()));

	QObject::connect(lobbyTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(lobbyTreeWidgetCostumPopupMenu()));
	QObject::connect(lobbyTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));

	QObject::connect(newlobbytoolButton, SIGNAL(clicked()), this, SLOT(createChatLobby()));

	compareRole = new RSTreeWidgetItemCompareRole;
	compareRole->setRole(COLUMN_NAME, ROLE_SORT);

	lobbyTreeWidget->setColumnCount(COLUMN_COUNT);
	lobbyTreeWidget->sortItems(COLUMN_NAME, Qt::AscendingOrder);

	QTreeWidgetItem *headerItem = lobbyTreeWidget->headerItem();
	headerItem->setText(COLUMN_NAME, tr("Name"));
	headerItem->setText(COLUMN_USER_COUNT, tr("Count"));
	headerItem->setText(COLUMN_TOPIC, tr("Topic"));
	headerItem->setTextAlignment(COLUMN_NAME, Qt::AlignHCenter | Qt::AlignVCenter);
  headerItem->setTextAlignment(COLUMN_TOPIC, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(COLUMN_USER_COUNT, Qt::AlignHCenter | Qt::AlignVCenter);

	QHeaderView *header = lobbyTreeWidget->header();
	header->setResizeMode(COLUMN_NAME, QHeaderView::Interactive);
	header->setResizeMode(COLUMN_USER_COUNT, QHeaderView::Interactive);
	header->setResizeMode(COLUMN_TOPIC, QHeaderView::Stretch);

	lobbyTreeWidget->setColumnWidth(COLUMN_NAME, 200);
	lobbyTreeWidget->setColumnWidth(COLUMN_USER_COUNT, 50);

	privateLobbyItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
	privateLobbyItem->setText(COLUMN_NAME, tr("Private Lobbies"));
	privateLobbyItem->setData(COLUMN_NAME, ROLE_SORT, "1");
	privateLobbyItem->setData(COLUMN_DATA, ROLE_PRIVACYLEVEL, RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE);
	lobbyTreeWidget->insertTopLevelItem(0, privateLobbyItem);

	publicLobbyItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
	publicLobbyItem->setText(COLUMN_NAME, tr("Public Lobbies"));
	publicLobbyItem->setData(COLUMN_NAME, ROLE_SORT, "2");
	publicLobbyItem->setData(COLUMN_DATA, ROLE_PRIVACYLEVEL, RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC);
	lobbyTreeWidget->insertTopLevelItem(1, publicLobbyItem);

	lobbyTreeWidget->expandAll();

	lobbyChanged();
}

ChatLobbyWidget::~ChatLobbyWidget()
{
	if (compareRole) {
		delete(compareRole);
	}
}

void ChatLobbyWidget::lobbyTreeWidgetCostumPopupMenu()
{
	QTreeWidgetItem *item = lobbyTreeWidget->currentItem();

	QMenu contextMnu(this);

	if (item && item->type() == TYPE_FOLDER) {
		QAction *action = contextMnu.addAction(QIcon(IMAGE_CREATE), tr("Create chat lobby"), this, SLOT(createChatLobby()));
		action->setData(item->data(COLUMN_DATA, ROLE_PRIVACYLEVEL).toInt());
	}

	if (item && item->type() == TYPE_LOBBY) {
		if (item->data(COLUMN_DATA, ROLE_SUBSCRIBED).toBool()) {
			contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe"), this, SLOT(unsubscribeItem()));
		} else {
			contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe"), this, SLOT(subscribeItem()));
		}
	}

	if (contextMnu.children().count() == 0) {
		return;
	}

	contextMnu.exec(QCursor::pos());
}

void ChatLobbyWidget::lobbyChanged()
{
	updateDisplay();
}

static void updateItem(QTreeWidgetItem *item, ChatLobbyId id, const std::string &name, const std::string &topic, int count, bool subscribed)
{
	item->setText(COLUMN_NAME, QString::fromUtf8(name.c_str()));
	item->setData(COLUMN_NAME, ROLE_SORT, QString::fromUtf8(name.c_str()));

	if(topic.empty())
	{
		item->setText(COLUMN_TOPIC, qApp->translate("ChatLobbyWidget", "[No topic provided]"));
		item->setData(COLUMN_TOPIC, ROLE_SORT, qApp->translate("ChatLobbyWidget", "[No topic provided]"));
	}
	else
	{
		item->setText(COLUMN_TOPIC, QString::fromUtf8(topic.c_str()));
		item->setData(COLUMN_TOPIC, ROLE_SORT, QString::fromUtf8(topic.c_str()));
	}

	item->setText(COLUMN_USER_COUNT, QString::number(count));

	item->setData(COLUMN_DATA, ROLE_ID, (qulonglong)id);
	item->setData(COLUMN_DATA, ROLE_SUBSCRIBED, subscribed);

	for (int column = 0; column < COLUMN_COUNT; ++column) {
		item->setTextColor(column, subscribed ? QColor() : QColor(Qt::gray));
	}
}

void ChatLobbyWidget::updateDisplay()
{
#ifdef CHAT_LOBBY_GUI_DEBUG
	std::cerr << "updating chat lobby display!" << std::endl;
#endif
	std::vector<VisibleChatLobbyRecord> visibleLobbies;
	rsMsgs->getListOfNearbyChatLobbies(visibleLobbies);

	std::list<ChatLobbyInfo> lobbies;
	rsMsgs->getChatLobbyList(lobbies);

#ifdef CHAT_LOBBY_GUI_DEBUG
	std::cerr << "got " << visibleLobbies.size() << " visible lobbies, and " << lobbies.size() << " private lobbies." << std::endl;
#endif

	// now, do a nice display of lobbies
	
	std::string vpid;
	uint32_t i;
	uint32_t size = visibleLobbies.size();
	std::list<ChatLobbyInfo>::const_iterator lobbyIt;

	// remove not existing public lobbies

	for(int p=0;p<2;++p)
	{
		QTreeWidgetItem *lobby_item = (p==0)?publicLobbyItem:privateLobbyItem ;

		int childCnt = lobby_item->childCount();
		int childIndex = 0;

		while (childIndex < childCnt) {
			QTreeWidgetItem *itemLoop = lobby_item->child(childIndex);
			if (itemLoop->type() == TYPE_LOBBY) 
			{
				// check for visible lobby
				for (i = 0; i < size; ++i) 
					if (itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == visibleLobbies[i].lobby_id) 
						break;

				if (i >= size) 
				{
					// Check for participating lobby with public level
					//
					for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) 
						if(itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobbyIt->lobby_id) 
							break;

					if (lobbyIt == lobbies.end()) 
					{
						delete(lobby_item->takeChild(lobby_item->indexOfChild(itemLoop)));
						childCnt = lobby_item->childCount();
						continue;
					}
				}
			}
			childIndex++;
		}
	}

	// Now add visible lobbies
	//
	for (i = 0; i < size; ++i) 
	{
		const VisibleChatLobbyRecord &lobby = visibleLobbies[i];

#ifdef CHAT_LOBBY_GUI_DEBUG
		std::cerr << "adding " << lobby.lobby_name << "topic " << lobby.lobby_topic << " #" << std::hex << lobby.lobby_id << std::dec << " public " << lobby.total_number_of_peers << " peers. Lobby type: " << lobby.lobby_privacy_level << std::endl;
#endif

		QTreeWidgetItem *item = NULL;
		QTreeWidgetItem *lobby_item = (visibleLobbies[i].lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC)?publicLobbyItem:privateLobbyItem ;

		// Search existing item
		//
		int childCnt = lobby_item->childCount();
		for (int childIndex = 0; childIndex < childCnt; childIndex++) 
		{
			QTreeWidgetItem *itemLoop = lobby_item->child(childIndex);
			if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobby.lobby_id) {
				item = itemLoop;
				break;
			}
		}

		if (item == NULL) {
			item = new RSTreeWidgetItem(compareRole, TYPE_LOBBY);
			lobby_item->addChild(item);
		}

		if(lobby_item == publicLobbyItem)
			item->setIcon(COLUMN_NAME, QIcon(IMAGE_PUBLIC));
		else
			item->setIcon(COLUMN_NAME, QIcon(IMAGE_PRIVATE));

		bool subscribed = false;
		if (rsMsgs->getVirtualPeerId(lobby.lobby_id, vpid)) {
			subscribed = true;
		}

		updateItem(item, lobby.lobby_id, lobby.lobby_name,lobby.lobby_topic, lobby.total_number_of_peers, subscribed);
	}

	// Now add participating lobbies.
	//
	for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) 
	{
		const ChatLobbyInfo &lobby = *lobbyIt;

#ifdef CHAT_LOBBY_GUI_DEBUG
		std::cerr << "adding " << lobby.lobby_name << "topic " << lobby.lobby_topic << " #" << std::hex << lobby.lobby_id << std::dec << " private " << lobby.nick_names.size() << " peers." << std::endl;
#endif

		QTreeWidgetItem *itemParent;
		if (lobby.lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC) {
			itemParent = publicLobbyItem;
		} else {
			itemParent = privateLobbyItem;
		}

		QTreeWidgetItem *item = NULL;

		// search existing item
		int childCount = itemParent->childCount();
		for (int childIndex = 0; childIndex < childCount; childIndex++) {
			QTreeWidgetItem *itemLoop = itemParent->child(childIndex);
			if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobby.lobby_id) {
				item = itemLoop;
				break;
			}
		}

		if (item == NULL) {
			item = new RSTreeWidgetItem(compareRole, TYPE_LOBBY);
			itemParent->addChild(item);
		}

		if (lobby.lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC) {
			item->setIcon(COLUMN_NAME, QIcon(IMAGE_PUBLIC));
		} else {
			item->setIcon(COLUMN_NAME, QIcon(IMAGE_PRIVATE));
		}

		updateItem(item, lobby.lobby_id, lobby.lobby_name,lobby.lobby_topic, lobby.nick_names.size(), true);
	}
}

void ChatLobbyWidget::createChatLobby()
{
	int privacyLevel = 0;
	QAction *action = qobject_cast<QAction*>(sender());
	if (action) {
		privacyLevel = action->data().toInt();
	}

	std::list<std::string> friends;
	CreateLobbyDialog(friends, privacyLevel).exec();
}

static void subscribeLobby(QTreeWidgetItem *item)
{
	if (item == NULL && item->type() != TYPE_LOBBY) {
		return;
	}

	ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();
	if (rsMsgs->joinVisibleChatLobby(id)) {
		std::string vpeer_id;
		if (rsMsgs->getVirtualPeerId(id, vpeer_id)) {
			ChatDialog::chatFriend(vpeer_id) ;
		}
	}
}

void ChatLobbyWidget::subscribeItem()
{
	subscribeLobby(lobbyTreeWidget->currentItem());
}

void ChatLobbyWidget::unsubscribeItem()
{
	QTreeWidgetItem *item = lobbyTreeWidget->currentItem();
	if (item == NULL && item->type() != TYPE_LOBBY) {
		return;
	}

	const ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();

	std::string vpeer_id;
	if (rsMsgs->getVirtualPeerId(id, vpeer_id)) {
		ChatDialog::closeChat(vpeer_id);
	}

	rsMsgs->unsubscribeChatLobby(id);
}

void ChatLobbyWidget::itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
	subscribeLobby(item);
}

void ChatLobbyWidget::displayChatLobbyEvent(qulonglong lobby_id, int event_type, const QString& nickname, const QString& str)
{
	std::cerr << "Received displayChatLobbyEvent()!" << std::endl;

	std::string vpid;
	if (rsMsgs->getVirtualPeerId(lobby_id, vpid)) {
		if (ChatLobbyDialog *cld = dynamic_cast<ChatLobbyDialog*>(ChatDialog::getExistingChat(vpid))) {
			cld->displayLobbyEvent(event_type, nickname, str);
		}
	}
}

void ChatLobbyWidget::readChatLobbyInvites()
{
	std::list<ChatLobbyInvite> invites;
	rsMsgs->getPendingChatLobbyInvites(invites);

	for(std::list<ChatLobbyInvite>::const_iterator it(invites.begin());it!=invites.end();++it) {
		if (QMessageBox::Ok == QMessageBox::question(this, tr("Invitation to chat lobby"), tr("%1  invites you to chat lobby named %2").arg(QString::fromUtf8(rsPeers->getPeerName((*it).peer_id).c_str())).arg(QString::fromUtf8((*it).lobby_name.c_str())), QMessageBox::Ok, QMessageBox::Ignore)) {
			std::cerr << "Accepting invite to lobby " << (*it).lobby_name << std::endl;

			rsMsgs->acceptLobbyInvite((*it).lobby_id);

			std::string vpid;
			if(rsMsgs->getVirtualPeerId((*it).lobby_id,vpid )) {
				ChatDialog::chatFriend(vpid);
			} else {
				std::cerr << "No lobby known with id 0x" << std::hex << (*it).lobby_id << std::dec << std::endl;
			}
		} else {
			rsMsgs->denyLobbyInvite((*it).lobby_id);
		}
	}
}
