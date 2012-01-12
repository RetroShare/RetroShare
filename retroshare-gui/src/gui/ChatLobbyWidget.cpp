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
#define COLUMN_COUNT      2

#define COLUMN_DATA       0
#define ROLE_SORT         Qt::UserRole
#define ROLE_ID           Qt::UserRole + 1
#define ROLE_SUBSCRIBED   Qt::UserRole + 2

#define TYPE_FOLDER       0
#define TYPE_LOBBY        1

#define IMAGE_CREATE      ""
#define IMAGE_PUBLIC      ""
#define IMAGE_PRIVATE     ""
#define IMAGE_SUBSCRIBE   ""
#define IMAGE_UNSUBSCRIBE ""

static ChatLobbyWidget *instance = NULL;

ChatLobbyWidget::ChatLobbyWidget(QWidget *parent, Qt::WFlags flags)
	: RsAutoUpdatePage(5000,parent)
{
	setupUi(this);

	if (instance == NULL) {
		instance = this;
	}

	QObject::connect(NotifyQt::getInstance(), SIGNAL(lobbyListChanged()), SLOT(lobbyChanged()));
	QObject::connect(NotifyQt::getInstance(), SIGNAL(chatLobbyEvent(qulonglong,int,const QString&,const QString&)), this, SLOT(displayChatLobbyEvent(qulonglong,int,const QString&,const QString&)));
	QObject::connect(NotifyQt::getInstance(), SIGNAL(chatLobbyInviteReceived()), this, SLOT(readChatLobbyInvites()));

	QObject::connect(lobbyTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(lobbyTreeWidgetCostumPopupMenu()));
	QObject::connect(lobbyTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
	QObject::connect(lobbyTabWidget, SIGNAL(infoChanged()), this, SLOT(tabInfoChanged()));

	compareRole = new RSTreeWidgetItemCompareRole;
	compareRole->setRole(COLUMN_NAME, ROLE_SORT);

	lobbyTreeWidget->setColumnCount(COLUMN_COUNT);
	lobbyTreeWidget->sortItems(COLUMN_NAME, Qt::AscendingOrder);

	QTreeWidgetItem *headerItem = lobbyTreeWidget->headerItem();
	headerItem->setText(COLUMN_NAME, tr("Name"));
	headerItem->setText(COLUMN_USER_COUNT, tr("Count"));
	headerItem->setTextAlignment(COLUMN_NAME, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(COLUMN_USER_COUNT, Qt::AlignHCenter | Qt::AlignVCenter);

	QHeaderView *header = lobbyTreeWidget->header();
	header->setResizeMode(COLUMN_NAME, QHeaderView::Interactive);
	header->setResizeMode(COLUMN_USER_COUNT, QHeaderView::Stretch);

	lobbyTreeWidget->setColumnWidth(COLUMN_NAME, 200);
	lobbyTreeWidget->setColumnWidth(COLUMN_USER_COUNT, 50);

	privateLobbyItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
	privateLobbyItem->setText(COLUMN_NAME, tr("Private Lobbies"));
	privateLobbyItem->setData(COLUMN_NAME, ROLE_SORT, "1");
	lobbyTreeWidget->insertTopLevelItem(0, privateLobbyItem);

	publicLobbyItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
	publicLobbyItem->setText(COLUMN_NAME, tr("Public Lobbies"));
	publicLobbyItem->setData(COLUMN_NAME, ROLE_SORT, "2");
	lobbyTreeWidget->insertTopLevelItem(1, publicLobbyItem);

	lobbyTreeWidget->expandAll();

	lobbyChanged();
}

ChatLobbyWidget::~ChatLobbyWidget()
{
	if (this == instance) {
		instance = NULL;
	}
}

/*static*/ ChatTabWidget *ChatLobbyWidget::getTabWidget()
{
	return instance ? instance->lobbyTabWidget : NULL;
}

void ChatLobbyWidget::lobbyTreeWidgetCostumPopupMenu()
{
	QMenu contextMnu(this);

	contextMnu.addAction(QIcon(IMAGE_CREATE), tr("Create chat lobby"), this, SLOT(createChatLobby()));

	QTreeWidgetItem *item = lobbyTreeWidget->currentItem();
	if (item && item->type() == TYPE_LOBBY) {
		if (item->data(COLUMN_DATA, ROLE_SUBSCRIBED).toBool()) {
			contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe"), this, SLOT(unsubscribeItem()));
		} else {
			contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe"), this, SLOT(subscribeItem()));
		}
	}
	

	contextMnu.exec(QCursor::pos());
}

void ChatLobbyWidget::lobbyChanged()
{
	updateDisplay();
}

static void updateItem(QTreeWidgetItem *item, ChatLobbyId id, const std::string &name, int count, bool subscribed)
{
	item->setText(COLUMN_NAME, QString::fromUtf8(name.c_str()));
	item->setData(COLUMN_NAME, ROLE_SORT, QString::fromUtf8(name.c_str()));

	item->setText(COLUMN_USER_COUNT, QString::number(count));

	item->setData(COLUMN_DATA, ROLE_ID, (qulonglong)id);
	item->setData(COLUMN_DATA, ROLE_SUBSCRIBED, subscribed);

	for (int column = 0; column < COLUMN_COUNT; ++column) {
		item->setTextColor(column, subscribed ? QColor() : QColor(Qt::gray));
	}
}

void ChatLobbyWidget::updateDisplay()
{
	std::cerr << "updating chat lobby display!" << std::endl;
	std::vector<PublicChatLobbyRecord> publicLobbies;
	rsMsgs->getListOfNearbyChatLobbies(publicLobbies);

	std::list<ChatLobbyInfo> lobbies;
	rsMsgs->getChatLobbyList(lobbies);

	std::cerr << "got " << publicLobbies.size() << " public lobbies, and " << lobbies.size() << " private lobbies." << std::endl;

	// now, do a nice display of lobbies
	
	std::string vpid;
	uint32_t i;
	uint32_t size = publicLobbies.size();
	std::list<ChatLobbyInfo>::const_iterator lobbyIt;

	// remove not existing public lobbies
	int childCount = publicLobbyItem->childCount();
	int childIndex = 0;
	while (childIndex < childCount) {
		QTreeWidgetItem *itemLoop = publicLobbyItem->child(childIndex);
		if (itemLoop->type() == TYPE_LOBBY) {
			// check for public lobby
			for (i = 0; i < size; ++i) {
				if (itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == publicLobbies[i].lobby_id) {
					break;
				}
			}

			if (i >= size) {
				// check for private lobby with public level
				for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) {
					if (lobbyIt->lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC &&
						itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobbyIt->lobby_id) {
						break;
					}
				}

				if (lobbyIt == lobbies.end()) {
					delete(publicLobbyItem->takeChild(publicLobbyItem->indexOfChild(itemLoop)));
					childCount = publicLobbyItem->childCount();
					continue;
				}
			}
		}
		childIndex++;
	}

	for (i = 0; i < size; ++i) {
		const PublicChatLobbyRecord &lobby = publicLobbies[i];

		std::cerr << "adding " << lobby.lobby_name << " #" << std::hex << lobby.lobby_id << std::dec << " public " << lobby.total_number_of_peers << " peers." << std::endl;

		QTreeWidgetItem *item = NULL;

		// search existing item
		childCount = publicLobbyItem->childCount();
		for (childIndex = 0; childIndex < childCount; childIndex++) {
			QTreeWidgetItem *itemLoop = publicLobbyItem->child(childIndex);
			if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobby.lobby_id) {
				item = itemLoop;
				break;
			}
		}

		if (item == NULL) {
			item = new RSTreeWidgetItem(compareRole, TYPE_LOBBY);
			publicLobbyItem->addChild(item);
		}

		item->setIcon(COLUMN_NAME, QIcon(IMAGE_PUBLIC));

		bool subscribed = false;
		if (rsMsgs->getVirtualPeerId(lobby.lobby_id, vpid)) {
			subscribed = true;
		}

		updateItem(item, lobby.lobby_id, lobby.lobby_name, lobby.total_number_of_peers, subscribed);
	}

	// remove not existing private lobbies
	childCount = privateLobbyItem->childCount();
	childIndex = 0;
	while (childIndex < childCount) {
		QTreeWidgetItem *itemLoop = privateLobbyItem->child(childIndex);
		if (itemLoop->type() == TYPE_LOBBY) {
			for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) {
				if (lobbyIt->lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE &&
					itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobbyIt->lobby_id) {
					break;
				}
			}

			if (lobbyIt == lobbies.end()) {
				delete(privateLobbyItem->takeChild(privateLobbyItem->indexOfChild(itemLoop)));
				childCount = privateLobbyItem->childCount();
				continue;
			}
		}
		childIndex++;
	}

	for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) {
		const ChatLobbyInfo &lobby = *lobbyIt;

		std::cerr << "adding " << lobby.lobby_name << " #" << std::hex << lobby.lobby_id << std::dec << " private " << lobby.nick_names.size() << " peers." << std::endl;

		QTreeWidgetItem *itemParent;
		if (lobby.lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC) {
			itemParent = publicLobbyItem;
		} else {
			itemParent = privateLobbyItem;
		}

		QTreeWidgetItem *item = NULL;

		// search existing item
		childCount = itemParent->childCount();
		for (childIndex = 0; childIndex < childCount; childIndex++) {
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

		updateItem(item, lobby.lobby_id, lobby.lobby_name, lobby.nick_names.size(), true);
	}
}

void ChatLobbyWidget::createChatLobby()
{
	std::list<std::string> friends;
	CreateLobbyDialog(friends).exec();
}

void ChatLobbyWidget::subscribeItem()
{
	QTreeWidgetItem *item = lobbyTreeWidget->currentItem();
	if (item == NULL && item->type() != TYPE_LOBBY) {
		return;
	}

	ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();
	if (rsMsgs->joinPublicChatLobby(id)) {
		std::string vpeer_id;
		if (rsMsgs->getVirtualPeerId(id, vpeer_id)) {
			PopupChatDialog::chatFriend(vpeer_id) ;
		}
	}
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
		PopupChatDialog::closeChat(vpeer_id);
	}

	rsMsgs->unsubscribeChatLobby(id);
}

void ChatLobbyWidget::itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	// Each lobby can be joined directly, by calling
	// 	rsMsgs->joinPublicLobby(chatLobbyId) ;

	// e.g. fill a list of public lobbies

	// also maintain a list of active chat lobbies. Each active (subscribed) lobby has a lobby tab in the gui.
	// Each tab knows its lobby id and its virtual peer id (the one to send private chat messages to)
	//
	// One possibility is to convert ChatLobbyDialog to be used at a chat lobby tab.

	// then the lobby can be accessed using the virtual peer id through
	// 	rsMsgs->getVirtualPeerId(ChatLobbyId,std::string& virtual_peer_id)
}

void ChatLobbyWidget::displayChatLobbyEvent(qulonglong lobby_id, int event_type, const QString& nickname, const QString& str)
{
	std::cerr << "Received displayChatLobbyEvent()!" << std::endl;

	std::string vpid;
	if (rsMsgs->getVirtualPeerId(lobby_id, vpid)) {
		if (ChatLobbyDialog *cld = dynamic_cast<ChatLobbyDialog*>(PopupChatDialog::getExistingInstance(vpid))) {
			cld->displayLobbyEvent(event_type, nickname, str);
		}
	}
}

void ChatLobbyWidget::readChatLobbyInvites()
{
	std::list<ChatLobbyInvite> invites;
	rsMsgs->getPendingChatLobbyInvites(invites);

	for(std::list<ChatLobbyInvite>::const_iterator it(invites.begin());it!=invites.end();++it) {
		if (QMessageBox::Ok == QMessageBox::question(NULL, tr("Invitation to chat lobby"), QString::fromUtf8(rsPeers->getPeerName((*it).peer_id).c_str()) + QString(" invites you to chat lobby named ") + QString::fromUtf8((*it).lobby_name.c_str()), QMessageBox::Ok, QMessageBox::Ignore)) {
			std::cerr << "Accepting invite to lobby " << (*it).lobby_name << std::endl;

			rsMsgs->acceptLobbyInvite((*it).lobby_id);

			std::string vpid;
			if(rsMsgs->getVirtualPeerId((*it).lobby_id,vpid )) {
				PopupChatDialog::chatFriend(vpid);
			} else {
				std::cerr << "No lobby known with id 0x" << std::hex << (*it).lobby_id << std::dec << std::endl;
			}
		} else {
			rsMsgs->denyLobbyInvite((*it).lobby_id);
		}
	}
}

void ChatLobbyWidget::tabInfoChanged()
{
	emit infoChanged();
}

void ChatLobbyWidget::getInfo(bool &isTyping, bool &hasNewMessage, QIcon *icon)
{
	lobbyTabWidget->getInfo(isTyping, hasNewMessage, icon);
}
