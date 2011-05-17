/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2011 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QShortcut>
#include <QWidgetAction>
#include <QScrollBar>
#include <QColorDialog>
#include <QFontDialog>
#include <QDropEvent>
#include "common/vmessagebox.h"
#include "common/StatusDefs.h"
#include "common/GroupDefs.h"
#include "common/Emoticons.h"
#include <gui/mainpagestack.h>

#include "retroshare/rsinit.h"
#include "FriendsDialog.h"
#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsnotify.h>
#include "settings/rsharesettings.h"
#include "notifyqt.h"

#include "chat/PopupChatDialog.h"
#include "msgs/MessageComposer.h"
#include "connect/ConfCertDialog.h"
#include "profile/ProfileView.h"
#include "profile/ProfileWidget.h"
#include "profile/StatusMessage.h"

#include "connect/ConnectFriendWizard.h"
#include "forums/CreateForum.h"
#include "channels/CreateChannel.h"
#include "groups/CreateGroup.h"
#include "feeds/AttachFileItem.h"
#include "im_history/ImHistoryBrowser.h"
#include "common/RSTreeWidgetItem.h"

#include "RetroShareLink.h"

#include "MainWindow.h"
#include "NewsFeed.h"
#include "util/misc.h"

#include <sstream>
#include <time.h>
#include <sys/stat.h>
#include <algorithm>

#include <QSound>

/* Images for context menu icons */
#define IMAGE_DENYFRIEND         ":/images/denied16.png"
#define IMAGE_REMOVEFRIEND       ":/images/removefriend16.png"
#define IMAGE_EXPORTFRIEND       ":/images/exportpeers_16x16.png"
#define IMAGE_ADDFRIEND          ":/images/user/add_user16.png"
#define IMAGE_FRIENDINFO         ":/images/peerdetails_16x16.png"
#define IMAGE_CHAT               ":/images/chat.png"
#define IMAGE_MSG                ":/images/mail_new.png"
#define IMAGE_CONNECT            ":/images/connect_friend.png"
#define IMAGE_COPYLINK           ":/images/copyrslink.png"
/* Images for Status icons */
#define IMAGE_AVAILABLE          ":/images/user/identityavaiblecyan24.png"
#define IMAGE_CONNECT2           ":/images/reload24.png"
#define IMAGE_PASTELINK          ":/images/pasterslink.png"
#define IMAGE_GROUP16            ":/images/user/group16.png"
#define IMAGE_GROUP24            ":/images/user/group24.png"
#define IMAGE_EDIT               ":/images/edit_16.png"
#define IMAGE_REMOVE             ":/images/delete.png"
#define IMAGE_EXPAND             ":/images/edit_add24.png"
#define IMAGE_COLLAPSE           ":/images/edit_remove24.png"
#define IMAGE_NEWSFEED           ""
#define IMAGE_NEWSFEED_NEW       ":/images/message-state-new.png"

#define COLUMN_COUNT    2
#define COLUMN_NAME     0
#define COLUMN_STATE    1

#define COLUMN_DATA     0 // column for storing the userdata id

#define ROLE_SORT        Qt::UserRole
#define ROLE_ID          Qt::UserRole + 1
#define ROLE_STANDARD    Qt::UserRole + 2

#define TYPE_GPG   0
#define TYPE_SSL   1
#define TYPE_GROUP 2

/******
 * #define PEERS_DEBUG 1
 *****/


/** Constructor */
FriendsDialog::FriendsDialog(QWidget *parent)
            : RsAutoUpdatePage(1500,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    last_status_send_time = 0 ;
    groupsHasChanged = false;
    correctColumnStatusSize = false;

    m_compareRole = new RSTreeWidgetItemCompareRole;
    m_compareRole->setRole(COLUMN_NAME, ROLE_SORT);
    m_compareRole->setRole(COLUMN_STATE, ROLE_SORT);

    connect( ui.peertreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( peertreeWidgetCostumPopupMenu( QPoint ) ) );
    connect( ui.peertreeWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int)), this, SLOT(chatfriend(QTreeWidgetItem *)));
    connect( ui.peertreeWidget->header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(peerSortIndicatorChanged(int,Qt::SortOrder)));

	connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(QString,int)), this, SLOT(updateOwnStatus(QString,int)));

    connect( ui.avatartoolButton, SIGNAL(clicked()), SLOT(getAvatar()));
    connect( ui.mypersonalstatuslabel, SIGNAL(clicked()), SLOT(statusmessage()));
    connect( ui.actionSet_your_Avatar, SIGNAL(triggered()), this, SLOT(getAvatar()));
    connect( ui.actionSet_your_Personal_Message, SIGNAL(triggered()), this, SLOT(statusmessage()));
    connect( ui.addfileButton, SIGNAL(clicked() ), this , SLOT(addExtraFile()));

    connect(ui.actionAdd_Friend, SIGNAL(triggered()), this, SLOT(addFriend()));
    connect(ui.action_Hide_Offline_Friends, SIGNAL(triggered()), this, SLOT(insertPeers()));
    connect(ui.action_Hide_Status_Column, SIGNAL(triggered()), this, SLOT(setStateColumn()));
    connect(ui.action_Sort_by_State, SIGNAL(triggered()), this, SLOT(setStateColumn()));
    connect(ui.actionSort_Peers_Ascending_Order, SIGNAL(triggered()), this, SLOT(sortPeersAscendingOrder()));
    connect(ui.actionSort_Peers_Descending_Order, SIGNAL(triggered()), this, SLOT(sortPeersDescendingOrder()));

    ui.peertabWidget->setTabPosition(QTabWidget::North);
    ui.peertabWidget->addTab(new ProfileWidget(), tr("Profile"));
    NewsFeed *newsFeed = new NewsFeed();
    newsFeedTabIndex = ui.peertabWidget->addTab(newsFeed, tr("News Feed"));
    ui.peertabWidget->tabBar()->setIconSize(QSize(10, 10));

    /* get the current text and text color of the tab bar */
    newsFeedTabColor = ui.peertabWidget->tabBar()->tabTextColor(newsFeedTabIndex);
    newsFeedText = ui.peertabWidget->tabBar()->tabText(newsFeedTabIndex);

    connect(newsFeed, SIGNAL(newsFeedChanged(int)), this, SLOT(newsFeedChanged(int)));

    ui.peertreeWidget->setColumnCount(COLUMN_COUNT);
    ui.peertreeWidget->sortItems(COLUMN_NAME, Qt::AscendingOrder);

    // set header text aligment
    QTreeWidgetItem * headerItem = ui.peertreeWidget->headerItem();
    headerItem->setTextAlignment(COLUMN_NAME, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(COLUMN_STATE, Qt::AlignLeft | Qt::AlignVCenter);

    wasStatusColumnHidden = ui.peertreeWidget->isColumnHidden(COLUMN_STATE);

    connect(ui.Sendbtn, SIGNAL(clicked()), this, SLOT(sendMsg()));
    connect(ui.emoticonBtn, SIGNAL(clicked()), this, SLOT(smileyWidgetgroupchat()));

    connect(ui.lineEdit,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));

    connect(ui.textboldChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(ui.textunderlineChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(ui.textitalicChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(ui.fontsButton, SIGNAL(clicked()), this, SLOT(getFont()));
    connect(ui.colorChatButton, SIGNAL(clicked()), this, SLOT(setColor()));
    connect(ui.actionSave_History, SIGNAL(triggered()), this, SLOT(fileSaveAs()));

    ui.fontsButton->setIcon(QIcon(QString(":/images/fonts.png")));

    mCurrentColor = Qt::black;
    mCurrentFont.fromString(Settings->getChatScreenFont());

    colorChanged(mCurrentColor);
    fontChanged(mCurrentFont);

    style.setStyleFromSettings(ChatStyle::TYPE_PUBLIC);

    setChatInfo(tr("Welcome to RetroShare's group chat."), QString::fromUtf8("blue"));

    if (Settings->valueFromGroup("Chat", QString::fromUtf8("GroupChat_History"), true).toBool()) {
        historyKeeper.init(QString::fromStdString(RsInit::RsProfileConfigDirectory()) + "/chatPublic.xml");

        int messageCount = Settings->getPublicChatHistoryCount();
        if (messageCount > 0) {
            QList<IMHistoryItem> historyItems;
            historyKeeper.getMessages(historyItems, messageCount);
            foreach(IMHistoryItem item, historyItems) {
                addChatMsg(item.incoming, true, item.name, item.recvTime, item.messageText);
            }
        }
    }

    QMenu * grpchatmenu = new QMenu();
    grpchatmenu->addAction(ui.actionClear_Chat_History);
    grpchatmenu->addAction(ui.actionDelete_Chat_History);
    grpchatmenu->addAction(ui.actionSave_History);
    grpchatmenu->addAction(ui.actionMessageHistory);
    ui.menuButton->setMenu(grpchatmenu);

    QMenu *menu = new QMenu();
    menu->addAction(ui.actionAdd_Friend);
    menu->addAction(ui.actionAdd_Group);

    menu->addSeparator();
    menu->addAction(ui.actionCreate_New_Forum);
#ifndef RS_RELEASE_VERSION
    menu->addAction(ui.actionCreate_New_Channel);
#endif
    menu->addAction(ui.actionSet_your_Avatar);
    menu->addAction(ui.actionSet_your_Personal_Message);

    ui.menupushButton->setMenu(menu);

    setAcceptDrops(true);
    ui.lineEdit->setAcceptDrops(false);

    updateAvatar();
    loadmypersonalstatus();
    displayMenu();

    // load settings
    processSettings(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    QHeaderView *header = ui.peertreeWidget->header();
    header->setStretchLastSection(true);
    ui.peertreeWidget->setColumnWidth(COLUMN_STATE, 20); // small enough
    header->setResizeMode(COLUMN_STATE, QHeaderView::Stretch);

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.peertreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removefriend ()));

    ui.lineEdit->installEventFilter(this);

    // add self nick and Avatar to Friends.
    RsPeerDetails pd ;
    if (rsPeers->getPeerDetails(rsPeers->getOwnId(),pd)) {
        QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                         "color:#32cd32;\">%1</span>");
        ui.nicklabel->setText(titleStr.arg(QString::fromStdString(pd.name) + " (" + tr("me") + ") " + QString::fromUtf8(pd.location.c_str())));
    }

    /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

FriendsDialog::~FriendsDialog ()
{
    // save settings
    processSettings(false);

    delete(m_compareRole);
}

void FriendsDialog::processSettings(bool bLoad)
{
    QHeaderView *header = ui.peertreeWidget->header ();

    Settings->beginGroup(QString("FriendsDialog"));

    if (bLoad) {
        // load settings

        // state of peer tree
        header->restoreState(Settings->value("PeerTree").toByteArray());
        wasStatusColumnHidden = ui.peertreeWidget->isColumnHidden(COLUMN_STATE);

        // state of hideUnconnected
        ui.action_Hide_Offline_Friends->setChecked(Settings->value("hideUnconnected", false).toBool());
        
        // state of the status
        ui.action_Hide_State->setChecked(Settings->value("hideState", false).toBool());

        // state of hideStatusColumn
        ui.action_Hide_Status_Column->setChecked(Settings->value("hideStatusColumn", false).toBool());

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
        ui.splitter_2->restoreState(Settings->value("GroupChatSplitter").toByteArray());

        // open groups
        openGroups.clear();
        int arrayIndex = Settings->beginReadArray("Groups");
        for (int index = 0; index < arrayIndex; index++) {
            Settings->setArrayIndex(index);
            openGroups.push_back(Settings->value("open").toString().toStdString());
        }
        Settings->endArray();

        if (ui.peertreeWidget->isColumnHidden(COLUMN_STATE) && header->sectionSize(COLUMN_STATE) < 100) {
            // Workaround
            correctColumnStatusSize = true;
        }

        setStateColumn();
    } else {
        // save settings

        // state of peer tree
        Settings->setValue("PeerTree", header->saveState());

        // state of hideUnconnected
        Settings->setValue("hideUnconnected", ui.action_Hide_Offline_Friends->isChecked());
        
        // state of the status
        Settings->setValue("hideState", ui.action_Hide_State->isChecked());

        // state of hideStatusColumn
        Settings->setValue("hideStatusColumn", ui.action_Hide_Status_Column->isChecked());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
        Settings->setValue("GroupChatSplitter", ui.splitter_2->saveState());

        // open groups
        Settings->beginWriteArray("Groups");
        int arrayIndex = 0;
        int itemCount = ui.peertreeWidget->topLevelItemCount();
        for (int index = 0; index < itemCount; index++) {
            QTreeWidgetItem *item = ui.peertreeWidget->topLevelItem(index);
            if (item->type() == TYPE_GROUP && item->isExpanded()) {
                Settings->setArrayIndex(arrayIndex++);
                Settings->setValue("open", item->data(COLUMN_DATA, ROLE_ID).toString());
            }
        }
        Settings->endArray();
    }

    Settings->endGroup();
}

void FriendsDialog::showEvent(QShowEvent *event)
{
    static bool first = true;
    if (first) {
        // Workaround: now the scroll position is correct calculated
        first = false;
        QScrollBar *scrollbar = ui.msgText->verticalScrollBar();
        scrollbar->setValue(scrollbar->maximum());
    }

    RsAutoUpdatePage::showEvent(event);
}

void FriendsDialog::pasteLink()
{
    ui.lineEdit->insertHtml(RSLinkClipboard::toHtml()) ;
}

void FriendsDialog::contextMenu(QPoint point)
{
    QMenu *contextMnu = ui.lineEdit->createStandardContextMenu();

    contextMnu->addSeparator();
    QAction *action = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
    action->setDisabled(RSLinkClipboard::empty());

    contextMnu->exec(QCursor::pos());
    delete(contextMnu);
}

void FriendsDialog::peertreeWidgetCostumPopupMenu( QPoint point )
{
    QTreeWidgetItem *c = getCurrentPeer();

    QMenu contextMnu( this );
    QAction *action;

    QWidget *widget = new QWidget(&contextMnu);
    widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

    // create menu header
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(6);

    iconLabel = new QLabel( this );
    iconLabel->setPixmap(QPixmap(":/images/user/friends24.png"));
    iconLabel->setMaximumSize( iconLabel->frameSize().height() + 24, 24 );
    hbox->addWidget(iconLabel);

    textLabel = new QLabel("<strong>RetroShare</strong>", widget );

    hbox->addWidget(textLabel);

    spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(spacerItem);

    widget->setLayout( hbox );

    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(widget);

    contextMnu.addAction( widgetAction);

    // create menu entries
    if (c) {
        int type = c->type();

        // define header
        switch (type) {
        case TYPE_GROUP:
                //this is a GPG key
                textLabel->setText("<strong>" + tr("Group") + "</strong>");
                break;
        case TYPE_GPG:
                //this is a GPG key
                textLabel->setText("<strong>" + tr("Friend") + "</strong>");
                break;
        case TYPE_SSL:
                //this is a SSL key
                textLabel->setText("<strong>" + tr("Location") + "</strong>");
                break;
        }

        switch (type) {
        case TYPE_GROUP:
            {
                bool standard = c->data(COLUMN_DATA, ROLE_STANDARD).toBool();

                contextMnu.addAction(QIcon(IMAGE_MSG), tr("Message Group"), this, SLOT(msgfriend()));
                contextMnu.addAction(QIcon(IMAGE_ADDFRIEND), tr("Add Friend"), this, SLOT(addFriend()));

                contextMnu.addSeparator();

                action = contextMnu.addAction(QIcon(IMAGE_EDIT), tr("Edit Group"), this, SLOT(editGroup()));
                action->setDisabled(standard);

                action = contextMnu.addAction(QIcon(IMAGE_REMOVE), tr("Remove Group"), this, SLOT(removeGroup()));
                action->setDisabled(standard);
            }
            break;
        case TYPE_GPG:
        case TYPE_SSL:
            {
                contextMnu.addAction(QIcon(IMAGE_CHAT), tr("Chat"), this, SLOT(chatfriendproxy()));
                contextMnu.addAction(QIcon(IMAGE_MSG), tr("Message Friend"), this, SLOT(msgfriend()));
                
                contextMnu.addSeparator();

                contextMnu.addAction(QIcon(IMAGE_FRIENDINFO), tr("Friend Details"), this, SLOT(configurefriend()));
//                contextMnu.addAction(QIcon(IMAGE_PEERINFO), tr("Profile View"), this, SLOT(viewprofile()));
//                action = contextMnu.addAction(QIcon(IMAGE_EXPORTFRIEND), tr("Export Friend"), this, SLOT(exportfriend()));

                if (type == TYPE_GPG) {
                    contextMnu.addAction(QIcon(IMAGE_EXPORTFRIEND), tr("Recommend this Friend to..."), this, SLOT(recommendfriend()));
                }

                contextMnu.addAction(QIcon(IMAGE_CONNECT), tr("Connect To Friend"), this, SLOT(connectfriend()));

                if (type == TYPE_GPG) {
                    contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyLink()));
                }

                action = contextMnu.addAction(QIcon(IMAGE_PASTELINK), tr("Paste Friend Link"), this, SLOT(pastePerson()));
                if (RSLinkClipboard::empty(RetroShareLink::TYPE_PERSON)) {
                    action->setDisabled(true);
                }

                if (type == TYPE_GPG) {
                    contextMnu.addAction(QIcon(IMAGE_DENYFRIEND), tr("Deny Friend"), this, SLOT(removefriend()));
                } else {
                    //this is a SSL key
                    contextMnu.addAction(QIcon(IMAGE_REMOVEFRIEND), tr("Remove Friend Location"), this, SLOT(removefriend()));
                }

                if (type == TYPE_GPG) {
                    QMenu* groupsMenu = NULL;
                    QMenu* addToGroupMenu = NULL;
                    QMenu* moveToGroupMenu = NULL;

                    std::list<RsGroupInfo> groupInfoList;
                    rsPeers->getGroupInfoList(groupInfoList);

                    GroupDefs::sortByName(groupInfoList);

                    std::string gpgId = c->data(COLUMN_DATA, ROLE_ID).toString().toStdString();

                    QTreeWidgetItem *parent = c->parent();

                    bool foundGroup = false;
                    // add action for all groups, except the own group
                    for (std::list<RsGroupInfo>::iterator groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
                        if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpgId) == groupIt->peerIds.end()) {
                            if (parent) {
                                if (addToGroupMenu == NULL) {
                                    addToGroupMenu = new QMenu(tr("Add to group"), &contextMnu);
                                }
                                QAction* addToGroupAction = new QAction(GroupDefs::name(*groupIt), addToGroupMenu);
                                addToGroupAction->setData(QString::fromStdString(groupIt->id));
                                connect(addToGroupAction, SIGNAL(triggered()), this, SLOT(addToGroup()));
                                addToGroupMenu->addAction(addToGroupAction);
                            }

                            if (moveToGroupMenu == NULL) {
                                moveToGroupMenu = new QMenu(tr("Move to group"), &contextMnu);
                            }
                            QAction* moveToGroupAction = new QAction(GroupDefs::name(*groupIt), moveToGroupMenu);
                            moveToGroupAction->setData(QString::fromStdString(groupIt->id));
                            connect(moveToGroupAction, SIGNAL(triggered()), this, SLOT(moveToGroup()));
                            moveToGroupMenu->addAction(moveToGroupAction);
                        } else {
                            foundGroup = true;
                        }
                    }

                    if (addToGroupMenu || moveToGroupMenu || foundGroup) {
                        groupsMenu = contextMnu.addMenu(QIcon(IMAGE_GROUP16), tr("Groups"));

                        if (addToGroupMenu) {
                            groupsMenu->addMenu(addToGroupMenu);
                        }

                        if (moveToGroupMenu) {
                            groupsMenu->addMenu(moveToGroupMenu);
                        }

                        if (foundGroup) {
                            // add remove from group
                            if (parent && parent->type() == TYPE_GROUP) {
                                QAction *removeFromGroup = groupsMenu->addAction(tr("Remove from group"));
                                removeFromGroup->setData(parent->data(COLUMN_DATA, ROLE_ID));
                                connect(removeFromGroup, SIGNAL(triggered()), this, SLOT(removeFromGroup()));
                            }

                            QAction *removeFromAllGroups = groupsMenu->addAction(tr("Remove from all groups"));
                            removeFromAllGroups->setData("");
                            connect(removeFromAllGroups, SIGNAL(triggered()), this, SLOT(removeFromGroup()));
                        }
                    }
                }
            }
        }
    } else {
        action = contextMnu.addAction(QIcon(IMAGE_PASTELINK), tr("Paste Friend Link"), this, SLOT(pastePerson()));
        if (RSLinkClipboard::empty(RetroShareLink::TYPE_PERSON)) {
            action->setDisabled(true);
        }
    }

    contextMnu.addSeparator();

    contextMnu.addAction(QIcon(IMAGE_EXPAND), tr("Expand all"), ui.peertreeWidget, SLOT(expandAll()));
    contextMnu.addAction(QIcon(IMAGE_COLLAPSE), tr("Collapse all"), ui.peertreeWidget, SLOT(collapseAll()));

    contextMnu.exec(QCursor::pos());
}

// replaced by shortcut
//void FriendsDialog::keyPressEvent(QKeyEvent *e)
//{
//	if(e->key() == Qt::Key_Delete)
//	{
//		removefriend() ;
//		e->accept() ;
//	}
//	else
//		MainPage::keyPressEvent(e) ;
//}

void FriendsDialog::updateDisplay()
{
    insertPeers() ;
}

/* get the list of peers from the RsIface.  */
void  FriendsDialog::insertPeers()
{
    #ifdef PEERS_DEBUG
    std::cerr << "FriendsDialog::insertPeers() called." << std::endl;
    #endif

    std::list<std::string> gpgFriends;
    std::list<std::string>::iterator gpgIt;

    std::list<StatusInfo> statusInfo;
    rsStatus->getStatusList(statusInfo);

    if (!rsPeers) {
        /* not ready yet! */
        std::cerr << "FriendsDialog::insertPeers() not ready yet : rsPeers unintialized."  << std::endl;
        return;
    }

    bool hideUnconnected = ui.action_Hide_Offline_Friends->isChecked();
    bool useStatusColumn = !ui.action_Hide_Status_Column->isChecked();
    bool showState = !ui.action_Hide_State->isChecked();

    // get ids of existing private chat messages
    std::list<std::string> privateChatIds;
    rsMsgs->getPrivateChatQueueIds(true, privateChatIds);

    // get existing groups
    std::list<RsGroupInfo> groupInfoList;
    std::list<RsGroupInfo>::iterator groupIt;
    rsPeers->getGroupInfoList(groupInfoList);

    rsPeers->getGPGAcceptedList(gpgFriends);

    //add own gpg id, if we have more than on location (ssl client)
    std::list<std::string> ownSslContacts;
    std::string ownId = rsPeers->getGPGOwnId();
    rsPeers->getSSLChildListOfGPGId(ownId, ownSslContacts);
    if (ownSslContacts.size() > 0) {
        gpgFriends.push_back(ownId);
    }

    /* get a link to the table */
    QTreeWidget *peertreeWidget = ui.peertreeWidget;

    // remove items don't exist anymore
    QTreeWidgetItemIterator itemIterator(peertreeWidget);
    QTreeWidgetItem *item;
    while ((item = *itemIterator) != NULL) {
        itemIterator++;
        switch (item->type()) {
        case TYPE_GPG:
            {
                QTreeWidgetItem *parent = item->parent();
                std::string gpg_widget_id = item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();

                // remove items that are not friends anymore
                if (std::find(gpgFriends.begin(), gpgFriends.end(), gpg_widget_id) == gpgFriends.end()) {
                    if (parent) {
                        delete(parent->takeChild(parent->indexOfChild(item)));
                    } else {
                        delete(peertreeWidget->takeTopLevelItem(peertreeWidget->indexOfTopLevelItem(item)));
                    }
                    break;
                }

                if (groupsHasChanged) {
                    if (parent) {
                        if (parent->type() == TYPE_GROUP) {
                            std::string groupId = parent->data(COLUMN_DATA, ROLE_ID).toString().toStdString();

                            // the parent is a group, check if the gpg id is assigned to the group
                            for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
                                if (groupIt->id == groupId) {
                                    if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpg_widget_id) == groupIt->peerIds.end()) {
                                        delete(parent->takeChild(parent->indexOfChild(item)));
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
                        // gpg item without group, check if the gpg id is assigned to the group
                        for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
                            if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpg_widget_id) != groupIt->peerIds.end()) {
                                delete(peertreeWidget->takeTopLevelItem(peertreeWidget->indexOfTopLevelItem(item)));
                                break;
                            }
                        }
                    }
                }
            }
            break;
        case TYPE_GROUP:
            {
                if (groupsHasChanged) {
                    // remove deleted groups
                    std::string groupId = item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
                    for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
                        if (groupIt->id == groupId) {
                            break;
                        }
                    }
                    if (groupIt == groupInfoList.end() || groupIt->peerIds.size() == 0) {
                        if (item->parent()) {
                            delete(item->parent()->takeChild(item->parent()->indexOfChild(item)));
                        } else {
                            delete(peertreeWidget->takeTopLevelItem(peertreeWidget->indexOfTopLevelItem(item)));
                        }
                    }
                }
            }
            break;
        }
    }

    std::list<std::string> fillGpgIds;

    // start with groups
    groupIt = groupInfoList.begin();
    while (true) {
        QTreeWidgetItem *groupItem = NULL;
        RsGroupInfo *groupInfo = NULL;
        int onlineCount = 0;
        int availableCount = 0;
        if (groupIt != groupInfoList.end()) {
            groupInfo = &(*groupIt);

            if ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) && groupInfo->peerIds.size() == 0) {
                // don't show empty standard groups
                groupIt++;
                continue;
            }

            // search existing group item
            int itemCount = peertreeWidget->topLevelItemCount();
            for (int index = 0; index < itemCount; index++) {
                QTreeWidgetItem *groupItemLoop = peertreeWidget->topLevelItem(index);
                if (groupItemLoop->type() == TYPE_GROUP && groupItemLoop->data(COLUMN_DATA, ROLE_ID).toString().toStdString() == groupInfo->id) {
                    groupItem = groupItemLoop;
                    break;
                }
            }

            if (groupItem == NULL) {
                // add group item
                groupItem = new RSTreeWidgetItem(m_compareRole, TYPE_GROUP);

                /* Add item to the list. Add here, because for setHidden the item must be added */
                peertreeWidget->addTopLevelItem(groupItem);

                groupItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
                groupItem->setSizeHint(COLUMN_NAME, QSize(26, 26));
                groupItem->setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter);
                groupItem->setIcon(COLUMN_NAME, QIcon(IMAGE_GROUP24));
                groupItem->setForeground(COLUMN_NAME, QBrush(QColor(123, 123, 123)));

                /* used to find back the item */
                groupItem->setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(groupInfo->id));
                groupItem->setData(COLUMN_DATA, ROLE_STANDARD, (groupInfo->flag & RS_GROUP_FLAG_STANDARD) ? true : false);

                if (openGroups.size()) {
                    if (std::find(openGroups.begin(), openGroups.end(), groupInfo->id) != openGroups.end()) {
                        groupItem->setExpanded(true);
                    }
                }
            } else {
                // remove all gpg items that are not more assigned
                int childCount = groupItem->childCount();
                int childIndex = 0;
                while (childIndex < childCount) {
                    QTreeWidgetItem *gpgItemLoop = groupItem->child(childIndex);
                    if (gpgItemLoop->type() == TYPE_GPG) {
                        if (std::find(groupInfo->peerIds.begin(), groupInfo->peerIds.end(), gpgItemLoop->data(COLUMN_DATA, ROLE_ID).toString().toStdString()) == groupInfo->peerIds.end()) {
                            delete(groupItem->takeChild(groupItem->indexOfChild(gpgItemLoop)));
                            childCount = groupItem->childCount();
                            continue;
                        }
                    }
                    childIndex++;
                }
            }

            // name is set after calculation of online/offline items
        }

        // iterate through gpg friends
        for (gpgIt = gpgFriends.begin(); gpgIt != gpgFriends.end(); gpgIt++) {
            std::string gpgId = *gpgIt;

            if (groupInfo) {
                // we fill a group, check if gpg id is assigned
                if (std::find(groupInfo->peerIds.begin(), groupInfo->peerIds.end(), gpgId) == groupInfo->peerIds.end()) {
                    continue;
                }
            } else {
                // we fill the not assigned gpg ids
                if (std::find(fillGpgIds.begin(), fillGpgIds.end(), gpgId) != fillGpgIds.end()) {
                    continue;
                }
            }

            // add equal too, its no problem
            fillGpgIds.push_back(gpgId);

            //add the gpg friends
#ifdef PEERS_DEBUG
            std::cerr << "FriendsDialog::insertPeers() inserting gpg_id : " << *it << std::endl;
#endif

            /* make a widget per friend */
            QTreeWidgetItem *gpgItem = NULL;
            QTreeWidgetItem *gpgItemLoop = NULL;

            // search existing gpg item
            int itemCount = groupItem ? groupItem->childCount() : peertreeWidget->topLevelItemCount();
            for (int index = 0; index < itemCount; index++) {
                gpgItemLoop = groupItem ? groupItem->child(index) : peertreeWidget->topLevelItem(index);
                if (gpgItemLoop->type() == TYPE_GPG && gpgItemLoop->data(COLUMN_DATA, ROLE_ID).toString().toStdString() == gpgId) {
                    gpgItem = gpgItemLoop;
                    break;
                }
            }

            RsPeerDetails detail;
            if ((!rsPeers->getPeerDetails(gpgId, detail) || !detail.accept_connection) && detail.gpg_id != ownId) {
                // don't accept anymore connection, remove from the view
                if (gpgItem) {
                    if (groupItem) {
                        delete(groupItem->takeChild(groupItem->indexOfChild(gpgItem)));
                    } else {
                        delete (peertreeWidget->takeTopLevelItem(peertreeWidget->indexOfTopLevelItem(gpgItem)));
                    }
                }
                continue;
            }

            if (gpgItem == NULL) {
                // create gpg item and add it to tree
                gpgItem = new RSTreeWidgetItem(m_compareRole, TYPE_GPG); //set type to 0 for custom popup menu

                /* Add gpg item to the list. Add here, because for setHidden the item must be added */
                if (groupItem) {
                    groupItem->addChild(gpgItem);
                } else {
                    peertreeWidget->addTopLevelItem(gpgItem);
                }

                gpgItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
                gpgItem->setSizeHint(COLUMN_NAME, QSize(26, 26));
                gpgItem->setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter);

                /* not displayed, used to find back the item */
                gpgItem->setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(detail.id));
            }

            availableCount++;

            QString gpgItemText = QString::fromStdString(detail.name);

            // remove items that are not friends anymore
            int childCount = gpgItem->childCount();
            int childIndex = 0;
            while (childIndex < childCount) {
                std::string ssl_id = gpgItem->child(childIndex)->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
                if (!rsPeers->isFriend(ssl_id)) {
                    delete (gpgItem->takeChild(childIndex));
                    // count again
                    childCount = gpgItem->childCount();
                } else {
                    childIndex++;
                }
            }

            // update the childs (ssl certs)
            bool gpg_connected = false;
            bool gpg_online = false;
            bool gpg_hasPrivateChat = false;
            std::list<std::string> sslContacts;

            rsPeers->getSSLChildListOfGPGId(detail.gpg_id, sslContacts);
            for (std::list<std::string>::iterator sslIt = sslContacts.begin(); sslIt != sslContacts.end(); sslIt++) {
                QTreeWidgetItem *sslItem = NULL;
                std::string sslId = *sslIt;

                //find the corresponding sslItem child item of the gpg item
                bool newChild = true;
                childCount = gpgItem->childCount();
                for (int childIndex = 0; childIndex < childCount; childIndex++) {
                    // we assume, that only ssl items are child of the gpg item, so we don't need to test the type
                    if (gpgItem->child(childIndex)->data(COLUMN_DATA, ROLE_ID).toString().toStdString() == sslId) {
                        sslItem = gpgItem->child(childIndex);
                        newChild = false;
                        break;
                    }
                }

                RsPeerDetails sslDetail;
                if (!rsPeers->getPeerDetails(sslId, sslDetail) || !rsPeers->isFriend(sslId)) {
#ifdef PEERS_DEBUG
                    std::cerr << "Removing widget from the view : id : " << sslId << std::endl;
#endif
                    //child has disappeared, remove it from the gpg_item
                    if (sslItem) {
                        gpgItem->removeChild(sslItem);
                    }
                    continue;
                }

                if (newChild) {
                    sslItem = new RSTreeWidgetItem(m_compareRole, TYPE_SSL); //set type to 1 for custom popup menu

#ifdef PEERS_DEBUG
                    std::cerr << "FriendsDialog::insertPeers() inserting sslItem." << std::endl;
#endif

                    /* Add ssl child to the list. Add here, because for setHidden the item must be added */
                    gpgItem->addChild(sslItem);
                }

                /* not displayed, used to find back the item */
                sslItem->setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(sslDetail.id));

                QString sText;
                QString customStateString;
                if (sslDetail.state & RS_PEER_STATE_CONNECTED) {
                    customStateString = QString::fromUtf8(rsMsgs->getCustomStateString(sslDetail.id).c_str());
                }
                sText = tr("location") + " : " + QString::fromUtf8(sslDetail.location.c_str());
                if (customStateString.isEmpty() == false) {
                    sText += " - " + customStateString;
                }

                QString connectStateString = StatusDefs::connectStateString(sslDetail);
                if (useStatusColumn) {
                    sslItem->setText(COLUMN_STATE, connectStateString);
                } else if (showState && connectStateString.isEmpty() == false) {
                    sText += " [" + StatusDefs::connectStateString(sslDetail) + "]";
                }
                sslItem->setText( COLUMN_NAME, sText);

                if (useStatusColumn == false && showState == false) {
                    /* Show the state as tooltip */
                    sslItem->setToolTip(COLUMN_NAME, connectStateString);
                } else {
                    sslItem->setToolTip(COLUMN_NAME, "");
                }

                // sort location
                sslItem->setData(COLUMN_STATE, ROLE_SORT, sText);

                /* change color and icon */
                QIcon sslIcon;
                QFont sslFont;
                QColor sslColor;
                if (sslDetail.state & RS_PEER_STATE_CONNECTED) {
                    sslItem->setHidden(false);
                    gpg_connected = true;

                    sslIcon = QIcon(":/images/connect_established.png");

                    sslFont.setBold(true);
                    sslColor = Qt::darkBlue;
                } else if (sslDetail.state & RS_PEER_STATE_ONLINE) {
                    sslItem->setHidden(hideUnconnected);
                    gpg_online = true;

                    if (sslDetail.connectState) {
                        sslIcon = QIcon(":/images/connect_creating.png");
                    } else {
                        sslIcon = QIcon(":/images/connect_no.png");
                    }

                    sslFont.setBold(true);
                    sslColor = Qt::black;
                } else {
                    sslItem->setHidden(hideUnconnected);
                    if (sslDetail.connectState) {
                        sslIcon = QIcon(":/images/connect_creating.png");
                    } else {
                        sslIcon = QIcon(":/images/connect_no.png");
                    }

                    sslFont.setBold(false);
                    sslColor = Qt::black;
                }

                if (std::find(privateChatIds.begin(), privateChatIds.end(), sslDetail.id) != privateChatIds.end()) {
                    // private chat is available
                    sslIcon = QIcon(":/images/chat.png");
                    gpg_hasPrivateChat = true;
                }
                sslItem -> setIcon(COLUMN_NAME, sslIcon);

                for (int i = 0; i < COLUMN_COUNT; i++) {
                    sslItem -> setTextColor(i, sslColor);
                    sslItem -> setFont(i, sslFont);
                }
            }

            int bestPeerState = 0;        // for gpg item
            int i = 0;
            QIcon gpgIcon;
            if (gpg_connected) {
                gpgItem->setHidden(false);

                onlineCount++;

                std::string bestSslId;        // for gpg item
                unsigned int bestRSState = 0; // for gpg item

                std::list<StatusInfo>::iterator it;
                for(it = statusInfo.begin(); it != statusInfo.end() ; it++) {

                    // don't forget the kids
                    std::list<std::string>::iterator cont_it;
                    for (cont_it = sslContacts.begin(); cont_it != sslContacts.end(); cont_it++) {

                        if((it->id == *cont_it) && (rsPeers->isOnline(*cont_it))){

                            int peerState = 0;

                            switch (it->status) {
                            case RS_STATUS_INACTIVE:
                                peerState = PEER_STATE_INACTIVE;
                                break;

                            case RS_STATUS_ONLINE:
                                peerState = PEER_STATE_ONLINE;
                                break;

                            case RS_STATUS_AWAY:
                                peerState = PEER_STATE_AWAY;
                                break;

                            case RS_STATUS_BUSY:
                                peerState = PEER_STATE_BUSY;
                                break;
                            }

                            /* find the best ssl contact for the gpg item */
                            if (bestPeerState == 0) {
                                /* first ssl contact */
                                bestPeerState = peerState;
                                bestSslId = *cont_it;
                                bestRSState = it->status;
                            } else if (peerState < bestPeerState) {
                                /* higher state */
                                bestPeerState = peerState;
                                bestSslId = *cont_it;
                                bestRSState = it->status;
                            } else if (peerState == bestPeerState) {
                                /* equal state ... use first */
                            }
                        }
                    }
                }

                if (bestPeerState == 0) {
                    // show as online
                    bestPeerState = PEER_STATE_ONLINE;
                    bestRSState = RS_STATUS_ONLINE;
                }

                QColor textColor = StatusDefs::textColor(bestRSState);
                QFont font = StatusDefs::font(bestRSState);
                for(i = 0; i < COLUMN_COUNT; i++) {
                    gpgItem->setTextColor(i, textColor);
                    gpgItem->setFont(i, font);
                }

                gpgIcon = QIcon(StatusDefs::imageUser(bestRSState));

                if (useStatusColumn) {
                    gpgItem->setText(COLUMN_STATE, StatusDefs::name(bestRSState));
                } else if (showState) {
                    gpgItemText += " [" + StatusDefs::name(bestRSState) + "]";
                }

//                gpgItem->setToolTip(COLUMN_NAME, StatusDefs::tooltip(bestRSState));
            } else if (gpg_online) {
                if (useStatusColumn) {
                    gpgItem->setText(COLUMN_STATE, tr("Available"));
                } else if (showState) {
                    gpgItemText += " [" + tr("Available") + "]";
                }

                bestPeerState = PEER_STATE_AVAILABLE;
                onlineCount++;
                gpgItem->setHidden(hideUnconnected);
                gpgIcon = QIcon(IMAGE_AVAILABLE);

                QFont font;
                font.setBold(true);
                for(i = 0; i < COLUMN_COUNT; i++) {
                    gpgItem->setTextColor(i,(Qt::black));
                    gpgItem->setFont(i,font);
                }
            } else {
                if (useStatusColumn) {
                    gpgItem->setText(COLUMN_STATE, StatusDefs::name(RS_STATUS_OFFLINE));
                } else if (showState) {
                    gpgItemText += " [" + StatusDefs::name(RS_STATUS_OFFLINE) + "]";
                }

                bestPeerState = PEER_STATE_OFFLINE;
                gpgItem->setHidden(hideUnconnected);
                gpgIcon = QIcon(StatusDefs::imageUser(RS_STATUS_OFFLINE));

                QColor textColor = StatusDefs::textColor(RS_STATUS_OFFLINE);
                QFont font = StatusDefs::font(RS_STATUS_OFFLINE);
                for(i = 0; i < COLUMN_COUNT; i++) {
                    gpgItem->setTextColor(i, textColor);
                    gpgItem->setFont(i, font);
                }
            }

            if (gpg_hasPrivateChat) {
                gpgIcon = QIcon(":/images/chat.png");
            }

            gpgItem->setText(COLUMN_NAME, gpgItemText);
            gpgItem->setData(COLUMN_NAME, ROLE_SORT, "2 " + gpgItemText);
            gpgItem->setData(COLUMN_STATE, ROLE_SORT, "2 " + BuildStateSortString(true, gpgItemText, bestPeerState));
            gpgItem->setIcon(COLUMN_NAME, gpgIcon);
        }

        if (groupInfo && groupItem) {
            if ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) && groupItem->childCount() == 0) {
                // there are some dead id's assigned
                groupItem->setHidden(true);
            } else {
                QString groupName = GroupDefs::name(*groupInfo);
                groupItem->setText(COLUMN_NAME, QString("%1 (%2/%3)").arg(groupName).arg(onlineCount).arg(availableCount));
                // show first the standard groups, than the user groups
                groupItem->setData(COLUMN_NAME, ROLE_SORT, ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) ? "0 " : "1 ") + groupName);
            }
        }

        if (groupIt != groupInfoList.end()) {
            groupIt++;
        } else {
            // all done
            break;
        }
    }

    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->isHidden()) {
        // active item is hidden, deselect it
        ui.peertreeWidget->setCurrentItem(NULL);
    }

    groupsHasChanged = false;
    openGroups.clear();
}

/* Utility Fns */
std::string getPeerRsCertId(QTreeWidgetItem *i)
{
    std::string id = i -> data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    return id;
}

/** Open a QFileDialog to browse for export a file. */
void FriendsDialog::exportfriend()
{
        QTreeWidgetItem *c = getCurrentPeer();

#ifdef PEERS_DEBUG
        std::cerr << "FriendsDialog::exportfriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG
                std::cerr << "FriendsDialog::exportfriend() None Selected -- sorry" << std::endl;
#endif
		return;
	}

	std::string id = getPeerRsCertId(c);

	if (misc::getSaveFileName(this, RshareSettings::LASTDIR_CERT, tr("Save Certificate"), tr("Certificates (*.pqi)"), fileName))
	{
#ifdef PEERS_DEBUG
                std::cerr << "FriendsDialog::exportfriend() Saving to: " << file << std::endl;
#endif
		if (rsPeers)
		{
                        rsPeers->saveCertificateToFile(id, fileName.toUtf8().constData());
		}
	}

}

void FriendsDialog::chatfriendproxy()
{
    chatfriend(getCurrentPeer());
}

void FriendsDialog::chatfriend(QTreeWidgetItem *pPeer)
{
    if (pPeer == NULL) {
        return;
    }

    std::string id = pPeer->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    PopupChatDialog::chatFriend(id);
}

void FriendsDialog::msgfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::string id = peer->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    MessageComposer::msgFriend(id, (peer->type() == TYPE_GROUP));
}

void FriendsDialog::recommendfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::list <std::string> ids;
    ids.push_back(peer->data(COLUMN_DATA, ROLE_ID).toString().toStdString());
    MessageComposer::recommendFriend(ids);
}

void FriendsDialog::pastePerson()
{
    RSLinkClipboard::process(RetroShareLink::TYPE_PERSON, RSLINK_PROCESS_NOTIFY_ERROR);
}

void FriendsDialog::copyLink()
{
    QTreeWidgetItem *c = getCurrentPeer();

    if (c == NULL) {
        return;
    }

    std::vector<RetroShareLink> urls;
    RetroShareLink link;
    if (link.createPerson(c->data(COLUMN_DATA, ROLE_ID).toString().toStdString())) {
        urls.push_back(link);
    }

    RSLinkClipboard::copyLinks(urls);
}

QTreeWidgetItem *FriendsDialog::getCurrentPeer()
{
	/* get the current, and extract the Id */

	/* get a link to the table */
        QTreeWidget *peerWidget = ui.peertreeWidget;
        QTreeWidgetItem *item = peerWidget -> currentItem();
        if (!item)
        {
#ifdef PEERS_DEBUG
		std::cerr << "Invalid Current Item" << std::endl;
#endif
		return NULL;
	}

#ifdef PEERS_DEBUG
	/* Display the columns of this item. */
	std::ostringstream out;
        out << "CurrentPeerItem: " << std::endl;

        for(int i = 1; i < COLUMN_COUNT; i++)
	{
		QString txt = item -> text(i);
		out << "\t" << i << ":" << txt.toStdString() << std::endl;
	}
	std::cerr << out.str();
#endif
	return item;
}

/* So from the Peers Dialog we can call the following control Functions:
 * (1) Remove Current.              FriendRemove(id)
 * (2) Allow/DisAllow.              FriendStatus(id, accept)
 * (2) Connect.                     FriendConnectAttempt(id, accept)
 * (3) Set Address.                 FriendSetAddress(id, str, port)
 * (4) Set Trust.                   FriendTrustSignature(id, bool)
 * (5) Configure (GUI Only) -> 3/4
 *
 * All of these rely on the finding of the current Id.
 */


void FriendsDialog::removefriend()
{
        QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG
        std::cerr << "FriendsDialog::removefriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG
        	std::cerr << "FriendsDialog::removefriend() None Selected -- sorry" << std::endl;
#endif
		return;
	}

	if (rsPeers)
	{
        if ((QMessageBox::question(this, "RetroShare",tr("Do you want to remove this Friend?"),QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
		{
      rsPeers->removeFriend(getPeerRsCertId(c));
      emit friendsUpdated() ;
		}
		else
		return;
	}
}

void FriendsDialog::connectfriend()
{
    QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG
    std::cerr << "FriendsDialog::connectfriend()" << std::endl;
#endif
    if (!c)
    {
#ifdef PEERS_DEBUG
        std::cerr << "FriendsDialog::connectfriend() Noone Selected -- sorry" << std::endl;
#endif
        return;
    }

    if (rsPeers)
    {
        if (c->type() == TYPE_GPG) {
            int childCount = c->childCount();
            for (int childIndex = 0; childIndex < childCount; childIndex++) {
                QTreeWidgetItem *item = c->child(childIndex);
                if (item->type() == TYPE_SSL) {
                    rsPeers->connectAttempt(getPeerRsCertId(item));
                    item->setIcon(COLUMN_NAME,(QIcon(IMAGE_CONNECT2)));
                }
            }
        } else {
            //this is a SSL key
            rsPeers->connectAttempt(getPeerRsCertId(c));
            c->setIcon(COLUMN_NAME,(QIcon(IMAGE_CONNECT2)));
        }
    }
}

/* GUI stuff -> don't do anything directly with Control */
void FriendsDialog::configurefriend()
{
    ConfCertDialog::showIt(getPeerRsCertId(getCurrentPeer()), ConfCertDialog::PageDetails);
}

void FriendsDialog::addFriend()
{
    std::string groupId;

    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->type() == TYPE_GROUP) {
        groupId = c->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    }

    ConnectFriendWizard connwiz (this);

    if (groupId.empty() == false) {
        connwiz.setGroup(groupId);
    }

    connwiz.exec ();
}

void FriendsDialog::resetStatusBar() 
{
        #ifdef PEERS_DEBUG
	std::cerr << "FriendsDialog: reseting status bar." << std::endl ;
        #endif

	ui.statusStringLabel->setText(QString("")) ;
}

void FriendsDialog::updateStatusTyping()
{
    if(time(NULL) - last_status_send_time > 5)	// limit 'peer is typing' packets to at most every 10 sec
    {
#ifdef PEERS_DEBUG
        std::cerr << "FriendsDialog: sending group chat typing info." << std::endl ;
#endif

#ifdef ONLY_FOR_LINGUIST
        tr("is typing...");
#endif

        rsMsgs->sendGroupChatStatusString("is typing...");
        last_status_send_time = time(NULL) ;
    }
}

// Called by libretroshare through notifyQt to display the peer's status
//
void FriendsDialog::updateStatusString(const QString& peer_id, const QString& status_string)
{
#ifdef PEERS_DEBUG
    std::cerr << "FriendsDialog: received group chat typing info. updating gui." << std::endl ;
#endif

    QString status = QString::fromStdString(rsPeers->getPeerName(peer_id.toStdString())) + " " + tr(status_string.toAscii());
    ui.statusStringLabel->setText(status) ; // displays info for 5 secs.

    QTimer::singleShot(5000,this,SLOT(resetStatusBar())) ;
}

void FriendsDialog::updatePeersAvatar(const QString& peer_id)
{
#ifdef PEERS_DEBUG
    std::cerr << "FriendsDialog: Got notified of new avatar for peer " << peer_id.toStdString() << std::endl ;
#endif

    PopupChatDialog *pcd = PopupChatDialog::getPrivateChat(peer_id.toStdString(), 0);
    if (pcd) {
        pcd->updatePeerAvatar(peer_id.toStdString());
    }
}

void FriendsDialog::updatePeerStatusString(const QString& peer_id,const QString& status_string,bool is_private_chat)
{
    if(is_private_chat)
    {
        PopupChatDialog *pcd = PopupChatDialog::getExistingInstance(peer_id.toStdString());
        if (pcd) {
            pcd->updateStatusString(peer_id, status_string);
        }
    }
    else
    {
#ifdef PEERS_DEBUG
        std::cerr << "Updating public chat msg from peer " << rsPeers->getPeerName(peer_id.toStdString()) << ": " << status_string.toStdString() << std::endl ;
#endif

        updateStatusString(peer_id, status_string);
    }
}

void FriendsDialog::publicChatChanged(int type)
{
    if (type == NOTIFY_TYPE_ADD) {
        insertChat();
    }
}

void FriendsDialog::addChatMsg(bool incoming, bool history, QString &name, QDateTime &recvTime, QString &message)
{
    unsigned int formatFlag = CHAT_FORMATMSG_EMBED_LINKS;

    // embed smileys ?
    if (Settings->valueFromGroup(QString("Chat"), QString::fromUtf8("Emoteicons_GroupChat"), true).toBool()) {
        formatFlag |= CHAT_FORMATMSG_EMBED_SMILEYS;
    }

    ChatStyle::enumFormatMessage type;
    if (incoming) {
        if (history) {
            type = ChatStyle::FORMATMSG_HINCOMING;
        } else {
            type = ChatStyle::FORMATMSG_INCOMING;
        }
    } else {
        if (history) {
            type = ChatStyle::FORMATMSG_HOUTGOING;
        } else {
            type = ChatStyle::FORMATMSG_OUTGOING;
        }
    }
    QString formatMsg = style.formatMessage(type, name, recvTime, message, formatFlag);

    ui.msgText->append(formatMsg);
}

void FriendsDialog::insertChat()
{
    std::list<ChatInfo> newchat;
    if (!rsMsgs->getPublicChatQueue(newchat))
    {
#ifdef PEERS_DEBUG
        std::cerr << "no chat available." << std::endl ;
#endif
        return;
    }
#ifdef PEERS_DEBUG
    std::cerr << "got new chat." << std::endl;
#endif
    std::list<ChatInfo>::iterator it;

    /* add in lines at the bottom */
    for(it = newchat.begin(); it != newchat.end(); it++)
    {
        /* are they private? */
        if (it->chatflags & RS_CHAT_PRIVATE)
        {
            /* this should not happen */
            continue;
        }

        QDateTime sendTime = QDateTime::fromTime_t(it->sendTime);
        QDateTime recvTime = QDateTime::fromTime_t(it->recvTime);
        QString name = QString::fromStdString(rsPeers->getPeerName(it->rsid));
        QString msg = QString::fromStdWString(it->msg);

#ifdef PEERS_DEBUG
        std::cerr << "FriendsDialog::insertChat(): " << msg << std::endl;
#endif

        bool incoming = false;

        // notify with a systray icon msg
        if(it->rsid != rsPeers->getOwnId())
        {
            incoming = true;

            // This is a trick to translate HTML into text.
            QTextEdit editor;
            editor.setHtml(msg);
            QString notifyMsg = name + ": " + editor.toPlainText();

            if(notifyMsg.length() > 30)
                emit notifyGroupChat(tr("New group chat"), notifyMsg.left(30) + QString("..."));
            else
                emit notifyGroupChat(tr("New group chat"), notifyMsg);
        }

        historyKeeper.addMessage(incoming, it->rsid, name, sendTime, recvTime, msg);
        addChatMsg(incoming, false, name, recvTime, msg);
    }
}

bool FriendsDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.lineEdit) {
        if (event->type() == QEvent::KeyPress) {
            updateStatusTyping() ;

            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
                // Enter pressed
                if (Settings->getChatSendMessageWithCtrlReturn()) {
                    if (keyEvent->modifiers() & Qt::ControlModifier) {
                        // send message with Ctrl+Enter
                        sendMsg();
                        return true; // eat event
                    }
                } else {
                    if (keyEvent->modifiers() & Qt::ControlModifier) {
                        // insert return
                        ui.lineEdit->textCursor().insertText("\n");
                    } else {
                        // send message with Enter
                        sendMsg();
                    }
                    return true; // eat event
                }
            }
        }
    }
    // pass the event on to the parent class
    return RsAutoUpdatePage::eventFilter(obj, event);
}

void FriendsDialog::sendMsg()
{
    QTextEdit *lineWidget = ui.lineEdit;

    if (lineWidget->toPlainText().isEmpty()) {
        // nothing to send
        return;
    }

    std::wstring message = lineWidget->toHtml().toStdWString();

#ifdef PEERS_DEBUG
    std::string msg(ci.msg.begin(), ci.msg.end());
    std::cerr << "FriendsDialog::sendMsg(): " << msg << std::endl;
#endif

    rsMsgs->sendPublicChat(message);
    ui.lineEdit->clear();
    // workaround for Qt bug - http://bugreports.qt.nokia.com/browse/QTBUG-2533
    // QTextEdit::clear() does not reset the CharFormat if document contains hyperlinks that have been accessed.
    ui.lineEdit->setCurrentCharFormat(QTextCharFormat ());

    setFont();

    /* redraw send list */
    insertSendList();
}

void  FriendsDialog::insertSendList()
{
#ifdef false
	std::list<std::string> peers;
	std::list<std::string>::iterator it;

	if (!rsPeers)
	{
		/* not ready yet! */
		return;
	}

	rsPeers->getOnlineList(peers);

        /* get a link to the table */
        //QTreeWidget *sendWidget = ui.msgSendList;
	QList<QTreeWidgetItem *> items;

	for(it = peers.begin(); it != peers.end(); it++)
	{

		RsPeerDetails details;
		if (!rsPeers->getPeerDetails(*it, details))
		{
			continue; /* BAD */
		}

		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		/* (0) Person */
		item -> setText(0, QString::fromStdString(details.name));

		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		//item -> setFlags(Qt::ItemIsUserCheckable);

		item -> setCheckState(0, Qt::Checked);

		if (rsicontrol->IsInChat(*it))
		{
			item -> setCheckState(0, Qt::Checked);
		}
		else
		{
			item -> setCheckState(0, Qt::Unchecked);
		}

		/* disable for the moment */
		item -> setFlags(Qt::ItemIsUserCheckable);
		item -> setCheckState(0, Qt::Checked);

		/* add to the list */
		items.append(item);
	}

        /* remove old items */
	//sendWidget->clear();
	//sendWidget->setColumnCount(1);

	/* add the items in! */
	//sendWidget->insertTopLevelItems(0, items);

	//sendWidget->update(); /* update display */
#endif
}


/* to toggle the state */


//void FriendsDialog::toggleSendItem( QTreeWidgetItem *item, int col )
//{
//        #ifdef PEERS_DEBUG
//	std::cerr << "ToggleSendItem()" << std::endl;
//        #endif
//
//	/* extract id */
//	std::string id = (item -> text(4)).toStdString();
//
//	/* get state */
//	bool inChat = (Qt::Checked == item -> checkState(0)); /* alway column 0 */
//
//	/* call control fns */
//
//	rsicontrol -> SetInChat(id, inChat);
//	return;
//}

//============================================================================

void FriendsDialog::setColor()
{
    bool ok;
    QRgb color = QColorDialog::getRgba(ui.lineEdit->textColor().rgba(), &ok, this);
    if (ok) {
        mCurrentColor = QColor(color);
        colorChanged(mCurrentColor);
    }
    setFont();
}

void FriendsDialog::colorChanged(const QColor &c)
{
    QPixmap pxm(16,16);
    pxm.fill(mCurrentColor);
    ui.colorChatButton->setIcon(pxm);
}

void FriendsDialog::getFont()
{
    bool ok;
    mCurrentFont = QFontDialog::getFont(&ok, mCurrentFont, this);
    if (ok) {
        fontChanged(mCurrentFont);
    }
}

void FriendsDialog::fontChanged(const QFont &font)
{
    mCurrentFont = font;

    ui.textboldChatButton->setChecked(mCurrentFont.bold());
    ui.textunderlineChatButton->setChecked(mCurrentFont.underline());
    ui.textitalicChatButton->setChecked(mCurrentFont.italic());

    setFont();
}

void FriendsDialog::setFont()
{
    mCurrentFont.setBold(ui.textboldChatButton->isChecked());
    mCurrentFont.setUnderline(ui.textunderlineChatButton->isChecked());
    mCurrentFont.setItalic(ui.textitalicChatButton->isChecked());
    ui.lineEdit->setFont(mCurrentFont);
    ui.lineEdit->setTextColor(mCurrentColor);
    Settings->setChatScreenFont(mCurrentFont.toString());

    ui.lineEdit->setFocus();
}

// Update Chat Info information
void FriendsDialog::setChatInfo(QString info, QColor color)
{
  static unsigned int nbLines = 0;
  ++nbLines;
  // Check log size, clear it if too big
  if(nbLines > 200) {
    ui.msgText->clear();
    nbLines = 1;
  }
  ui.msgText->append(QString::fromUtf8("<font color='grey'>")+ QTime::currentTime().toString(QString::fromUtf8("hh:mm:ss")) + QString::fromUtf8("</font> - <font color='") + color.name() +QString::fromUtf8("'><i>") + info + QString::fromUtf8("</i></font>"));
}

void FriendsDialog::on_actionClear_Chat_History_triggered()
{
    ui.msgText->clear();
}

void FriendsDialog::on_actionDelete_Chat_History_triggered()
{
    if ((QMessageBox::question(this, "RetroShare", tr("Do you really want to physically delete the history?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes) {
        on_actionClear_Chat_History_triggered();
        historyKeeper.clear();
    }
}

void FriendsDialog::smileyWidgetgroupchat()
{
    Emoticons::showSmileyWidget(this, ui.emoticonBtn, SLOT(addSmileys()), true);
}

void FriendsDialog::addSmileys()
{
    ui.lineEdit->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

/* GUI stuff -> don't do anything directly with Control */
void FriendsDialog::viewprofile()
{
	/* display Dialog */

	QTreeWidgetItem *c = getCurrentPeer();


	static ProfileView *profileview = new ProfileView();


	if (!c)
		return;

	/* set the Id */
	std::string id = getPeerRsCertId(c);

	profileview -> setPeerId(id);
	profileview -> show();
}

void FriendsDialog::updateAvatar()
{
	unsigned char *data = NULL;
	int size = 0 ;

	rsMsgs->getOwnAvatarData(data,size); 

        #ifdef PEERS_DEBUG
	std::cerr << "Image size = " << size << std::endl ;
        #endif

	if(size == 0)
	   std::cerr << "Got no image" << std::endl ;

	// set the image
	QPixmap pix ;
	pix.loadFromData(data,size,"PNG") ;
	ui.avatartoolButton->setIcon(pix); // writes image into ba in PNG format

        PopupChatDialog::updateAllAvatars();

	delete[] data ;
}

void FriendsDialog::getAvatar()
{
	QString fileName;
	if (misc::getOpenFileName(this, RshareSettings::LASTDIR_IMAGES, tr("Load File"), tr("Pictures (*.png *.xpm *.jpg *.tiff *.gif)"), fileName))
	{
		QPixmap picture;
		picture = QPixmap(fileName).scaled(96,96, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

                #ifdef PEERS_DEBUG
		std::cerr << "Sending avatar image down the pipe" << std::endl ;
                #endif

		// send avatar down the pipe for other peers to get it.
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format

                #ifdef PEERS_DEBUG
		std::cerr << "Image size = " << ba.size() << std::endl ;
                #endif

		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()),ba.size()) ;	// last char 0 included.

		// I suppressed this because it gets called already by rsMsgs->setOwnAvatarData() through a Qt notification signal
		//updateAvatar() ;
	}
}

void FriendsDialog::changeAvatarClicked() 
{

	updateAvatar();
}

void FriendsDialog::on_actionCreate_New_Forum_activated()
{
    MainWindow::activatePage (MainWindow::Forums);

    CreateForum cf (this);
    cf.exec();
    
}

void FriendsDialog::on_actionCreate_New_Channel_activated()
{
#ifndef RS_RELEASE_VERSION
    MainWindow::activatePage (MainWindow::Channels);

    CreateChannel cf (this);
    cf.exec();
#endif
}


/** Loads own personal status */
void FriendsDialog::loadmypersonalstatus()
{
    ui.mypersonalstatuslabel->setText(QString::fromUtf8(rsMsgs->getCustomStateString().c_str()));
}

void FriendsDialog::statusmessage()
{
    StatusMessage statusmsgdialog (this);
    statusmsgdialog.exec();
}

void FriendsDialog::addExtraFile()
{
    QString file;
    if (misc::getOpenFileName(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", file)) {
        addAttachment(file.toUtf8().constData());
    }
}

void FriendsDialog::addAttachment(std::string filePath) {
	    /* add a AttachFileItem to the attachment section */
            std::cerr << "PopupChatDialog::addExtraFile() hashing file." << std::endl;

	    /* add widget in for new destination */
	    AttachFileItem *file = new AttachFileItem(filePath);
	    //file->

	    ui.verticalLayout->addWidget(file, 1, 0);

	    //when the file is local or is finished hashing, call the fileHashingFinished method to send a chat message
	    if (file->getState() == AFI_STATE_LOCAL) {
		fileHashingFinished(file);
	    } else {
		QObject::connect(file,SIGNAL(fileFinished(AttachFileItem *)), SLOT(fileHashingFinished(AttachFileItem *))) ;
	    }
}

void FriendsDialog::fileHashingFinished(AttachFileItem* file)
{
    std::cerr << "FriendsDialog::fileHashingFinished() started." << std::endl;

    //check that the file is ok tos end
    if (file->getState() == AFI_STATE_ERROR) {
#ifdef PEERS_DEBUG
        std::cerr << "PopupChatDialog::fileHashingFinished error file is not hashed." << std::endl;
#endif
        return;
    }

    //convert fileSize from uint_64 to string for html link
    //	char fileSizeChar [100];
    //	sprintf(fileSizeChar, "%lld", file->FileSize());
    //	std::string fileSize = *(&fileSizeChar);

    RetroShareLink link;
    if (!link.createFile(QString::fromUtf8(file->FileName().c_str()), file->FileSize(), QString::fromStdString(file->FileHash()))) {
        return;
    }
    QString mesgString = link.toHtmlSize();

    //	std::string mesgString = "<a href='retroshare://file|" + (file->FileName()) + "|" + fileSize + "|" + (file->FileHash()) + "'>"
    //	+ "retroshare://file|" + (file->FileName()) + "|" + fileSize +  "|" + (file->FileHash())  + "</a>";
#ifdef PEERS_DEBUG
    std::cerr << "FriendsDialog::fileHashingFinished mesgString : " << mesgString << std::endl;
#endif

    /* convert to real html document */
    QTextBrowser textBrowser;
    textBrowser.setHtml(mesgString);
    std::wstring msg = textBrowser.toHtml().toStdWString();

    rsMsgs->sendPublicChat(msg);
    setFont();
}

void FriendsDialog::dropEvent(QDropEvent *event)
{
    if (!(Qt::CopyAction & event->possibleActions()))
    {
        std::cerr << "FriendsDialog::dropEvent() Rejecting uncopyable DropAction" << std::endl;

        /* can't do it */
        return;
    }

    std::cerr << "FriendsDialog::dropEvent() Formats" << std::endl;
    QStringList formats = event->mimeData()->formats();
    QStringList::iterator it;
    for(it = formats.begin(); it != formats.end(); it++)
    {
        std::cerr << "Format: " << (*it).toStdString() << std::endl;
    }

    if (event->mimeData()->hasUrls())
    {
        std::cerr << "FriendsDialog::dropEvent() Urls:" << std::endl;

        QList<QUrl> urls = event->mimeData()->urls();
        QList<QUrl>::iterator uit;
        for(uit = urls.begin(); uit != urls.end(); uit++)
        {
            QString localpath = uit->toLocalFile();
            std::cerr << "Whole URL: " << uit->toString().toStdString() << std::endl;
            std::cerr << "or As Local File: " << localpath.toStdString() << std::endl;

            if (localpath.isEmpty() == false)
            {
                //Check that the file does exist and is not a directory
                QDir dir(localpath);
                if (dir.exists()) {
                    std::cerr << "FriendsDialog::dropEvent() directory not accepted."<< std::endl;
                    QMessageBox mb(tr("Drop file error."), tr("Directory can't be dropped, only files are accepted."),QMessageBox::Information,QMessageBox::Ok,0,0,this);
                    mb.exec();
                } else if (QFile::exists(localpath)) {
                    FriendsDialog::addAttachment(localpath.toUtf8().constData());
                } else {
                    std::cerr << "FriendsDialog::dropEvent() file does not exists."<< std::endl;
                    QMessageBox mb(tr("Drop file error."), tr("File not found or file name not accepted."),QMessageBox::Information,QMessageBox::Ok,0,0,this);
                    mb.exec();
                }
            }
        }
    }

    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void FriendsDialog::dragEnterEvent(QDragEnterEvent *event)
{
	/* print out mimeType */
        std::cerr << "FriendsDialog::dragEnterEvent() Formats" << std::endl;
	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;
	for(it = formats.begin(); it != formats.end(); it++)
	{
                std::cerr << "Format: " << (*it).toStdString() << std::endl;
	}

	if (event->mimeData()->hasUrls())
	{
                std::cerr << "FriendsDialog::dragEnterEvent() Accepting Urls" << std::endl;
		event->acceptProposedAction();
	}
	else
	{
                std::cerr << "FriendsDialog::dragEnterEvent() No Urls" << std::endl;
	}
}

bool FriendsDialog::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.msgText->document()->toPlainText();
    ui.msgText->document()->setModified(false);
    return true;
}

bool FriendsDialog::fileSaveAs()
{
    QString fn;
    if (misc::getSaveFileName(this, RshareSettings::LASTDIR_HISTORY, tr("Save as..."), tr("Text File (*.txt );;All Files (*)"), fn)) {
        setCurrentFileName(fn);
        return fileSave();
    }

    return false;
}

void FriendsDialog::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgText->document()->setModified(false);

    setWindowModified(false);
}

////play sound when recv a message
void FriendsDialog::playsound(){
    Settings->beginGroup("Sound");
        Settings->beginGroup("SoundFilePath");
            QString OnlineSound = Settings->value("NewChatMessage","").toString();
        Settings->endGroup();
        Settings->beginGroup("Enable");
             bool flag = Settings->value("NewChatMessage",false).toBool();
        Settings->endGroup();
    Settings->endGroup();
    if(!OnlineSound.isEmpty()&&flag)
        if(QSound::isAvailable())
            QSound::play(OnlineSound);
}

void FriendsDialog::displayMenu()
{
    QMenu *displaymenu = new QMenu();

    displaymenu->addAction(ui.actionSort_Peers_Descending_Order);
    displaymenu->addAction(ui.actionSort_Peers_Ascending_Order);
    displaymenu->addAction(ui.action_Hide_Offline_Friends);
    displaymenu->addAction(ui.action_Sort_by_State);
    displaymenu->addAction(ui.action_Hide_Status_Column);
    displaymenu->addAction(ui.action_Hide_State);

    ui.displayButton->setMenu(displaymenu);
}

void FriendsDialog::setStateColumn()
{
    if (ui.action_Hide_Status_Column->isChecked()) {
        ui.peertreeWidget->setColumnHidden(COLUMN_STATE, true);
        ui.peertreeWidget->setHeaderHidden(true);
        ui.action_Hide_State->setEnabled(true);
        wasStatusColumnHidden = true;
    } else {
        ui.peertreeWidget->setColumnHidden(COLUMN_STATE, false);
        ui.peertreeWidget->setHeaderHidden(false);
        ui.action_Hide_State->setEnabled(false);
        if (correctColumnStatusSize) {
            correctColumnStatusSize = false;
            ui.peertreeWidget->header()->resizeSection(COLUMN_STATE, 100);
        }
        if (wasStatusColumnHidden) {
            ui.peertreeWidget->header()->resizeSection(COLUMN_NAME, ui.peertreeWidget->header()->sectionSize(COLUMN_NAME) - ui.peertreeWidget->header()->sectionSize(COLUMN_STATE));
        }
        wasStatusColumnHidden = false;
    }

    if(ui.action_Sort_by_State->isChecked()) {
        ui.peertreeWidget->sortByColumn(COLUMN_STATE);
    } else {
        ui.peertreeWidget->sortByColumn(COLUMN_NAME);
    }
}

void FriendsDialog::sortPeersAscendingOrder()
{
    ui.peertreeWidget->sortByColumn(ui.peertreeWidget->sortColumn(), Qt::AscendingOrder);
}

void FriendsDialog::sortPeersDescendingOrder()
{
    ui.peertreeWidget->sortByColumn(ui.peertreeWidget->sortColumn(), Qt::DescendingOrder);
}

void FriendsDialog::peerSortIndicatorChanged(int column, Qt::SortOrder)
{
    ui.action_Sort_by_State->setChecked(column == COLUMN_STATE);
}

void FriendsDialog::on_actionMessageHistory_triggered()
{
    ImHistoryBrowser imBrowser("", historyKeeper, ui.lineEdit, this);
    imBrowser.exec();
}

void FriendsDialog::on_actionAdd_Group_activated()
{
    CreateGroup createGrpDialog ("", this);
    createGrpDialog.exec();
}

void FriendsDialog::addToGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    std::string groupId = qobject_cast<QAction*>(sender())->data().toString().toStdString();
    std::string gpgId = c->data(COLUMN_DATA, ROLE_ID).toString().toStdString();

    if (gpgId.empty() || groupId.empty()) {
        return;
    }

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
}

void FriendsDialog::moveToGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    std::string groupId = qobject_cast<QAction*>(sender())->data().toString().toStdString();
    std::string gpgId = c->data(COLUMN_DATA, ROLE_ID).toString().toStdString();

    if (gpgId.empty() || groupId.empty()) {
        return;
    }

    // remove from all groups
    rsPeers->assignPeerToGroup("", gpgId, false);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
}

void FriendsDialog::removeFromGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    std::string groupId = qobject_cast<QAction*>(sender())->data().toString().toStdString();
    std::string gpgId = c->data(COLUMN_DATA, ROLE_ID).toString().toStdString();

    if (gpgId.empty()) {
        return;
    }

    // remove from (all) group(s)
    rsPeers->assignPeerToGroup(groupId, gpgId, false);
}

void FriendsDialog::editGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GROUP) {
        // wrong type
        return;
    }

    std::string groupId = c->data(COLUMN_DATA, ROLE_ID).toString().toStdString();

    if (groupId.empty()) {
        return;
    }

    CreateGroup editGrpDialog (groupId, this);
    editGrpDialog.exec();
}

void FriendsDialog::removeGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GROUP) {
        // wrong type
        return;
    }

    std::string groupId = c->data(COLUMN_DATA, ROLE_ID).toString().toStdString();

    if (groupId.empty()) {
        return;
    }

    rsPeers->removeGroup(groupId);
}

void FriendsDialog::groupsChanged(int type)
{
    Q_UNUSED(type);

    groupsHasChanged = true;
}

void FriendsDialog::newsFeedChanged(int count)
{
    if (count) {
        ui.peertabWidget->tabBar()->setTabText(newsFeedTabIndex, QString("%1 (%2)").arg(newsFeedText).arg(count));
        ui.peertabWidget->tabBar()->setTabTextColor(newsFeedTabIndex, Qt::blue);
        ui.peertabWidget->tabBar()->setTabIcon(newsFeedTabIndex, QIcon(IMAGE_NEWSFEED_NEW));
    } else {
        ui.peertabWidget->tabBar()->setTabText(newsFeedTabIndex, newsFeedText);
        ui.peertabWidget->tabBar()->setTabTextColor(newsFeedTabIndex, newsFeedTabColor);
        ui.peertabWidget->tabBar()->setTabIcon(newsFeedTabIndex,  QIcon(IMAGE_NEWSFEED));
    }
}

void FriendsDialog::updateOwnStatus(const QString &peer_id, int status)
{
    // add self nick + own status
    if (peer_id.toStdString() == rsPeers->getOwnId())
    {
        // my status has changed

        switch (status) {
        case RS_STATUS_OFFLINE:
            ui.avatartoolButton->setStyleSheet("QToolButton#avatartoolButton{border-image:url(:/images/mystatus_bg_offline.png); }");
            break;

        case RS_STATUS_INACTIVE:
            ui.avatartoolButton->setStyleSheet("QToolButton#avatartoolButton{border-image:url(:/images/mystatus_bg_idle.png); }");
            break;

        case RS_STATUS_ONLINE:
            ui.avatartoolButton->setStyleSheet("QToolButton#avatartoolButton{border-image:url(:/images/mystatus_bg_online.png); }");
            break;

        case RS_STATUS_AWAY:
            ui.avatartoolButton->setStyleSheet("QToolButton#avatartoolButton{border-image:url(:/images/mystatus_bg_idle.png); }");
            break;

        case RS_STATUS_BUSY:
            ui.avatartoolButton->setStyleSheet("QToolButton#avatartoolButton{border-image:url(:/images/mystatus_bg_busy.png); }");
            break;
        }

        return;
    }
}
