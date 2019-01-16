
#include "ChatLobbyWidget.h"

#include "notifyqt.h"
#include "chat/ChatLobbyDialog.h"
#include "chat/ChatLobbyUserNotify.h"
#include "chat/ChatTabWidget.h"
#include "chat/CreateLobbyDialog.h"
#include "common/RSTreeWidgetItem.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/Identity/IdEditDialog.h"
#include "gui/settings/rsharesettings.h"
#include "util/HandleRichText.h"
#include "util/QtVersion.h"

//meiyousixin - add more PopupChatDialog into ChatLobbyWidget
#include "chat/PopupChatDialog.h"
#include "gui/common/AvatarDefs.h"
#include "gui/common/StatusDefs.h"

#include <QPainter>
#include "util/rsdir.h"
#include "retroshare/rsinit.h"
#include <stdio.h>
#include <retroshare/rshistory.h>


#include "retroshare/rsmsgs.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsnotify.h"
#include "retroshare/rsidentity.h"

#include <QGridLayout>
#include <QMenu>
#include <QMessageBox>
#include <QTextBrowser>
#include <QTimer>
#include <QTreeWidget>
#include <QPainter>
#include <algorithm>
#include <time.h>

#define CHAT_LOBBY_GUI_DEBUG 1

#define COLUMN_NAME       0
#define COLUMN_USER_COUNT 1
#define COLUMN_TOPIC      2
#define COLUMN_SUBSCRIBED 3
#define COLUMN_COUNT      4
#define COLUMN_RECENT_TIME      5
#define COLUMN_DATA       0

#define ROLE_SORT          Qt::UserRole
#define ROLE_ID            Qt::UserRole + 1
#define ROLE_SUBSCRIBED    Qt::UserRole + 2
#define ROLE_PRIVACYLEVEL  Qt::UserRole + 3
#define ROLE_AUTOSUBSCRIBE Qt::UserRole + 4
#define ROLE_FLAGS         Qt::UserRole + 5
#define ROLE_ONE2ONE       Qt::UserRole + 6 //meiyousixin - add for one2one


#define TYPE_FOLDER       0
#define TYPE_LOBBY        1
#define TYPE_ONE2ONE      2   //meiyousixin - add for one2one

#define IMAGE_CREATE          ""
#define IMAGE_PUBLIC          ":/chat/img/groundchat.png"               //d: Update unseen icon
#define IMAGE_PRIVATE         ":/chat/img/groundchat_private.png"       //d: Update unseen icon
#define IMAGE_SUBSCRIBE       ":/images/edit_add24.png"  
#define IMAGE_UNSUBSCRIBE     ":/images/cancel.png"
#define IMAGE_PEER_ENTERING   ":/chat/img/personal_add_64.png"          //d: Update unseen icon
#define IMAGE_PEER_LEAVING    ":/chat/img/personal_remove_64.png"       //d: Update unseen icon
#define IMAGE_TYPING		  ":/chat/img/typing.png"                   //d: Update unseen icon
#define IMAGE_MESSAGE	      ":/chat/img/chat_32.png"                  //d: Update unseen icon
#define IMAGE_MESSAGE_PRIVATE ":/chat/img/chat_g_32.png"                //d: Notification icon for private group
#define IMAGE_AUTOSUBSCRIBE   ":/images/accepted16.png"
#define IMAGE_COPYRSLINK      ":/images/copyrslink.png"
#define IMAGE_UNSEEN          ":/app/images/unseen32.png"
#define IMAGE_UNREAD_ICON      ":/home/img/face_icon/un_chat_icon_d_128.png"


#define GUI_DIR_NAME                  "gui"
#define RECENT_CHAT_FILENAME          "recent_chat.txt"
#define UNREAD_CHAT_FILENAME          "unread_chat.txt"

ChatLobbyWidget::ChatLobbyWidget(QWidget *parent, Qt::WindowFlags flags)
  : RsAutoUpdatePage(5000, parent, flags)
{
	ui.setupUi(this);

	m_bProcessSettings = false;
	myChatLobbyUserNotify = NULL;
	myInviteYesButton = NULL;
	myInviteIdChooser = NULL;

	QObject::connect( NotifyQt::getInstance(), SIGNAL(lobbyListChanged()), SLOT(lobbyChanged()));
	QObject::connect( NotifyQt::getInstance(), SIGNAL(chatLobbyEvent(qulonglong,int,const RsGxsId&,const QString&)), this, SLOT(displayChatLobbyEvent(qulonglong,int,const RsGxsId&,const QString&)));
	QObject::connect( NotifyQt::getInstance(), SIGNAL(chatLobbyInviteReceived()), this, SLOT(readChatLobbyInvites()));

    QObject::connect( NotifyQt::getInstance(), SIGNAL(alreadySendChat(const ChatId&, uint)), this, SLOT(updateRecentTime(const ChatId&, uint)));
    QObject::connect( NotifyQt::getInstance(), SIGNAL(newChatMessageReceive(const ChatId&, uint)), this, SLOT(updateRecentTime(const ChatId&, uint)));


    QObject::connect( ui.lobbyTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(lobbyTreeWidgetCustomPopupMenu(QPoint)));
	QObject::connect( ui.lobbyTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
	QObject::connect( ui.lobbyTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateCurrentLobby()));

	QObject::connect( ui.filterLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterItems(QString)));
	QObject::connect( ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));
	QObject::connect( ui.createLobbyToolButton, SIGNAL(clicked()), this, SLOT(createChatLobby()));

	compareRole = new RSTreeWidgetItemCompareRole;
	compareRole->setRole(COLUMN_NAME, ROLE_SORT);
    compareRole->setRole(COLUMN_RECENT_TIME, ROLE_SORT);

	ui.lobbyTreeWidget->setColumnCount(COLUMN_COUNT);
	ui.lobbyTreeWidget->sortItems(COLUMN_NAME, Qt::AscendingOrder);

    ui.lobbyTreeWidget->sortItems(COLUMN_RECENT_TIME, Qt::DescendingOrder);

	ui.lobbyTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu) ;

    ui.lobbyTreeWidget->setStyleSheet("color: white; background-color: rgb(47, 60, 76); "
                                      "selection-color: rgb(255,255,255); selection-background-color: rgb(32, 41, 53);");
    QTreeWidgetItem *headerItem = ui.lobbyTreeWidget->headerItem();
    headerItem->setText(COLUMN_NAME, tr("Name"));
    headerItem->setText(COLUMN_RECENT_TIME, tr("Recent time"));

	QHeaderView *header = ui.lobbyTreeWidget->header();
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_NAME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(header, COLUMN_RECENT_TIME, QHeaderView::Interactive);

    ui.lobbyTreeWidget->setIconSize(QSize(32,32));

    commonItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
    commonItem->setText(COLUMN_NAME, tr("Conversations"));
    commonItem->setData(COLUMN_NAME, ROLE_SORT, "0");
    commonItem->setData(COLUMN_DATA, ROLE_PRIVACYLEVEL, CHAT_LOBBY_ONE2ONE_LEVEL);
    ui.lobbyTreeWidget->insertTopLevelItem(0, commonItem);

	ui.lobbyTreeWidget->expandAll();
	ui.lobbyTreeWidget->setColumnHidden(COLUMN_NAME,false) ;
    ui.lobbyTreeWidget->setColumnHidden(COLUMN_RECENT_TIME,true) ;
	ui.lobbyTreeWidget->setSortingEnabled(true) ;

    float fact = QFontMetricsF(font()).height()/14.0f;
        
	ui.lobbyTreeWidget->adjustSize();
    ui.lobbyTreeWidget->setColumnWidth(COLUMN_NAME,200*fact);

	/** Setup the actions for the header context menu */
//    showUserCountAct= new QAction(headerItem->text(COLUMN_USER_COUNT),this);
//    showUserCountAct->setCheckable(true); showUserCountAct->setToolTip(tr("Show")+" "+showUserCountAct->text()+" "+tr("column"));
//    connect(showUserCountAct,SIGNAL(triggered(bool)),this,SLOT(setShowUserCountColumn(bool))) ;
//    showTopicAct= new QAction(headerItem->text(COLUMN_TOPIC),this);
//    showTopicAct->setCheckable(true); showTopicAct->setToolTip(tr("Show")+" "+showTopicAct->text()+" "+tr("column"));
//    connect(showTopicAct,SIGNAL(triggered(bool)),this,SLOT(setShowTopicColumn(bool))) ;
//    showSubscribeAct= new QAction(headerItem->text(COLUMN_SUBSCRIBED),this);
//    showSubscribeAct->setCheckable(true); showSubscribeAct->setToolTip(tr("Show")+" "+showSubscribeAct->text()+" "+tr("column"));
//    connect(showSubscribeAct,SIGNAL(triggered(bool)),this,SLOT(setShowSubscribeColumn(bool))) ;

	// Set initial size of the splitter
	ui.splitter->setStretchFactor(0, 0);
	ui.splitter->setStretchFactor(1, 1);

	QList<int> sizes;
	sizes << 200*fact << width(); // Qt calculates the right sizes
	ui.splitter->setSizes(sizes);

    getHistoryForRecentList();
    UpdateStatusForAllContacts();
    QObject::connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(QString,int)), this, SLOT(ContactStatusChanged(QString, int)));

    //lobbyChanged();
    showBlankPage(0) ;

	/* add filter actions */
	ui.filterLineEdit->addFilter(QIcon(), tr("Name"), COLUMN_NAME, tr("Search Name"));
	ui.filterLineEdit->setCurrentFilter(COLUMN_NAME);

	// load settings
	processSettings(true);

    int S = QFontMetricsF(font()).height();
    QString help_str = tr("\
                          <h1><img width=\"%1\" src=\":/home/img/question-64.png\">&nbsp;&nbsp;Chat Rooms</h1>                              \
            <p>Chat rooms work pretty much like IRC.                                      \
            They allow you to talk anonymously with tons of people without the need to make friends.</p>                    \
            <p>A chat room can be public (your friends see it) or private (your friends can't see it, unless you           \
                                                                           invite them with <img src=\":/images/add_24x24.png\" width=%2/>). \
            Once you have been invited to a private room, you will be able to see it when your friends   \
                                                                           are using it.</p>                                                                                               \
                                                                           <p>The list at left shows                                                                                     \
                                                                           chat lobbies your friends are participating in. You can either                                 \
                                                                           <ul>                                                                                                            \
                                                                           <li>Right click to create a new chat room</li>                                                              \
                                                                           <li>Double click a chat room to enter, chat, and show it to your friends</li>                      \
                                                                           </ul> \
                                                                           Note: For the chat rooms to work properly, your computer needs be on time.  So check your system clock!\
                                                                           </p>                                      \
                                                                           "
                                                                           ).arg(QString::number(2*S)).arg(QString::number(S)) ;

            registerHelpButton(ui.helpButton,help_str,"ChatLobbyDialog") ;
}

ChatLobbyWidget::~ChatLobbyWidget()
{
    // save settings
    processSettings(false);

	if (compareRole) {
		delete(compareRole);
	}
}

UserNotify *ChatLobbyWidget::getUserNotify(QObject *parent)
{
	if (!myChatLobbyUserNotify){
		myChatLobbyUserNotify = new ChatLobbyUserNotify(parent);
		connect(myChatLobbyUserNotify, SIGNAL(countChanged(ChatLobbyId, unsigned int)), this, SLOT(updateNotify(ChatLobbyId, unsigned int)));
        connect(myChatLobbyUserNotify, SIGNAL(countChangedFromP2P(ChatId, unsigned int)), this, SLOT(updateNotifyFromP2P(ChatId, unsigned int)));
	}
	return myChatLobbyUserNotify;
}

void ChatLobbyWidget::updateNotify(ChatLobbyId id, unsigned int count)
{
	ChatLobbyDialog *dialog=NULL;
	dialog=_lobby_infos[id].dialog;
	if(!dialog) return;

	QToolButton* notifyButton=dialog->getChatWidget()->getNotifyButton();
	if (!notifyButton) return;
	dialog->getChatWidget()->setNotify(myChatLobbyUserNotify);
	if (count>0){
		notifyButton->setVisible(true);
		//notifyButton->setIcon(_lobby_infos[id].default_icon);
		notifyButton->setToolTip(QString("(%1)").arg(count));
	} else {
		notifyButton->setVisible(false);
        ChatId chatId(id);
        rsHistory->updateMessageAsRead(chatId);
	}
}


void ChatLobbyWidget::updateNotifyFromP2P(ChatId id, unsigned int count)
{
    PopupChatDialog *dialog=NULL;
    dialog=_chatOne2One_infos[id.toPeerId().toStdString()].dialog;
    if(!dialog) return;

    QToolButton* notifyButton=dialog->getChatWidget()->getNotifyButton();
    if (!notifyButton) return;
    dialog->getChatWidget()->setNotify(myChatLobbyUserNotify);
    if (count>0){
        notifyButton->setVisible(true);
        //notifyButton->setIcon(_lobby_infos[id].default_icon);
        notifyButton->setToolTip(QString("(%1)").arg(count));
    } else {
        notifyButton->setVisible(false);

        //Need to update msg as read by updating the history
        rsHistory->updateMessageAsRead(id);
        //And need to update for all items on the left, return to their avatars
        std::set<ChatId>::iterator i;

        if (recentUnreadListOfChatId.size() > 0)
        {
            if (recentUnreadListOfChatId.count(id) != 0) recentUnreadListOfChatId.erase(id);
        }
    }
}

static bool trimAnonIds(std::list<RsGxsId>& lst)
{
    // trim down identities that are unsigned, because the lobby requires it.
    bool removed = false ;

    RsIdentityDetails idd ;

    for(std::list<RsGxsId>::iterator it = lst.begin();it!=lst.end();)
	    if(!rsIdentity->getIdDetails(*it,idd) || !(idd.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
	    {
		    it = lst.erase(it) ;
		    removed= true ;
	    }
	    else
		    ++it ;

    return removed ;
}

void ChatLobbyWidget::lobbyTreeWidgetCustomPopupMenu(QPoint)
{
    return;
	std::cerr << "Creating customPopupMennu" << std::endl;
	QTreeWidgetItem *item = ui.lobbyTreeWidget->currentItem();

	QMenu contextMnu(this);

	if (item && item->type() == TYPE_FOLDER) {
		QAction *action = contextMnu.addAction(QIcon(IMAGE_CREATE), tr("Create chat room"), this, SLOT(createChatLobby()));
		action->setData(item->data(COLUMN_DATA, ROLE_PRIVACYLEVEL).toInt());
	}

        if (item && item->type() == TYPE_LOBBY)
        {
            std::list<RsGxsId> own_identities ;
            rsIdentity->getOwnIds(own_identities) ;

            if (item->data(COLUMN_DATA, ROLE_SUBSCRIBED).toBool())
                contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Leave this room"), this, SLOT(unsubscribeItem()));
            else
            {
                QTreeWidgetItem *item = ui.lobbyTreeWidget->currentItem();

                //ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();
            ChatLobbyFlags flags(item->data(COLUMN_DATA, ROLE_FLAGS).toUInt());

                bool removed = false ;
                if(flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED)
                    removed = trimAnonIds(own_identities) ;

                if(own_identities.empty())
                {
                    if(removed)
                    contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Create a non anonymous identity and enter this room"), this, SLOT(createIdentityAndSubscribe()));
                        else
                    contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Create an identity and enter this chat room"), this, SLOT(createIdentityAndSubscribe()));
                }
                else if(own_identities.size() == 1)
                {
                    QAction *action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Enter this chat room"), this, SLOT(subscribeChatLobbyAs()));
                    action->setData(QString::fromStdString((own_identities.front()).toStdString())) ;
                }
                else
                {
                    QMenu *mnu = contextMnu.addMenu(QIcon(IMAGE_SUBSCRIBE),tr("Enter this chat room as...")) ;

                    for(std::list<RsGxsId>::const_iterator it=own_identities.begin();it!=own_identities.end();++it)
                    {
                        RsIdentityDetails idd ;
                        rsIdentity->getIdDetails(*it,idd) ;

                        QPixmap pixmap ;

                        if(idd.mAvatar.mSize == 0 || !pixmap.loadFromData(idd.mAvatar.mData, idd.mAvatar.mSize, "PNG"))
                            pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(*it)) ;

                        QAction *action = mnu->addAction(QIcon(pixmap), QString("%1 (%2)").arg(QString::fromUtf8(idd.mNickname.c_str()), QString::fromStdString((*it).toStdString())), this, SLOT(subscribeChatLobbyAs()));
                        action->setData(QString::fromStdString((*it).toStdString())) ;
                    }
                }
            }

            if (item->data(COLUMN_DATA, ROLE_AUTOSUBSCRIBE).toBool())
                contextMnu.addAction(QIcon(IMAGE_AUTOSUBSCRIBE), tr("Remove Auto Subscribe"), this, SLOT(autoSubscribeItem()));
            else if(!own_identities.empty())
                contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Add Auto Subscribe"), this, SLOT(autoSubscribeItem()));
            contextMnu.addAction(QIcon(IMAGE_COPYRSLINK), tr("Copy UnseenP2P Link"), this, SLOT(copyItemLink()));
        }

        contextMnu.addSeparator();//-------------------------------------------------------------------

        showUserCountAct->setChecked(!ui.lobbyTreeWidget->isColumnHidden(COLUMN_USER_COUNT));
        showTopicAct->setChecked(!ui.lobbyTreeWidget->isColumnHidden(COLUMN_TOPIC));
        showSubscribeAct->setChecked(!ui.lobbyTreeWidget->isColumnHidden(COLUMN_SUBSCRIBED));

        QMenu *menu = contextMnu.addMenu(tr("Columns"));
        menu->addAction(showUserCountAct);
        menu->addAction(showTopicAct);
        menu->addAction(showSubscribeAct);

        contextMnu.exec(QCursor::pos());
}

void ChatLobbyWidget::lobbyChanged()
{
	updateDisplay();
}

static void updateItem(QTreeWidget *treeWidget, QTreeWidgetItem *item, ChatLobbyId id, const std::string &name, const std::string &topic, int count, bool subscribed, bool autoSubscribe,ChatLobbyFlags lobby_flags)
{
	item->setText(COLUMN_NAME, QString::fromUtf8(name.c_str()));
	item->setData(COLUMN_NAME, ROLE_SORT, QString::fromUtf8(name.c_str()));

	item->setData(COLUMN_DATA, ROLE_ID, (qulonglong)id);
	item->setData(COLUMN_DATA, ROLE_FLAGS, lobby_flags.toUInt32());

	QColor color = treeWidget->palette().color(QPalette::Active, QPalette::Text);
    
	for (int column = 0; column < COLUMN_COUNT; ++column) {
		item->setTextColor(column, color);
	}
    QString tooltipstr = QObject::tr("Group name:")+" "+item->text(COLUMN_NAME)+"\n"
                     +QObject::tr("Group type:")+" "+ (lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC? QObject::tr("Public group (Community group) "): QObject::tr("Private group (members only)"))+"\n"
                     //+QObject::tr("Participants:")+" "+QString::number(count)+"\n"
                     +QObject::tr("Id:")+" "+QString::number(id,16) ;
    
    if(lobby_flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED)
    {
        tooltipstr += QObject::tr("\nSecurity: no anonymous IDs") ;
        QColor foreground = QColor(0, 128, 0); // green
        for (int column = 0; column < COLUMN_COUNT; ++column)
            item->setTextColor(column, foreground);
    }
    item->setToolTip(0,tooltipstr) ;
}

void ChatLobbyWidget::addChatPage(ChatLobbyDialog *d)
{
	// check that the page does not already exist. 

	if(_lobby_infos.find(d->id()) == _lobby_infos.end())
	{
		ui.stackedWidget->addWidget(d) ;

		connect(d,SIGNAL(lobbyLeave(ChatLobbyId)),this,SLOT(unsubscribeChatLobby(ChatLobbyId))) ;
		connect(d,SIGNAL(typingEventReceived(ChatLobbyId)),this,SLOT(updateTypingStatus(ChatLobbyId))) ;
		connect(d,SIGNAL(messageReceived(bool,ChatLobbyId,QDateTime,QString,QString)),this,SLOT(updateMessageChanged(bool,ChatLobbyId,QDateTime,QString,QString))) ;
		connect(d,SIGNAL(peerJoined(ChatLobbyId)),this,SLOT(updatePeerEntering(ChatLobbyId))) ;
		connect(d,SIGNAL(peerLeft(ChatLobbyId)),this,SLOT(updatePeerLeaving(ChatLobbyId))) ;

		ChatLobbyId id = d->id();
		_lobby_infos[id].dialog = d ;
		_lobby_infos[id].default_icon = QIcon() ;
		_lobby_infos[id].last_typing_event = time(NULL) ;

                ChatLobbyInfo linfo ;
                if(rsMsgs->getChatLobbyInfo(id,linfo))
                    _lobby_infos[id].default_icon = (linfo.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC):QIcon(IMAGE_PRIVATE) ;
                else
                     std::cerr << "(EE) cannot find info for room " << std::hex << id << std::dec << std::endl;
	}
}

void ChatLobbyWidget::addOne2OneChatPage(PopupChatDialog *d)
{
	// check that the page does not already exist.
	if (_chatOne2One_infos.count(d->chatId().toPeerId().toStdString()) < 1)
	{
        //Check if the item already exist in the contact chat folder, if yes, just add to widget and save to _chatOne2One_infos
        QTreeWidgetItem *checkItem = getTreeWidgetItemForChatId(d->chatId());
        uint current_time = QDateTime::currentDateTime().toTime_t();
        if (checkItem == NULL)
        {
            QTreeWidgetItem *item =  new RSTreeWidgetItem(compareRole, TYPE_ONE2ONE);
            RsPgpId pgpId = rsPeers->getGPGId(d->chatId().toPeerId());
            std::string nickname = rsPeers->getGPGName(pgpId);
            updateContactItem(ui.lobbyTreeWidget, item, nickname, d->chatId(), d->chatId().toPeerId().toStdString(), current_time, false );

            //add to common
            commonItem->addChild(item);
            commonItem->treeWidget()->setItemSelected(item, true);

            QTreeWidgetItemIterator it2(commonItem->treeWidget());
            while (*it2) {
                if ((*it2)!= item && (*it2)->isSelected()) (*it2)->setSelected(false) ;
                ++it2;
              }
        }

        connect(d,SIGNAL(messageP2PReceived(ChatMessage)),this,SLOT(updateP2PMessageChanged(ChatMessage))) ;

		ui.stackedWidget->addWidget(d) ;
		_chatOne2One_infos[d->chatId().toPeerId().toStdString()].dialog = d ;
        if (checkItem == NULL) _chatOne2One_infos[d->chatId().toPeerId().toStdString()].last_typing_event = current_time; //QDateTime::currentDateTime().toTime_t();

   }


}

void ChatLobbyWidget::setCurrentChatPage(ChatLobbyDialog *d)
{
	ui.stackedWidget->setCurrentWidget(d) ;

	if (d) {
		QTreeWidgetItem *item = getTreeWidgetItem(d->id());
		if (item) {
			ui.lobbyTreeWidget->setCurrentItem(item);
		}
	}
}

void ChatLobbyWidget::setCurrentOne2OneChatPage(PopupChatDialog *d)
{
	ui.stackedWidget->setCurrentWidget(d) ;

	if (d) {
		QTreeWidgetItem *item = getTreeWidgetItemForChatId(d->chatId());
		if (item) {
			ui.lobbyTreeWidget->setCurrentItem(item);
		}
	}
}

void ChatLobbyWidget::updateDisplay()
{
    /* Meiyousixin - Need to get all local group chats (include both community and private group) to show in group chats tree */

    rsMsgs->getGroupChatInfoList(_groupchat_infos);

#ifdef CHAT_LOBBY_GUI_DEBUG
    std::cerr << "Show all group chats !" << std::endl;
    for(std::map<ChatLobbyId,ChatLobbyInfo>::const_iterator it(_groupchat_infos.begin());it!=_groupchat_infos.end();++it)
    {
        std::cerr << "Group Chat: " << it->second.lobby_name << std::endl;
    }
#endif

    for(std::map<ChatLobbyId,ChatLobbyInfo>::const_iterator it(_groupchat_infos.begin());it!=_groupchat_infos.end();++it)
    {
        QIcon icon;
        QTreeWidgetItem *item = NULL;
        //check before add
        int childCnt = commonItem->childCount();
        for (int childIndex = 0; childIndex < childCnt; ++childIndex)
        {
            QTreeWidgetItem *itemLoop = commonItem->child(childIndex);
            if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == it->second.lobby_id)
            {
                item = itemLoop;
                break;
            }
        }
        if (item == NULL)
        {
            item = new RSTreeWidgetItem(compareRole, TYPE_LOBBY);

            icon = (it->second.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
            std::cerr << " Add group chat item to the common item if there is no item, group name: " << it->second.lobby_name << std::endl;
            if (!icon.isNull())
            {
                item->setIcon(COLUMN_NAME, true ? icon : icon.pixmap(ui.lobbyTreeWidget->iconSize(), QIcon::Disabled));
            }

            updateItem(ui.lobbyTreeWidget, item, it->second.lobby_id, it->second.lobby_name,it->second.lobby_topic, it->second.participating_friends.size(), true, true,it->second.lobby_flags);

            commonItem->addChild(item);
            joinGroupChatInBackground(it->second);
        }
    }

    //commonItem->setHidden(false);
}

void ChatLobbyWidget::createChatLobby()
{
    int privacyLevel = 2; //meiyousixin - change to 2 - private as default , 1 - public
	QAction *action = qobject_cast<QAction*>(sender());
	if (action) {
		privacyLevel = action->data().toInt();
	}

	std::set<RsPeerId> friends;
	CreateLobbyDialog(friends, privacyLevel).exec();
}

void ChatLobbyWidget::showLobby(QTreeWidgetItem *item)
{
	if (item == NULL || item->type() != TYPE_LOBBY) {
		showBlankPage(0) ;
		return;
	}

	ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();

	if(_lobby_infos.find(id) == _lobby_infos.end())
    {
        ChatDialog::chatFriend(ChatId(id),true) ;
        //showBlankPage(id) ;
    }
	else
		ui.stackedWidget->setCurrentWidget(_lobby_infos[id].dialog) ;
}
// convert from RsPgpId to chatId
void ChatLobbyWidget::fromGpgIdToChatId(const RsPgpId &gpgId, ChatId &chatId)
{
  RsPeerDetails detail;
  if (rsPeers->getGPGDetails(gpgId, detail))
    {
          //let's get the ssl child details
          std::list<RsPeerId> sslIds;
          rsPeers->getAssociatedSSLIds(detail.gpg_id, sslIds);

          if (sslIds.size() == 1) {
                  // chat with the one ssl id (online or offline)
                  chatId = ChatId(sslIds.front());
                  return ;
          }

          // more than one ssl ids available, check for online
          std::list<RsPeerId> onlineIds;
          for (std::list<RsPeerId>::iterator it = sslIds.begin(); it != sslIds.end(); ++it) {
                  if (rsPeers->isOnline(*it)) {
                          onlineIds.push_back(*it);
                  }
          }

          if (onlineIds.size() == 1) {
                  // chat with the online ssl id
                  chatId = ChatId(onlineIds.front());
                  return ;
          }

  }
}

//22 Sep 2018 - meiyousixin - show contact chat like showLobby
void ChatLobbyWidget::showContactChat(QTreeWidgetItem *item)
{
	if (item == NULL || item->type() != TYPE_ONE2ONE) {
		return;
	}

	QString chatIdStr = item->data(COLUMN_DATA, ROLE_ID).toString();
    if(_chatOne2One_infos.count(chatIdStr.toStdString()) > 0)
    {
        ui.stackedWidget->setCurrentWidget(_chatOne2One_infos[chatIdStr.toStdString()].dialog) ;
    }
    else
    {
        //create chat dialog and add into this widget
        RsPeerId peerId(chatIdStr.toStdString());
        ChatId chatId(peerId);
        ChatDialog::chatFriend(chatId);
    }


}

// Sort recent list by history messages of all conversations
void ChatLobbyWidget::getHistoryForRecentList()
{
    int messageCount = 1;
    std::list<HistoryMsg> historyMsgs;
    if (messageCount > 0)
    {
        //get one history msg for all contacts and sort by recent time
        std::list<RsPeerId> friendList;
        rsPeers->getFriendList(friendList);
        std::list<RsPeerId>::const_iterator peerIt;
        for (peerIt = friendList.begin(); peerIt != friendList.end(); ++peerIt)
        {
            ChatId chatId(*peerIt);
            rsHistory->getMessages(chatId, historyMsgs, messageCount);
            std::list<HistoryMsg>::iterator historyIt;
            for (historyIt = historyMsgs.begin(); historyIt != historyMsgs.end(); ++historyIt)
            {
                QTreeWidgetItem *item =  new RSTreeWidgetItem(compareRole, TYPE_ONE2ONE);
                RsPgpId pgpId = rsPeers->getGPGId(chatId.toPeerId());
                std::string nickname = rsPeers->getGPGName(pgpId);
                uint current_time = (historyIt->incoming ? historyIt->recvTime :  historyIt->sendTime) ;
                updateContactItem(ui.lobbyTreeWidget, item, nickname, chatId, chatId.toPeerId().toStdString(), current_time, historyIt->unread );
                //add contact item into common tree
                commonItem->addChild(item);
                ui.lobbyTreeWidget->sortItems(COLUMN_RECENT_TIME, Qt::DescendingOrder);
            }
        }

        //get one history msg  for all group chats and sort by recent time
        std::list<HistoryMsg> historyGroupChatMsgs;
        rsMsgs->getGroupChatInfoList(_groupchat_infos);

        for(std::map<ChatLobbyId,ChatLobbyInfo>::const_iterator it(_groupchat_infos.begin());it!=_groupchat_infos.end();++it)
        {
            ChatId chatId(it->second.lobby_id);
            rsHistory->getMessages(chatId, historyGroupChatMsgs, messageCount);

            std::list<HistoryMsg>::iterator historyIt;
            for (historyIt = historyGroupChatMsgs.begin(); historyIt != historyGroupChatMsgs.end(); ++historyIt)
            {
                QTreeWidgetItem *item =  new RSTreeWidgetItem(compareRole, TYPE_LOBBY);
                uint current_time = (historyIt->incoming ? historyIt->recvTime :  historyIt->sendTime) ;

                std::cerr << "History for group chat " <<  (*it).second.lobby_name <<"last msg is unread: " << historyIt->unread << std::endl;
                QIcon icon = (it->second.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
                if (!icon.isNull())
                {
                    item->setIcon(COLUMN_NAME, true ? icon : icon.pixmap(ui.lobbyTreeWidget->iconSize(), QIcon::Disabled));
                }
                updateGroupChatItem(ui.lobbyTreeWidget, item, it->second.lobby_name, it->second.lobby_id, current_time, historyIt->unread, it->second.lobby_flags );
                commonItem->addChild(item);
                ui.lobbyTreeWidget->sortItems(COLUMN_RECENT_TIME, Qt::DescendingOrder);
            }
        }


    }
}

// 22 Sep 2018 - meiyousixin - this function is for the case where we don't have any identity yet
void ChatLobbyWidget::createIdentityAndSubscribe()
{
    QTreeWidgetItem *item = ui.lobbyTreeWidget->currentItem();

    if(!item)
        return ;

    ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();
    ChatLobbyFlags flags(item->data(COLUMN_DATA, ROLE_FLAGS).toUInt());

    IdEditDialog dlg(this);
    dlg.setupNewId(false);
    
    if(flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED) //
	    dlg.enforceNoAnonIds() ;
    
    dlg.exec();
    // fetch new id
    std::list<RsGxsId> own_ids;
    if(!rsIdentity->getOwnIds(own_ids) || own_ids.empty())
        return;

    if(rsMsgs->joinVisibleChatLobby(id,own_ids.front()))
        ChatDialog::chatFriend(ChatId(id),true) ;
}

void ChatLobbyWidget::subscribeChatLobbyAs()
{
    QTreeWidgetItem *item = ui.lobbyTreeWidget->currentItem();

    if(!item)
        return ;

    ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();

    QAction *action = qobject_cast<QAction *>(QObject::sender());
    if (!action)
        return ;

    RsGxsId gxs_id(action->data().toString().toStdString());
        //uint32_t error_code ;

    if(rsMsgs->joinVisibleChatLobby(id,gxs_id))
        ChatDialog::chatFriend(ChatId(id),true) ;
}

bool ChatLobbyWidget::showLobbyAnchor(ChatLobbyId id, QString anchor)
{
	QTreeWidgetItem *item = getTreeWidgetItem(id) ;

	if(item != NULL) {
        if(item->type() == TYPE_LOBBY) {

            if(_lobby_infos.find(id) == _lobby_infos.end()) {
				showBlankPage(id) ;
			} else {
				//ChatLobbyDialog cldChatLobby =_lobby_infos[id].dialog;
				ui.stackedWidget->setCurrentWidget(_lobby_infos[id].dialog) ;
				ChatLobbyDialog *cldCW=NULL ;
				if (NULL != (cldCW = dynamic_cast<ChatLobbyDialog *>(ui.stackedWidget->currentWidget())))
					cldCW->getChatWidget()->scrollToAnchor(anchor);
			}

			ui.lobbyTreeWidget->setCurrentItem(item);
			return true;
		}
	}

	return false;
}

bool ChatLobbyWidget::showContactAnchor(RsPeerId id, QString anchor)
{
    ChatId chatId(id);
    QTreeWidgetItem *item = getTreeWidgetItemForChatId(chatId) ;

    if(item != NULL) {
        if(item->type() == TYPE_ONE2ONE) {

            if(_chatOne2One_infos.find(id.toStdString()) == _chatOne2One_infos.end()) {
                //showBlankPage(id) ;
            } else {
                //ChatLobbyDialog cldChatLobby =_lobby_infos[id].dialog;
                ui.stackedWidget->setCurrentWidget(_chatOne2One_infos[id.toStdString()].dialog) ;
                PopupChatDialog *cldCW=NULL ;
                if (NULL != (cldCW = dynamic_cast<PopupChatDialog *>(ui.stackedWidget->currentWidget())))
                    cldCW->getChatWidget()->scrollToAnchor(anchor);
            }

            ui.lobbyTreeWidget->setCurrentItem(item);
            return true;
        }
    }

    return false;
}

void ChatLobbyWidget::subscribeChatLobbyAtItem(QTreeWidgetItem *item)
{
    if (item == NULL || item->type() != TYPE_LOBBY) {
        return;
    }

    ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();
    ChatLobbyFlags flags ( item->data(COLUMN_DATA, ROLE_FLAGS).toUInt());
    RsGxsId gxs_id ;

    std::list<RsGxsId> own_ids;
    // not using rsMsgs->getDefaultIdentityForChatLobby(), to check if we have an identity
    // to work around the case when the identity was deleted and is invalid
    // (the chatservice does not know if a default identity was deleted)
    // only rsIdentity knows the truth at the moment!
    if(!rsIdentity->getOwnIds(own_ids))
        return;

    // if there is no identity yet, show the dialog to create a new identity
    if(own_ids.empty())
    {
        IdEditDialog dlg(this);
        dlg.setupNewId(false);
        
        if(flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED) //
		dlg.enforceNoAnonIds() ;
        dlg.exec();
        // fetch new id
        if(!rsIdentity->getOwnIds(own_ids) || own_ids.empty())
            return;
        gxs_id = own_ids.front();
    }
    else
    {
        rsMsgs->getDefaultIdentityForChatLobby(gxs_id);
        
        RsIdentityDetails idd ;
        if(!rsIdentity->getIdDetails(gxs_id,idd))
            return ;
        
        if( (flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED) && !(idd.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
        {
            QMessageBox::warning(NULL,tr("Default identity is anonymous"),tr("You cannot join this chat room with your default identity, since it is anonymous and the chat room forbids it.")) ;
            return ;
        }
    }

    if(rsMsgs->joinVisibleChatLobby(id,gxs_id))
        ChatDialog::chatFriend(ChatId(id),true) ;
}

void ChatLobbyWidget::joinGroupChatInBackground(ChatLobbyInfo lobbyInfo)
{
    ChatLobbyFlags flags = lobbyInfo.lobby_flags;
    RsGxsId gxs_id ;

    std::list<RsGxsId> own_ids;
    // not using rsMsgs->getDefaultIdentityForChatLobby(), to check if we have an identity
    // to work around the case when the identity was deleted and is invalid
    // (the chatservice does not know if a default identity was deleted)
    // only rsIdentity knows the truth at the moment!
    if(!rsIdentity->getOwnIds(own_ids))
    {

        rsMsgs->getDefaultIdentityForChatLobby(gxs_id);
        if(rsMsgs->joinVisibleChatLobby(lobbyInfo.lobby_id,gxs_id))
        {
#ifdef CHAT_LOBBY_GUI_DEBUG
            std::cerr << " Joining groupchat: " << lobbyInfo.lobby_name << ", topic " << lobbyInfo.lobby_topic << " #" << std::hex << lobbyInfo.lobby_id << std::dec << " public. Lobby flags: " << lobbyInfo.lobby_flags << std::endl;
#endif

        }
        RsIdentityDetails idd ;
        if(!rsIdentity->getIdDetails(gxs_id,idd))
            return ;

        if( (flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED) && !(idd.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED))
        {
            QMessageBox::warning(NULL,tr("Default identity is anonymous"),tr("You cannot join this chat room with your default identity, since it is anonymous and the chat room forbids it.")) ;
            return ;
        }
    }

}

void ChatLobbyWidget::autoSubscribeLobby(QTreeWidgetItem *item)
{
    if (item == NULL || item->type() != TYPE_LOBBY) {
        return;
    }

    ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();
    bool isAutoSubscribe = rsMsgs->getLobbyAutoSubscribe(id);
    rsMsgs->setLobbyAutoSubscribe(id, !isAutoSubscribe);
    if (!isAutoSubscribe && !item->data(COLUMN_DATA, ROLE_SUBSCRIBED).toBool())
        subscribeChatLobbyAtItem(item);
}

void ChatLobbyWidget::showBlankPage(ChatLobbyId id)
{
	// show the default blank page.
	ui.stackedWidget->setCurrentWidget(ui._lobby_blank_page) ;

    QString text = tr("UnseenP2P is a decentralized, private and secure commmunication and sharing platform. \nUnseenP2P provides filesharing, chat, messages, forums and channels. ") ;
	ui.lobbyInfoLabel->setText(text) ;
}

void ChatLobbyWidget::subscribeItem()
{
    subscribeChatLobbyAtItem(ui.lobbyTreeWidget->currentItem());
}

void ChatLobbyWidget::autoSubscribeItem()
{
    autoSubscribeLobby(ui.lobbyTreeWidget->currentItem());
}

void ChatLobbyWidget::copyItemLink()
{
	QTreeWidgetItem *item = ui.lobbyTreeWidget->currentItem();
	if (item == NULL || item->type() != TYPE_LOBBY) {
		return;
	}

	ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();
	QString name = item->text(COLUMN_NAME);

	RetroShareLink link = RetroShareLink::createChatRoom(ChatId(id),name);
	if (link.valid()) {
		QList<RetroShareLink> urls;
		urls.push_back(link);
		RSLinkClipboard::copyLinks(urls);
    }
}

void ChatLobbyWidget::updateRecentTime(const ChatId & chatId, uint current_time)
{
       // std::cerr << "current time for this contact is " << current_time;
       std::cerr << "we want to update the recent time now current time is " << current_time << "\n";
       QTreeWidgetItem *item = NULL;
       std::string chatIdStr;
       if (chatId.isLobbyId())
       {
           item = getTreeWidgetItem(chatId.toLobbyId());
           chatIdStr = std::to_string(chatId.toLobbyId());
       }
       else if (chatId.isPeerId())
       {
           item = getTreeWidgetItemForChatId(chatId);
           chatIdStr = chatId.toPeerId().toStdString();
       }
       if (item)
            item->setData(COLUMN_RECENT_TIME, ROLE_SORT,current_time);

       ui.lobbyTreeWidget->sortItems(COLUMN_RECENT_TIME, Qt::DescendingOrder);
}

QTreeWidgetItem *ChatLobbyWidget::getTreeWidgetItemForChatId(ChatId id)
{
     QTreeWidgetItem *lobby_item = commonItem;
     int childCnt = lobby_item->childCount();
     int childIndex = 0;

     while (childIndex < childCnt) {
             QTreeWidgetItem *itemLoop = lobby_item->child(childIndex);

	     if (itemLoop->type() == TYPE_ONE2ONE && itemLoop->data(COLUMN_DATA, ROLE_ID) == QString::fromUtf8(id.toPeerId().toStdString().c_str()))
		     return itemLoop ;

             ++childIndex ;
     }
     return NULL ;

}
QTreeWidgetItem *ChatLobbyWidget::getTreeWidgetItem(ChatLobbyId id)
{

    QTreeWidgetItem *lobby_item = commonItem;
    int childCnt = lobby_item->childCount();
    int childIndex = 0;

    while (childIndex < childCnt) {
        QTreeWidgetItem *itemLoop = lobby_item->child(childIndex);

        if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == id)
            return itemLoop ;

        ++childIndex ;
    }

	return NULL ;
}
void ChatLobbyWidget::updateTypingStatus(ChatLobbyId id)
{
	QTreeWidgetItem *item = getTreeWidgetItem(id) ;
	
	if(item != NULL)
	{
		item->setIcon(COLUMN_NAME,QIcon(IMAGE_TYPING)) ;
		_lobby_infos[id].last_typing_event = time(NULL) ;

		QTimer::singleShot(5000,this,SLOT(resetLobbyTreeIcons())) ;
	}
}
void ChatLobbyWidget::updatePeerLeaving(ChatLobbyId id)
{
	QTreeWidgetItem *item = getTreeWidgetItem(id) ;
	
	if(item != NULL)
	{
		item->setIcon(COLUMN_NAME,QIcon(IMAGE_PEER_LEAVING)) ;
		_lobby_infos[id].last_typing_event = time(NULL) ;

		QTimer::singleShot(5000,this,SLOT(resetLobbyTreeIcons())) ;
	}
}
void ChatLobbyWidget::updatePeerEntering(ChatLobbyId id)
{
	QTreeWidgetItem *item = getTreeWidgetItem(id) ;
	
	if(item != NULL)
	{
		item->setIcon(COLUMN_NAME,QIcon(IMAGE_PEER_ENTERING)) ;
		_lobby_infos[id].last_typing_event = time(NULL) ;

		QTimer::singleShot(5000,this,SLOT(resetLobbyTreeIcons())) ;
	}
}
void ChatLobbyWidget::resetLobbyTreeIcons()
{
	time_t now = time(NULL) ;

	for(std::map<ChatLobbyId,ChatLobbyInfoStruct>::iterator it(_lobby_infos.begin());it!=_lobby_infos.end();++it)
		if(it->second.last_typing_event + 5 <= now)
		{
			getTreeWidgetItem(it->first)->setIcon(COLUMN_NAME,it->second.default_icon) ;
			//std::cerr << "Reseted 1 lobby icon." << std::endl;
		}
}

void ChatLobbyWidget::unsubscribeItem()
{
	QTreeWidgetItem *item = ui.lobbyTreeWidget->currentItem();
	if (item == NULL || item->type() != TYPE_LOBBY) {
		return;
	}

	const ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();

	unsubscribeChatLobby(id) ;
}

void ChatLobbyWidget::unsubscribeChatLobby(ChatLobbyId id)
{
	std::cerr << "Unsubscribing from chat room" << std::endl;

	// close the tab.

	std::map<ChatLobbyId,ChatLobbyInfoStruct>::iterator it = _lobby_infos.find(id) ;

	if(it != _lobby_infos.end())
	{
		if (myChatLobbyUserNotify){
			myChatLobbyUserNotify->chatLobbyCleared(id, "");
		}

		ui.stackedWidget->removeWidget(it->second.dialog) ;
		_lobby_infos.erase(it) ;
    }

    //remove item from conversations list
    QTreeWidgetItem *rItem = getTreeWidgetItem(id);
    if (rItem)
    {
        commonItem->removeChild(rItem);
    }

	// Unsubscribe the chat lobby
    ChatDialog::closeChat(ChatId(id));
	rsMsgs->unsubscribeChatLobby(id);
    bool isAutoSubscribe = rsMsgs->getLobbyAutoSubscribe(id);
	if (isAutoSubscribe) rsMsgs->setLobbyAutoSubscribe(id, !isAutoSubscribe);

	ChatLobbyDialog *cldCW=NULL ;
	if (NULL != (cldCW = dynamic_cast<ChatLobbyDialog *>(ui.stackedWidget->currentWidget())))
	{

		QTreeWidgetItem *qtwiFound = getTreeWidgetItem(cldCW->id());
		if (qtwiFound) {
			ui.lobbyTreeWidget->setCurrentItem(qtwiFound);
		}
	} else {
		ui.lobbyTreeWidget->clearSelection();

	}
}
// Try to add selection of contact chat in the same tree widget
void ChatLobbyWidget::updateCurrentLobby()
{
	QList<QTreeWidgetItem *> items = ui.lobbyTreeWidget->selectedItems() ;
	if(items.empty())
	{
		// need to check more about contact item selection
		showLobby(0) ;
	}
	else
	{
        //if this is a group chat
        QTreeWidgetItem *item = items.front();
        if (item->type() == TYPE_LOBBY)
		{
            showLobby(item);

            //update unread notification here
            ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();

            std::map<ChatLobbyId, ChatLobbyInfo>::const_iterator it = _groupchat_infos.find(id);
            if (it != _groupchat_infos.end())
            {
                QIcon icon = (it->second.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
                if (!icon.isNull())
                {
                    _lobby_infos[id].default_icon = icon ;
                    item->setIcon(COLUMN_NAME, _lobby_infos[id].default_icon);
                }
            }
            ChatId chatId(id);
            if (recentUnreadListOfChatId.count(chatId) != 0) recentUnreadListOfChatId.erase(chatId);
            rsHistory->updateMessageAsRead(chatId);
		}
		// if this is a contact chat
        else if (item->type() == TYPE_ONE2ONE) //if (item->parent() && item->parent()->text(COLUMN_NAME) == tr("Contact chats"))
		  {
		      showContactChat(item);
              QPixmap avatar;
              QString id = item->data(COLUMN_DATA, ROLE_ID).toString();
              RsPeerId peerId(id.toStdString());
              ChatId chatId(peerId);
              if (recentUnreadListOfChatId.count(chatId) != 0) recentUnreadListOfChatId.erase(chatId);

              QFont gpgFont;
              QPixmap gpgOverlayIcon = currentStatusIcon(peerId, gpgFont);
              QIcon unreadIcon = lastIconForPeerId(peerId, false);
              item->setIcon(COLUMN_NAME,unreadIcon) ;
              item->setFont(COLUMN_NAME,gpgFont) ;

              // Need to update msg as read here, it will mark the last msg as read
              rsHistory->updateMessageAsRead(chatId);

		  }

	}

	//keep for lobby chat

    if (ui.filterLineEdit->text().isEmpty() == false) {
        filterItems(ui.filterLineEdit->text());
    }

}
void ChatLobbyWidget::updateMessageChanged(bool incoming, ChatLobbyId id, QDateTime time, QString senderName, QString msg)
{
	QTreeWidgetItem *current_item = ui.lobbyTreeWidget->currentItem();
	bool bIsCurrentItem = (current_item != NULL && current_item->data(COLUMN_DATA, ROLE_ID).toULongLong() == id);

	if (myChatLobbyUserNotify){
		if (incoming) myChatLobbyUserNotify->chatLobbyNewMessage(id, time, senderName, msg);
	}

	// Don't show anything for current lobby.
	//
    ChatId chatId(id);
	if(bIsCurrentItem)
    {
        //return to old icon for that group chat
        ChatLobbyFlags flags(current_item->data(COLUMN_DATA, ROLE_FLAGS).toUInt());
        QIcon icon = (flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
        if (!icon.isNull())
        {
            current_item->setIcon(COLUMN_NAME, true ? icon : icon.pixmap(ui.lobbyTreeWidget->iconSize(), QIcon::Disabled));
        }

        //Need to update the message as read: When user receive new msg in the active chat window
        rsHistory->updateMessageAsRead(chatId);
		return ;
    }

	QTreeWidgetItem *item = getTreeWidgetItem(id) ;

	if(item == NULL)
		return ;

    if (incoming && item)
    {
        //ChatId chatId(id);
        if (recentUnreadListOfChatId.count(chatId) == 0) recentUnreadListOfChatId.insert(chatId);

        //_lobby_infos[id].default_icon = QIcon(IMAGE_MESSAGE) ;
        ChatLobbyFlags flags(current_item->data(COLUMN_DATA, ROLE_FLAGS).toUInt());
        QIcon icon = (flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_MESSAGE) : QIcon(IMAGE_MESSAGE_PRIVATE);
        item->setIcon(COLUMN_NAME,icon) ;
    }

}

//void ChatLobbyWidget::updateP2PMessageChanged(bool incoming, const ChatId& chatId, QDateTime time, QString senderName, QString msg)
void ChatLobbyWidget::updateP2PMessageChanged(ChatMessage msg)
{
    QTreeWidgetItem *item = getTreeWidgetItemForChatId(msg.chat_id);
    QTreeWidgetItem *current_item = ui.lobbyTreeWidget->currentItem();

    if (myChatLobbyUserNotify)
    {
        QDateTime sendTime = QDateTime::fromTime_t(msg.sendTime);
        QString message = QString::fromUtf8(msg.msg.c_str());
        QString ownName = QString::fromUtf8(rsPeers->getPeerName(rsPeers->getOwnId()).c_str());
        QString name = msg.incoming? QString::fromStdString(rsPeers->getGPGName(rsPeers->getGPGId(msg.chat_id.toPeerId()))): ownName;
        if (msg.incoming) myChatLobbyUserNotify->chatP2PNewMessage(msg.chat_id, sendTime, name, message);
    }

    if (item && current_item && item == current_item)
    {
        //Need to update the message as read: When user receive new msg in the active chat window
        rsHistory->updateMessageAsRead(msg.chat_id);
        return;
    }
    if (item)
    {
        if (msg.incoming)
        {
            if (recentUnreadListOfChatId.count(msg.chat_id) == 0) recentUnreadListOfChatId.insert(msg.chat_id);

            QFont gpgFont;
            QPixmap gpgOverlayIcon = currentStatusIcon(msg.chat_id.toPeerId(), gpgFont);

            QIcon unreadIcon = lastIconForPeerId(msg.chat_id.toPeerId(), true);
            item->setIcon(COLUMN_NAME,unreadIcon) ;
            item->setFont(COLUMN_NAME,gpgFont) ;
        }
    }
}

void ChatLobbyWidget::itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    //subscribeChatLobbyAtItem(item);
}

void ChatLobbyWidget::displayChatLobbyEvent(qulonglong lobby_id, int event_type, const RsGxsId &gxs_id, const QString& str)
{
    if (ChatLobbyDialog *cld = dynamic_cast<ChatLobbyDialog*>(ChatDialog::getExistingChat(ChatId(lobby_id)))) {
        cld->displayLobbyEvent(event_type, gxs_id, str);
    }
}

void ChatLobbyWidget::readChatLobbyInvites()
{
    std::list<ChatLobbyInvite> invites;
    rsMsgs->getPendingChatLobbyInvites(invites);

    RsGxsId default_id ;
    rsMsgs->getDefaultIdentityForChatLobby(default_id) ;

	std::list<ChatLobbyId> subscribed_lobbies ;
	rsMsgs->getChatLobbyList(subscribed_lobbies) ;

	for(std::list<ChatLobbyInvite>::const_iterator it(invites.begin());it!=invites.end();++it)
	{
		// first check if the lobby is already subscribed. If so, just ignore the request.

		bool found = false ;
		for(auto it2(subscribed_lobbies.begin());it2!=subscribed_lobbies.end() && !found;++it2)
			found = found || (*it2 == (*it).lobby_id) ;

		if(found)
			continue ;

        QMessageBox mb(QObject::tr("Join chat room"),
                       tr("%1 invites you to chat room named %2").arg(QString::fromUtf8(rsPeers->getPeerName((*it).peer_id).c_str())).arg(RsHtml::plainText(it->lobby_name)),
                       QMessageBox::Question, QMessageBox::Yes,QMessageBox::No, 0);


		QLabel *label;
		GxsIdChooser *idchooser = new GxsIdChooser;

		if( (*it).lobby_flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED )
		{
			idchooser->loadIds(IDCHOOSER_ID_REQUIRED | IDCHOOSER_NON_ANONYMOUS,default_id) ;
			label = new QLabel(tr("Choose a non anonymous identity for this chat room:"));
		}
		else
		{
			idchooser->loadIds(IDCHOOSER_ID_REQUIRED,default_id) ;
			label = new QLabel(tr("Choose an identity for this chat room:"));
		}
		myInviteYesButton = mb.button(QMessageBox::Yes);
		myInviteIdChooser = idchooser;
		connect(idchooser, SIGNAL(currentIndexChanged(int)), this, SLOT(idChooserCurrentIndexChanged(int)));
		idChooserCurrentIndexChanged(0);

		QGridLayout* layout = qobject_cast<QGridLayout*>(mb.layout());
		if (layout) {
			layout->addWidget(label, layout->rowCount(), 0, 1, layout->columnCount(), Qt::AlignHCenter ) ;
			layout->addWidget(idchooser, layout->rowCount(), 0, 1, layout->columnCount(), Qt::AlignRight ) ;
		} else {
			//Not QGridLayout so add at end
			mb.layout()->addWidget(label) ;
			mb.layout()->addWidget(idchooser) ;
		}

		int res = mb.exec();
		myInviteYesButton = NULL;
		myInviteIdChooser = NULL;

        if (res == QMessageBox::No)
        {
            rsMsgs->denyLobbyInvite((*it).lobby_id);
            continue ;
        }

        RsGxsId chosen_id ;
        idchooser->getChosenId(chosen_id) ;

        if(chosen_id.isNull())
        {
            rsMsgs->denyLobbyInvite((*it).lobby_id);
            continue ;
        }

        if(rsMsgs->acceptLobbyInvite((*it).lobby_id,chosen_id))
        {
            ChatDialog::chatFriend(ChatId((*it).lobby_id),true);
            rsMsgs->setLobbyAutoSubscribe((*it).lobby_id, true);
            rsMsgs->joinVisibleChatLobby((*it).lobby_id, chosen_id);
        }
        else
            std::cerr << "Can't join chat room with id 0x" << std::hex << (*it).lobby_id << std::dec << std::endl;

    }

	myInviteYesButton = NULL;
	myInviteIdChooser = NULL;
}

void ChatLobbyWidget::idChooserCurrentIndexChanged(int /*index*/)
{
	if (myInviteYesButton && myInviteIdChooser)
	{
		RsGxsId chosen_id;
		switch (myInviteIdChooser->getChosenId(chosen_id))
		{
			case GxsIdChooser::KnowId:
			case GxsIdChooser::UnKnowId:
				myInviteYesButton->setEnabled(true);
			break;
			case GxsIdChooser::NoId:
			case GxsIdChooser::None:
			default: ;
				myInviteYesButton->setEnabled(false);
		}
	}
}

void ChatLobbyWidget::filterColumnChanged(int)
{
	filterItems(ui.filterLineEdit->text());
}

void ChatLobbyWidget::filterItems(const QString &text)
{
	int filterColumn = ui.filterLineEdit->currentFilter();

	int count = ui.lobbyTreeWidget->topLevelItemCount ();
	for (int index = 0; index < count; ++index) {
		filterItem(ui.lobbyTreeWidget->topLevelItem(index), text, filterColumn);
	}
}

bool ChatLobbyWidget::filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn)
{
    bool visible = true;

    if (text.isEmpty() == false) {
        if (item->text(filterColumn).contains(text, Qt::CaseInsensitive) == false) {
            visible = false;
        }
    }

    int visibleChildCount = 0;
    int count = item->childCount();
    for (int index = 0; index < count; ++index) {
        if (filterItem(item->child(index), text, filterColumn)) {
            ++visibleChildCount;
        }
    }

    if (visible || visibleChildCount) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    return (visible || visibleChildCount);
}


void ChatLobbyWidget::processSettings(bool bLoad)
{
	m_bProcessSettings = true;

	QHeaderView *Header = ui.lobbyTreeWidget->header () ;

	Settings->beginGroup(QString("ChatLobbyWidget"));

    if (bLoad) {
        // load settings
		ui.splitter->restoreState(Settings->value("splitter").toByteArray());
		// state of the lists
		Header->restoreState(Settings->value("lobbyList").toByteArray());

	} else {
		// save settings
		Settings->setValue("splitter", ui.splitter->saveState());
		// state of the lists
		Settings->setValue("lobbyList", Header->saveState());
	}

	Settings->endGroup();
    m_bProcessSettings = false;
}

void ChatLobbyWidget::setShowUserCountColumn(bool show)
{
//	if (ui.lobbyTreeWidget->isColumnHidden(COLUMN_USER_COUNT) == show) {
//		ui.lobbyTreeWidget->setColumnHidden(COLUMN_USER_COUNT, !show);
//	}
	ui.lobbyTreeWidget->header()->setVisible(getNumColVisible()>1);
}

void ChatLobbyWidget::setShowTopicColumn(bool show)
{
//	if (ui.lobbyTreeWidget->isColumnHidden(COLUMN_TOPIC) == show) {
//		ui.lobbyTreeWidget->setColumnHidden(COLUMN_TOPIC, !show);
//	}
	ui.lobbyTreeWidget->header()->setVisible(getNumColVisible()>1);
}

void ChatLobbyWidget::setShowSubscribeColumn(bool show)
{
//	if (ui.lobbyTreeWidget->isColumnHidden(COLUMN_SUBSCRIBED) == show) {
//		ui.lobbyTreeWidget->setColumnHidden(COLUMN_SUBSCRIBED, !show);
//	}
	ui.lobbyTreeWidget->header()->setVisible(getNumColVisible()>1);
}

int ChatLobbyWidget::getNumColVisible()
{
	int iNumColVis=0;
	for (int iColumn = 0; iColumn < COLUMN_COUNT; ++iColumn) {
		if (!ui.lobbyTreeWidget->isColumnHidden(iColumn)) {
			++iNumColVis;
		}
	}
    return iNumColVis;
}

void ChatLobbyWidget::openOne2OneChat(std::string rsId, std::string nickname)
{
  RsPgpId pgpId = RsPgpId(rsId);
  RsPeerDetails detail;
  if (rsPeers->getGPGDetails(pgpId, detail))
    {
        // just open a window as normal (using RS calling), add the one2one chat page will be processed in addOne2OneChatPage.
        ChatDialog::chatFriend(pgpId);
    }
}



void ChatLobbyWidget::resetAvatarForContactItem( const ChatId& chatId)
{
    QTreeWidgetItem *item = getTreeWidgetItemForChatId(chatId);
    QPixmap avatar;
    AvatarDefs::getAvatarFromSslId(chatId.toPeerId(), avatar);
    if (!avatar.isNull() && item)
          item->setIcon(COLUMN_NAME,QIcon(avatar)) ;
}

static QIcon createAvatar(const QPixmap &avatar, const QPixmap &overlay, bool unread)
{
    int avatarWidth = avatar.width();
    int avatarHeight = avatar.height();

    QPixmap pixmap(avatar);

    int overlaySize = (avatarWidth > avatarHeight) ? (avatarWidth/2.5) :  (avatarHeight/2.5);
    int overlayX = avatarWidth - overlaySize;
    int overlayY = avatarHeight - overlaySize;

    QPainter painter(&pixmap);
    painter.drawPixmap(overlayX, overlayY, overlaySize, overlaySize, overlay);
    if (unread)
    {
        QIcon unreadIcon = QIcon(IMAGE_UNREAD_ICON);
        int overlayUnreadY = avatarHeight - 2.5*overlaySize;
        int overlayUnreadX = avatarHeight - overlaySize;
        QPixmap unreadOverlay = unreadIcon.pixmap(unreadIcon.actualSize(QSize(25, 15)));
        painter.drawPixmap(overlayUnreadX, overlayUnreadY, overlaySize, overlaySize*1.5, unreadOverlay);    //d: increase size unreadIcon
    }

    QIcon icon;
    icon.addPixmap(pixmap);
    return icon;
}
QPixmap ChatLobbyWidget::currentStatusIcon(RsPeerId peerId, QFont& gpgFontOut)
{
    StatusInfo statusContactInfo;
    rsStatus->getStatus(peerId,statusContactInfo);
    QFont gpgFont;
    QPixmap gpgOverlayIcon;
    gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_OFFLINE));
    switch (statusContactInfo.status)
    {
        case RS_STATUS_INACTIVE:
            gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_OFFLINE));
            gpgFont.setBold(false);
            break;

        case RS_STATUS_ONLINE:
            gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_ONLINE));
            gpgFont.setBold(true);
            break;

        case RS_STATUS_AWAY:
            gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_AWAY));
            gpgFont.setBold(true);
            break;

        case RS_STATUS_BUSY:
            gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_BUSY));
            gpgFont.setBold(true);
            break;
    }
    gpgFontOut = gpgFont;
    return gpgOverlayIcon;
}

void ChatLobbyWidget::updateContactItem(QTreeWidget *treeWidget, QTreeWidgetItem *item, const std::string &nickname, const ChatId& chatId, const std::string &rsId, uint current_time, bool unread)
{
      item->setText(COLUMN_NAME, QString::fromUtf8(nickname.c_str()));

      if (unread)
      {
          if (recentUnreadListOfChatId.count(chatId) == 0) recentUnreadListOfChatId.insert(chatId);
      }
      QFont gpgFont;
      QPixmap gpgOverlayIcon = currentStatusIcon(chatId.toPeerId(), gpgFont);
      QIcon unreadIcon = lastIconForPeerId(chatId.toPeerId(), unread);
      item->setIcon(COLUMN_NAME, unreadIcon);
      item->setFont(COLUMN_NAME, gpgFont);

      item->setData(COLUMN_NAME, ROLE_SORT, QString::fromUtf8(nickname.c_str()));
      item->setData(COLUMN_DATA, ROLE_ID, QString::fromUtf8(rsId.c_str()));
      item->setData(COLUMN_RECENT_TIME, ROLE_SORT,current_time);
}

void ChatLobbyWidget::updateGroupChatItem(QTreeWidget *treeWidget, QTreeWidgetItem *item, const std::string &name, const ChatLobbyId& id, uint current_time, bool unread, ChatLobbyFlags lobby_flags)
{

    std::cerr << " Add group chat item to the common item when get history, group name: " << name << std::endl;
    item->setText(COLUMN_NAME, QString::fromUtf8(name.c_str()));
    item->setData(COLUMN_NAME, ROLE_SORT, QString::fromUtf8(name.c_str()));

    if (unread)
    {
        ChatId chatId(id);
        if (recentUnreadListOfChatId.count(chatId) == 0) recentUnreadListOfChatId.insert(chatId);
        QIcon icon = (lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_MESSAGE) : QIcon(IMAGE_MESSAGE_PRIVATE);
        item->setIcon(COLUMN_NAME, icon) ;
    }

    item->setData(COLUMN_DATA, ROLE_ID, (qulonglong)id);
    item->setData(COLUMN_DATA, ROLE_FLAGS, lobby_flags.toUInt32());

    QColor color = treeWidget->palette().color(QPalette::Active, QPalette::Text);

    for (int column = 0; column < COLUMN_COUNT; ++column) {
        item->setTextColor(column, color);
    }
    QString tooltipstr = QObject::tr("Group name:")+" "+item->text(COLUMN_NAME)+"\n"
                     +QObject::tr("Group type:")+" "+ (lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC? QObject::tr("Public group (Community group) "): QObject::tr("Private group (members only)"))+"\n"
                     +QObject::tr("Id:")+" "+QString::number(id,16) ;

//    if(lobby_flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED)
//    {
//        tooltipstr += QObject::tr("\nSecurity: no anonymous IDs") ;
//        QColor foreground = QColor(0, 128, 0); // green
//        for (int column = 0; column < COLUMN_COUNT; ++column)
//            item->setTextColor(column, foreground);
//    }
    item->setToolTip(0,tooltipstr) ;

    item->setData(COLUMN_RECENT_TIME, ROLE_SORT,current_time);
}

QIcon ChatLobbyWidget::lastIconForPeerId(RsPeerId peerId, bool unread)
{
    QPixmap bestAvatar;
    AvatarDefs::getAvatarFromSslId(peerId, bestAvatar);
    QFont gpgFont;
    QPixmap gpgOverlayIcon = currentStatusIcon(peerId, gpgFont);
    return createAvatar(bestAvatar.isNull() ? QPixmap(AVATAR_DEFAULT_IMAGE) : bestAvatar, gpgOverlayIcon, unread);

}

void ChatLobbyWidget::UpdateStatusForContact(QTreeWidgetItem* gpgItem , const RsPeerId peerId)
{
    StatusInfo statusContactInfo;
    rsStatus->getStatus(peerId,statusContactInfo);
    QFont gpgFont;
    QPixmap gpgOverlayIcon;
    gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_OFFLINE));
    switch (statusContactInfo.status)
    {
        case RS_STATUS_INACTIVE:
            gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_OFFLINE));
            gpgFont.setBold(false);
            break;

        case RS_STATUS_ONLINE:
            gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_ONLINE));
            gpgFont.setBold(true);
            break;

        case RS_STATUS_AWAY:
            gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_AWAY));
            gpgFont.setBold(true);
            break;

        case RS_STATUS_BUSY:
            gpgOverlayIcon = QPixmap(StatusDefs::imageStatus(RS_STATUS_BUSY));
            gpgFont.setBold(true);
            break;
    }

    //Need to set status Icon to avatar
    QPixmap bestAvatar;
    AvatarDefs::getAvatarFromSslId(peerId, bestAvatar);
    ChatId chatId(peerId);
    bool unread = (recentUnreadListOfChatId.count(chatId) != 0? true: false);
    gpgItem->setIcon(COLUMN_NAME, createAvatar(bestAvatar.isNull() ? QPixmap(AVATAR_DEFAULT_IMAGE) : bestAvatar, gpgOverlayIcon, unread));
    gpgItem->setFont(COLUMN_NAME, gpgFont);

}

void ChatLobbyWidget::ContactStatusChanged(QString peerIdStr, int status)
{
    RsPeerId peerId(peerIdStr.toStdString());
    ChatId chatId(peerId);
    QTreeWidgetItem* curItem = getTreeWidgetItemForChatId(chatId);
    if (curItem)
        UpdateStatusForContact(curItem, peerId);
}

void ChatLobbyWidget::UpdateStatusForAllContacts()
{
    std::list<RsPeerId> peerIds;
    if (rsPeers->getFriendList(peerIds))
    {
        std::list<RsPeerId>::iterator peerId;
        for (peerId = peerIds.begin(); peerId != peerIds.end(); ++peerId)
        {
            ChatId chatId(*peerId);
            QTreeWidgetItem* curItem = getTreeWidgetItemForChatId(chatId);
            if (curItem)
                UpdateStatusForContact(curItem, *peerId);
        }
    }
}
