/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 RetroShare Team
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

#include <algorithm>

#include <QShortcut>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QWidgetAction>
#include <QDateTime>

#include "retroshare/rspeers.h"

#include "GroupDefs.h"
#include "gui/chat/ChatDialog.h"
//#include "gui/chat/CreateLobbyDialog.h"
#include "gui/common/AvatarDefs.h"
#include "gui/ServicePermissionDialog.h"
#include "gui/FriendRecommendDialog.h"

#include "gui/connect/ConfCertDialog.h"
#include "gui/connect/ConnectFriendWizard.h"
#include "gui/groups/CreateGroup.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/notifyqt.h"
#include "gui/RetroShareLink.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#ifdef UNFINISHED_FD
#include "gui/unfinished/profile/ProfileView.h"
#endif
#include "RSTreeWidgetItem.h"
#include "StatusDefs.h"
#include "util/misc.h"
#include "vmessagebox.h"

#include "gui/connect/ConnectProgressDialog.h"

#include "FriendList.h"
#include "ui_FriendList.h"

/* Images for context menu icons */
#define IMAGE_DENYFRIEND         ":/images/denied16.png"
#define IMAGE_REMOVEFRIEND       ":/images/remove_user24.png"
#define IMAGE_EXPORTFRIEND       ":/images/user/friend_suggestion16.png"
#define IMAGE_ADDFRIEND          ":/images/user/add_user16.png"
#define IMAGE_FRIENDINFO         ":/images/info16.png"
#define IMAGE_CHAT               ":/images/chat_24.png"
#define IMAGE_MSG                ":/images/mail_new.png"
#define IMAGE_CONNECT            ":/images/connect_friend.png"
#define IMAGE_COPYLINK           ":/images/copyrslink.png"
#define IMAGE_GROUP16            ":/images/user/group16.png"
#define IMAGE_EDIT               ":/images/edit_16.png"
#define IMAGE_REMOVE             ":/images/delete.png"
#define IMAGE_EXPAND             ":/images/edit_add24.png"
#define IMAGE_COLLAPSE           ":/images/edit_remove24.png"
#define IMAGE_PERMISSIONS        ":/images/admin-16.png"
/* Images for Status icons */
#define IMAGE_AVAILABLE          ":/images/user/identityavaiblecyan24.png"
#define IMAGE_CONNECT2           ":/images/reload24.png"
#define IMAGE_PASTELINK          ":/images/pasterslink.png"
#define IMAGE_GROUP24            ":/images/user/group24.png"

#define COLUMN_COUNT        5
#define COLUMN_NAME         0
#define COLUMN_STATE        1
#define COLUMN_LAST_CONTACT 2
#define COLUMN_AVATAR       3
#define COLUMN_IP           4

#define COLUMN_DATA     0 // column for storing the userdata id

#define COLUMN_AVATAR_WIDTH 42

#define ROLE_SORT        Qt::UserRole
#define ROLE_ID          Qt::UserRole + 1
#define ROLE_STANDARD    Qt::UserRole + 2

#define TYPE_GPG   0
#define TYPE_SSL   1
#define TYPE_GROUP 2

// states for sorting (equal values are possible)
// used in BuildSortString - state + name
#define PEER_STATE_ONLINE       1
#define PEER_STATE_BUSY         2
#define PEER_STATE_AWAY         3
#define PEER_STATE_AVAILABLE    4
#define PEER_STATE_INACTIVE     5
#define PEER_STATE_OFFLINE      6

#define BuildStateSortString(bEnabled,sName,nState) bEnabled ? (QString ("%1").arg(nState) + " " + sName) : sName


/******
 * #define FRIENDS_DEBUG 1
 *****/

FriendList::FriendList(QWidget *parent) :
    RsAutoUpdatePage(1500, parent),
    ui(new Ui::FriendList),
    m_compareRole(new RSTreeWidgetItemCompareRole),
    mBigName(false),
    mShowGroups(true),
    mHideState(false),
    mHideUnconnected(false),
    groupsHasChanged(false),
    openGroups(NULL),
    openPeers(NULL)
{
    ui->setupUi(this);

    m_compareRole->setRole(COLUMN_NAME, ROLE_SORT);
    m_compareRole->setRole(COLUMN_STATE, ROLE_SORT);
    m_compareRole->setRole(COLUMN_LAST_CONTACT, ROLE_SORT);
    m_compareRole->setRole(COLUMN_IP, ROLE_SORT);
    m_compareRole->setRole(COLUMN_AVATAR, ROLE_STANDARD);

    connect(ui->peerTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(peerTreeWidgetCostumPopupMenu()));
    connect(ui->peerTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(chatfriend(QTreeWidgetItem *)));

    connect(NotifyQt::getInstance(), SIGNAL(groupsChanged(int)), this, SLOT(groupsChanged()));
    connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(insertPeers()));
    connect(NotifyQt::getInstance(), SIGNAL(peerHasNewAvatar(const QString&)), this, SLOT(updateAvatar(const QString&)));

    connect(ui->actionHideOfflineFriends, SIGNAL(triggered(bool)), this, SLOT(setHideUnconnected(bool)));
    connect(ui->actionShowStatusColumn, SIGNAL(triggered(bool)), this, SLOT(setShowStatusColumn(bool)));
    connect(ui->actionShowAvatarColumn, SIGNAL(triggered(bool)), this, SLOT(setShowAvatarColumn(bool)));
    connect(ui->actionShowLastContactColumn, SIGNAL(triggered(bool)), this, SLOT(setShowLastContactColumn(bool)));
    connect(ui->actionShowIPColumn, SIGNAL(triggered(bool)), this, SLOT(setShowIPColumn(bool)));
    connect(ui->actionHideState, SIGNAL(triggered(bool)), this, SLOT(setHideState(bool)));
    connect(ui->actionRootIsDecorated, SIGNAL(triggered(bool)), this, SLOT(setRootIsDecorated(bool)));
    connect(ui->actionShowGroups, SIGNAL(triggered(bool)), this, SLOT(setShowGroups(bool)));

    connect(ui->actionSortByName, SIGNAL(triggered()), this, SLOT(setSortByName()));
    connect(ui->actionSortByState, SIGNAL(triggered()), this, SLOT(setSortByState()));
    connect(ui->actionSortByLastContact, SIGNAL(triggered()), this, SLOT(setSortByLastContact()));
    connect(ui->actionSortByIP, SIGNAL(triggered()), this, SLOT(setSortByIP()));
    connect(ui->actionSortPeersAscendingOrder, SIGNAL(triggered()), this, SLOT(sortPeersAscendingOrder()));
    connect(ui->actionSortPeersDescendingOrder, SIGNAL(triggered()), this, SLOT(sortPeersDescendingOrder()));

    initializeHeader(false);

    ui->peerTreeWidget->sortItems(COLUMN_NAME, Qt::AscendingOrder);

    // set header text aligment
    QTreeWidgetItem *headerItem = ui->peerTreeWidget->headerItem();
    headerItem->setTextAlignment(COLUMN_NAME, Qt::AlignHCenter | Qt::AlignVCenter);
    headerItem->setTextAlignment(COLUMN_STATE, Qt::AlignLeft | Qt::AlignVCenter);
    headerItem->setTextAlignment(COLUMN_AVATAR, Qt::AlignLeft | Qt::AlignVCenter);

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), ui->peerTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT(removefriend()));
}

FriendList::~FriendList()
{
    delete ui;
    delete(m_compareRole);
}

void FriendList::processSettings(bool bLoad)
{
    int peerTreeVersion = 2; // version number for the settings to solve problems when modifying the column count

    if (bLoad) {
        // load settings

        // state of peer tree
        if (Settings->value("peerTreeVersion").toInt() == peerTreeVersion) {
            ui->peerTreeWidget->header()->restoreState(Settings->value("peerTree").toByteArray());
        }
//        ui->peerTreeWidget->header()->doItemsLayout(); // is needed because I added a third column
                                                       // restoreState would corrupt the internal sectionCount

        // state of the columns
        setShowStatusColumn(Settings->value("showStatusColumn", !ui->peerTreeWidget->isColumnHidden(COLUMN_STATE)).toBool());
        setShowLastContactColumn(Settings->value("showLastContactColumn", !ui->peerTreeWidget->isColumnHidden(COLUMN_LAST_CONTACT)).toBool());
        setShowIPColumn(Settings->value("showIPColumn", !ui->peerTreeWidget->isColumnHidden(COLUMN_IP)).toBool());
        setShowAvatarColumn(Settings->value("showAvatarColumn", !ui->peerTreeWidget->isColumnHidden(COLUMN_AVATAR)).toBool());

        // states
        setHideUnconnected(Settings->value("hideUnconnected", mHideUnconnected).toBool());
        setHideState(Settings->value("hideState", mHideState).toBool());
        //setRootIsDecorated(Settings->value("rootIsDecorated", ui->peerTreeWidget->rootIsDecorated()).toBool());
        setRootIsDecorated(true) ;
        setShowGroups(Settings->value("showGroups", mShowGroups).toBool());

        // open groups
        int arrayIndex = Settings->beginReadArray("Groups");
        for (int index = 0; index < arrayIndex; index++) {
            Settings->setArrayIndex(index);
            addGroupToExpand(Settings->value("open").toString().toStdString());
        }
        Settings->endArray();

        initializeHeader(true);
        updateHeader();
    } else {
        // save settings

        // state of peer tree
        Settings->setValue("peerTree", ui->peerTreeWidget->header()->saveState());
        Settings->setValue("peerTreeVersion", peerTreeVersion);

        // state of the columns
        Settings->setValue("showStatusColumn", !ui->peerTreeWidget->isColumnHidden(COLUMN_STATE));
        Settings->setValue("showLastContactColumn", !ui->peerTreeWidget->isColumnHidden(COLUMN_LAST_CONTACT));
        Settings->setValue("showIPColumn", !ui->peerTreeWidget->isColumnHidden(COLUMN_IP));
        Settings->setValue("showAvatarColumn", !ui->peerTreeWidget->isColumnHidden(COLUMN_AVATAR));

        // states
        Settings->setValue("hideUnconnected", mHideUnconnected);
        Settings->setValue("hideState", mHideState);
        Settings->setValue("rootIsDecorated", ui->peerTreeWidget->rootIsDecorated());
        Settings->setValue("showGroups", mShowGroups);

        // open groups
        Settings->beginWriteArray("Groups");
        int arrayIndex = 0;
        std::set<std::string> expandedPeers;
        getExpandedGroups(expandedPeers);
        foreach (std::string groupId, expandedPeers) {
            Settings->setArrayIndex(arrayIndex++);
            Settings->setValue("open", QString::fromStdString(groupId));
        }
        Settings->endArray();
    }
}

void FriendList::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::StyleChange:
        insertPeers();
        break;
    default:
        // remove compiler warnings
        break;
    }
}

void FriendList::initializeHeader(bool afterLoadSettings)
{
    // set column size
    QHeaderView *header = ui->peerTreeWidget->header();
    header->setMovable(true);
    //header->setResizeMode(COLUMN_NAME, QHeaderView::Stretch);
    header->setResizeMode(COLUMN_NAME, QHeaderView::Interactive);
    header->setResizeMode(COLUMN_STATE, QHeaderView::Interactive);
    header->setResizeMode(COLUMN_LAST_CONTACT, QHeaderView::Interactive);
    header->setResizeMode(COLUMN_IP, QHeaderView::Interactive);
    header->setResizeMode(COLUMN_AVATAR, QHeaderView::Fixed);

/*    if (!afterLoadSettings) {
        header->resizeSection(COLUMN_NAME, 150);
        header->resizeSection(COLUMN_LAST_CONTACT, 120);
    }*/
    header->resizeSection(COLUMN_AVATAR, COLUMN_AVATAR_WIDTH);
}

/* Utility Fns */
inline std::string getRsId(QTreeWidgetItem *item)
{
    return item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
}

/**
 * Creates the context popup menu and its submenus,
 * then shows it at the current cursor position.
 */
void FriendList::peerTreeWidgetCostumPopupMenu()
{
    QTreeWidgetItem *c = getCurrentPeer();

    QMenu contextMnu(this);

    QWidget *widget = new QWidget(&contextMnu);
    widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

    // create menu header
    QHBoxLayout *hbox = new QHBoxLayout(widget);
    hbox->setMargin(0);
    hbox->setSpacing(6);

    QLabel *iconLabel = new QLabel(widget);
    iconLabel->setPixmap(QPixmap(":/images/user/friends24.png"));
    iconLabel->setMaximumSize(iconLabel->frameSize().height() + 24, 24);
    hbox->addWidget(iconLabel);

    QLabel *textLabel = new QLabel("<strong>RetroShare</strong>", widget);
    hbox->addWidget(textLabel);

    QSpacerItem *spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(spacerItem);

    widget->setLayout(hbox);

    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(widget);
    contextMnu.addAction(widgetAction);

    // create menu entries
    if (c)
     { // if a peer is selected
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

//         QMenu *lobbyMenu = NULL;

         switch (type) {
         case TYPE_GROUP:
             {
                 bool standard = c->data(COLUMN_DATA, ROLE_STANDARD).toBool();

                 contextMnu.addAction(QIcon(IMAGE_MSG), tr("Message Group"), this, SLOT(msgfriend()));
//                 contextMnu.addAction(QIcon(IMAGE_ADDFRIEND), tr("Add Friend"), this, SLOT(addFriend()));

                 contextMnu.addSeparator();

                 contextMnu.addAction(QIcon(IMAGE_EDIT), tr("Edit Group"), this, SLOT(editGroup()));

                 QAction *action = contextMnu.addAction(QIcon(IMAGE_REMOVE), tr("Remove Group"), this, SLOT(removeGroup()));
                 action->setDisabled(standard);

//                 lobbyMenu = contextMnu.addMenu(QIcon(IMAGE_CHAT), tr("Chat lobbies")) ;
             }
             break;
         case TYPE_GPG:
         case TYPE_SSL:
             {
                 contextMnu.addAction(QIcon(IMAGE_CHAT), tr("Chat"), this, SLOT(chatfriendproxy()));
//                lobbyMenu = contextMnu.addMenu(QIcon(IMAGE_CHAT), tr("Chat lobbies")) ;

                 contextMnu.addAction(QIcon(IMAGE_MSG), tr("Message Friend"), this, SLOT(msgfriend()));

                 contextMnu.addSeparator();

                 contextMnu.addAction(QIcon(IMAGE_FRIENDINFO), tr("Friend Details"), this, SLOT(configurefriend()));
                 //            contextMnu.addAction(QIcon(IMAGE_PEERINFO), tr("Profile View"), this, SLOT(viewprofile()));
                 //            action = contextMnu.addAction(QIcon(IMAGE_EXPORTFRIEND), tr("Export Friend"), this, SLOT(exportfriend()));

                 if (type == TYPE_GPG || type == TYPE_SSL) {
                     contextMnu.addAction(QIcon(IMAGE_EXPORTFRIEND), tr("Recommend this Friend to..."), this, SLOT(recommendfriend()));
                 }

                 contextMnu.addAction(QIcon(IMAGE_CONNECT), tr("Attempt to connect"), this, SLOT(connectfriend()));

                 if (type == TYPE_SSL) {
                     contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy certificate link"), this, SLOT(copyFullCertificate()));
                 }
                 if (type == TYPE_GPG) {
                     contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyLink()));
                 }

                 QAction *action = contextMnu.addAction(QIcon(IMAGE_PASTELINK), tr("Paste Friend Link"), this, SLOT(pastePerson()));
                 if (RSLinkClipboard::empty(RetroShareLink::TYPE_PERSON) && RSLinkClipboard::empty(RetroShareLink::TYPE_CERTIFICATE)) {
                     action->setDisabled(true);
                 }

                 if (type == TYPE_GPG) {
                     contextMnu.addAction(QIcon(IMAGE_DENYFRIEND), tr("Deny Friend"), this, SLOT(removefriend()));
                 } else {
                     //this is a SSL key
                     contextMnu.addAction(QIcon(IMAGE_REMOVEFRIEND), tr("Remove Friend Location"), this, SLOT(removefriend()));
                 }

                 if (mShowGroups && type == TYPE_GPG) {
                     QMenu* addToGroupMenu = NULL;
                     QMenu* moveToGroupMenu = NULL;

                     std::list<RsGroupInfo> groupInfoList;
                     rsPeers->getGroupInfoList(groupInfoList);

                     GroupDefs::sortByName(groupInfoList);

                     std::string gpgId = getRsId(c);

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
                         QMenu *groupsMenu = contextMnu.addMenu(QIcon(IMAGE_GROUP16), tr("Groups"));

								 groupsMenu->addAction(QIcon(IMAGE_EXPAND), tr("Create new group"), this, SLOT(createNewGroup()));

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

//         if (lobbyMenu) {
//            lobbyMenu->addAction(QIcon(IMAGE_ADDFRIEND), tr("Create new"), this, SLOT(createchatlobby()));
//
//             // Get existing lobbies
//             //
//             std::list<ChatLobbyInfo> cl_infos ;
//             rsMsgs->getChatLobbyList(cl_infos) ;
//
//             for(std::list<ChatLobbyInfo>::const_iterator it(cl_infos.begin());it!=cl_infos.end();++it)
//             {
//                 std::cerr << "Adding meny entry with lobby id " << std::hex << (*it).lobby_id << std::dec << std::endl;
//
//                 QMenu *mnu2 = lobbyMenu->addMenu(QIcon(IMAGE_CHAT), QString::fromUtf8((*it).lobby_name.c_str())) ;
//
//                 QAction* inviteToLobbyAction = new QAction((type == TYPE_GROUP) ? tr("Invite this group") : tr("Invite this friend"), mnu2);
//                 inviteToLobbyAction->setData(QString::number((*it).lobby_id));
//                 connect(inviteToLobbyAction, SIGNAL(triggered()), this, SLOT(inviteToLobby()));
//                 mnu2->addAction(inviteToLobbyAction);
//
//                 QAction* showLobbyAction = new QAction(tr("Show"), mnu2);
//                 showLobbyAction->setData(QString::number((*it).lobby_id));
//                 connect(showLobbyAction, SIGNAL(triggered()), this, SLOT(showLobby()));
//                 mnu2->addAction(showLobbyAction);
//
//                 QAction* unsubscribeToLobbyAction = new QAction(tr("Unsubscribe"), mnu2);
//                 unsubscribeToLobbyAction->setData(QString::number((*it).lobby_id));
//                 connect(unsubscribeToLobbyAction, SIGNAL(triggered()), this, SLOT(unsubscribeToLobby()));
//                 mnu2->addAction(unsubscribeToLobbyAction);
//             }
//         }
     } else {
        QAction *action = contextMnu.addAction(QIcon(IMAGE_PASTELINK), tr("Paste Friend Link"), this, SLOT(pastePerson()));
        if (RSLinkClipboard::empty(RetroShareLink::TYPE_PERSON) && RSLinkClipboard::empty(RetroShareLink::TYPE_CERTIFICATE)) {
            action->setDisabled(true);
        }
    }


    contextMnu.addSeparator();

    contextMnu.addAction(QIcon(IMAGE_EXPAND), tr("Recommend many friends to each others"), this, SLOT(recommendFriends()));
    contextMnu.addAction(QIcon(IMAGE_PERMISSIONS), tr("Service permissions matrix"), this, SLOT(servicePermission()));

    contextMnu.addSeparator();

    contextMnu.addAction(QIcon(IMAGE_EXPAND), tr("Expand all"), ui->peerTreeWidget, SLOT(expandAll()));
    contextMnu.addAction(QIcon(IMAGE_COLLAPSE), tr("Collapse all"), ui->peerTreeWidget, SLOT(collapseAll()));

    contextMnu.exec(QCursor::pos());
}
void FriendList::createNewGroup()
{
    CreateGroup createGrpDialog ("", this);
    createGrpDialog.exec();
}

void FriendList::recommendFriends()
{
	FriendRecommendDialog::showYourself();
}
void FriendList::servicePermission()
{
	ServicePermissionDialog dlg;
	dlg.exec();
}

void FriendList::updateDisplay()
{
    insertPeers();
}

void FriendList::groupsChanged()
{
    groupsHasChanged = true;
    insertPeers();
}

void FriendList::updateAvatar(const QString& id)
{
    if (ui->peerTreeWidget->isColumnHidden(COLUMN_AVATAR))
        return;

    QTreeWidgetItemIterator it(ui->peerTreeWidget);
    while (*it) {
        if ((*it)->type() == TYPE_SSL && id == (*it)->data(COLUMN_DATA, ROLE_ID).toString()) {
            if ((*it)->parent() != NULL && (*it)->parent()->type() == TYPE_GPG) {
                QPixmap avatar;
                AvatarDefs::getAvatarFromSslId(id.toStdString(), avatar);
                QIcon avatar_icon(avatar);
                (*it)->parent()->setIcon(COLUMN_AVATAR, avatar_icon);
            }
        }
        ++it;
    }
}

/**
 * Get the list of peers from the RsIface.
 * Adds all friend gpg ids, with their locations as children to the peerTreeWidget.
 * If enabled, peers are sorted in their associated groups.
 * Groups are only updated, when groupsHasChanged is true.
 */
void  FriendList::insertPeers()
{
    if (RsAutoUpdatePage::eventsLocked())
        return;

#ifdef FRIENDS_DEBUG
    std::cerr << "FriendList::insertPeers() called." << std::endl;
#endif

    bool isStatusColumnHidden = ui->peerTreeWidget->isColumnHidden(COLUMN_STATE);
    bool isAvatarColumnHidden = ui->peerTreeWidget->isColumnHidden(COLUMN_AVATAR);
    bool isIPColumnHidden = ui->peerTreeWidget->isColumnHidden(COLUMN_IP);

    std::list<StatusInfo> statusInfo;
    rsStatus->getStatusList(statusInfo);

    if (!rsPeers) {
        /* not ready yet! */
        std::cerr << "FriendList::insertPeers() not ready yet : rsPeers unintialized."  << std::endl;
        return;
    }

    // get ids of existing private chat messages
    std::list<std::string> privateChatIds;
    rsMsgs->getPrivateChatQueueIds(true, privateChatIds);

    // get existing groups
    std::list<RsGroupInfo> groupInfoList;
    std::list<RsGroupInfo>::iterator groupIt;
    rsPeers->getGroupInfoList(groupInfoList);

    std::list<std::string> gpgFriends;
    std::list<std::string>::iterator gpgIt;
    rsPeers->getGPGAcceptedList(gpgFriends);

    //add own gpg id, if we have more than on location (ssl client)
    std::list<std::string> ownSslContacts;
    std::string ownId = rsPeers->getGPGOwnId();
    rsPeers->getAssociatedSSLIds(ownId, ownSslContacts);
    if (ownSslContacts.size() > 0) {
        gpgFriends.push_back(ownId);
    }

    /* get a link to the table */
    QTreeWidget *peerTreeWidget = ui->peerTreeWidget;

    // remove items don't exist anymore
    QTreeWidgetItemIterator itemIterator(peerTreeWidget);
    QTreeWidgetItem *item;
    while ((item = *itemIterator) != NULL) {
        itemIterator++;
        switch (item->type()) {
        case TYPE_GPG:
        {
            QTreeWidgetItem *parent = item->parent();
            std::string gpg_widget_id = getRsId(item);

            // remove items that are not friends anymore
            if (std::find(gpgFriends.begin(), gpgFriends.end(), gpg_widget_id) == gpgFriends.end()) {
                if (parent) {
                    delete(parent->takeChild(parent->indexOfChild(item)));
                } else {
                    delete(peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(item)));
                }
                break;
            }

            if (mShowGroups && groupsHasChanged) {
                if (parent) {
                    if (parent->type() == TYPE_GROUP) {
                        std::string groupId = getRsId(parent);

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
                    // gpg item without group, check if the gpg id is assigned to a group
                    for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
                        if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpg_widget_id) != groupIt->peerIds.end()) {
                            delete(peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(item)));
                            break;
                        }
                    }
                }
            }
        }
            break;
        case TYPE_GROUP:
        {
            if (!mShowGroups) {
                if (item->parent()) {
                    delete(item->parent()->takeChild(item->parent()->indexOfChild(item)));
                } else {
                    delete(peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(item)));
                }
            } else if (groupsHasChanged) {
                // remove deleted groups
                std::string groupId = getRsId(item);
                for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
                    if (groupIt->id == groupId) {
                        break;
                    }
                }
                if (groupIt == groupInfoList.end() || groupIt->peerIds.size() == 0) {
                    if (item->parent()) {
                        delete(item->parent()->takeChild(item->parent()->indexOfChild(item)));
                    } else {
                        delete(peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(item)));
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
        if (mShowGroups && groupIt != groupInfoList.end()) {
            groupInfo = &(*groupIt);

            if ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) && groupInfo->peerIds.size() == 0) {
                // don't show empty standard groups
                groupIt++;
                continue;
            }

            // search existing group item
            int itemCount = peerTreeWidget->topLevelItemCount();
            for (int index = 0; index < itemCount; index++) {
                QTreeWidgetItem *groupItemLoop = peerTreeWidget->topLevelItem(index);
                if (groupItemLoop->type() == TYPE_GROUP && getRsId(groupItemLoop) == groupInfo->id) {
                    groupItem = groupItemLoop;
                    break;
                }
            }

            if (groupItem == NULL) {
                // add group item
                groupItem = new RSTreeWidgetItem(m_compareRole, TYPE_GROUP);

                /* Add item to the list. Add here, because for setHidden the item must be added */
                peerTreeWidget->addTopLevelItem(groupItem);

                groupItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
                groupItem->setSizeHint(COLUMN_NAME, QSize(26, 26));
                groupItem->setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter);
                groupItem->setIcon(COLUMN_NAME, QIcon(IMAGE_GROUP24));
                groupItem->setForeground(COLUMN_NAME, QBrush(textColorGroup()));

                /* used to find back the item */
                groupItem->setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(groupInfo->id));
                groupItem->setData(COLUMN_DATA, ROLE_STANDARD, (groupInfo->flag & RS_GROUP_FLAG_STANDARD) ? true : false);

            } else {
                // remove all gpg items that are not more assigned
                int childCount = groupItem->childCount();
                int childIndex = 0;
                while (childIndex < childCount) {
                    QTreeWidgetItem *gpgItemLoop = groupItem->child(childIndex);
                    if (gpgItemLoop->type() == TYPE_GPG) {
                        if (std::find(groupInfo->peerIds.begin(), groupInfo->peerIds.end(), getRsId(gpgItemLoop)) == groupInfo->peerIds.end()) {
                            delete(groupItem->takeChild(groupItem->indexOfChild(gpgItemLoop)));
                            childCount = groupItem->childCount();
                            continue;
                        }
                    }
                    childIndex++;
                }
            }

            if (openGroups != NULL && openGroups->find(groupInfo->id) != openGroups->end()) {
                groupItem->setExpanded(true);
            }

            // name is set after calculation of online/offline items
        }

        // iterate through gpg friends
        for (gpgIt = gpgFriends.begin(); gpgIt != gpgFriends.end(); gpgIt++) {
            std::string gpgId = *gpgIt;

            if (mShowGroups) {
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
            }

            //add the gpg friends
#ifdef FRIENDS_DEBUG
            std::cerr << "FriendList::insertPeers() inserting gpg_id : " << gpgId << std::endl;
#endif

            /* make a widget per friend */
            QTreeWidgetItem *gpgItem = NULL;
            QTreeWidgetItem *gpgItemLoop = NULL;

            // search existing gpg item
            int itemCount = groupItem ? groupItem->childCount() : peerTreeWidget->topLevelItemCount();
            for (int index = 0; index < itemCount; index++) {
                gpgItemLoop = groupItem ? groupItem->child(index) : peerTreeWidget->topLevelItem(index);
                if (gpgItemLoop->type() == TYPE_GPG && getRsId(gpgItemLoop) == gpgId) {
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
                        delete (peerTreeWidget->takeTopLevelItem(peerTreeWidget->indexOfTopLevelItem(gpgItem)));
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
                    peerTreeWidget->addTopLevelItem(gpgItem);
                }

                gpgItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
                gpgItem->setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter);

                /* not displayed, used to find back the item */
                gpgItem->setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(detail.id));
            }

            if (mBigName && !mHideState && isStatusColumnHidden) {
                gpgItem->setSizeHint(COLUMN_NAME, QSize(40, 40));
            } else {
                gpgItem->setSizeHint(COLUMN_NAME, QSize(26, 26));
            }

            availableCount++;

            QString gpgItemText = QString::fromUtf8(detail.name.c_str());

            // remove items that are not friends anymore
            int childCount = gpgItem->childCount();
            int childIndex = 0;
            while (childIndex < childCount) {
                std::string ssl_id = getRsId(gpgItem->child(childIndex));
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
            int bestPeerState = 0;        // for gpg item
            unsigned int bestRSState = 0; // for gpg item
            std::string bestSslId;        // for gpg item
            QString bestCustomStateString;// for gpg item
            std::list<std::string> sslContacts;
            QDateTime lastContact;
            QString itemIP;

            rsPeers->getAssociatedSSLIds(detail.gpg_id, sslContacts);
            for (std::list<std::string>::iterator sslIt = sslContacts.begin(); sslIt != sslContacts.end(); sslIt++) {
                QTreeWidgetItem *sslItem = NULL;
                std::string sslId = *sslIt;

                //find the corresponding sslItem child item of the gpg item
                bool newChild = true;
                childCount = gpgItem->childCount();
                for (int childIndex = 0; childIndex < childCount; childIndex++) {
                    // we assume, that only ssl items are child of the gpg item, so we don't need to test the type
                    if (getRsId(gpgItem->child(childIndex)) == sslId) {
                        sslItem = gpgItem->child(childIndex);
                        newChild = false;
                        break;
                    }
                }

                RsPeerDetails sslDetail;
                if (!rsPeers->getPeerDetails(sslId, sslDetail) || !rsPeers->isFriend(sslId)) {
#ifdef FRIENDS_DEBUG
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

#ifdef FRIENDS_DEBUG
                    std::cerr << "FriendList::insertPeers() inserting sslItem." << std::endl;
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
                sText = QString::fromUtf8(sslDetail.location.c_str());
                if (customStateString.isEmpty() == false) {
                    sText += " - " + customStateString;
                }

                QString connectStateString = StatusDefs::connectStateString(sslDetail);
                if (!isStatusColumnHidden) {
                    sslItem->setText(COLUMN_STATE, connectStateString);
                } else if (!mHideState && connectStateString.isEmpty() == false) {
                    sText += " [" + StatusDefs::connectStateString(sslDetail) + "]";
                }
                sslItem->setText( COLUMN_NAME, sText);

                if (isStatusColumnHidden == true && mHideState == true) {
                    /* Show the state as tooltip */
                    sslItem->setToolTip(COLUMN_NAME, connectStateString);
                } else {
                    sslItem->setToolTip(COLUMN_NAME, "");
                }

                // sort location
                sslItem->setData(COLUMN_STATE, ROLE_SORT, sText);

                /* last contact */
                QDateTime sslLastContact = QDateTime::fromTime_t(sslDetail.lastConnect);
                sslItem->setData(COLUMN_LAST_CONTACT, Qt::DisplayRole, QVariant(sslLastContact));
                sslItem->setData(COLUMN_LAST_CONTACT, ROLE_SORT, sslLastContact);
                if (sslLastContact > lastContact) {
                    lastContact = sslLastContact;
                }

                /* IP */
					 
                QString sslItemIP = (sslDetail.state & RS_PEER_STATE_CONNECTED)?QString(sslDetail.extAddr.c_str()):QString("---");
                sslItem->setData(COLUMN_IP, Qt::DisplayRole, QVariant(sslItemIP));
                sslItem->setData(COLUMN_IP, ROLE_SORT, sslItemIP);
                if (sslItemIP != itemIP) {
                    itemIP = sslItemIP;
                }

                /* change color and icon */
                QIcon sslIcon;
                QFont sslFont;
                QColor sslColor;
                if (sslDetail.state & RS_PEER_STATE_CONNECTED) {
                    // get the status info for this ssl id
                    int peerState = 0;
                    int rsState = 0;
                    std::list<StatusInfo>::iterator it;
                    for(it = statusInfo.begin(); it != statusInfo.end(); it++) {
                        if(it->id == sslId){
                            rsState = it->status;
                            switch (rsState) {
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
                            if (bestPeerState == 0 || peerState < bestPeerState) {
                                /* first ssl contact or higher state */
                                bestPeerState = peerState;
                                bestSslId = sslId;
                                bestRSState = rsState;
                                bestCustomStateString = customStateString;
                            } else if (peerState == bestPeerState) {
                                /* equal state */
                                if (mBigName && bestCustomStateString.isEmpty() && !customStateString.isEmpty()) {
                                    /* when customStateString is shown in name item, use sslId with customStateString.
                                       second with a custom state string ... use second */
                                    bestPeerState = peerState;
                                    bestSslId = sslId;
                                    bestRSState = rsState;
                                    bestCustomStateString = customStateString;
                                }
                            }
                            break;
                        }
                    }

                    sslItem->setHidden(false);
                    gpg_connected = true;

                    sslIcon = QIcon(":/images/connect_established.png");

                    if (rsState == 0) {
                        sslFont.setBold(true);
                        sslColor = mTextColorStatus[RS_STATUS_ONLINE];
                    } else {
                        sslFont = StatusDefs::font(rsState);
                        sslColor = mTextColorStatus[rsState];
                    }
                } else if (sslDetail.state & RS_PEER_STATE_ONLINE) {
                    sslItem->setHidden(mHideUnconnected);
                    gpg_online = true;

                    if (sslDetail.connectState) {
                        sslIcon = QIcon(":/images/connect_creating.png");
                    } else {
                        sslIcon = QIcon(":/images/connect_no.png");
                    }

                    sslFont.setBold(true);
                    sslColor = mTextColorStatus[RS_STATUS_ONLINE];
                } else {
                    sslItem->setHidden(mHideUnconnected);
                    if (sslDetail.connectState) {
                        sslIcon = QIcon(":/images/connect_creating.png");
                    } else {
                        sslIcon = QIcon(":/images/connect_no.png");
                    }

                    sslFont.setBold(false);
                    sslColor = mTextColorStatus[RS_STATUS_OFFLINE];
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

            QIcon gpgIcon;
            if (gpg_connected) {
                gpgItem->setHidden(false);

                onlineCount++;

                if (bestPeerState == 0) {
                    // show as online
                    bestPeerState = PEER_STATE_ONLINE;
                    bestRSState = RS_STATUS_ONLINE;
                }

                QColor textColor = mTextColorStatus[bestRSState];
                QFont font = StatusDefs::font(bestRSState);
                for(int i = 0; i < COLUMN_COUNT; i++) {
                    gpgItem->setTextColor(i, textColor);
                    gpgItem->setFont(i, font);
                }

                gpgIcon = QIcon(StatusDefs::imageUser(bestRSState));

                if (!isStatusColumnHidden) {
                    gpgItem->setText(COLUMN_STATE, StatusDefs::name(bestRSState));
                }

                if (isStatusColumnHidden && mBigName && !mHideState) {
                    if (bestCustomStateString.isEmpty()) {
                        gpgItemText += "\n" + StatusDefs::name(bestRSState);
                    } else {
                        gpgItemText += "\n" + bestCustomStateString;
                    }
                } else if (isStatusColumnHidden && !mHideState){
                    gpgItemText += " [" + StatusDefs::name(bestRSState) + "]";
                }
            } else if (gpg_online) {
                if (!isStatusColumnHidden) {
                    gpgItem->setText(COLUMN_STATE, tr("Available"));
                } else if (!mHideState && !mBigName) {
                    gpgItemText += " [" + tr("Available") + "]";
                }

                bestPeerState = PEER_STATE_AVAILABLE;
                onlineCount++;
                gpgItem->setHidden(mHideUnconnected);
                gpgIcon = QIcon(IMAGE_AVAILABLE);

                QFont font;
                font.setBold(true);
                QColor textColor = mTextColorStatus[RS_STATUS_ONLINE];
                for(int i = 0; i < COLUMN_COUNT; i++) {
                    gpgItem->setTextColor(i, textColor);
                    gpgItem->setFont(i,font);
                }
            } else {
                if (!isStatusColumnHidden) {
                    gpgItem->setText(COLUMN_STATE, StatusDefs::name(RS_STATUS_OFFLINE));
                } else if (!mHideState && !mBigName) {
                    gpgItemText += " [" + StatusDefs::name(RS_STATUS_OFFLINE) + "]";
                }

                bestPeerState = PEER_STATE_OFFLINE;
                gpgItem->setHidden(mHideUnconnected);
                gpgIcon = QIcon(StatusDefs::imageUser(RS_STATUS_OFFLINE));

                QColor textColor = mTextColorStatus[RS_STATUS_OFFLINE];
                QFont font = StatusDefs::font(RS_STATUS_OFFLINE);
                for(int i = 0; i < COLUMN_COUNT; i++) {
                    gpgItem->setTextColor(i, textColor);
                    gpgItem->setFont(i, font);
                }
            }

            if (gpg_hasPrivateChat) {
                gpgIcon = QIcon(":/images/chat.png");
            }

            if (!isAvatarColumnHidden && gpgItem->icon(COLUMN_AVATAR).isNull()) {
                // only set the avatar image the first time, or when it changed
                // otherwise getAvatarFromSslId sends request packages to peers.
                QPixmap avatar;
                AvatarDefs::getAvatarFromSslId(bestSslId, avatar);
                QIcon avatar_icon(avatar);
                gpgItem->setIcon(COLUMN_AVATAR, avatar_icon);
            }

            gpgItem->setText(COLUMN_NAME, gpgItemText);
            gpgItem->setData(COLUMN_NAME, ROLE_SORT, "2 " + gpgItemText);
            gpgItem->setData(COLUMN_STATE, ROLE_SORT, "2 " + BuildStateSortString(true, gpgItemText, bestPeerState));
            gpgItem->setIcon(COLUMN_NAME, gpgIcon);
            gpgItem->setData(COLUMN_LAST_CONTACT, Qt::DisplayRole, QVariant(lastContact));
            gpgItem->setData(COLUMN_LAST_CONTACT, ROLE_SORT, "2 " + lastContact.toString("yyyyMMdd_hhmmss"));
            gpgItem->setData(COLUMN_IP, Qt::DisplayRole, QVariant());
            gpgItem->setData(COLUMN_IP, ROLE_SORT, "2 " + itemIP);

            if (openPeers != NULL && openPeers->find(gpgId) != openPeers->end()) {
                gpgItem->setExpanded(true);
            }
        }

        if (groupInfo && groupItem) {
            if ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) && groupItem->childCount() == 0) {
                // there are some dead id's assigned
                groupItem->setHidden(true);
            } else {
                groupItem->setHidden(false);
                QString groupName = GroupDefs::name(*groupInfo);
                groupItem->setText(COLUMN_NAME, QString("%1 (%2/%3)").arg(groupName).arg(onlineCount).arg(availableCount));
                // show first the standard groups, than the user groups
                groupItem->setData(COLUMN_NAME, ROLE_SORT, ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) ? "0 " : "1 ") + groupName);
            }
        }

        if (mShowGroups && groupIt != groupInfoList.end()) {
            groupIt++;
        } else {
            // all done
            break;
        }
    }

    if (filterText.isEmpty() == false) {
        filterItems(filterText);
    }

    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->isHidden()) {
        // active item is hidden, deselect it
        ui->peerTreeWidget->setCurrentItem(NULL);
    }

    groupsHasChanged = false;
    if (openGroups != NULL) {
        delete(openGroups);
        openGroups = NULL;
    }
    if (openPeers != NULL) {
        delete(openPeers);
        openPeers = NULL;
    }
}

/**
 * Returns a list with all groupIds that are expanded
 */
bool FriendList::getExpandedGroups(std::set<std::string> &groups) const
{
    int itemCount = ui->peerTreeWidget->topLevelItemCount();
    for (int index = 0; index < itemCount; index++) {
        QTreeWidgetItem *item = ui->peerTreeWidget->topLevelItem(index);
        if (item->type() == TYPE_GROUP && item->isExpanded()) {
            groups.insert(item->data(COLUMN_DATA, ROLE_ID).toString().toStdString());
        }
    }
    return true;
}

/**
 * Returns a list with all gpg ids that are expanded
 */
bool FriendList::getExpandedPeers(std::set<std::string> &peers) const
{
    peers.clear();
    QTreeWidgetItemIterator it(ui->peerTreeWidget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (item->type() == TYPE_GPG && item->isExpanded()) {
            peers.insert(peers.end(), getRsId(item));
        }
        ++it;
    }
    return true;
}

///** Open a QFileDialog to browse for export a file. */
//void FriendList::exportfriend()
//{
//    QTreeWidgetItem *c = getCurrentPeer();

//#ifdef FRIENDS_DEBUG
//    std::cerr << "FriendList::exportfriend()" << std::endl;
//#endif
//    if (!c)
//    {
//#ifdef FRIENDS_DEBUG
//        std::cerr << "FriendList::exportfriend() None Selected -- sorry" << std::endl;
//#endif
//        return;
//    }

//    std::string id = getPeerRsCertId(c);

//    if (misc::getSaveFileName(this, RshareSettings::LASTDIR_CERT, tr("Save Certificate"), tr("Certificates (*.pqi)"), fileName))
//    {
//#ifdef FRIENDS_DEBUG
//        std::cerr << "FriendList::exportfriend() Saving to: " << fileName.toStdString() << std::endl;
//#endif
//        if (rsPeers)
//        {
//            rsPeers->saveCertificateToFile(id, fileName.toUtf8().constData());
//        }
//    }

//}

void FriendList::chatfriendproxy()
{
    chatfriend(getCurrentPeer());
}

/**
 * Start a chat with a friend
 *
 * @param pPeer the gpg or ssl QTreeWidgetItem to chat with
 */
void FriendList::chatfriend(QTreeWidgetItem *pPeer)
{
    if (pPeer == NULL) {
        return;
    }

    std::string id = getRsId(pPeer);
    ChatDialog::chatFriend(id);
}

void FriendList::addFriend()
{
    std::string groupId = getSelectedGroupId();

    ConnectFriendWizard connwiz (this);

    if (groupId.empty() == false) {
        connwiz.setGroup(groupId);
    }

    connwiz.exec ();
}

void FriendList::msgfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::string id = getRsId(peer);
    MessageComposer::msgFriend(id, (peer->type() == TYPE_GROUP));
}

void FriendList::recommendfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::string peerId = getRsId(peer);
    std::list<std::string> ids;

    switch (peer->type()) {
    case TYPE_SSL:
        ids.push_back(peerId);
        break;
    case TYPE_GPG:
        rsPeers->getAssociatedSSLIds(peerId, ids);
        break;
    default:
        return;
    }

    MessageComposer::recommendFriend(ids);
}

void FriendList::pastePerson()
{
    RSLinkClipboard::process(RetroShareLink::TYPE_PERSON);
    RSLinkClipboard::process(RetroShareLink::TYPE_CERTIFICATE);
}

void FriendList::copyFullCertificate()
{
	QTreeWidgetItem *c = getCurrentPeer();
	QList<RetroShareLink> urls;
	RetroShareLink link ;
	link.createCertificate(getRsId(c)) ;
	urls.push_back(link);

	std::cerr << "link: " << std::endl;

	std::cerr<< link.toString().toStdString() << std::endl;
	RSLinkClipboard::copyLinks(urls);
}

void FriendList::copyLink()
{
    QTreeWidgetItem *c = getCurrentPeer();

    if (c == NULL) {
        return;
    }

    QList<RetroShareLink> urls;
    RetroShareLink link;
    if (link.createPerson(getRsId(c))) {
        urls.push_back(link);
    }

    RSLinkClipboard::copyLinks(urls);
}

/**
 * Find out which group is selected
 *
 * @return If a group item is selected, its groupId otherwise an empty string
 */
std::string FriendList::getSelectedGroupId() const
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->type() == TYPE_GROUP) {
        return getRsId(c);
    }

    return std::string();
}

QTreeWidgetItem *FriendList::getCurrentPeer() const
{
    /* get the current, and extract the Id */
    QTreeWidgetItem *item = ui->peerTreeWidget->currentItem();
#ifdef FRIENDS_DEBUG
    if (!item)
    {
        std::cerr << "Invalid Current Item" << std::endl;
        return NULL;
    }

    /* Display the columns of this item. */
    QString out = "CurrentPeerItem: \n";

    for(int i = 1; i < COLUMN_COUNT; i++)
    {
        QString txt = item -> text(i);
        out += QString("\t%1:%2\n").arg(i).arg(txt);
    }
    std::cerr << out.toStdString();
#endif
    return item;
}

#ifdef UNFINISHED_FD
/* GUI stuff -> don't do anything directly with Control */
void FriendsDialog::viewprofile()
{
    /* display Dialog */

    QTreeWidgetItem *c = getCurrentPeer();


    //	static ProfileView *profileview = new ProfileView();


    if (!c)
        return;

    /* set the Id */
    std::string id = getRsId(c);

    profileview -> setPeerId(id);
    profileview -> show();
}
#endif

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

void FriendList::removefriend()
{
    QTreeWidgetItem *c = getCurrentPeer();
#ifdef FRIENDS_DEBUG
    std::cerr << "FriendList::removefriend()" << std::endl;
#endif
    if (!c)
    {
#ifdef FRIENDS_DEBUG
        std::cerr << "FriendList::removefriend() None Selected -- sorry" << std::endl;
#endif
        return;
    }

    if (rsPeers)
    {
        if ((QMessageBox::question(this, "RetroShare", tr("Do you want to remove this Friend?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
        {
            rsPeers->removeFriend(getRsId(c));
        }
    }
}

void FriendList::connectfriend()
{
    QTreeWidgetItem *c = getCurrentPeer();
#ifdef FRIENDS_DEBUG
    std::cerr << "FriendList::connectfriend()" << std::endl;
#endif
    if (!c)
    {
#ifdef FRIENDS_DEBUG
        std::cerr << "FriendList::connectfriend() None Selected -- sorry" << std::endl;
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
                    rsPeers->connectAttempt(getRsId(item));
                    item->setIcon(COLUMN_NAME,(QIcon(IMAGE_CONNECT2)));

	    	    // Launch ProgressDialog, only if single SSL child.
		    if (childCount == 1)
		    {
	    		ConnectProgressDialog::showProgress(getRsId(item));
		    }
                }
            }
        } else {
            //this is a SSL key
            rsPeers->connectAttempt(getRsId(c));
            c->setIcon(COLUMN_NAME,(QIcon(IMAGE_CONNECT2)));
	    // Launch ProgressDialog.
	    ConnectProgressDialog::showProgress(getRsId(c));
        }
    }
}

/* GUI stuff -> don't do anything directly with Control */
void FriendList::configurefriend()
{
    ConfCertDialog::showIt(getRsId(getCurrentPeer()), ConfCertDialog::PageDetails);
}

// void FriendList::showLobby()
// {
//     std::string lobby_id = qobject_cast<QAction*>(sender())->data().toString().toStdString();
// 
//     if (lobby_id.empty())
//         return;
// 
//     std::string vpeer_id;
// 
//     if (rsMsgs->getVirtualPeerId(ChatLobbyId(QString::fromStdString(lobby_id).toULongLong()), vpeer_id))
//         ChatDialog::chatFriend(vpeer_id);
// }
// 
// void FriendList::unsubscribeToLobby()
// {
//     std::string lobby_id = qobject_cast<QAction*>(sender())->data().toString().toStdString();
// 
//     if (lobby_id.empty())
//         return;
// 
//     std::string vpeer_id ;
//     rsMsgs->getVirtualPeerId (ChatLobbyId(QString::fromStdString(lobby_id).toULongLong()), vpeer_id);
// 
//     if (QMessageBox::Ok == QMessageBox::question(this,tr("Unsubscribe to lobby"),tr("You are about to unsubscribe a chat lobby<br>You can only re-enter if your friends invite you again."),QMessageBox::Ok | QMessageBox::Cancel))
//         rsMsgs->unsubscribeChatLobby(ChatLobbyId(QString::fromStdString(lobby_id).toULongLong())) ;
// 
//     // we should also close existing windows.
//     ChatDialog::closeChat(vpeer_id);
// }

void FriendList::getSslIdsFromItem(QTreeWidgetItem *item, std::list<std::string> &sslIds)
{
    if (item == NULL) {
        return;
    }

    std::string peerId = getRsId(item);

    switch (item->type()) {
    case TYPE_SSL:
        sslIds.push_back(peerId);
        break;
    case TYPE_GPG:
        rsPeers->getAssociatedSSLIds(peerId, sslIds);
        break;
    case TYPE_GROUP:
        {
            RsGroupInfo groupInfo;
            if (rsPeers->getGroupInfo(peerId, groupInfo)) {
                std::list<std::string>::iterator gpgIt;
                for (gpgIt = groupInfo.peerIds.begin(); gpgIt != groupInfo.peerIds.end(); ++gpgIt) {
                    rsPeers->getAssociatedSSLIds(*gpgIt, sslIds);
                }
            }
        }
        break;
    }
}

#ifdef TO_REMOVE
void FriendList::inviteToLobby()
{
    QTreeWidgetItem *c = getCurrentPeer();

    std::list<std::string> sslIds;
    getSslIdsFromItem(c, sslIds);

    std::string lobby_id = qobject_cast<QAction*>(sender())->data().toString().toStdString();
    if (lobby_id.empty())
        return;

    ChatLobbyId lobbyId = QString::fromStdString(lobby_id).toULongLong();

    // add to group
    std::list<std::string>::iterator it;
    for (it = sslIds.begin(); it != sslIds.end(); ++it) {
        rsMsgs->invitePeerToLobby(lobbyId, *it);
    }

    std::string vpeer_id;
    if (rsMsgs->getVirtualPeerId(lobbyId, vpeer_id))
        ChatDialog::chatFriend(vpeer_id);
}

void FriendList::createchatlobby()
{
    QTreeWidgetItem *c = getCurrentPeer();

    std::list<std::string> sslIds;
    getSslIdsFromItem(c, sslIds);

    CreateLobbyDialog(sslIds).exec();
}
#endif

void FriendList::addToGroup()
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
    std::string gpgId = getRsId(c);

    if (gpgId.empty() || groupId.empty()) {
        return;
    }

    // automatically expand the group, the peer is added to
    addGroupToExpand(groupId);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
}

void FriendList::moveToGroup()
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
    std::string gpgId = getRsId(c);

    if (gpgId.empty() || groupId.empty()) {
        return;
    }

    // remove from all groups
    rsPeers->assignPeerToGroup("", gpgId, false);

    // automatically expand the group, the peer is added to
    addGroupToExpand(groupId);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
}

void FriendList::removeFromGroup()
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
    std::string gpgId = getRsId(c);

    if (gpgId.empty()) {
        return;
    }

    // remove from (all) group(s)
    rsPeers->assignPeerToGroup(groupId, gpgId, false);
}

void FriendList::editGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GROUP) {
        // wrong type
        return;
    }

    std::string groupId = getRsId(c);

    if (groupId.empty()) {
        return;
    }

    CreateGroup editGrpDialog(groupId, this);
    editGrpDialog.exec();
}

void FriendList::removeGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GROUP) {
        // wrong type
        return;
    }

    std::string groupId = getRsId(c);

    if (groupId.empty()) {
        return;
    }

    rsPeers->removeGroup(groupId);
}

void FriendList::setHideUnconnected(bool hidden)
{
    if (mHideUnconnected != hidden) {
        mHideUnconnected = hidden;
        insertPeers();
    }
}

void FriendList::setShowStatusColumn(bool show)
{
    ui->actionHideState->setEnabled(!show);
    bool isColumnVisible = !ui->peerTreeWidget->isColumnHidden(COLUMN_STATE);

    if (isColumnVisible != show) {
        ui->peerTreeWidget->setColumnHidden(COLUMN_STATE, !show);

        updateHeader();
        insertPeers();
    }
}

void FriendList::setShowLastContactColumn(bool show)
{
    bool isColumnVisible = !ui->peerTreeWidget->isColumnHidden(COLUMN_LAST_CONTACT);

    if (isColumnVisible != show) {
        ui->peerTreeWidget->setColumnHidden(COLUMN_LAST_CONTACT, !show);

        updateHeader();
        insertPeers();
    }
}

void FriendList::setShowIPColumn(bool show)
{
    bool isColumnVisible = !ui->peerTreeWidget->isColumnHidden(COLUMN_IP);

    if (isColumnVisible != show) {
        ui->peerTreeWidget->setColumnHidden(COLUMN_IP, !show);

        updateHeader();
        insertPeers();
    }
}

void FriendList::setShowAvatarColumn(bool show)
{
    bool isColumnVisible = !ui->peerTreeWidget->isColumnHidden(COLUMN_AVATAR);
    if (isColumnVisible == show)
        return;

    ui->peerTreeWidget->setColumnHidden(COLUMN_AVATAR, !show);

    updateHeader();
    insertPeers();
}

void FriendList::setHideState(bool hidden)
{
    if (mHideState != hidden) {
        mHideState = hidden;
        insertPeers();
    }
}

/**
 * Set the header visible.
 */
void FriendList::updateHeader()
{
    if (ui->peerTreeWidget->isColumnHidden(COLUMN_STATE) \
            && ui->peerTreeWidget->isColumnHidden(COLUMN_LAST_CONTACT) \
            && ui->peerTreeWidget->isColumnHidden(COLUMN_IP)) {
        ui->peerTreeWidget->setHeaderHidden(true);
    } else {
        ui->peerTreeWidget->setHeaderHidden(false);
    }
}

void FriendList::setSortByName()
{
    ui->peerTreeWidget->sortByColumn(COLUMN_NAME);
    sortPeersAscendingOrder();
}

void FriendList::setSortByState()
{
    ui->peerTreeWidget->sortByColumn(COLUMN_STATE);
    sortPeersAscendingOrder();
}

void FriendList::setSortByLastContact()
{
    ui->peerTreeWidget->sortByColumn(COLUMN_LAST_CONTACT);
    sortPeersDescendingOrder();
}

void FriendList::setSortByIP()
{
    ui->peerTreeWidget->sortByColumn(COLUMN_IP);
    sortPeersDescendingOrder();
}

void FriendList::sortPeersAscendingOrder()
{
    ui->peerTreeWidget->sortByColumn(ui->peerTreeWidget->sortColumn(), Qt::AscendingOrder);
}

void FriendList::sortPeersDescendingOrder()
{
    ui->peerTreeWidget->sortByColumn(ui->peerTreeWidget->sortColumn(), Qt::DescendingOrder);
}

void FriendList::setRootIsDecorated(bool show)
{
    ui->peerTreeWidget->setRootIsDecorated(show);
}

void FriendList::setShowGroups(bool show)
{
    if (mShowGroups != show) {
        mShowGroups = show;
        if (mShowGroups) {
            // remove all not assigned gpg ids
            int childCount = ui->peerTreeWidget->topLevelItemCount();
            int childIndex = 0;
            while (childIndex < childCount) {
                QTreeWidgetItem *item = ui->peerTreeWidget->topLevelItem(childIndex);
                if (item->type() == TYPE_GPG) {
                    delete(ui->peerTreeWidget->takeTopLevelItem(childIndex));
                    childCount = ui->peerTreeWidget->topLevelItemCount();
                    continue;
                }
                childIndex++;
            }
        }
        insertPeers();
    }
}

/**
 * If set to true, the customStateString will be shwon in all gpg peer items,
 * not only in the ssl ids (used in MessengerWindow).
 * These items will then be doublespaced.
 */
void FriendList::setBigName(bool bigName)
{
    if (mBigName != bigName) {
        mBigName = bigName;

        // Change the size of the already existing items
        QSize newSize;
        if (mBigName) {
            newSize.setHeight(40);
            newSize.setWidth(40);
        } else {
            newSize.setHeight(26);
            newSize.setWidth(26);
        }
        QTreeWidgetItemIterator it(ui->peerTreeWidget);
        while (*it) {
            if ((*it)->type() == TYPE_GPG) {
                (*it)->setSizeHint(COLUMN_NAME, newSize);
            }
            it++;
        }
    }
}

/**
 * Hides all items that don't contain sPattern in the name column.
 */
void FriendList::filterItems(const QString &text)
{
    filterText = text;
    int count = ui->peerTreeWidget->topLevelItemCount();
    for (int index = 0; index < count; index++) {
        FriendList::filterItem(ui->peerTreeWidget->topLevelItem(index), filterText);
    }

    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->isHidden()) {
        // active item is hidden, deselect it
        ui->peerTreeWidget->setCurrentItem(NULL);
    }
}

bool FriendList::filterItem(QTreeWidgetItem *item, const QString &text)
{
    bool visible = true;

    if (text.isEmpty() == false) {
        if (item->text(0).contains(text, Qt::CaseInsensitive) == false) {
            visible = false;
        }
    }

    int visibleChildCount = 0;
    int count = item->childCount();
    for (int index = 0; index < count; index++) {
        if (FriendList::filterItem(item->child(index), text)) {
            visibleChildCount++;
        }
    }

    if (visible || visibleChildCount) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    return (visible || visibleChildCount);
}

/**
 * Add a groupId to the openGroups list. These groups
 * will be expanded, when they're added to the QTreeWidget
 */
void FriendList::addGroupToExpand(const std::string &groupId)
{
    if (openGroups == NULL) {
        openGroups = new std::set<std::string>;
    }
    openGroups->insert(groupId);
}

/**
 * Add a gpgId to the openPeers list. These peers
 * will be expanded, when they're added to the QTreeWidget
 */
void FriendList::addPeerToExpand(const std::string &gpgId)
{
    if (openPeers == NULL) {
        openPeers = new std::set<std::string>;
    }
    openPeers->insert(gpgId);
}

QMenu *FriendList::createDisplayMenu()
{
    QMenu *displayMenu = new QMenu(this);
    connect(displayMenu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));

//    displayMenu->addAction(ui->actionSortPeersDescendingOrder);
//    displayMenu->addAction(ui->actionSortPeersAscendingOrder);
    QMenu *menu = displayMenu->addMenu(tr("Columns"));
    menu->addAction(ui->actionShowAvatarColumn);
    menu->addAction(ui->actionShowLastContactColumn);
    menu->addAction(ui->actionShowIPColumn);
    menu->addAction(ui->actionShowStatusColumn);
//    menu = displayMenu->addMenu(tr("Sort by"));
//    menu->addAction(ui->actionSortByName);
//    menu->addAction(ui->actionSortByState);
//    menu->addAction(ui->actionSortByLastContact);
//    menu->addAction(ui->actionSortByIP);
    displayMenu->addAction(ui->actionHideOfflineFriends);
    displayMenu->addAction(ui->actionHideState);
//    displayMenu->addAction(ui->actionRootIsDecorated);
    displayMenu->addAction(ui->actionShowGroups);

//    QActionGroup *group = new QActionGroup(this);
//    group->addAction(ui->actionSortByName);
//    group->addAction(ui->actionSortByState);
//    group->addAction(ui->actionSortByLastContact);
//    group->addAction(ui->actionSortByIP);

    return displayMenu;
}

void FriendList::updateMenu()
{
    switch (ui->peerTreeWidget->sortColumn()) {
    case COLUMN_NAME:
        ui->actionSortByName->setChecked(true);
        break;
    case COLUMN_STATE:
        ui->actionSortByState->setChecked(true);
        break;
    case COLUMN_LAST_CONTACT:
        ui->actionSortByLastContact->setChecked(true);
        break;
    case COLUMN_IP:
        ui->actionSortByIP->setChecked(true);
        break;
    case COLUMN_AVATAR:
        break;
    }
    ui->actionShowStatusColumn->setChecked(!ui->peerTreeWidget->isColumnHidden(COLUMN_STATE));
    ui->actionShowLastContactColumn->setChecked(!ui->peerTreeWidget->isColumnHidden(COLUMN_LAST_CONTACT));
    ui->actionShowIPColumn->setChecked(!ui->peerTreeWidget->isColumnHidden(COLUMN_IP));
    ui->actionShowAvatarColumn->setChecked(!ui->peerTreeWidget->isColumnHidden(COLUMN_AVATAR));
    ui->actionHideOfflineFriends->setChecked(mHideUnconnected);
    ui->actionHideState->setChecked(mHideState);
    ui->actionRootIsDecorated->setChecked(ui->peerTreeWidget->rootIsDecorated());
    ui->actionShowGroups->setChecked(mShowGroups);
}
