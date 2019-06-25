/*******************************************************************************
 * gui/common/NewFriendList.cpp                                                *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <algorithm>

#include <QShortcut>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QWidgetAction>
#include <QDateTime>
#include <QPainter>
#include <QtXml>

#include "rsserver/rsaccounts.h"
#include "retroshare/rspeers.h"

#include "GroupDefs.h"
#include "gui/chat/ChatDialog.h"
//#include "gui/chat/CreateLobbyDialog.h"
#include "gui/common/AvatarDefs.h"

#include "gui/connect/ConfCertDialog.h"
#include "gui/connect/PGPKeyDialog.h"
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
#include "util/QtVersion.h"
#include "gui/chat/ChatUserNotify.h"
#include "gui/connect/ConnectProgressDialog.h"
#include "gui/common/ElidedLabel.h"

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
/* Images for Status icons */
#define IMAGE_AVAILABLE          ":/images/user/identityavaiblecyan24.png"
#define IMAGE_PASTELINK          ":/images/pasterslink.png"
#define IMAGE_GROUP24            ":/images/user/group24.png"

#define COLUMN_DATA     0 // column for storing the userdata id

#define ROLE_ID                  Qt::UserRole
#define ROLE_STANDARD            Qt::UserRole + 1
#define ROLE_SORT_GROUP          Qt::UserRole + 2
#define ROLE_SORT_STANDARD_GROUP Qt::UserRole + 3
#define ROLE_SORT_NAME           Qt::UserRole + 4
#define ROLE_SORT_STATE          Qt::UserRole + 5
#define ROLE_FILTER              Qt::UserRole + 6

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

/******
 * #define FRIENDS_DEBUG 1
 *****/

Q_DECLARE_METATYPE(ElidedLabel*)

NewFriendList::NewFriendList(QWidget *parent) :
	 QAbstractItemView(parent),
    mCompareRole(new RSTreeWidgetItemCompareRole),
    mShowGroups(true),
    mShowState(false),
    mHideUnconnected(false),
    groupsHasChanged(false)
{
    connect(ui->peerTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(peerTreeWidgetCustomPopupMenu()));
    connect(ui->peerTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(expandItem(QTreeWidgetItem *)));
    connect(ui->peerTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem *)), this, SLOT(collapseItem(QTreeWidgetItem *)));
	 connect(ui->peerTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(expandItem(QTreeWidgetItem *)) );

    connect(NotifyQt::getInstance(), SIGNAL(groupsChanged(int)), this, SLOT(groupsChanged()));
    connect(NotifyQt::getInstance(), SIGNAL(friendsChanged()), this, SLOT(insertPeers()));

    connect(ui->actionHideOfflineFriends, SIGNAL(triggered(bool)), this, SLOT(setHideUnconnected(bool)));
    connect(ui->actionShowState, SIGNAL(triggered(bool)), this, SLOT(setShowState(bool)));
    connect(ui->actionShowGroups, SIGNAL(triggered(bool)), this, SLOT(setShowGroups(bool)));
    connect(ui->actionExportFriendlist, SIGNAL(triggered()), this, SLOT(exportFriendlistClicked()));
    connect(ui->actionImportFriendlist, SIGNAL(triggered()), this, SLOT(importFriendlistClicked()));

    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));

    ui->filterLineEdit->setPlaceholderText(tr("Search")) ;
    ui->filterLineEdit->showFilterIcon();

    /* Add filter actions */
    QTreeWidgetItem *headerItem = ui->peerTreeWidget->headerItem();
    QString headerText = headerItem->text(COLUMN_NAME);
    ui->filterLineEdit->addFilter(QIcon(), headerText, COLUMN_NAME, QString("%1 %2").arg(tr("Search"), headerText));
    ui->filterLineEdit->addFilter(QIcon(), tr("ID"), COLUMN_ID, tr("Search ID"));

    mActionSortByState = new QAction(tr("Sort by state"), this);
    mActionSortByState->setCheckable(true);
    connect(mActionSortByState, SIGNAL(toggled(bool)), this, SLOT(sortByState(bool)));
    ui->peerTreeWidget->addContextMenuAction(mActionSortByState);

    /* Set sort */
    sortByColumn(COLUMN_NAME, Qt::AscendingOrder);
    sortByState(false);

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), ui->peerTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT(removefriend()));

    /* Initialize tree */
    ui->peerTreeWidget->enableColumnCustomize(true);
    ui->peerTreeWidget->setColumnCustomizable(COLUMN_NAME, false);
    connect(ui->peerTreeWidget, SIGNAL(columnVisibleChanged(int,bool)), this, SLOT(peerTreeColumnVisibleChanged(int,bool)));
    connect(ui->peerTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(peerTreeItemCollapsedExpanded(QTreeWidgetItem*)));
    connect(ui->peerTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(peerTreeItemCollapsedExpanded(QTreeWidgetItem*)));

    QFontMetricsF fontMetrics(ui->peerTreeWidget->font());

    /* Set initial column width */
    int fontWidth = fontMetrics.width("W");
    ui->peerTreeWidget->setColumnWidth(COLUMN_NAME, 22 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(COLUMN_LAST_CONTACT, 12 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(COLUMN_IP, 15 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(COLUMN_ID, 32 * fontWidth);

    int avatarHeight = fontMetrics.height() * 3;
    ui->peerTreeWidget->setIconSize(QSize(avatarHeight, avatarHeight));

    /* Initialize display menu */
    createDisplayMenu();
}

NewFriendList::~NewFriendList()
{
    delete ui;
    delete(mCompareRole);
}

void NewFriendList::addToolButton(QToolButton *toolButton)
{
    if (!toolButton) {
        return;
    }

    /* Initialize button */
    toolButton->setAutoRaise(true);
    float S = QFontMetricsF(ui->filterLineEdit->font()).height() ;
    toolButton->setIconSize(QSize(S*1.5,S*1.5));
    toolButton->setFocusPolicy(Qt::NoFocus);

    ui->titleBarFrame->layout()->addWidget(toolButton);
}

void NewFriendList::processSettings(bool load)
{
    // state of peer tree
    ui->peerTreeWidget->setSettingsVersion(2);
    ui->peerTreeWidget->processSettings(load);

    if (load) {
        // load settings

//        ui->peerTreeWidget->header()->doItemsLayout(); // is needed because I added a third column
                                                       // restoreState would corrupt the internal sectionCount

        // states
        setHideUnconnected(Settings->value("hideUnconnected", mHideUnconnected).toBool());
        setShowState(Settings->value("showState", mShowState).toBool());
        setShowGroups(Settings->value("showGroups", mShowGroups).toBool());

        // sort
        sortByState(Settings->value("sortByState", isSortByState()).toBool());

        // open groups
        int arrayIndex = Settings->beginReadArray("Groups");
        for (int index = 0; index < arrayIndex; ++index) {
            Settings->setArrayIndex(index);

            std::string gids = Settings->value("open").toString().toStdString();

            RsGroupInfo ginfo ;

            if(rsPeers->getGroupInfoByName(gids,ginfo)) // backward compatibility
                addGroupToExpand(ginfo.id) ;
            else if(rsPeers->getGroupInfo(RsNodeGroupId(gids),ginfo)) // backward compatibility
                addGroupToExpand(ginfo.id) ;
            else
                std::cerr << "(EE) Cannot find group info for openned group \"" << gids << "\"" << std::endl;
        }
        Settings->endArray();
    } else {
        // save settings

        // states
        Settings->setValue("hideUnconnected", mHideUnconnected);
        Settings->setValue("showState", mShowState);
        Settings->setValue("showGroups", mShowGroups);

        // sort
        Settings->setValue("sortByState", isSortByState());

        // open groups
        Settings->beginWriteArray("Groups");
        int arrayIndex = 0;
        std::set<RsNodeGroupId> expandedPeers;
        getExpandedGroups(expandedPeers);
        foreach (RsNodeGroupId groupId, expandedPeers) {
            Settings->setArrayIndex(arrayIndex++);
            Settings->setValue("open", QString::fromStdString(groupId.toStdString()));
        }
        Settings->endArray();
    }
}

void NewFriendList::changeEvent(QEvent *e)
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

/* Utility Fns */
inline std::string getRsId(QTreeWidgetItem *item)
{
    return item->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
}

/**
 * Creates the context popup menu and its submenus,
 * then shows it at the current cursor position.
 */
void NewFriendList::peerTreeWidgetCustomPopupMenu()
{
    QTreeWidgetItem *c = getCurrentPeer();

    QMenu *contextMenu = new QMenu(this);

    QWidget *widget = new QWidget(contextMenu);
    widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

    // create menu header
    QHBoxLayout *hbox = new QHBoxLayout(widget);
    hbox->setMargin(0);
    hbox->setSpacing(6);

    QLabel *iconLabel = new QLabel(widget);
    QPixmap pix = QPixmap(":/images/user/friends24.png").scaledToHeight(QFontMetricsF(iconLabel->font()).height()*1.5);
    iconLabel->setPixmap(pix);
    iconLabel->setMaximumSize(iconLabel->frameSize().height() + pix.height(), pix.width());
    hbox->addWidget(iconLabel);

    QLabel *textLabel = new QLabel("<strong>RetroShare</strong>", widget);
    hbox->addWidget(textLabel);

    QSpacerItem *spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(spacerItem);

    widget->setLayout(hbox);

    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(widget);
    contextMenu->addAction(widgetAction);

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
                 textLabel->setText("<strong>" + tr("Node") + "</strong>");
                 break;
         }

//         QMenu *lobbyMenu = NULL;

         switch (type)
		 {
         case TYPE_GROUP:
             {
                 bool standard = c->data(COLUMN_DATA, ROLE_STANDARD).toBool();
#ifdef RS_DIRECT_CHAT
                 contextMenu->addAction(QIcon(IMAGE_MSG), tr("Send message to whole group"), this, SLOT(msgfriend()));
                 contextMenu->addSeparator();
#endif // RS_DIRECT_CHAT
                 contextMenu->addAction(QIcon(IMAGE_EDIT), tr("Edit Group"), this, SLOT(editGroup()));

                 QAction *action = contextMenu->addAction(QIcon(IMAGE_REMOVE), tr("Remove Group"), this, SLOT(removeGroup()));
                 action->setDisabled(standard);
             }
             break;
         case TYPE_GPG:
        {
#ifdef RS_DIRECT_CHAT
             contextMenu->addAction(QIcon(IMAGE_CHAT), tr("Chat"), this, SLOT(chatfriendproxy()));
             contextMenu->addAction(QIcon(IMAGE_MSG), tr("Send message"), this, SLOT(msgfriend()));
             contextMenu->addSeparator();
#endif // RS_DIRECT_CHAT

             contextMenu->addAction(QIcon(IMAGE_FRIENDINFO), tr("Profile details"), this, SLOT(configurefriend()));
             contextMenu->addAction(QIcon(IMAGE_DENYFRIEND), tr("Deny connections"), this, SLOT(removefriend()));

             if(mShowGroups)
             {
                 QMenu* addToGroupMenu = NULL;
                 QMenu* moveToGroupMenu = NULL;

                 std::list<RsGroupInfo> groupInfoList;
                 rsPeers->getGroupInfoList(groupInfoList);

                 GroupDefs::sortByName(groupInfoList);

                 RsPgpId gpgId ( getRsId(c));

                 QTreeWidgetItem *parent = c->parent();

                 bool foundGroup = false;
                 // add action for all groups, except the own group
                 for (std::list<RsGroupInfo>::iterator groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
                     if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpgId) == groupIt->peerIds.end()) {
                         if (parent) {
                             if (addToGroupMenu == NULL) {
                                 addToGroupMenu = new QMenu(tr("Add to group"), contextMenu);
                             }
                             QAction* addToGroupAction = new QAction(GroupDefs::name(*groupIt), addToGroupMenu);
                             addToGroupAction->setData(QString::fromStdString(groupIt->id.toStdString()));
                             connect(addToGroupAction, SIGNAL(triggered()), this, SLOT(addToGroup()));
                             addToGroupMenu->addAction(addToGroupAction);
                         }

                         if (moveToGroupMenu == NULL) {
                             moveToGroupMenu = new QMenu(tr("Move to group"), contextMenu);
                         }
                         QAction* moveToGroupAction = new QAction(GroupDefs::name(*groupIt), moveToGroupMenu);
                         moveToGroupAction->setData(QString::fromStdString(groupIt->id.toStdString()));
                         connect(moveToGroupAction, SIGNAL(triggered()), this, SLOT(moveToGroup()));
                         moveToGroupMenu->addAction(moveToGroupAction);
                     } else {
                         foundGroup = true;
                     }
                 }

                 QMenu *groupsMenu = contextMenu->addMenu(QIcon(IMAGE_GROUP16), tr("Groups"));
                 groupsMenu->addAction(QIcon(IMAGE_EXPAND), tr("Create new group"), this, SLOT(createNewGroup()));

                 if (addToGroupMenu || moveToGroupMenu || foundGroup) {
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
         break ;

         case TYPE_SSL:
             {
#ifdef RS_DIRECT_CHAT
                 contextMenu->addAction(QIcon(IMAGE_CHAT), tr("Chat"), this, SLOT(chatfriendproxy()));
                 contextMenu->addAction(QIcon(IMAGE_MSG), tr("Send message to this node"), this, SLOT(msgfriend()));
                 contextMenu->addSeparator();
#endif // RS_DIRECT_CHAT

                 contextMenu->addAction(QIcon(IMAGE_FRIENDINFO), tr("Node details"), this, SLOT(configurefriend()));

                 if (type == TYPE_GPG || type == TYPE_SSL) {
                     contextMenu->addAction(QIcon(IMAGE_EXPORTFRIEND), tr("Recommend this node to..."), this, SLOT(recommendfriend()));
                 }

				 if(!rsPeers->isHiddenNode(rsPeers->getOwnId()) || rsPeers->isHiddenNode( RsPeerId(getRsId(c)) ))
					 contextMenu->addAction(QIcon(IMAGE_CONNECT), tr("Attempt to connect"), this, SLOT(connectfriend()));

                 contextMenu->addAction(QIcon(IMAGE_COPYLINK), tr("Copy certificate link"), this, SLOT(copyFullCertificate()));

                 //this is a SSL key
                 contextMenu->addAction(QIcon(IMAGE_REMOVEFRIEND), tr("Remove Friend Node"), this, SLOT(removefriend()));

             }
         }

     }

    contextMenu->addSeparator();

    QAction *action = contextMenu->addAction(QIcon(IMAGE_PASTELINK), tr("Paste certificate link"), this, SLOT(pastePerson()));
    if (RSLinkClipboard::empty(RetroShareLink::TYPE_CERTIFICATE))
        action->setDisabled(true);

    contextMenu->addAction(QIcon(IMAGE_EXPAND), tr("Expand all"), ui->peerTreeWidget, SLOT(expandAll()));
    contextMenu->addAction(QIcon(IMAGE_COLLAPSE), tr("Collapse all"), ui->peerTreeWidget, SLOT(collapseAll()));

    contextMenu = ui->peerTreeWidget->createStandardContextMenu(contextMenu);

    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

void NewFriendList::createNewGroup()
{
    CreateGroup createGrpDialog (RsNodeGroupId(), this);
    createGrpDialog.exec();
}

void NewFriendList::groupsChanged()
{
    groupsHasChanged = true;
    insertPeers();
}

static QIcon createAvatar(const QPixmap &avatar, const QPixmap &overlay)
{
	int avatarWidth = avatar.width();
	int avatarHeight = avatar.height();

	QPixmap pixmap(avatar);

	int overlaySize = (avatarWidth > avatarHeight) ? (avatarWidth/2.5) :  (avatarHeight/2.5);
	int overlayX = avatarWidth - overlaySize;
	int overlayY = avatarHeight - overlaySize;

	QPainter painter(&pixmap);
	painter.drawPixmap(overlayX, overlayY, overlaySize, overlaySize, overlay);

	QIcon icon;
	icon.addPixmap(pixmap);
	return icon;
}

static void getNameWidget(QTreeWidget *treeWidget, QTreeWidgetItem *item, ElidedLabel *&nameLabel, ElidedLabel *&textLabel)
{
    QWidget *widget = treeWidget->itemWidget(item, NewFriendList::COLUMN_NAME);

    if (!widget) {
        widget = new QWidget;
        widget->setAttribute(Qt::WA_TranslucentBackground);
        nameLabel = new ElidedLabel(widget);
        textLabel = new ElidedLabel(widget);

        widget->setProperty("nameLabel", qVariantFromValue(nameLabel));
        widget->setProperty("textLabel", qVariantFromValue(textLabel));

        QVBoxLayout *layout = new QVBoxLayout;
        layout->setSpacing(0);
        layout->setContentsMargins(3, 0, 0, 0);

        nameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        textLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        layout->addWidget(nameLabel);
        layout->addWidget(textLabel);

        widget->setLayout(layout);

        treeWidget->setItemWidget(item, NewFriendList::COLUMN_NAME, widget);
    } else {
        nameLabel = widget->property("nameLabel").value<ElidedLabel*>();
        textLabel = widget->property("textLabel").value<ElidedLabel*>();
    }
}

/**
 * Returns a list with all groupIds that are expanded
 */
bool NewFriendList::getExpandedGroups(std::set<RsNodeGroupId> &groups) const
{
    int itemCount = ui->peerTreeWidget->topLevelItemCount();
    for (int index = 0; index < itemCount; ++index) {
        QTreeWidgetItem *item = ui->peerTreeWidget->topLevelItem(index);
        if (item->type() == TYPE_GROUP && item->isExpanded()) {
            groups.insert(RsNodeGroupId(item->data(COLUMN_DATA, ROLE_ID).toString().toStdString()));
        }
    }
    return true;
}

/**
 * Returns a list with all gpg ids that are expanded
 */
bool NewFriendList::getExpandedPeers(std::set<RsPgpId> &peers) const
{
    peers.clear();
    QTreeWidgetItemIterator it(ui->peerTreeWidget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (item->type() == TYPE_GPG && item->isExpanded()) {
            peers.insert(peers.end(), RsPgpId(getRsId(item)));
        }
        ++it;
    }
    return true;
}

void NewFriendList::collapseItem(QTreeWidgetItem *item)
{
	switch (item->type())
	{
	case TYPE_GROUP:
		openGroups.erase(RsNodeGroupId(getRsId(item))) ;
		break;
	case TYPE_GPG:
		openPeers.erase(RsPgpId(getRsId(item))) ;
	default:
		break;
	}
}
void NewFriendList::expandItem(QTreeWidgetItem *item)
{
	switch (item->type())
	{
	case TYPE_GROUP:
		openGroups.insert(RsNodeGroupId(getRsId(item))) ;
		break;
	case TYPE_GPG:
		openPeers.insert(RsPgpId(getRsId(item))) ;
	default:
		break;
	}
}

void NewFriendList::addFriend()
{
    std::string groupId = getSelectedGroupId();

    ConnectFriendWizard connwiz (this);

    if (groupId.empty() == false) {
        connwiz.setGroup(groupId);
    }

    connwiz.exec ();
}

void NewFriendList::msgfriend()
{
    QTreeWidgetItem *item = getCurrentPeer();

    if (!item)
        return;

    switch (item->type()) {
    case TYPE_GROUP:
        break;
    case TYPE_GPG:
        MessageComposer::msgFriend(RsPgpId(getRsId(item)));
        break;
    case TYPE_SSL:
        MessageComposer::msgFriend(RsPeerId(getRsId(item)));
        break;
    }
}

void NewFriendList::recommendfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::string peerId = getRsId(peer);
    std::list<RsPeerId> ids;

    switch (peer->type())
    {
    case TYPE_SSL:
        ids.push_back(RsPeerId(peerId));
        break;
    case TYPE_GPG:
        rsPeers->getAssociatedSSLIds(RsPgpId(peerId), ids);
        break;
    default:
        return;
    }
    std::set<RsPeerId> sids ;
    for(std::list<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
        sids.insert(*it) ;

    MessageComposer::recommendFriend(sids);
}

void NewFriendList::pastePerson()
{
    //RSLinkClipboard::process(RetroShareLink::TYPE_PERSON);
    RSLinkClipboard::process(RetroShareLink::TYPE_CERTIFICATE);
}

void NewFriendList::copyFullCertificate()
{
	QTreeWidgetItem *c = getCurrentPeer();
	QList<RetroShareLink> urls;
	RetroShareLink link = RetroShareLink::createCertificate(RsPeerId(getRsId(c)));
	urls.push_back(link);

	std::cerr << "link: " << std::endl;
	std::cerr<< link.toString().toStdString() << std::endl;

	RSLinkClipboard::copyLinks(urls);
}

/**
 * Find out which group is selected
 *
 * @return If a group item is selected, its groupId otherwise an empty string
 */
std::string NewFriendList::getSelectedGroupId() const
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c && c->type() == TYPE_GROUP) {
        return getRsId(c);
    }

    return std::string();
}

QTreeWidgetItem *NewFriendList::getCurrentPeer() const
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

    for(int i = 1; i < COLUMN_COUNT; ++i)
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

void NewFriendList::removefriend()
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
        switch (c->type()) {
        case TYPE_GPG:
            if(!RsPgpId(getRsId(c)).isNull()) {
                if ((QMessageBox::question(this, "RetroShare", tr("Do you want to remove this Friend?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
                {
                    rsPeers->removeFriend(RsPgpId(getRsId(c)));
                }
            }
            break;
        case TYPE_SSL:
            if (!RsPeerId(getRsId(c)).isNull()) {
                if ((QMessageBox::question(this, "RetroShare", tr("Do you want to remove this node?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
                {
                    rsPeers->removeFriendLocation(RsPeerId(getRsId(c)));
                }
            }
            break;
        }
    }
}

void NewFriendList::connectfriend()
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
            for (int childIndex = 0; childIndex < childCount; ++childIndex) {
                QTreeWidgetItem *item = c->child(childIndex);
                if (item->type() == TYPE_SSL) {
                    rsPeers->connectAttempt(RsPeerId(getRsId(item)));

	    	    // Launch ProgressDialog, only if single SSL child.
		    if (childCount == 1)
		    {
                ConnectProgressDialog::showProgress(RsPeerId(getRsId(item)));
		    }
                }
            }
        } else {
            //this is a SSL key
            rsPeers->connectAttempt(RsPeerId(getRsId(c)));
	    // Launch ProgressDialog.
        ConnectProgressDialog::showProgress(RsPeerId(getRsId(c)));
        }
    }
}

/* GUI stuff -> don't do anything directly with Control */
void NewFriendList::configurefriend()
{
    if(!RsPeerId(getRsId(getCurrentPeer())).isNull())
        ConfCertDialog::showIt(RsPeerId(getRsId(getCurrentPeer())), ConfCertDialog::PageDetails);
    else if(!RsPgpId(getRsId(getCurrentPeer())).isNull())
        PGPKeyDialog::showIt(RsPgpId(getRsId(getCurrentPeer())), PGPKeyDialog::PageDetails);
    else
        std::cerr << "FriendList::configurefriend: id is not an SSL nor a PGP id." << std::endl;
}

void NewFriendList::getSslIdsFromIndex(const QModelIndex& item, std::list<RsPeerId> &sslIds)
{
    if (item == NULL) {
        return;
    }

    std::string peerId = getRsId(item);

    switch (item->type()) {
    case TYPE_SSL:
        sslIds.push_back(RsPeerId(peerId));
        break;
    case TYPE_GPG:
        rsPeers->getAssociatedSSLIds(RsPgpId(peerId), sslIds);
        break;
    case TYPE_GROUP:
        {
            RsGroupInfo groupInfo;
            if (rsPeers->getGroupInfo(RsNodeGroupId(peerId), groupInfo)) {
                std::set<RsPgpId>::iterator gpgIt;
                for (gpgIt = groupInfo.peerIds.begin(); gpgIt != groupInfo.peerIds.end(); ++gpgIt) {
                    rsPeers->getAssociatedSSLIds(*gpgIt, sslIds);
                }
            }
        }
        break;
    }
}

void NewFriendList::addToGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId ( getRsId(c));

    if (gpgId.isNull() || groupId.isNull()) {
        return;
    }

    // automatically expand the group, the peer is added to
    addGroupToExpand(groupId);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
}

void NewFriendList::moveToGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId ( getRsId(c));

    if (gpgId.isNull() || groupId.isNull()) {
        return;
    }

    // remove from all groups
    rsPeers->assignPeerToGroup(RsNodeGroupId(), gpgId, false);

    // automatically expand the group, the peer is added to
    addGroupToExpand(groupId);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
}

void NewFriendList::removeFromGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GPG) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId ( getRsId(c));

    if (gpgId.isNull()) {
        return;
    }

    // remove from (all) group(s)
    rsPeers->assignPeerToGroup(groupId, gpgId, false);
}

void NewFriendList::editGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GROUP) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( getRsId(c));

    if (!groupId.isNull())
    {
        CreateGroup editGrpDialog(groupId, this);
        editGrpDialog.exec();
    }
}

void NewFriendList::removeGroup()
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (c == NULL) {
        return;
    }

    if (c->type() != TYPE_GROUP) {
        // wrong type
        return;
    }

    RsNodeGroupId groupId ( getRsId(c));

    if (!groupId.isNull())
        rsPeers->removeGroup(groupId);
}

void NewFriendList::exportFriendlistClicked()
{
    QString fileName;
    if(!importExportFriendlistFileDialog(fileName, false))
        // error was already shown - just return
        return;

    if(!exportFriendlist(fileName))
        // error was already shown - just return
        return;

    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Information);
    mbox.setText(tr("Done!"));
    mbox.setInformativeText(tr("Your friendlist is stored at:\n") + fileName +
                            tr("\n(keep in mind that the file is unencrypted!)"));
    mbox.setStandardButtons(QMessageBox::Ok);
    mbox.exec();
}

void NewFriendList::importFriendlistClicked()
{
    QString fileName;
    if(!importExportFriendlistFileDialog(fileName, true))
        // error was already shown - just return
        return;

    bool errorPeers, errorGroups;
    if(importFriendlist(fileName, errorPeers, errorGroups)) {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Information);
        mbox.setText(tr("Done!"));
        mbox.setInformativeText(tr("Your friendlist was imported from:\n") + fileName);
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
    } else {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setText(tr("Done - but errors happened!"));
        mbox.setInformativeText(tr("Your friendlist was imported from:\n") + fileName +
                                (errorPeers  ? tr("\nat least one peer was not added") : "") +
                                (errorGroups ? tr("\nat least one peer was not added to a group") : "")
                                );
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
    }
}

/**
 * @brief opens a file dialog to select a file containing a friendlist
 * @param fileName file containing a friendlist
 * @param import show dialog for importing (true) or exporting (false) friendlist
 * @return success or failure
 *
 * This function also shows an error message when no valid file was selected
 */
bool NewFriendList::importExportFriendlistFileDialog(QString &fileName, bool import)
{
	bool res = true;
	if (import) {
		res = misc::getOpenFileName(this, RshareSettings::LASTDIR_CERT
		                            , tr("Select file for importing your friendlist from")
		                            , tr("XML File (*.xml);;All Files (*)")
		                            , fileName
		                            , QFileDialog::DontConfirmOverwrite
		                            );
		} else {
		res = misc::getSaveFileName(this, RshareSettings::LASTDIR_CERT
		                            , tr("Select a file for exporting your friendlist to")
		                            , tr("XML File (*.xml);;All Files (*)")
		                            , fileName, NULL
		                            , (QFileDialog::Options)0
		                            );
	}
	if ( res && !fileName.endsWith(".xml",Qt::CaseInsensitive) )
		fileName = fileName.append(".xml");

	return res;
}

/**
 * @brief exports friendlist to a given file
 * @param fileName file for storing friendlist
 * @return success or failure
 *
 * This function also shows an error message when the selected file is invalid/not writable
 */
bool NewFriendList::exportFriendlist(QString &fileName)
{
    QDomDocument doc("FriendListWithGroups");
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        // show error to user
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setText(tr("Error"));
        mbox.setInformativeText(tr("File is not writeable!\n") + fileName);
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
        return false;
    }

    std::list<RsPgpId> gpg_ids;
    rsPeers->getGPGAcceptedList(gpg_ids);

    std::list<RsGroupInfo> group_info_list;
    rsPeers->getGroupInfoList(group_info_list);

    QDomElement pgpIDs = doc.createElement("pgpIDs");
    RsPeerDetails detailPGP;
    for(std::list<RsPgpId>::iterator list_iter = gpg_ids.begin(); list_iter !=  gpg_ids.end(); list_iter++)	{
        rsPeers->getGPGDetails(*list_iter, detailPGP);
        QDomElement pgpID = doc.createElement("pgpID");
        // these values aren't used and just stored for better human readability
        pgpID.setAttribute("id", QString::fromStdString(detailPGP.gpg_id.toStdString()));
        pgpID.setAttribute("name", QString::fromUtf8(detailPGP.name.c_str()));

        std::list<RsPeerId> ssl_ids;
        rsPeers->getAssociatedSSLIds(*list_iter, ssl_ids);
        for(std::list<RsPeerId>::iterator list_iter = ssl_ids.begin(); list_iter !=  ssl_ids.end(); list_iter++) {
            RsPeerDetails detailSSL;
            if (!rsPeers->getPeerDetails(*list_iter, detailSSL))
                continue;

            std::string certificate = rsPeers->GetRetroshareInvite(detailSSL.id, true,true);
            // remove \n from certificate
            certificate.erase(std::remove(certificate.begin(), certificate.end(), '\n'), certificate.end());

            QDomElement sslID = doc.createElement("sslID");
            // these values aren't used and just stored for better human readability
            sslID.setAttribute("sslID", QString::fromStdString(detailSSL.id.toStdString()));
            if(!detailSSL.location.empty())
                sslID.setAttribute("location", QString::fromUtf8(detailSSL.location.c_str()));

            // required values
            sslID.setAttribute("certificate", QString::fromStdString(certificate));
            sslID.setAttribute("service_perm_flags", detailSSL.service_perm_flags.toUInt32());

            pgpID.appendChild(sslID);
        }
        pgpIDs.appendChild(pgpID);
    }
    root.appendChild(pgpIDs);

    QDomElement groups = doc.createElement("groups");
    for(std::list<RsGroupInfo>::iterator list_iter = group_info_list.begin(); list_iter !=  group_info_list.end(); list_iter++)	{
        RsGroupInfo group_info = *list_iter;

        //skip groups without peers
        if(group_info.peerIds.empty())
            continue;

        QDomElement group = doc.createElement("group");
        // id is not needed since it may differ between locatiosn / pgp ids (groups are identified by name)
        group.setAttribute("name", QString::fromUtf8(group_info.name.c_str()));
        group.setAttribute("flag", group_info.flag);

        for(std::set<RsPgpId>::iterator i = group_info.peerIds.begin(); i !=  group_info.peerIds.end(); i++) {
            QDomElement pgpID = doc.createElement("pgpID");
            std::string pid = i->toStdString();
            pgpID.setAttribute("id", QString::fromStdString(pid));
            group.appendChild(pgpID);
        }
        groups.appendChild(group);
    }
    root.appendChild(groups);

    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ts << doc.toString();
    file.close();

    return true;
}

/**
 * @brief helper function to show a message box
 */
void showXMLParsingError()
{
    // show error to user
    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Warning);
    mbox.setText(QObject::tr("Error"));
    mbox.setInformativeText(QObject::tr("unable to parse XML file!"));
    mbox.setStandardButtons(QMessageBox::Ok);
    mbox.exec();
}

/**
 * @brief Imports friends from a given file
 * @param fileName file to load friends from
 * @param errorPeers an error occured while adding a peer
 * @param errorGroups an error occured while adding a peer to a group
 * @return success or failure (an error can also happen when adding a peer and/or adding a peer to a group fails at least once)
 */
bool NewFriendList::importFriendlist(QString &fileName, bool &errorPeers, bool &errorGroups)
{
    QDomDocument doc;
    // load from file
    {
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly)) {
            // show error to user
            QMessageBox mbox;
            mbox.setIcon(QMessageBox::Warning);
            mbox.setText(tr("Error"));
            mbox.setInformativeText(tr("File is not readable!\n") + fileName);
            mbox.setStandardButtons(QMessageBox::Ok);
            mbox.exec();
            return false;
        }

        bool ok = doc.setContent(&file);
        file.close();

        if(!ok) {
            showXMLParsingError();
            return false;
        }
    }

    QDomElement root = doc.documentElement();
    if(root.tagName() != "root") {
        showXMLParsingError();
        return false;
    }

    errorPeers = false;
    errorGroups = false;

    std::string error_string;
    RsPeerDetails rsPeerDetails;
    RsPeerId rsPeerID;
    RsPgpId rsPgpID;

    // lock all events for faster processing
    RsAutoUpdatePage::lockAllEvents();

    // pgp and ssl IDs
    QDomElement pgpIDs;
    {
        QDomNodeList nodes = root.elementsByTagName("pgpIDs");
        if(nodes.isEmpty() || nodes.size() != 1){
            showXMLParsingError();
            return false;
        }

        pgpIDs = nodes.item(0).toElement();
        if(pgpIDs.isNull()){
            showXMLParsingError();
            return false;
        }
    }
    QDomNode pgpIDElem = pgpIDs.firstChildElement("pgpID");
    while (!pgpIDElem.isNull()) {
        QDomElement sslIDElem = pgpIDElem.firstChildElement("sslID");
        while (!sslIDElem.isNull()) {
            rsPeerID.clear();
            rsPgpID.clear();

            // load everything needed from the pubkey string
            std::string pubkey = sslIDElem.attribute("certificate").toStdString();
			ServicePermissionFlags service_perm_flags(sslIDElem.attribute("service_perm_flags").toInt());
			if (!rsPeers->acceptInvite(pubkey, service_perm_flags)) {
                errorPeers = true;
				std::cerr << "FriendList::importFriendlist(): failed to get peer detaisl from public key (SSL id: " << sslIDElem.attribute("sslID", "invalid").toStdString() << ")" << std::endl;
            }
            sslIDElem = sslIDElem.nextSiblingElement("sslID");
        }
        pgpIDElem = pgpIDElem.nextSiblingElement("pgpID");
    }

    // groups
    QDomElement groups;
    {
        QDomNodeList nodes = root.elementsByTagName("groups");
        if(nodes.isEmpty() || nodes.size() != 1){
            showXMLParsingError();
            return false;
        }

        groups = nodes.item(0).toElement();
        if(groups.isNull()){
            showXMLParsingError();
            return false;
        }
    }
    QDomElement group = groups.firstChildElement("group");
    while (!group.isNull()) {
        // get name and flags and try to get the group ID
        std::string groupName = group.attribute("name").toStdString();
        uint32_t flag = group.attribute("flag").toInt();
        RsNodeGroupId groupId;
        if(getOrCreateGroup(groupName, flag, groupId)) {
            // group id found!
            QDomElement pgpID = group.firstChildElement("pgpID");
            while (!pgpID.isNull()) {
                // add pgp id to group
                RsPgpId rsPgpId(pgpID.attribute("id").toStdString());
                if(rsPgpID.isNull() || !rsPeers->assignPeerToGroup(groupId, rsPgpId, true)) {
                    errorGroups = true;
                    std::cerr << "FriendList::importFriendlist(): failed to add '" << rsPeers->getGPGName(rsPgpId) << "'' to group '" << groupName << "'" << std::endl;
                }

                pgpID = pgpID.nextSiblingElement("pgpID");
            }
            pgpID = pgpID.nextSiblingElement("pgpID");
        } else {
            errorGroups = true;
            std::cerr << "FriendList::importFriendlist(): failed to find/create group '" << groupName << "'" << std::endl;
        }
        group = group.nextSiblingElement("group");
    }

    // unlock events
    RsAutoUpdatePage::unlockAllEvents();

    return !(errorPeers || errorGroups);
}

/**
 * @brief Gets the groups ID for a given group name
 * @param name group name to search for
 * @param id groupd id for the given name
 * @return success or fail
 */
bool NewFriendList::getGroupIdByName(const std::string &name, RsNodeGroupId &id)
{
    std::list<RsGroupInfo> grpList;
    if(!rsPeers->getGroupInfoList(grpList))
        return false;

    foreach (const RsGroupInfo &grp, grpList) {
        if(grp.name == name) {
            id = grp.id;
            return true;
        }
    }

    return false;
}

/**
 * @brief Gets the groups ID for a given group name. If no groupd was it will create one
 * @param name group name to search for
 * @param flag flag to use when creating the group
 * @param id groupd id
 * @return success or failure
 */
bool NewFriendList::getOrCreateGroup(const std::string &name, const uint &flag, RsNodeGroupId &id)
{
    if(getGroupIdByName(name, id))
        return true;

    // -> create one
    RsGroupInfo grp;
    grp.id.clear(); // RS will generate an ID
    grp.name = name;
    grp.flag = flag;

    if(!rsPeers->addGroup(grp))
        return false;

    // try again
    return getGroupIdByName(name, id);
}


void NewFriendList::setHideUnconnected(bool hidden)
{
    if (mHideUnconnected != hidden) {
        mHideUnconnected = hidden;
        insertPeers();
    }
}

void NewFriendList::setColumnVisible(Column column, bool visible)
{
    ui->peerTreeWidget->setColumnHidden(column, !visible);
    peerTreeColumnVisibleChanged(column, visible);
}

void NewFriendList::sortByColumn(Column column, Qt::SortOrder sortOrder)
{
    ui->peerTreeWidget->sortByColumn(column, sortOrder);
}

void NewFriendList::peerTreeColumnVisibleChanged(int /*column*/, bool visible)
{
    if (visible) {
        insertPeers();
    }
}

void NewFriendList::peerTreeItemCollapsedExpanded(QTreeWidgetItem *item)
{
    if (!item) {
        return;
    }

    if (item->type() == TYPE_GPG) {
        insertPeers();
    }
}

void NewFriendList::setShowState(bool show)
{
    if (mShowState != show) {
        mShowState = show;
        insertPeers();
    }
}

void NewFriendList::sortByState(bool sort)
{
    int columnCount = ui->peerTreeWidget->columnCount();
    for (int i = 0; i < columnCount; ++i) {
        mCompareRole->setRole(i, ROLE_SORT_GROUP);
        mCompareRole->addRole(i, ROLE_SORT_STANDARD_GROUP);

        if (sort) {
            mCompareRole->addRole(i, ROLE_SORT_STATE);
            mCompareRole->addRole(i, ROLE_SORT_NAME);
        } else {
            mCompareRole->addRole(i, ROLE_SORT_NAME);
            mCompareRole->addRole(i, ROLE_SORT_STATE);
        }
    }

    mActionSortByState->setChecked(sort);

    ui->peerTreeWidget->resort();
}

bool NewFriendList::isSortByState()
{
    return mActionSortByState->isChecked();
}

void NewFriendList::setShowGroups(bool show)
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
                ++childIndex;
            }
        }
        insertPeers();
    }
}

/**
 * Hides all items that don't contain text in the name column.
 */
void NewFriendList::filterItems(const QString &text)
{
	int filterColumn = ui->filterLineEdit->currentFilter();
	mFilterText = text;
	ui->peerTreeWidget->filterItems(filterColumn, mFilterText, ROLE_FILTER);
}

/**
 * Add a groupId to the openGroups list. These groups
 * will be expanded, when they're added to the QTreeWidget
 */
void NewFriendList::addGroupToExpand(const RsNodeGroupId &groupId)
{
    openGroups.insert(groupId);
}

/**
 * Add a gpgId to the openPeers list. These peers
 * will be expanded, when they're added to the QTreeWidget
 */
void NewFriendList::addPeerToExpand(const RsPgpId& gpgId)
{
    openPeers.insert(gpgId);
}

void NewFriendList::createDisplayMenu()
{
    QMenu *displayMenu = new QMenu(tr("Show Items"), this);
    connect(displayMenu, SIGNAL(aboutToShow()), this, SLOT(updateMenu()));

    displayMenu->addAction(ui->actionHideOfflineFriends);
    displayMenu->addAction(ui->actionShowState);
    displayMenu->addAction(ui->actionShowGroups);

    ui->peerTreeWidget->addContextMenuMenu(displayMenu);
    ui->peerTreeWidget->addContextMenuAction(ui->actionExportFriendlist);
    ui->peerTreeWidget->addContextMenuAction(ui->actionImportFriendlist);
}

void NewFriendList::updateMenu()
{
    ui->actionHideOfflineFriends->setChecked(mHideUnconnected);
    ui->actionShowState->setChecked(mShowState);
    ui->actionShowGroups->setChecked(mShowGroups);
}
