/*******************************************************************************
 * gui/ChatLobbyWidget.cpp                                                     *
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

#include <algorithm>
#include <time.h>

//#define CHAT_LOBBY_GUI_DEBUG 1

#define COLUMN_NAME       0
#define COLUMN_USER_COUNT 1
#define COLUMN_TOPIC      2
#define COLUMN_COUNT      3
#define COLUMN_DATA       0

#define ROLE_SORT          Qt::UserRole
#define ROLE_ID            Qt::UserRole + 1
#define ROLE_SUBSCRIBED    Qt::UserRole + 2
#define ROLE_PRIVACYLEVEL  Qt::UserRole + 3
#define ROLE_AUTOSUBSCRIBE Qt::UserRole + 4
#define ROLE_FLAGS         Qt::UserRole + 5


#define TYPE_FOLDER       0
#define TYPE_LOBBY        1

#define IMAGE_CREATE          ""
#define IMAGE_PUBLIC          ":/images/chat_x24.png"
#define IMAGE_PRIVATE         ":/images/chat_red24.png"
#define IMAGE_SUBSCRIBE       ":/images/edit_add24.png"  
#define IMAGE_UNSUBSCRIBE     ":/images/cancel.png"
#define IMAGE_PEER_ENTERING   ":images/user/add_user24.png"
#define IMAGE_PEER_LEAVING    ":images/user/remove_user24.png"
#define IMAGE_TYPING		      ":images/typing.png" 
#define IMAGE_MESSAGE	      ":images/chat.png" 
#define IMAGE_AUTOSUBSCRIBE   ":images/accepted16.png"
#define IMAGE_COPYRSLINK      ":/images/copyrslink.png"

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

	QObject::connect( ui.lobbyTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(lobbyTreeWidgetCustomPopupMenu(QPoint)));
	QObject::connect( ui.lobbyTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
	QObject::connect( ui.lobbyTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateCurrentLobby()));

	QObject::connect( ui.filterLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterItems(QString)));
	QObject::connect( ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));
	QObject::connect( ui.createLobbyToolButton, SIGNAL(clicked()), this, SLOT(createChatLobby()));

	compareRole = new RSTreeWidgetItemCompareRole;
	compareRole->setRole(COLUMN_NAME, ROLE_SORT);

	ui.lobbyTreeWidget->setColumnCount(COLUMN_COUNT);
	ui.lobbyTreeWidget->sortItems(COLUMN_NAME, Qt::AscendingOrder);

	ui.lobbyTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu) ;

	QTreeWidgetItem *headerItem = ui.lobbyTreeWidget->headerItem();
	headerItem->setText(COLUMN_NAME, tr("Name"));
	headerItem->setText(COLUMN_USER_COUNT, tr("Count"));
	headerItem->setText(COLUMN_TOPIC, tr("Topic"));
	headerItem->setTextAlignment(COLUMN_NAME, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(COLUMN_TOPIC, Qt::AlignHCenter | Qt::AlignVCenter);
	headerItem->setTextAlignment(COLUMN_USER_COUNT, Qt::AlignHCenter | Qt::AlignVCenter);

	QHeaderView *header = ui.lobbyTreeWidget->header();
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_NAME, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_USER_COUNT, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_TOPIC, QHeaderView::Interactive);

    privateSubLobbyItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
    privateSubLobbyItem->setText(COLUMN_NAME, tr("Private Subscribed chat rooms"));
	privateSubLobbyItem->setData(COLUMN_NAME, ROLE_SORT, "1");
	//	privateLobbyItem->setIcon(COLUMN_NAME, QIcon(IMAGE_PRIVATE));
    privateSubLobbyItem->setData(COLUMN_DATA, ROLE_PRIVACYLEVEL, CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE);
	ui.lobbyTreeWidget->insertTopLevelItem(0, privateSubLobbyItem);

	publicSubLobbyItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
	publicSubLobbyItem->setText(COLUMN_NAME, tr("Public Subscribed chat rooms"));
	publicSubLobbyItem->setData(COLUMN_NAME, ROLE_SORT, "2");
	//	publicLobbyItem->setIcon(COLUMN_NAME, QIcon(IMAGE_PUBLIC));
    publicSubLobbyItem->setData(COLUMN_DATA, ROLE_PRIVACYLEVEL, CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC);
	ui.lobbyTreeWidget->insertTopLevelItem(1, publicSubLobbyItem);

	privateLobbyItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
	privateLobbyItem->setText(COLUMN_NAME, tr("Private chat rooms"));
	privateLobbyItem->setData(COLUMN_NAME, ROLE_SORT, "3");
	//	privateLobbyItem->setIcon(COLUMN_NAME, QIcon(IMAGE_PRIVATE));
    privateLobbyItem->setData(COLUMN_DATA, ROLE_PRIVACYLEVEL, CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE);
	ui.lobbyTreeWidget->insertTopLevelItem(2, privateLobbyItem);

	publicLobbyItem = new RSTreeWidgetItem(compareRole, TYPE_FOLDER);
	publicLobbyItem->setText(COLUMN_NAME, tr("Public chat rooms"));
	publicLobbyItem->setData(COLUMN_NAME, ROLE_SORT, "4");
	//	publicLobbyItem->setIcon(COLUMN_NAME, QIcon(IMAGE_PUBLIC));
    publicLobbyItem->setData(COLUMN_DATA, ROLE_PRIVACYLEVEL, CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC);
	ui.lobbyTreeWidget->insertTopLevelItem(3, publicLobbyItem);

	ui.lobbyTreeWidget->expandAll();
	ui.lobbyTreeWidget->setColumnHidden(COLUMN_NAME,false) ;
	ui.lobbyTreeWidget->setColumnHidden(COLUMN_USER_COUNT,true) ;
	ui.lobbyTreeWidget->setColumnHidden(COLUMN_TOPIC,true) ;
	ui.lobbyTreeWidget->setSortingEnabled(true) ;

    	float fact = QFontMetricsF(font()).height()/14.0f;
        
	ui.lobbyTreeWidget->adjustSize();
	ui.lobbyTreeWidget->setColumnWidth(COLUMN_NAME,100*fact);
	ui.lobbyTreeWidget->setColumnWidth(COLUMN_USER_COUNT, 50*fact);
	ui.lobbyTreeWidget->setColumnWidth(COLUMN_TOPIC, 50*fact);

	/** Setup the actions for the header context menu */
	showUserCountAct= new QAction(headerItem->text(COLUMN_USER_COUNT),this);
    showUserCountAct->setCheckable(true); showUserCountAct->setToolTip(tr("Show")+" "+showUserCountAct->text()+" "+tr("column"));
    connect(showUserCountAct,SIGNAL(triggered(bool)),this,SLOT(setShowUserCountColumn(bool))) ;
    showTopicAct= new QAction(headerItem->text(COLUMN_TOPIC),this);
    showTopicAct->setCheckable(true); showTopicAct->setToolTip(tr("Show")+" "+showTopicAct->text()+" "+tr("column"));
    connect(showTopicAct,SIGNAL(triggered(bool)),this,SLOT(setShowTopicColumn(bool))) ;

	// Set initial size of the splitter
	ui.splitter->setStretchFactor(0, 0);
	ui.splitter->setStretchFactor(1, 1);

	QList<int> sizes;
	sizes << 200*fact << width(); // Qt calculates the right sizes
	ui.splitter->setSizes(sizes);

	lobbyChanged();
	showBlankPage(0) ;

	/* add filter actions */
	ui.filterLineEdit->addFilter(QIcon(), tr("Name"), COLUMN_NAME, tr("Search Name"));
	ui.filterLineEdit->setCurrentFilter(COLUMN_NAME);

	// load settings
	processSettings(true);

    int S = QFontMetricsF(font()).height();
    QString help_str = tr("\
                          <h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Chat Rooms</h1>                              \
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

                    if(idd.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idd.mAvatar.mData, idd.mAvatar.mSize, pixmap, GxsIdDetails::SMALL))
                        pixmap = GxsIdDetails::makeDefaultIcon(*it,GxsIdDetails::SMALL) ;

                    QAction *action = mnu->addAction(QIcon(pixmap), QString("%1 (%2)").arg(QString::fromUtf8(idd.mNickname.c_str()), QString::fromStdString((*it).toStdString())), this, SLOT(subscribeChatLobbyAs()));
                    action->setData(QString::fromStdString((*it).toStdString())) ;
                }
            }
        }

        if (item->data(COLUMN_DATA, ROLE_AUTOSUBSCRIBE).toBool())
            contextMnu.addAction(QIcon(IMAGE_AUTOSUBSCRIBE), tr("Remove Auto Subscribe"), this, SLOT(autoSubscribeItem()));
        else if(!own_identities.empty())
            contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Add Auto Subscribe"), this, SLOT(autoSubscribeItem()));

        contextMnu.addAction(QIcon(IMAGE_COPYRSLINK), tr("Copy RetroShare Link"), this, SLOT(copyItemLink()));
    }

        contextMnu.addSeparator();//-------------------------------------------------------------------

        showUserCountAct->setChecked(!ui.lobbyTreeWidget->isColumnHidden(COLUMN_USER_COUNT));
        showTopicAct->setChecked(!ui.lobbyTreeWidget->isColumnHidden(COLUMN_TOPIC));

        QMenu *menu = contextMnu.addMenu(tr("Columns"));
        menu->addAction(showUserCountAct);
        menu->addAction(showTopicAct);

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

    //item->setText(COLUMN_USER_COUNT, QString::number(count));
    item->setData(COLUMN_USER_COUNT, Qt::EditRole, count);

	item->setData(COLUMN_DATA, ROLE_ID, (qulonglong)id);
	item->setData(COLUMN_DATA, ROLE_SUBSCRIBED, subscribed);
	item->setData(COLUMN_DATA, ROLE_FLAGS, lobby_flags.toUInt32());
    item->setData(COLUMN_DATA, ROLE_AUTOSUBSCRIBE, autoSubscribe);

	QColor color = treeWidget->palette().color(QPalette::Active, QPalette::Text);
    
	if (!subscribed) {
		// Average between Base and Text colors
		QColor color2 = treeWidget->palette().color(QPalette::Active, QPalette::Base);
		color.setRgbF((color2.redF()+color.redF())/2, (color2.greenF()+color.greenF())/2, (color2.blueF()+color.blueF())/2);
	}

	for (int column = 0; column < COLUMN_COUNT; ++column) {
		item->setTextColor(column, color);
	}
    QString tooltipstr = QObject::tr("Subject:")+" "+item->text(COLUMN_TOPIC)+"\n"
                     +QObject::tr("Participants:")+" "+QString::number(count)+"\n"
                     +QObject::tr("Auto Subscribe:")+" "+(autoSubscribe? QObject::tr("enabled"): QObject::tr("disabled"))+"\n"
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

void ChatLobbyWidget::updateDisplay()
{
#ifdef CHAT_LOBBY_GUI_DEBUG
	std::cerr << "updating chat room display!" << std::endl;
#endif
	std::vector<VisibleChatLobbyRecord> visibleLobbies;
	rsMsgs->getListOfNearbyChatLobbies(visibleLobbies);

    std::list<ChatLobbyId> lobbies;
	rsMsgs->getChatLobbyList(lobbies);

#ifdef CHAT_LOBBY_GUI_DEBUG
	std::cerr << "got " << visibleLobbies.size() << " visible lobbies" << std::endl;
#endif

	// now, do a nice display of lobbies
	
    RsPeerId vpid;
    std::list<ChatLobbyId>::const_iterator lobbyIt;

	// remove not existing public lobbies

	for(int p=0;p<4;++p)
	{
		QTreeWidgetItem *lobby_item =NULL;
		switch (p) 
		{
			case 0: lobby_item = privateSubLobbyItem; break;
			case 1: lobby_item = publicSubLobbyItem; break;
			case 2: lobby_item = privateLobbyItem; break;
			default:
			case 3: lobby_item = publicLobbyItem; break;
		}
		//QTreeWidgetItem *lobby_item = (p==0)?publicLobbyItem:privateLobbyItem ;

		int childCnt = lobby_item->childCount();
		int childIndex = 0;

		while (childIndex < childCnt) {
			QTreeWidgetItem *itemLoop = lobby_item->child(childIndex);
			if (itemLoop->type() == TYPE_LOBBY) 
			{
				// check for visible lobby
				//
				uint32_t i;
	
				for (i = 0; i < visibleLobbies.size(); ++i) 
					if (itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == visibleLobbies[i].lobby_id) 
						break;

				if (i >= visibleLobbies.size()) 
				{
					// Check for participating lobby with public level
					//
					for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) 
                        if(itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == *lobbyIt)
							break;

					if (lobbyIt == lobbies.end()) 
					{
						delete(lobby_item->takeChild(lobby_item->indexOfChild(itemLoop)));
						childCnt = lobby_item->childCount();
						continue;
					}
				}
			}
			++childIndex;
		}
	}

	// Now add visible lobbies
	//
	for (uint32_t i = 0; i < visibleLobbies.size(); ++i) 
	{
		const VisibleChatLobbyRecord &lobby = visibleLobbies[i];

#ifdef CHAT_LOBBY_GUI_DEBUG
		std::cerr << "adding " << lobby.lobby_name << "topic " << lobby.lobby_topic << " #" << std::hex << lobby.lobby_id << std::dec << " public " << lobby.total_number_of_peers << " peers. Lobby type: " << lobby.lobby_privacy_level << std::endl;
#endif


        bool subscribed = std::find(lobbies.begin(), lobbies.end(), lobby.lobby_id) != lobbies.end();

		QTreeWidgetItem *item = NULL;
		QTreeWidgetItem *lobby_item =NULL;
        QTreeWidgetItem *lobby_other_item =NULL;

        if (lobby.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC)
        {
            if (subscribed)
            {
                lobby_item = publicSubLobbyItem;
                lobby_other_item = publicLobbyItem;
            }
            else
            {
                lobby_item = publicLobbyItem;
                lobby_other_item = publicSubLobbyItem;
            }
        }
        else
        {
            if (subscribed)
            {
                lobby_item = privateSubLobbyItem;
                lobby_other_item = privateLobbyItem;
            }
            else
            {
                lobby_item = privateLobbyItem;
                lobby_other_item = privateSubLobbyItem;
            }
        }
		//QTreeWidgetItem *lobby_item = (lobby.lobby_privacy_level == RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC)?publicLobbyItem:privateLobbyItem ;

		// Search existing item
		//
		int childCnt = lobby_other_item->childCount();
		for (int childIndex = 0; childIndex < childCnt; ++childIndex)
		{
			QTreeWidgetItem *itemLoop = lobby_other_item->child(childIndex);
			if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobby.lobby_id) {
				delete(lobby_other_item->takeChild(lobby_other_item->indexOfChild(itemLoop)));
				childCnt = lobby_other_item->childCount();
				break;
			}
		}
		childCnt = lobby_item->childCount();
		for (int childIndex = 0; childIndex < childCnt; ++childIndex)
		{
			QTreeWidgetItem *itemLoop = lobby_item->child(childIndex);
			if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobby.lobby_id) {
				item = itemLoop;
				break;
			}
		}

	ChatLobbyFlags lobby_flags = lobby.lobby_flags ;
    
		QIcon icon;
		if (item == NULL) 
		{
			item = new RSTreeWidgetItem(compareRole, TYPE_LOBBY);
            icon = (lobby.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
			lobby_item->addChild(item);
            
		} 
		else
		{
			if (item->data(COLUMN_DATA, ROLE_SUBSCRIBED).toBool() != subscribed) {
				// Replace icon
                icon = (lobby.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
			}
		}
		if (!icon.isNull()) {
			item->setIcon(COLUMN_NAME, subscribed ? icon : icon.pixmap(ui.lobbyTreeWidget->iconSize(), QIcon::Disabled));
		}

		bool autoSubscribe = rsMsgs->getLobbyAutoSubscribe(lobby.lobby_id);

		if (autoSubscribe && subscribed)
		{
			if(_lobby_infos.find(lobby.lobby_id) == _lobby_infos.end())
			{
				if (item == ui.lobbyTreeWidget->currentItem())
				{
                    ChatDialog::chatFriend(ChatId(lobby.lobby_id)) ;
				}else{
                    ChatDialog::chatFriend(ChatId(lobby.lobby_id),false) ;
				}
			}
		}

		updateItem(ui.lobbyTreeWidget, item, lobby.lobby_id, lobby.lobby_name,lobby.lobby_topic, lobby.total_number_of_peers, subscribed, autoSubscribe,lobby_flags);
	}

	//	time_t now = time(NULL) ;

	// Now add participating lobbies.
	//
	for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) 
    {
        ChatLobbyInfo lobby ;
        rsMsgs->getChatLobbyInfo(*lobbyIt,lobby) ;

#ifdef CHAT_LOBBY_GUI_DEBUG
		std::cerr << "adding " << lobby.lobby_name << "topic " << lobby.lobby_topic << " #" << std::hex << lobby.lobby_id << std::dec << " private " << lobby.nick_names.size() << " peers." << std::endl;
#endif

		QTreeWidgetItem *itemParent;
        if (lobby.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC)
            itemParent = publicSubLobbyItem;
        else
            itemParent = privateSubLobbyItem;

		QTreeWidgetItem *item = NULL;

		// search existing item
		int childCount = itemParent->childCount();
		for (int childIndex = 0; childIndex < childCount; ++childIndex) {
			QTreeWidgetItem *itemLoop = itemParent->child(childIndex);
			if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == lobby.lobby_id) {
				item = itemLoop;
				break;
			}
		}

		QIcon icon;
        
        ChatLobbyFlags lobby_flags = lobby.lobby_flags ;
        
        if (item == NULL) 
        {
            item = new RSTreeWidgetItem(compareRole, TYPE_LOBBY);
            icon = (lobby.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
            itemParent->addChild(item);
        } else {
            if (!item->data(COLUMN_DATA, ROLE_SUBSCRIBED).toBool()) {
                // Replace icon
                icon = (lobby.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
            }
        }
		if (!icon.isNull()) {
			item->setIcon(COLUMN_NAME, icon);
		}

		bool autoSubscribe = rsMsgs->getLobbyAutoSubscribe(lobby.lobby_id);

        updateItem(ui.lobbyTreeWidget, item, lobby.lobby_id, lobby.lobby_name,lobby.lobby_topic, lobby.gxs_ids.size(), true, autoSubscribe,lobby_flags);
	}
	publicSubLobbyItem->setHidden(publicSubLobbyItem->childCount()==0);
	publicSubLobbyItem->setText(COLUMN_NAME, tr("Public Subscribed chat rooms")+ QString(" (") + QString::number(publicSubLobbyItem->childCount())+QString(")"));
	privateSubLobbyItem->setHidden(privateSubLobbyItem->childCount()==0);
	publicLobbyItem->setText(COLUMN_NAME, tr("Public chat rooms")+ " (" + QString::number(publicLobbyItem->childCount())+QString(")"));
}

void ChatLobbyWidget::createChatLobby()
{
	int privacyLevel = 0;
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
		showBlankPage(id) ;
	else
		ui.stackedWidget->setCurrentWidget(_lobby_infos[id].dialog) ;
}

// this function is for the case where we don't have any identity yet
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

	// Update information
	std::vector<VisibleChatLobbyRecord> lobbies;
    rsMsgs->getListOfNearbyChatLobbies(lobbies);

    std::list<RsGxsId> my_ids ;
    rsIdentity->getOwnIds(my_ids) ;

                trimAnonIds(my_ids) ;
                
	for(std::vector<VisibleChatLobbyRecord>::const_iterator it(lobbies.begin());it!=lobbies.end();++it)
		if( (*it).lobby_id == id)
		{
			ui.lobbyname_lineEdit->setText( RsHtml::plainText(it->lobby_name) );
			ui.lobbyid_lineEdit->setText( QString::number((*it).lobby_id,16) );
			ui.lobbytopic_lineEdit->setText( RsHtml::plainText(it->lobby_topic) );
			ui.lobbytype_lineEdit->setText( (( (*it).lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC)?tr("Public"):tr("Private")) );
			ui.lobbysec_lineEdit->setText( (( (*it).lobby_flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED)?tr("No anonymous IDs"):tr("Anonymous IDs accepted")) );
			ui.lobbypeers_lineEdit->setText( QString::number((*it).total_number_of_peers) );

            QString text = tr("You're not subscribed to this chat room; Double click-it to enter and chat.") ;
            
				if(my_ids.empty())
				{
					if( (*it).lobby_flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED)
						text += "\n\n"+tr("You will need to create a non anonymous identity in order to join this chat room.") ;
					else
						text += "\n\n"+tr("You will need to create an identity in order to join chat rooms.") ;
				}

            ui.lobbyInfoLabel->setText(text);
			return ;
		}

	ui.lobbyname_lineEdit->clear();
	ui.lobbyid_lineEdit->clear();
	ui.lobbytopic_lineEdit->clear();
	ui.lobbytype_lineEdit->clear();
	ui.lobbypeers_lineEdit->clear();
	ui.lobbysec_lineEdit->clear();

	QString text = tr("No chat room selected. \nSelect chat rooms at left to show details.\nDouble click a chat room to enter and chat.") ;
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

QTreeWidgetItem *ChatLobbyWidget::getTreeWidgetItem(ChatLobbyId id)
{
    for(int p=0;p<4;++p)
	{
        QTreeWidgetItem *lobby_item =NULL;
        switch (p) {
        case 0: lobby_item = privateSubLobbyItem; break;
        case 1: lobby_item = publicSubLobbyItem; break;
        case 2: lobby_item = privateLobbyItem; break;
        case 3: lobby_item = publicLobbyItem; break;
        default: lobby_item = publicLobbyItem;
        }
        //QTreeWidgetItem *lobby_item = (p==0)?publicLobbyItem:privateLobbyItem ;

		int childCnt = lobby_item->childCount();
		int childIndex = 0;

		while (childIndex < childCnt) {
			QTreeWidgetItem *itemLoop = lobby_item->child(childIndex);

			if (itemLoop->type() == TYPE_LOBBY && itemLoop->data(COLUMN_DATA, ROLE_ID).toULongLong() == id) 
				return itemLoop ;

			++childIndex ;
		}
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

void ChatLobbyWidget::updateCurrentLobby()
{
	QList<QTreeWidgetItem *> items = ui.lobbyTreeWidget->selectedItems() ;

	if(items.empty())
		showLobby(0) ;
	else
	{
		QTreeWidgetItem *item = items.front();
		showLobby(item);

		ChatLobbyId id = item->data(COLUMN_DATA, ROLE_ID).toULongLong();

		if(_lobby_infos.find(id) != _lobby_infos.end()) {
            int iPrivacyLevel= item->parent()->data(COLUMN_DATA, ROLE_PRIVACYLEVEL).toInt();
            QIcon icon = (iPrivacyLevel==CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC) ? QIcon(IMAGE_PUBLIC) : QIcon(IMAGE_PRIVATE);
			_lobby_infos[id].default_icon = icon ;
			item->setIcon(COLUMN_NAME, icon) ;
		}
	}

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
	if(bIsCurrentItem)
		return ;

	_lobby_infos[id].default_icon = QIcon(IMAGE_MESSAGE) ;

	QTreeWidgetItem *item = getTreeWidgetItem(id) ;

	if(item == NULL)
		return ;

	item->setIcon(COLUMN_NAME,_lobby_infos[id].default_icon) ;
}

void ChatLobbyWidget::itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    subscribeChatLobbyAtItem(item);
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
            ChatDialog::chatFriend(ChatId((*it).lobby_id),true);
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

		setShowUserCountColumn(Settings->value("showUserCountColumn", !ui.lobbyTreeWidget->isColumnHidden(COLUMN_USER_COUNT)).toBool());
		setShowTopicColumn(Settings->value("showTopicColumn", !ui.lobbyTreeWidget->isColumnHidden(COLUMN_TOPIC)).toBool());
	} else {
		// save settings
		Settings->setValue("splitter", ui.splitter->saveState());
		// state of the lists
		Settings->setValue("lobbyList", Header->saveState());

		Settings->setValue("showUserCountColumn", !ui.lobbyTreeWidget->isColumnHidden(COLUMN_USER_COUNT));
		Settings->setValue("showTopicColumn", !ui.lobbyTreeWidget->isColumnHidden(COLUMN_TOPIC));
	}

	Settings->endGroup();
    m_bProcessSettings = false;
}

void ChatLobbyWidget::setShowUserCountColumn(bool show)
{
	if (ui.lobbyTreeWidget->isColumnHidden(COLUMN_USER_COUNT) == show) {
		ui.lobbyTreeWidget->setColumnHidden(COLUMN_USER_COUNT, !show);
	}
	ui.lobbyTreeWidget->header()->setVisible(getNumColVisible()>1);
}

void ChatLobbyWidget::setShowTopicColumn(bool show)
{
	if (ui.lobbyTreeWidget->isColumnHidden(COLUMN_TOPIC) == show) {
		ui.lobbyTreeWidget->setColumnHidden(COLUMN_TOPIC, !show);
	}
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
